#include "btremote.h"

#include "commands.h"
#include "hidreportparser.h"
#include "logging.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

// ESP-IDF Bluedroid includes
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_hidh_api.h" // Classic BT HID Host API

// Arduino BT helper (handles controller init correctly)
#include "esp32-hal-bt.h"

// Global instance pointer for static callbacks
static BTRemote *g_btremote_instance = nullptr;
#define SERVICE_UUID	   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define REQUEST_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESPONSE_CHAR_UUID "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define EVENT_CHAR_UUID	   "d8de624e-140f-4a22-8594-e2216b84a5f2"
#define CONTROL_CHAR_UUID  "f78ebbff-c8b7-4107-93de-889a6a06d408"

// Convert UUID string to esp_bt_uuid_t, handling ESP32's little-endian byte order requirement
static esp_bt_uuid_t stringToUUID128(const char *uuid_str) {
	esp_bt_uuid_t uuid;
	uuid.len = ESP_UUID_LEN_128;

	uint8_t tmp[16];
	int idx = 0;

	for (int i = 0; i < strlen(uuid_str) && idx < 16; i++) {
		char c = uuid_str[i];
		if (c == '-') {
			continue;
		}

		char hex[3] = {uuid_str[i], uuid_str[i + 1], 0};
		tmp[idx++] = (uint8_t) strtol(hex, NULL, 16);
		i++;
	}
	for (int i = 0; i < 16; i++) {
		uuid.uuid.uuid128[i] = tmp[15 - i];
	}

	return uuid;
}

static esp_ble_adv_params_t g_adv_params = {
	.adv_int_min = 0x20,
	.adv_int_max = 0x40,
	.adv_type = ADV_TYPE_IND,
	.own_addr_type = BLE_ADDR_TYPE_PUBLIC,
	.peer_addr = {0},
	.peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
	.channel_map = ADV_CHNL_ALL,
	.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define FLAG_LAST_FRAGMENT 0x01
#define FLAG_ERROR		   0x02

#define APP_REQUEST_TYPE_STOP_PROGRAM	  0x01
#define APP_REQUEST_TYPE_GET_PROJECT_FILE 0x02
#define APP_REQUEST_TYPE_PUT_PROJECT_FILE 0x03
#define APP_REQUEST_TYPE_DELETE_PROJECT	  0x04
#define APP_REQUEST_TYPE_SYNTAX_CHECK	  0x05
#define APP_REQUEST_TYPE_RUN_PROGRAM	  0x06
#define APP_REQUEST_TYPE_GET_PROJECTS	  0x07
#define APP_REQUEST_TYPE_GET_AUTOSTART	  0x08
#define APP_REQUEST_TYPE_PUT_AUTOSTART	  0x09
#define APP_REQUEST_TYPE_READY_FOR_EVENTS 0x0A
#define APP_REQUEST_TYPE_REQUEST_PAIRING  0x0B
#define APP_REQUEST_TYPE_REMOVE_PAIRING	  0x0C
#define APP_REQUEST_TYPE_START_DISCOVERY  0x0D

#define APP_EVENT_TYPE_LOG				0x01
#define APP_EVENT_TYPE_PORTSTATUS		0x02
#define APP_EVENT_TYPE_COMMAND			0x03
#define APP_EVENT_TYPE_BTCLASSICDEVICES 0x04

const uint32_t TASK_LOOP_DELAY_MS = 10;

/**
 * Convert Arduino String to std::vector<uint8_t>
 *
 * @param str Arduino String to convert
 * @param buffer Output vector (will be cleared and filled)
 *
 * Example:
 *   String message = "Hello from ESP32";
 *   std::vector<uint8_t> buffer;
 *   stringToVector(message, buffer);
 *   btRemote.sendEvent(1, buffer);
 */
inline void stringToVector(const String &str, std::vector<uint8_t> &buffer) {
	buffer.clear();
	buffer.reserve(str.length());
	for (size_t i = 0; i < str.length(); i++) {
		buffer.push_back(str[i]);
	}
}

/**
 * Serialize a JsonDocument to std::vector<uint8_t>
 *
 * @param doc JsonDocument to serialize
 * @param buffer Output vector (will be cleared and filled)
 * @return true if serialization successful, false otherwise
 *
 * Example:
 *   JsonDocument doc;
 *   doc["temperature"] = 25.5;
 *   doc["humidity"] = 60;
 *
 *   std::vector<uint8_t> buffer;
 *   if (serializeJsonToVector(doc, buffer)) {
 *       btRemote.sendResponse(messageId, buffer);
 *   }
 */
inline bool serializeJsonToVector(const JsonDocument &doc, std::vector<uint8_t> &buffer) {
	buffer.clear();
	size_t size = measureJson(doc);
	if (size == 0) {
		WARN("JSON document is empty or invalid");
		return false;
	}

	buffer.resize(size);
	size_t written = serializeJson(doc, (char *) buffer.data(), size);

	if (written != size) {
		WARN("Expected %d bytes, wrote %d bytes", size, written);
		buffer.resize(written);
	}

	return written > 0;
}

/**
 * Deserialize std::vector<uint8_t> to JsonDocument
 *
 * @param buffer Input vector containing JSON data
 * @param doc Output JsonDocument (must be sized appropriately)
 * @return DeserializationError (check with error == DeserializationError::Ok)
 *
 * Example:
 *   JsonDocument doc;
 *   DeserializationError error = deserializeVectorToJson(payload, doc);
 *
 *   if (error) {
 *       Serial.printf("JSON parse error: %s\n", error.c_str());
 *   } else {
 *       float temp = doc["temperature"];
 *       int humid = doc["humidity"];
 *   }
 */
inline DeserializationError deserializeVectorToJson(const std::vector<uint8_t> &buffer, JsonDocument &doc) {
	if (buffer.empty()) {
		WARN("Buffer is empty");
		return DeserializationError::EmptyInput;
	}

	DeserializationError error = deserializeJson(doc, (const char *) buffer.data(), buffer.size());
	if (error) {
		WARN("Deserialization error: %s", error.c_str());
	}
	return error;
}

// Create JSON response via lambda and serialize to vector
template <typename Func>
inline bool createJsonResponse(std::vector<uint8_t> &buffer, Func fn) {
	JsonDocument doc;
	fn(doc);
	return serializeJsonToVector(doc, buffer);
}

// Parse JSON from vector and execute callback if successful
template <typename Func>
inline bool parseJsonFromVector(const std::vector<uint8_t> &buffer, Func fn) {
	JsonDocument doc;
	DeserializationError error = deserializeVectorToJson(buffer, doc);

	if (error) {
		return false;
	}

	fn(doc);
	return true;
}

enum class MessageProcessorItemType : uint8_t {
	INCOMING_REQUEST,
	FILE_TRANSFER,
	MTU_NOTIFICATION,
	STREAM_START,
	STREAM_DATA,
	STREAM_END
};

struct PendingFileTransfer {
	uint8_t messageId;
	String project;
	String filename;
};

struct IncomingRequest {
	ProtocolMessageType protocolType;
	uint8_t messageId;
	std::vector<uint8_t> data;
};

// Stream operation data structures (queued for task processing to avoid blocking BLE callback)
struct StreamStartData {
	uint8_t streamId;
	uint32_t totalSize;
	uint16_t chunkCount;
	uint8_t flags;
	String projectId;
	String filename;
};

struct StreamChunkData {
	uint8_t streamId;
	uint16_t chunkIndex;
	uint8_t flags;
	std::vector<uint8_t> payload;
};

struct StreamEndData {
	uint8_t streamId;
	uint32_t reportedBytes;
};

// CRITICAL: Trivially copyable (no destructor) - FreeRTOS queues use memcpy() which bypasses
// C++ copy/move semantics. Memory allocated in factory methods MUST be freed manually in
// processMessageQueue() after processing, OR if xQueueSend() fails!
//
// Memory Ownership Pattern:
// 1. Factory method allocates with 'new' -> caller owns pointer
// 2. If xQueueSend() succeeds -> queue owns pointer, will be freed in processMessageQueue()
// 3. If xQueueSend() fails -> caller MUST delete to prevent leak
struct MessageProcessorItem {
	MessageProcessorItemType type;
	void *dataPtr;

	static MessageProcessorItem createFileTransfer(uint8_t messageId, const String &project, const String &filename) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::FILE_TRANSFER;
		item.dataPtr = new PendingFileTransfer {messageId, project, filename}; // Caller owns, must free on queue failure
		return item;
	}

	static MessageProcessorItem createRequest(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::INCOMING_REQUEST;
		item.dataPtr = new IncomingRequest {protocolType, messageId, data}; // Caller owns, must free on queue failure
		return item;
	}

	static MessageProcessorItem createMTUNotification() {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::MTU_NOTIFICATION;
		item.dataPtr = nullptr; // No allocation, no cleanup needed
		return item;
	}

	static MessageProcessorItem createStreamStart(uint8_t streamId, uint32_t totalSize, uint16_t chunkCount,
		uint8_t flags, const String &projectId, const String &filename) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::STREAM_START;
		item.dataPtr = new StreamStartData {streamId, totalSize, chunkCount, flags, projectId, filename};
		return item;
	}

	static MessageProcessorItem createStreamData(uint8_t streamId, uint16_t chunkIndex, uint8_t flags,
		const uint8_t *payload, size_t payloadLen) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::STREAM_DATA;
		item.dataPtr = new StreamChunkData {streamId, chunkIndex, flags,
			std::vector<uint8_t>(payload, payload + payloadLen)};
		return item;
	}

	static MessageProcessorItem createStreamEnd(uint8_t streamId, uint32_t reportedBytes) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::STREAM_END;
		item.dataPtr = new StreamEndData {streamId, reportedBytes};
		return item;
	}
};

void btMessageProcessorTask(void *param) {
	BTRemote *server = (BTRemote *) param;
	while (true) {

		server->processMessageQueue();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// Bluetooth Classic discovery interval (30 seconds)
const uint32_t BT_DISCOVERY_INTERVAL_MS = 30000;

BTRemote::BTRemote(FS *fs, InputDevices *inputDevices, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuration)
	: gatts_if_(ESP_GATT_IF_NONE)
	, app_id_(0x55)
	, inputDevices_(inputDevices)
	, loggingOutput_(loggingOutput)
	, configuration_(configuration)
	, hub_(hub)
	, deviceConnected_(false)
	, readyForEvents_(false)
	, mtu_(23)
	, nextMessageId_(0)
	, fragmentBuffersMutex_(nullptr)
	, responseQueue_(nullptr)
	, responseSenderTaskHandle_(nullptr)
	, indicationConfirmSemaphore_(nullptr)
	, discoveredDevicesMutex_(nullptr)
	, lastDeviceListPublishTime_(0)
	, discoveryInProgress_(false)
	, pairingInProgress_(false)
	, hidDevicesMutex_(nullptr)
	, hidEventQueue_(nullptr)
	, hidEventTaskHandle_(nullptr)
	, streamsMutex_(nullptr) {
	memset(&pairingDeviceAddress_, 0, sizeof(pairingDeviceAddress_));
	responseQueue_ = xQueueCreate(10, sizeof(MessageProcessorItem));
	fragmentBuffersMutex_ = xSemaphoreCreateMutex();
	indicationConfirmSemaphore_ = xSemaphoreCreateBinary();
	discoveredDevicesMutex_ = xSemaphoreCreateMutex();
	hidDevicesMutex_ = xSemaphoreCreateMutex();
	streamsMutex_ = xSemaphoreCreateMutex();

	// Create HID event queue (10 items max)
	hidEventQueue_ = xQueueCreate(10, sizeof(HIDEventItem));

	memset(&handles_, 0, sizeof(handles_));
	memset(&connState_, 0, sizeof(connState_));
	connState_.connected = false;
	memset(&pairingDeviceAddress_, 0, sizeof(pairingDeviceAddress_));

	g_btremote_instance = this;
}

void BTRemote::begin(const char *deviceName) {
	esp_err_t ret;

	// Initialize BT controller in dual-mode (BLE + Classic for future HID Host).
	// Arduino's btStart() correctly handles dual-mode per sdkconfig (CONFIG_BTDM_CTRL_MODE_BTDM=y)
	INFO("Starting Bluetooth controller in dual-mode (BTDM)...");
	if (!btStart()) {
		ERROR("Bluetooth controller start failed!");
		ERROR("Check that CONFIG_BT_ENABLED=y and CONFIG_BTDM_CTRL_MODE_BTDM=y in sdkconfig");
		return;
	}
	INFO("Bluetooth controller started successfully in dual-mode");

	ret = esp_bluedroid_init();
	if (ret) {
		ERROR("Bluedroid init failed: %s", esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ERROR("Bluedroid enable failed: %s", esp_err_to_name(ret));
		return;
	}

	ret = esp_ble_gap_set_device_name(deviceName);
	if (ret) {
		ERROR("Set device name failed: %s", esp_err_to_name(ret));
	}

	ret = esp_ble_gatt_set_local_mtu(517);
	if (ret) {
		ERROR("Set local MTU failed: %s", esp_err_to_name(ret));
	}

	ret = esp_ble_gatts_register_callback(BTRemote::gattsEventHandler);
	if (ret) {
		ERROR("GATTS register callback failed: %s", esp_err_to_name(ret));
		return;
	}

	ret = esp_ble_gap_register_callback(BTRemote::gapEventHandler);
	if (ret) {
		ERROR("GAP register callback failed: %s", esp_err_to_name(ret));
		return;
	}

	// Register Bluetooth Classic GAP callback for discovery and pairing
	ret = esp_bt_gap_register_callback(BTRemote::gapBTEventHandler);
	if (ret) {
		ERROR("BT GAP register callback failed: %s", esp_err_to_name(ret));
		return;
	}
	INFO("Bluetooth Classic GAP callback registered successfully");

	// Set device to connectable and discoverable for Classic BT
	ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
	if (ret) {
		ERROR("Set BT scan mode failed: %s", esp_err_to_name(ret));
	} else {
		INFO("Bluetooth Classic scan mode set to connectable and discoverable");
	}

	// Configure Simple Secure Pairing (SSP) - auto-accept all pairing requests
	esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE; // No input/output capability
	ret = esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
	if (ret) {
		ERROR("Set IO capability failed: %s", esp_err_to_name(ret));
	} else {
		INFO("Bluetooth Classic IO capability set to NONE (auto-pairing enabled)");
	}

	// Enable auto-connect after pairing
	esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
	uint8_t pin_code_len = 4;
	esp_bt_pin_code_t pin_code = {'0', '0', '0', '0'}; // Default PIN: 0000
	ret = esp_bt_gap_set_pin(pin_type, pin_code_len, pin_code);
	if (ret) {
		WARN("Set BT PIN failed: %s (continuing without PIN)", esp_err_to_name(ret));
	} else {
		INFO("Bluetooth Classic PIN set to default (0000)");
	}

	// CRITICAL: Create HID event processing task BEFORE registering callback
	// This ensures the queue consumer is ready when events start arriving
	INFO("Creating HID event processing task...");
	BaseType_t taskRet = xTaskCreate(
		hidEventTask, // Task function
		"HIDEventTask", // Task name
		4096, // Stack size (4KB)
		this, // Task parameter (this instance)
		5, // Priority (higher than default)
		&hidEventTaskHandle_ // Task handle
	);
	if (taskRet != pdPASS) {
		ERROR("Failed to create HID event task!");
	} else {
		INFO("HID event task created successfully");
	}

	// Initialize HID Host (CRITICAL: callback must be registered BEFORE init)
	INFO("Registering HID Host callback...");
	ret = esp_bt_hid_host_register_callback(BTRemote::hidHostEventHandler);
	if (ret) {
		ERROR("HID Host register callback failed: %s", esp_err_to_name(ret));
	} else {
		INFO("HID Host callback registered successfully");
	}

	// Now initialize HID Host (will trigger ESP_HIDH_INIT_EVT via callback)
	INFO("Initializing HID Host...");
	ret = esp_bt_hid_host_init();
	if (ret) {
		ERROR("HID Host init failed: %s", esp_err_to_name(ret));
	} else {
		INFO("HID Host init function returned successfully");
	}

	ret = esp_ble_gatts_app_register(app_id_);
	if (ret) {
		ERROR("GATTS app register failed: %s", esp_err_to_name(ret));
		return;
	}

	INFO("Bluedroid BLE initialization started, waiting for registration...");

	onRequest([this](uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t> &payload) {
		INFO("Processing BT message with id %d and appRequestType %d, size of payload is %d", messageId, appRequestType, payload.size());

		parseJsonFromVector(payload, [this, messageId, appRequestType](const JsonDocument &requestDoc) {
			std::vector<uint8_t> response;
			if (appRequestType == APP_REQUEST_TYPE_GET_PROJECT_FILE) {
				reqGetProjectFile(messageId, requestDoc);
				return;
			} else {
				createJsonResponse(response, [this, appRequestType, requestDoc](JsonDocument &responseDoc) {
					bool result = false;
					switch (appRequestType) {
						case APP_REQUEST_TYPE_STOP_PROGRAM:
							result = reqStopProgram(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_PUT_PROJECT_FILE:
							result = reqPutProjectFile(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_DELETE_PROJECT:
							result = reqDeleteProject(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_SYNTAX_CHECK:
							result = reqSyntaxCheck(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_RUN_PROGRAM:
							result = reqRun(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_GET_PROJECTS:
							result = reqGetProjects(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_GET_AUTOSTART:
							result = reqGetAutostart(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_PUT_AUTOSTART:
							result = reqPutAutostart(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_READY_FOR_EVENTS:
							result = reqReadyForEvents(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_REQUEST_PAIRING:
							result = reqRequestPairing(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_REMOVE_PAIRING:
							result = reqRemovePairing(requestDoc, responseDoc);
							break;
						case APP_REQUEST_TYPE_START_DISCOVERY:
							result = reqStartDiscovery(requestDoc, responseDoc);
							break;
						default:
							WARN("Not supported appRequestType: %d", appRequestType);
							responseDoc["error"] = "Not supported appRequestType!";
							break;
					}
					responseDoc["result"] = result;
				});
			}

			sendResponse(messageId, response);
		});
	});

	xTaskCreate(
		btMessageProcessorTask,
		"BTMsgProcessor",
		8192,
		(void *) this,
		2,
		&responseSenderTaskHandle_);

	INFO("BLE Server started as ID %s, waiting for connections...", SERVICE_UUID);
}

void BTRemote::handleFragment(const uint8_t *data, size_t length) {
	ProtocolMessageType protocolType = (ProtocolMessageType) data[0];

	// Route streaming protocol messages to dedicated handlers (bypass fragment reassembly)
	if (protocolType == ProtocolMessageType::STREAM_START) {
		handleStreamStart(data, length);
		return;
	} else if (protocolType == ProtocolMessageType::STREAM_DATA) {
		handleStreamData(data, length);
		return;
	} else if (protocolType == ProtocolMessageType::STREAM_END) {
		handleStreamEnd(data, length);
		return;
	}

	uint8_t messageId = data[1];
	uint16_t fragmentNum = (data[2] << 8) | data[3];
	uint8_t flags = data[4];

	DEBUG("Fragment received: ProtocolType=%d, ID=%d, Num=%d, Flags=0x%02X",
		(uint8_t) protocolType, messageId, fragmentNum, flags);

	// CRITICAL: Lock fragmentBuffers_ for thread-safe access (GATTS callback vs loop task)
	if (xSemaphoreTake(fragmentBuffersMutex_, portMAX_DELAY) != pdTRUE) {
		ERROR("Failed to take fragmentBuffersMutex_");
		return;
	}

	// Memory safety: Limit concurrent messages to prevent memory exhaustion
	if (fragmentBuffers_.size() >= MAX_CONCURRENT_MESSAGES && fragmentBuffers_.find(messageId) == fragmentBuffers_.end()) {
		WARN("Max concurrent messages reached (%d), rejecting message ID %d",
			MAX_CONCURRENT_MESSAGES, messageId);
		xSemaphoreGive(fragmentBuffersMutex_);
		sendControlMessage(ControlMessageType::BUFFER_FULL, messageId);
		return;
	}

	auto &buffer = fragmentBuffers_[messageId];

	if (fragmentNum == 0) {
		buffer.data.clear();
		buffer.lastFragmentTime = millis();
		buffer.receivedFragments = 0;
		buffer.protocolType = protocolType;
	}

	if (millis() - buffer.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
		WARN("Fragment timeout for message ID %d", messageId);
		fragmentBuffers_.erase(messageId);
		xSemaphoreGive(fragmentBuffersMutex_);
		sendControlMessage(ControlMessageType::NACK, messageId);
		return;
	}

	buffer.lastFragmentTime = millis();

	const uint8_t *payload = data + FRAGMENT_HEADER_SIZE;
	size_t payloadSize = length - FRAGMENT_HEADER_SIZE;

	if (buffer.data.size() + payloadSize > MAX_BUFFER_SIZE) {
		WARN("Buffer overflow for message ID %d", messageId);
		fragmentBuffers_.erase(messageId);
		xSemaphoreGive(fragmentBuffersMutex_);
		sendControlMessage(ControlMessageType::BUFFER_FULL, messageId);
		return;
	}

	// CRITICAL: Catch memory allocation failures to prevent abort()
	// std::vector::insert() can throw std::bad_alloc if heap is exhausted or fragmented
	try {
		buffer.data.insert(buffer.data.end(), payload, payload + payloadSize);
		buffer.receivedFragments++;
	} catch (const std::bad_alloc &e) {
		ERROR("Out of memory inserting %d bytes into fragment buffer (message ID %d)", payloadSize, messageId);
		ERROR("Heap free: %d bytes, largest block: %d bytes",
			esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
		fragmentBuffers_.erase(messageId);
		xSemaphoreGive(fragmentBuffersMutex_);
		sendControlMessage(ControlMessageType::NACK, messageId);
		return;
	}

	if (flags & FLAG_LAST_FRAGMENT) {
		DEBUG("Complete message received: ID=%d, Size=%d",
			messageId, buffer.data.size());

		// CRITICAL: Queue request for processing in separate task to prevent deadlock
		// (callback must not block waiting for indication ACK)
		// Also catch allocation failures when creating the request (copies buffer.data vector)
		try {
			MessageProcessorItem item = MessageProcessorItem::createRequest(protocolType, messageId, buffer.data);

			if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
				ERROR("Message processor queue full, dropping request");
				// CRITICAL: Free allocated memory on queue send failure to prevent memory leak
				delete static_cast<IncomingRequest *>(item.dataPtr);
				sendControlMessage(ControlMessageType::NACK, messageId);
			} else {
				DEBUG("Request queued for processing in background task");
				sendControlMessage(ControlMessageType::ACK, messageId);
			}
		} catch (const std::bad_alloc &e) {
			ERROR("Out of memory creating request for message ID %d (%d bytes)",
				messageId, buffer.data.size());
			ERROR("Heap free: %d bytes, largest block: %d bytes",
				esp_get_free_heap_size(), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
			sendControlMessage(ControlMessageType::NACK, messageId);
		}

		fragmentBuffers_.erase(messageId);
	}

	xSemaphoreGive(fragmentBuffersMutex_);
}

void BTRemote::processCompleteMessage(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data) {
	if (protocolType == ProtocolMessageType::REQUEST && data.size() > 0) {
		uint8_t appRequestType = data[0];
		std::vector<uint8_t> payload(data.begin() + 1, data.end());

		DEBUG("Processing request AppRequestType=%d, Payload-Size=%d",
			appRequestType, payload.size());

		if (onRequestCallback_) {
			onRequestCallback_(appRequestType, messageId, payload);
		}
	}
}

void BTRemote::handleControlMessage(const uint8_t *data, size_t length) {
	if (length < 2) {
		return;
	}

	ControlMessageType ctrlType = (ControlMessageType) data[0];
	uint8_t messageId = data[1];

	DEBUG("Control message: Type=%d, ID=%d", (uint8_t) ctrlType, messageId);

	switch (ctrlType) {
		case ControlMessageType::RETRY:
			WARN("Client requested retry for message ID %d", messageId);
			break;
		case ControlMessageType::RESET:
			INFO("Client requested reset");
			if (xSemaphoreTake(fragmentBuffersMutex_, portMAX_DELAY) == pdTRUE) {
				fragmentBuffers_.clear();
				xSemaphoreGive(fragmentBuffersMutex_);
			}
			break;
		default:
			break;
	}
}

// ============================================================================
// Streaming File Transfer Handlers
// ============================================================================

void BTRemote::handleStreamStart(const uint8_t *data, size_t length) {
	// Minimum header: ProtocolType(1) + StreamId(1) + TotalSize(4) + ChunkCount(2) + Flags(1) = 9 bytes
	if (length < 9) {
		WARN("STREAM_START too short: %d bytes", length);
		sendStreamError(0, 0x01);
		return;
	}

	uint8_t streamId = data[1];
	uint32_t totalSize = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
	uint16_t chunkCount = data[6] | (data[7] << 8);
	uint8_t flags = data[8];

	// Parse metadata JSON (starts at byte 9) - fast operation, OK in callback
	const char *metadataJson = (const char *) &data[9];
	size_t metadataLen = length - 9;

	JsonDocument metaDoc;
	DeserializationError err = deserializeJson(metaDoc, metadataJson, metadataLen);
	if (err) {
		WARN("STREAM_START invalid metadata JSON: %s", err.c_str());
		sendStreamError(streamId, 0x02);
		return;
	}

	String projectId = metaDoc["project"].as<String>();
	String filename = metaDoc["filename"].as<String>();

	if (projectId.isEmpty() || filename.isEmpty()) {
		WARN("STREAM_START missing project or filename");
		sendStreamError(streamId, 0x02);
		return;
	}

	DEBUG("STREAM_START received: id=%d, project=%s, file=%s, size=%d, chunks=%d - queueing for processing",
		streamId, projectId.c_str(), filename.c_str(), totalSize, chunkCount);

	// Queue for task processing to avoid blocking BLE callback
	MessageProcessorItem item = MessageProcessorItem::createStreamStart(
		streamId, totalSize, chunkCount, flags, projectId, filename);

	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("STREAM_START queue full, dropping");
		delete static_cast<StreamStartData *>(item.dataPtr);
		sendStreamError(streamId, 0x04);
	}
}

void BTRemote::handleStreamData(const uint8_t *data, size_t length) {
	// Header: ProtocolType(1) + StreamId(1) + ChunkIndex(2) + Flags(1) = 5 bytes
	if (length < 5) {
		return;
	}

	uint8_t streamId = data[1];
	uint16_t chunkIndex = data[2] | (data[3] << 8);
	uint8_t flags = data[4];

	const uint8_t *payload = data + 5;
	size_t payloadLen = length - 5;

	// Queue for task processing - SD card I/O must NOT block BLE callback
	MessageProcessorItem item = MessageProcessorItem::createStreamData(
		streamId, chunkIndex, flags, payload, payloadLen);

	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("STREAM_DATA queue full, dropping chunk %d for stream %d", chunkIndex, streamId);
		delete static_cast<StreamChunkData *>(item.dataPtr);
		// Don't send error here - client will timeout and retry
	}
}

void BTRemote::handleStreamEnd(const uint8_t *data, size_t length) {
	// Header: ProtocolType(1) + StreamId(1) + BytesWritten(4) = 6 bytes
	if (length < 6) {
		return;
	}

	uint8_t streamId = data[1];
	uint32_t reportedBytes = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);

	// Queue for task processing for consistency with other stream operations
	MessageProcessorItem item = MessageProcessorItem::createStreamEnd(streamId, reportedBytes);

	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("STREAM_END queue full for stream %d", streamId);
		delete static_cast<StreamEndData *>(item.dataPtr);
	}
}

void BTRemote::sendStreamAck(uint8_t streamId, uint16_t chunkIndex) {
	if (!deviceConnected_) {
		return;
	}

	// STREAM_ACK format: ControlType(1) + StreamId(1) + ChunkIndex(2) = 4 bytes
	uint8_t ackData[4];
	ackData[0] = (uint8_t) ControlMessageType::STREAM_ACK;
	ackData[1] = streamId;
	ackData[2] = chunkIndex & 0xFF;
	ackData[3] = (chunkIndex >> 8) & 0xFF;

	esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_value_handle, sizeof(ackData), ackData, false);
}

void BTRemote::sendStreamError(uint8_t streamId, uint8_t errorCode) {
	if (!deviceConnected_) {
		return;
	}

	// STREAM_ERROR format: ControlType(1) + StreamId(1) + ErrorCode(1) = 3 bytes
	uint8_t errData[3];
	errData[0] = (uint8_t) ControlMessageType::STREAM_ERROR;
	errData[1] = streamId;
	errData[2] = errorCode;

	esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_value_handle, sizeof(errData), errData, false);

	WARN("Stream %d error sent: code=0x%02X", streamId, errorCode);
}

void BTRemote::cleanupTimedOutStreams() {
	if (xSemaphoreTake(streamsMutex_, pdMS_TO_TICKS(50)) != pdTRUE) {
		return;
	}

	uint32_t now = millis();
	std::vector<uint8_t> toRemove;

	for (auto &pair : activeStreams_) {
		if (now - pair.second.lastActivityTime > STREAM_TIMEOUT_MS) {
			// Note: Incomplete file remains on SD card but will be overwritten on next upload
			// (writeFileChunkToProject with position=0 truncates existing file)
			WARN("Stream %d timed out (%s/%s), cleaning up",
				pair.first, pair.second.projectId.c_str(), pair.second.filename.c_str());
			toRemove.push_back(pair.first);
		}
	}

	for (uint8_t id : toRemove) {
		activeStreams_.erase(id);
	}

	xSemaphoreGive(streamsMutex_);
}

// ============================================================================

bool BTRemote::sendLargeResponse(uint8_t messageId, size_t totalSize, std::function<int(const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer)> chunkProvider) {
	if (!deviceConnected_) {
		WARN("No client connected");
		return false;
	}

	size_t payloadSize = min((size_t) 512, mtu_) - FRAGMENT_HEADER_SIZE;
	size_t totalFragments = (totalSize + payloadSize - 1) / payloadSize;

	if (totalFragments == 0) {
		totalFragments = 1;
	}

	DEBUG("Sending fragmented: ID=%d, Size=%d, Fragments=%d",
		messageId, totalSize, totalFragments);

	size_t remaining = totalSize;
	for (int i = 0; i < totalFragments; i++) {
		DEBUG("Sending fragment %d/%d with MTU %d and payload size %d", i + 1, totalFragments, mtu_, payloadSize);
		std::vector<uint8_t> fragment;
		fragment.reserve(payloadSize);

		fragment.push_back((uint8_t) ProtocolMessageType::RESPONSE);
		fragment.push_back(messageId);
		fragment.push_back((i >> 8) & 0xFF);
		fragment.push_back(i & 0xFF);

		uint8_t flags = (i == totalFragments - 1) ? FLAG_LAST_FRAGMENT : 0x00;
		fragment.push_back(flags);

		int read = chunkProvider(i, payloadSize, fragment);
		DEBUG("Read %d bytes for chunk with index %d, fragment size is %d, flags is %d", read, i, fragment.size(), flags);

		// Clear semaphore before sending to handle race (confirmation may arrive before xSemaphoreTake)
		xSemaphoreTake(indicationConfirmSemaphore_, 0);

		esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
			handles_.response_char_handle, fragment.size(), fragment.data(), true);

		if (ret != ESP_OK) {
			ERROR("Failed to indicate fragment %d of %d: %s", i + 1, totalFragments, esp_err_to_name(ret));
			return false;
		}

		DEBUG("Indicate sent, waiting for confirmation for fragment %d/%d", i + 1, totalFragments);

		// Wait for indication confirmation (ESP_GATTS_CONF_EVT) with 1s timeout for poor RF
		if (xSemaphoreTake(indicationConfirmSemaphore_, pdMS_TO_TICKS(1000)) == pdTRUE) {
			DEBUG("Confirmation received for fragment %d/%d", i + 1, totalFragments);
		} else {
			WARN("Confirmation timeout for fragment %d of %d", i + 1, totalFragments);
			return false;
		}
	}

	return true;
}

bool BTRemote::sendFragmented(uint16_t char_handle, ProtocolMessageType protocolType,
	uint8_t messageId, const std::vector<uint8_t> &data) {
	if (!deviceConnected_) {
		WARN("No client connected");
		return false;
	}

	size_t payloadSize = min((size_t) 512, mtu_) - FRAGMENT_HEADER_SIZE;
	size_t totalFragments = (data.size() + payloadSize - 1) / payloadSize;

	if (totalFragments == 0) {
		totalFragments = 1;
	}

	DEBUG("Sending fragmented: ProtocolType=%d, ID=%d, Size=%d, Fragments=%d",
		(uint8_t) protocolType, messageId, data.size(), totalFragments);

	for (size_t i = 0; i < totalFragments; i++) {
		DEBUG("Sending fragment %d with MTU %d and payload size %d", i, mtu_, payloadSize);
		std::vector<uint8_t> fragment;
		fragment.reserve(payloadSize);

		fragment.push_back((uint8_t) protocolType);
		fragment.push_back(messageId);
		fragment.push_back((i >> 8) & 0xFF);
		fragment.push_back(i & 0xFF);

		uint8_t flags = (i == totalFragments - 1) ? FLAG_LAST_FRAGMENT : 0x00;
		fragment.push_back(flags);

		size_t start = i * payloadSize;
		size_t end = std::min(start + payloadSize, data.size());

		if (start < data.size()) {
			fragment.insert(fragment.end(), data.begin() + start, data.begin() + end);
		}

		// Clear semaphore before sending to handle race (confirmation may arrive before xSemaphoreTake)
		xSemaphoreTake(indicationConfirmSemaphore_, 0);

		DEBUG("Sending indicate: gatts_if=%d, conn_id=%d, handle=%d, len=%d, protocolType=%d, msgId=%d",
			gatts_if_, connState_.conn_id, char_handle, fragment.size(), (uint8_t) protocolType, messageId);

		esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
			char_handle, fragment.size(), fragment.data(), true);

		if (ret != ESP_OK) {
			ERROR("Failed to indicate fragment %d of %d: %s", i + 1, totalFragments, esp_err_to_name(ret));
			return false;
		}

		DEBUG("Indicate sent, waiting for confirmation for fragment %d/%d", i + 1, totalFragments);

		// Wait for indication confirmation (ESP_GATTS_CONF_EVT) with 1s timeout for poor RF
		if (xSemaphoreTake(indicationConfirmSemaphore_, pdMS_TO_TICKS(1000)) == pdTRUE) {
			DEBUG("Confirmation received for fragment %d/%d", i + 1, totalFragments);
		} else {
			WARN("Confirmation timeout for fragment %d of %d", i + 1, totalFragments);
			return false;
		}
	}

	return true;
}

bool BTRemote::sendResponse(uint8_t messageId, const std::vector<uint8_t> &data) {
	DEBUG("sendResponse: msgId=%d, handle=%d (response=%d, event=%d)",
		messageId, handles_.response_char_handle, handles_.response_char_handle, handles_.event_char_handle);
	return sendFragmented(handles_.response_char_handle, ProtocolMessageType::RESPONSE, messageId, data);
}

// Called from background task context (publishLogMessages/publishCommands/publishPortstatus), not ISR
bool BTRemote::sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data) {
	std::vector<uint8_t> eventData;
	eventData.reserve(data.size() + 1);
	eventData.push_back(appEventType);
	eventData.insert(eventData.end(), data.begin(), data.end());

	return sendFragmented(handles_.event_char_handle, ProtocolMessageType::EVENT, nextMessageId_++, eventData);
}

void BTRemote::sendControlMessage(ControlMessageType type, uint8_t messageId) {
	if (!deviceConnected_) {
		return;
	}

	uint8_t data[2] = {(uint8_t) type, messageId};
	esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_handle, 2, data, false);

	if (ret != ESP_OK) {
		ERROR("Failed to send control message type %d: %s", (uint8_t) type, esp_err_to_name(ret));
	}
}

void BTRemote::sendMTUNotification() {
	if (!deviceConnected_) {
		return;
	}

	uint8_t data[4];
	data[0] = (uint8_t) ControlMessageType::MTU_INFO;
	data[1] = 0;
	data[2] = (mtu_ >> 8) & 0xFF;
	data[3] = mtu_ & 0xFF;

	esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_handle, 4, data, false);

	if (ret != ESP_OK) {
		ERROR("Failed to send MTU notification: %s", esp_err_to_name(ret));
	} else {
		INFO("MTU-Info sent: %d bytes", mtu_);
	}
}

void BTRemote::onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback) {
	onRequestCallback_ = callback;
}

void BTRemote::loop() {
	uint32_t now = millis();

	// Fragment timeout cleanup
	// CRITICAL: Lock fragmentBuffers_ for thread-safe access (loop task vs GATTS callback)
	if (xSemaphoreTake(fragmentBuffersMutex_, portMAX_DELAY) == pdTRUE) {
		for (auto it = fragmentBuffers_.begin(); it != fragmentBuffers_.end();) {
			if (now - it->second.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
				WARN("Timeout: Message ID %d removed", it->first);
				sendControlMessage(ControlMessageType::NACK, it->first);
				it = fragmentBuffers_.erase(it);
			} else {
				++it;
			}
		}
		xSemaphoreGive(fragmentBuffersMutex_);
	}

	// Streaming file transfer timeout cleanup (every 10 seconds)
	static uint32_t lastStreamCleanup = 0;
	if (now - lastStreamCleanup > 10000) {
		cleanupTimedOutStreams();
		lastStreamCleanup = now;
	}
}

bool BTRemote::isConnected() const {
	return deviceConnected_;
}

size_t BTRemote::getMTU() const {
	return mtu_;
}

void BTRemote::publishLogMessages() {
	String logMessage = loggingOutput_->waitForLogMessage(0);
	while (logMessage.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {
			std::vector<uint8_t> response(logMessage.begin(), logMessage.end());
			sendEvent(APP_EVENT_TYPE_LOG, response);
		}

		logMessage = loggingOutput_->waitForLogMessage(0);
	}
}

void BTRemote::publishCommands() {
	String command = Commands::instance()->waitForCommand(0);
	while (command.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(command, response);

			sendEvent(APP_EVENT_TYPE_COMMAND, response);
		}

		command = Commands::instance()->waitForCommand(0);
	}
}

void BTRemote::publishPortstatus() {
	String status = Portstatus::instance()->waitForStatus(0);
	while (status.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(status, response);

			sendEvent(APP_EVENT_TYPE_PORTSTATUS, response);
		}

		status = Portstatus::instance()->waitForStatus(0);
	}
}

void BTRemote::publishBTClassicDevices() {
	uint32_t now = millis();

	// Publish device list every 30 seconds (same interval as discovery)
	if ((now - lastDeviceListPublishTime_) < BT_DISCOVERY_INTERVAL_MS) {
		return; // Not yet time to publish
	}

	// Thread-safe access to discovered devices
	if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(10)) != pdTRUE) {
		// Mutex not available, skip this cycle to avoid blocking
		return;
	}

	// Create JSON array of discovered devices
	std::vector<uint8_t> response;
	createJsonResponse(response, [this](JsonDocument &responseDoc) {
		JsonArray devices = responseDoc["devices"].to<JsonArray>();

		for (const auto &pair : discoveredDevices_) {
			const BTClassicDevice &device = pair.second;

			JsonObject deviceObj = devices.add<JsonObject>();
			deviceObj["mac"] = bdAddrToString(device.address);
			deviceObj["name"] = String(device.name);
			deviceObj["type"] = static_cast<uint8_t>(device.deviceType);
			deviceObj["paired"] = device.paired;
			deviceObj["rssi"] = device.rssi;
			deviceObj["cod"] = device.cod;
		}

		responseDoc["count"] = discoveredDevices_.size();
		responseDoc["discoveryActive"] = discoveryInProgress_;
	});

	xSemaphoreGive(discoveredDevicesMutex_);

	// Send event with device list
	INFO("Publishing Bluetooth Classic device list: %d devices", discoveredDevices_.size());
	sendEvent(APP_EVENT_TYPE_BTCLASSICDEVICES, response);

	lastDeviceListPublishTime_ = now;
}

bool BTRemote::reqStopProgram(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	return hub_->stopLUACode();
}

bool BTRemote::reqGetProjectFile(uint8_t messageId, const JsonDocument &requestDoc) {
	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();

	INFO("Queueing file transfer request for %s of project %s", filename.c_str(), project.c_str());

	MessageProcessorItem item = MessageProcessorItem::createFileTransfer(messageId, project, filename);
	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("Message processor queue full, dropping file transfer request");
		// CRITICAL: Free allocated memory on queue send failure to prevent memory leak
		delete static_cast<PendingFileTransfer *>(item.dataPtr);
		return false;
	}

	return true;
}

void BTRemote::processMessageQueue() {
	MessageProcessorItem item;

	if (xQueueReceive(responseQueue_, &item, 0) == pdTRUE) {

		if (item.type == MessageProcessorItemType::INCOMING_REQUEST) {
			IncomingRequest *req = static_cast<IncomingRequest *>(item.dataPtr);
			DEBUG("Processing incoming request: msgId=%d, protocolType=%d, size=%d",
				req->messageId, (int) req->protocolType, req->data.size());

			processCompleteMessage(req->protocolType, req->messageId, req->data);

			// CRITICAL: Manual free required - FreeRTOS queues use memcpy, bypassing destructors
			delete req;

		} else if (item.type == MessageProcessorItemType::FILE_TRANSFER) {
			PendingFileTransfer *transfer = static_cast<PendingFileTransfer *>(item.dataPtr);
			DEBUG("Processing queued file transfer for %s of project %s",
				transfer->filename.c_str(), transfer->project.c_str());

			File projectFile = configuration_->getProjectFile(transfer->project, transfer->filename);
			if (projectFile) {
				size_t projectSize = projectFile.size();
				DEBUG("Response has size %d", projectSize);

				sendLargeResponse(transfer->messageId, projectSize, [&projectFile](const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer) {
					DEBUG("Reading chunk with index %d from file", index);
					char buffer[maxChunkSize];
					size_t read = projectFile.readBytes(&buffer[0], maxChunkSize);
					for (int i = 0; i < read; i++) {
						dataContainer.push_back(buffer[i]);
					}
					DEBUG("Read %d bytes from file for chunk %d", read, index);
					return read;
				});

				projectFile.close();
			} else {
				WARN("File not found: %s/%s", transfer->project.c_str(), transfer->filename.c_str());
			}

			// CRITICAL: Manual free required - FreeRTOS queues use memcpy, bypassing destructors
			delete transfer;

		} else if (item.type == MessageProcessorItemType::MTU_NOTIFICATION) {
			DEBUG("Processing MTU notification from task context");
			sendMTUNotification();

		} else if (item.type == MessageProcessorItemType::STREAM_START) {
			StreamStartData *startData = static_cast<StreamStartData *>(item.dataPtr);
			DEBUG("Processing STREAM_START: id=%d, project=%s, file=%s",
				startData->streamId, startData->projectId.c_str(), startData->filename.c_str());

			if (xSemaphoreTake(streamsMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
				// Check concurrent stream limit
				if (activeStreams_.size() >= MAX_CONCURRENT_STREAMS) {
					xSemaphoreGive(streamsMutex_);
					WARN("STREAM_START max concurrent streams reached");
					sendStreamError(startData->streamId, 0x04);
				} else {
					// Initialize stream state
					ActiveStreamTransfer stream;
					stream.streamId = startData->streamId;
					stream.state = StreamState::RECEIVING;
					stream.projectId = startData->projectId;
					stream.filename = startData->filename;
					stream.totalSize = startData->totalSize;
					stream.bytesReceived = 0;
					stream.expectedChunks = startData->chunkCount;
					stream.receivedChunks = 0;
					stream.lastActivityTime = millis();

					activeStreams_[startData->streamId] = std::move(stream);
					xSemaphoreGive(streamsMutex_);

					// Send ACK for stream start (0xFFFF indicates start ACK)
					sendStreamAck(startData->streamId, 0xFFFF);
					INFO("Stream %d started: %s/%s (%d bytes expected)",
						startData->streamId, startData->projectId.c_str(),
						startData->filename.c_str(), startData->totalSize);
				}
			} else {
				WARN("STREAM_START failed to take streamsMutex_");
				sendStreamError(startData->streamId, 0x03);
			}

			delete startData;

		} else if (item.type == MessageProcessorItemType::STREAM_DATA) {
			StreamChunkData *chunkData = static_cast<StreamChunkData *>(item.dataPtr);

			if (xSemaphoreTake(streamsMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
				auto it = activeStreams_.find(chunkData->streamId);
				if (it == activeStreams_.end()) {
					xSemaphoreGive(streamsMutex_);
					WARN("STREAM_DATA unknown stream ID: %d", chunkData->streamId);
					sendStreamError(chunkData->streamId, 0x06);
				} else {
					ActiveStreamTransfer &stream = it->second;
					stream.lastActivityTime = millis();

					// SD card I/O - safe in task context
					bool success = configuration_->writeFileChunkToProject(
						stream.projectId,
						stream.filename,
						stream.bytesReceived,
						(uint8_t *) chunkData->payload.data(),
						chunkData->payload.size()
					);

					if (!success) {
						stream.state = StreamState::ERROR;
						xSemaphoreGive(streamsMutex_);
						WARN("STREAM_DATA writeFileChunkToProject failed for stream %d", chunkData->streamId);
						sendStreamError(chunkData->streamId, 0x07);
					} else {
						stream.bytesReceived += chunkData->payload.size();
						stream.receivedChunks++;
						xSemaphoreGive(streamsMutex_);

						// Send ACK for this chunk
						sendStreamAck(chunkData->streamId, chunkData->chunkIndex);

						if (chunkData->flags & 0x01) {
							DEBUG("Stream %d: last chunk received, waiting for STREAM_END", chunkData->streamId);
						}
					}
				}
			} else {
				WARN("STREAM_DATA failed to take streamsMutex_");
			}

			delete chunkData;

		} else if (item.type == MessageProcessorItemType::STREAM_END) {
			StreamEndData *endData = static_cast<StreamEndData *>(item.dataPtr);

			if (xSemaphoreTake(streamsMutex_, portMAX_DELAY) == pdTRUE) {
				auto it = activeStreams_.find(endData->streamId);
				if (it == activeStreams_.end()) {
					xSemaphoreGive(streamsMutex_);
					WARN("STREAM_END unknown stream ID: %d", endData->streamId);
				} else {
					ActiveStreamTransfer &stream = it->second;

					// Verify bytes received match
					if (stream.bytesReceived != endData->reportedBytes) {
						WARN("Stream %d: byte mismatch! Expected %d, received %d",
							endData->streamId, endData->reportedBytes, stream.bytesReceived);
					}

					INFO("Stream %d complete: %s/%s (%d bytes written)",
						endData->streamId, stream.projectId.c_str(),
						stream.filename.c_str(), stream.bytesReceived);

					// Clean up
					activeStreams_.erase(it);
					xSemaphoreGive(streamsMutex_);

					// Send completion ACK (0xFFFE indicates completion)
					sendStreamAck(endData->streamId, 0xFFFE);
				}
			}

			delete endData;
		}
	}

	// Serialize all BLE communication through this task to prevent race conditions
	if (deviceConnected_ && readyForEvents_ && !pairingInProgress_) {
		publishLogMessages();
		publishCommands();
		publishPortstatus();
		publishBTClassicDevices();
	}
}

bool BTRemote::reqPutProjectFile(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();
	String content = requestDoc["content"].as<String>();

	INFO("Saving file %s to project %s", filename.c_str(), project.c_str());
	return configuration_->writeProjectFileContent(project, filename, content);
}

bool BTRemote::reqDeleteProject(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String project = requestDoc["project"].as<String>();

	configuration_->deleteProject(project);

	return true;
}

bool BTRemote::reqSyntaxCheck(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String luaScript = requestDoc["luaScript"].as<String>();

	LuaCheckResult result = hub_->checkLUACode(luaScript);
	responseDoc["parseTime"] = result.parseTime;
	responseDoc["errorMessage"] = String(result.errorMessage.c_str());

	return result.success;
}

bool BTRemote::reqRun(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String luaScript = requestDoc["luaScript"].as<String>();

	hub_->executeLUACode(luaScript);
	return true;
}

bool BTRemote::reqGetProjects(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	JsonArray projectList = responseDoc["projects"].to<JsonArray>();

	std::vector<String> foundProjects = configuration_->getProjects();
	INFO("%d projects found", foundProjects.size());
	for (size_t i = 0; i < foundProjects.size(); i++) {
		JsonObject entry = projectList.add<JsonObject>();
		entry["name"] = String(foundProjects[i]);
	}

	return true;
}

bool BTRemote::reqGetAutostart(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String autostartProject = configuration_->getAutostartProject();
	if (autostartProject.length() > 0) {
		responseDoc["project"] = autostartProject;
		return true;
	}
	WARN("No autostart project found");
	return false;
}

bool BTRemote::reqPutAutostart(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	if (configuration_->setAutostartProject(requestDoc["project"].as<String>())) {
		return true;
	}

	WARN("Could not set autostart project!");
	return false;
}

bool BTRemote::reqReadyForEvents(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	INFO("Client notified that it is ready to receive events!");
	readyForEvents_ = true;
	return true;
}

bool BTRemote::reqRequestPairing(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String macId = requestDoc["mac"].as<String>();
	INFO("Client requested a pairing for MAC %s", macId.c_str());

	this->startPairing(macId.c_str());

	return true;
}

bool BTRemote::reqRemovePairing(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	String macId = requestDoc["mac"].as<String>();
	INFO("Client wans to remove pairing for MAC %s", macId.c_str());

	this->removePairing(macId.c_str());

	return true;
}

bool BTRemote::reqStartDiscovery(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	if (discoveryInProgress_) {
		WARN("Discovery already in progress, skipping");
		return false;
	}

	INFO("Starting Bluetooth Classic device discovery (mode: ESP_BT_INQ_MODE_GENERAL_INQUIRY)");

	esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
	if (ret != ESP_OK) {
		ERROR("Start discovery failed: %s", esp_err_to_name(ret));
		discoveryInProgress_ = false;
		return false;
	} else {
		INFO("Discovery started successfully");
		discoveryInProgress_ = true;
	}

	return true;
}

// ============================================================================
// Bluetooth Classic Helper Methods
// ============================================================================

std::string BTRemote::bdAddrToString(const esp_bd_addr_t address) {
	char addr_str[18];
	snprintf(addr_str, sizeof(addr_str), "%02X:%02X:%02X:%02X:%02X:%02X",
		address[0], address[1], address[2], address[3], address[4], address[5]);
	return std::string(addr_str);
}

void BTRemote::stringToBdAddr(const std::string &str, esp_bd_addr_t address) {
	unsigned int addr[6];
	if (sscanf(str.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
			&addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5])
		== 6) {
		for (int i = 0; i < 6; i++) {
			address[i] = (uint8_t) addr[i];
		}
	} else {
		ERROR("Invalid MAC address format: %s", str.c_str());
		memset(address, 0, sizeof(esp_bd_addr_t));
	}
}

BTClassicDeviceType BTRemote::classifyDevice(uint32_t cod) {
	// Class of Device (CoD) - Major Device Class is bits 8-12
	uint8_t major = (cod >> 8) & 0x1F;
	uint8_t minor = (cod >> 2) & 0x3F;

	DEBUG("Classifying device: CoD=0x%06X, Major=0x%02X, Minor=0x%02X", cod, major, minor);

	switch (major) {
		case 0x01:
			return BTClassicDeviceType::COMPUTER;
		case 0x02:
			return BTClassicDeviceType::PHONE;
		case 0x04:
			return BTClassicDeviceType::AUDIO;
		case 0x05: // Peripheral (HID devices)
			// Check minor device class for specific peripheral types
			if ((minor & 0x0C) == 0x04) {
				return BTClassicDeviceType::KEYBOARD;
			}
			if ((minor & 0x0C) == 0x08) {
				return BTClassicDeviceType::MOUSE;
			}
			if ((minor & 0x0C) == 0x0C) {
				return BTClassicDeviceType::JOYSTICK;
			}
			if (minor & 0x10) {
				return BTClassicDeviceType::GAMEPAD; // Gamepad/Gaming control
			}
			return BTClassicDeviceType::PERIPHERAL;
		case 0x06:
			return BTClassicDeviceType::IMAGING;
		case 0x07:
			return BTClassicDeviceType::WEARABLE;
		case 0x08:
			return BTClassicDeviceType::TOY;
		case 0x09:
			return BTClassicDeviceType::HEALTH;
		default:
			INFO("Unknown device class: 0x%02X", major);
			return BTClassicDeviceType::UNKNOWN;
	}
}

void BTRemote::stopDiscoveryInternal() {
	if (!discoveryInProgress_) {
		return;
	}

	INFO("Stopping Bluetooth Classic device discovery");

	esp_err_t ret = esp_bt_gap_cancel_discovery();
	if (ret != ESP_OK) {
		ERROR("Stop discovery failed: %s", esp_err_to_name(ret));
	} else {
		INFO("Discovery stopped successfully");
	}
	discoveryInProgress_ = false;
}

// ============================================================================
// Public API Methods - Bluetooth Classic Discovery
// ============================================================================

std::vector<BTClassicDevice> BTRemote::getDiscoveredDevices() {
	std::vector<BTClassicDevice> devices;

	if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		INFO("Retrieving %d discovered Bluetooth Classic devices", discoveredDevices_.size());
		for (const auto &pair : discoveredDevices_) {
			devices.push_back(pair.second);
		}
		xSemaphoreGive(discoveredDevicesMutex_);
	} else {
		WARN("Failed to acquire mutex for discovered devices");
	}

	return devices;
}

bool BTRemote::isDiscoveryInProgress() const {
	return discoveryInProgress_;
}

void BTRemote::clearDiscoveredDevices() {
	if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		INFO("Clearing %d discovered devices", discoveredDevices_.size());
		discoveredDevices_.clear();
		xSemaphoreGive(discoveredDevicesMutex_);
	} else {
		WARN("Failed to acquire mutex for clearing discovered devices");
	}
}

// ============================================================================
// Public API Methods - Pairing Management
// ============================================================================

bool BTRemote::removePairing(const char *macAddress) {
	esp_bd_addr_t address;
	stringToBdAddr(std::string(macAddress), address);

	INFO("Removing pairing for device: %s", macAddress);

	// Disconnect HID device first if connected (virtual cable unplug)
	// This ensures clean disconnect before removing pairing
	INFO("Attempting virtual cable unplug for device: %s", macAddress);
	esp_err_t hid_ret = esp_bt_hid_host_virtual_cable_unplug(address);
	if (hid_ret != ESP_OK) {
		WARN("Virtual cable unplug failed: %s (device may not be HID or already disconnected)", esp_err_to_name(hid_ret));
	} else {
		INFO("Virtual cable unplug initiated successfully");
		// Give some time for the disconnect to complete
		vTaskDelay(pdMS_TO_TICKS(500));
	}

	// Remove the bond device (pairing information)
	esp_err_t ret = esp_bt_gap_remove_bond_device(address);
	if (ret != ESP_OK) {
		ERROR("Remove bond device failed: %s", esp_err_to_name(ret));
		return false;
	}

	INFO("Pairing removed successfully for device: %s", macAddress);
	return true;
}

bool BTRemote::startPairing(const char *macAddress) {
	if (pairingInProgress_) {
		WARN("Pairing already in progress with another device");
		//		return false;
	}

	esp_bd_addr_t address;
	stringToBdAddr(std::string(macAddress), address);
	memcpy(pairingDeviceAddress_, address, sizeof(esp_bd_addr_t));

	INFO("Starting pairing/connection with HID device: %s", macAddress);

	// CRITICAL: Device must be in inquiry database before connection
	// Check if device was discovered
	bool deviceDiscovered = false;
	if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		deviceDiscovered = (discoveredDevices_.find(std::string(macAddress)) != discoveredDevices_.end());
		xSemaphoreGive(discoveredDevicesMutex_);
	}

	stopDiscoveryInternal();

	INFO("Stopping BLE advertising to reduce interference with Classic BT connection...");
	esp_err_t ret = esp_ble_gap_stop_advertising();
	if (ret != ESP_OK) {
		WARN("Failed to stop BLE advertising: %s (continuing anyway)", esp_err_to_name(ret));
	} else {
		INFO("BLE advertising stopped successfully");
	}

	vTaskDelay(pdMS_TO_TICKS(500));

	pairingInProgress_ = true;

	// For HID devices, initiate connection which triggers pairing automatically
	// Use Bluedroid Classic BT HID Host API
	ret = esp_bt_hid_host_connect(address);

	if (ret != ESP_OK) {
		ERROR("HID Host connect failed: %s", esp_err_to_name(ret));
		pairingInProgress_ = false;
		memset(&pairingDeviceAddress_, 0, sizeof(pairingDeviceAddress_));
		return false;
	}

	INFO("HID connection initiated for device: %s (pairing will happen automatically)", macAddress);
	return true;
}

bool BTRemote::isPairingInProgress() const {
	return pairingInProgress_;
}

// ============================================================================
// Public API Methods - HID Host
// ============================================================================

bool BTRemote::hidConnect(const char *macAddress) {
	esp_bd_addr_t address;
	stringToBdAddr(std::string(macAddress), address);

	INFO("Connecting to HID device: %s", macAddress);

	// Stop discovery if in progress to avoid conflicts
	if (discoveryInProgress_) {
		stopDiscoveryInternal();
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	// Use Bluedroid Classic BT HID Host API
	// Connection will trigger ESP_HIDH_OPEN_EVT on success
	esp_err_t ret = esp_bt_hid_host_connect(address);

	if (ret != ESP_OK) {
		ERROR("HID device connect failed for address: %s, error: %s", macAddress, esp_err_to_name(ret));
		return false;
	}

	// Store device in map with CONNECTING state
	// State will be updated to CONNECTED in handleHIDHostOpen when ESP_HIDH_OPEN_EVT arrives
	if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		std::string key = bdAddrToString(address);

		HIDDevice hidDev;
		memcpy(hidDev.address, address, sizeof(esp_bd_addr_t));
		strncpy(hidDev.name, macAddress, sizeof(hidDev.name) - 1);
		hidDev.state = HIDConnectionState::CONNECTING;
		hidDev.handle = 0; // Will be set in OPEN event

		hidDevices_[key] = hidDev;

		INFO("HID connection initiated: %s (waiting for ESP_HIDH_OPEN_EVT)", macAddress);
		xSemaphoreGive(hidDevicesMutex_);
	} else {
		WARN("Failed to acquire mutex for HID devices");
		return false;
	}

	return true;
}

bool BTRemote::hidDisconnect(const char *macAddress) {
	esp_bd_addr_t address;
	stringToBdAddr(std::string(macAddress), address);

	INFO("Disconnecting HID device: %s", macAddress);

	// Use Bluedroid Classic BT HID Host API
	// Disconnection will trigger ESP_HIDH_CLOSE_EVT
	esp_err_t ret = esp_bt_hid_host_disconnect(address);

	if (ret != ESP_OK) {
		ERROR("HID device disconnect failed: %s, error: %s", macAddress, esp_err_to_name(ret));
		return false;
	}

	INFO("HID disconnection initiated: %s (waiting for ESP_HIDH_CLOSE_EVT)", macAddress);
	return true;
}

std::vector<HIDDevice> BTRemote::getConnectedHIDDevices() {
	std::vector<HIDDevice> devices;

	if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		INFO("Retrieving %d connected HID devices", hidDevices_.size());
		for (const auto &pair : hidDevices_) {
			devices.push_back(pair.second);
		}
		xSemaphoreGive(hidDevicesMutex_);
	} else {
		WARN("Failed to acquire mutex for HID devices");
	}

	return devices;
}

// ============================================================================
// Bluetooth Classic GAP Event Handlers
// ============================================================================

void BTRemote::handleGapBTDiscoveryResult(esp_bt_gap_cb_param_t *param) {
	// Access discovery result from param union
	// In ESP-IDF 4.4, disc_res contains: bda, num_prop, prop
	esp_bd_addr_t bda;
	memcpy(bda, param->disc_res.bda, sizeof(esp_bd_addr_t));

	// Extract COD and RSSI from properties
	uint32_t cod = 0;
	int rssi = 0;
	uint8_t *eir_data = nullptr;

	for (int i = 0; i < param->disc_res.num_prop; i++) {
		esp_bt_gap_dev_prop_t *prop = &param->disc_res.prop[i];
		switch (prop->type) {
			case ESP_BT_GAP_DEV_PROP_COD:
				if (prop->len == sizeof(uint32_t)) {
					cod = *(uint32_t *) prop->val;
				}
				break;
			case ESP_BT_GAP_DEV_PROP_RSSI:
				if (prop->len == sizeof(int8_t)) {
					rssi = *(int8_t *) prop->val;
				}
				break;
			case ESP_BT_GAP_DEV_PROP_EIR:
				eir_data = (uint8_t *) prop->val;
				break;
			default:
				break;
		}
	}

	std::string addrStr = bdAddrToString(bda);
	BTClassicDeviceType deviceType = classifyDevice(cod);

	DEBUG("BT Discovery result: MAC=%s, RSSI=%d, CoD=0x%06X, Type=%d",
		addrStr.c_str(), rssi, cod, (int) deviceType);

	// Store or update device in discovered devices map (thread-safe)
	if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		BTClassicDevice device;
		memcpy(device.address, bda, sizeof(esp_bd_addr_t));
		device.deviceType = deviceType;
		device.rssi = rssi;
		device.cod = cod;
		device.paired = false; // Will be updated by auth events

		// Try to get device name from EIR data
		uint8_t eir_len = 0;
		uint8_t *name_ptr = nullptr;

		if (eir_data != nullptr) {
			// Parse EIR data for device name (type 0x09 = complete local name)
			uint8_t offset = 0;
			while (offset < ESP_BT_GAP_EIR_DATA_LEN) {
				eir_len = eir_data[offset];
				if (eir_len == 0) {
					break;
				}

				uint8_t eir_type = eir_data[offset + 1];
				if (eir_type == 0x09) { // Complete local name
					name_ptr = &eir_data[offset + 2];
					uint8_t name_len = eir_len - 1;
					if (name_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
						name_len = ESP_BT_GAP_MAX_BDNAME_LEN;
					}
					memcpy(device.name, name_ptr, name_len);
					device.name[name_len] = '\0';
					DEBUG("Device name from EIR: %s", device.name);
					break;
				}

				offset += eir_len + 1;
			}
		}

		// If no name in EIR, request remote name
		if (strlen(device.name) == 0) {
			DEBUG("Requesting remote name for device: %s", addrStr.c_str());
			esp_bt_gap_read_remote_name(bda);
			strncpy(device.name, "Unknown", sizeof(device.name) - 1);
		}

		discoveredDevices_[addrStr] = device;
		DEBUG("Device added/updated in discovered list: %s (total: %d)",
			addrStr.c_str(), discoveredDevices_.size());

		xSemaphoreGive(discoveredDevicesMutex_);
	} else {
		WARN("Failed to acquire mutex for discovered devices");
	}
}

void BTRemote::handleGapBTDiscoveryStateChanged(esp_bt_gap_cb_param_t *param) {
	esp_bt_gap_discovery_state_t state = param->disc_st_chg.state;

	INFO("BT Discovery state changed: %s",
		state == ESP_BT_GAP_DISCOVERY_STOPPED ? "STOPPED" : state == ESP_BT_GAP_DISCOVERY_STARTED ? "STARTED"
																								  : "UNKNOWN");

	if (state == ESP_BT_GAP_DISCOVERY_STOPPED) {
		discoveryInProgress_ = false;
		INFO("Discovery stopped, found %d devices", discoveredDevices_.size());

	} else if (state == ESP_BT_GAP_DISCOVERY_STARTED) {
		discoveryInProgress_ = true;
		INFO("Discovery started");
	}
}

void BTRemote::handleGapBTAuthComplete(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->auth_cmpl.bda);

	// Enhanced debugging for authentication results
	INFO("BT Authentication complete for device: %s, status: %d (%s)",
		addrStr.c_str(), param->auth_cmpl.stat,
		param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS ? "SUCCESS" : "FAILED");

	if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
		INFO("BT Authentication successful for device: %s", addrStr.c_str());

		// Update device paired status
		if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
			auto it = discoveredDevices_.find(addrStr);
			if (it != discoveredDevices_.end()) {
				it->second.paired = true;
				INFO("Device marked as paired: %s", addrStr.c_str());
			}
			xSemaphoreGive(discoveredDevicesMutex_);
		}

		pairingInProgress_ = false;
		memset(&pairingDeviceAddress_, 0, sizeof(pairingDeviceAddress_));
	} else {
		ERROR("BT Authentication failed for device: %s, status: %d",
			addrStr.c_str(), param->auth_cmpl.stat);
		ERROR("Check if device is already paired or if there's a security mismatch");
		pairingInProgress_ = false;
		memset(&pairingDeviceAddress_, 0, sizeof(pairingDeviceAddress_));
	}

	// Copy device name if available
	if (param->auth_cmpl.device_name[0] != '\0') {
		if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
			auto it = discoveredDevices_.find(addrStr);
			if (it != discoveredDevices_.end()) {
				strncpy(it->second.name, (char *) param->auth_cmpl.device_name,
					ESP_BT_GAP_MAX_BDNAME_LEN);
				it->second.name[ESP_BT_GAP_MAX_BDNAME_LEN] = '\0';
				INFO("Device name updated: %s -> %s", addrStr.c_str(), it->second.name);
			}
			xSemaphoreGive(discoveredDevicesMutex_);
		}
	}
}

void BTRemote::handleGapBTPinRequest(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->pin_req.bda);
	INFO("BT PIN request from device: %s (auto-responding with default PIN)", addrStr.c_str());

	// Auto-accept with default PIN: 0000
	esp_bt_pin_code_t pin_code = {'0', '0', '0', '0'};
	esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
}

void BTRemote::handleGapBTCfmRequest(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->cfm_req.bda);
	INFO("BT SSP confirmation request from device: %s, passkey: %d (auto-accepting)",
		addrStr.c_str(), param->cfm_req.num_val);

	// Auto-accept confirmation request
	esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
}

void BTRemote::handleGapBTKeyNotify(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->key_notif.bda);
	INFO("BT SSP key notification from device: %s, passkey: %d",
		addrStr.c_str(), param->key_notif.passkey);
}

void BTRemote::handleGapBTKeyRequest(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->key_req.bda);
	INFO("BT SSP key request from device: %s (auto-accepting)", addrStr.c_str());

	// Auto-accept key request
	esp_bt_gap_ssp_passkey_reply(param->key_req.bda, true, 0);
}

void BTRemote::handleGapBTReadRemoteName(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->read_rmt_name.bda);

	if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
		INFO("BT Remote name read successful: %s -> %s",
			addrStr.c_str(), param->read_rmt_name.rmt_name);

		// Update device name in discovered devices
		if (xSemaphoreTake(discoveredDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
			auto it = discoveredDevices_.find(addrStr);
			if (it != discoveredDevices_.end()) {
				strncpy(it->second.name, (char *) param->read_rmt_name.rmt_name,
					ESP_BT_GAP_MAX_BDNAME_LEN);
				it->second.name[ESP_BT_GAP_MAX_BDNAME_LEN] = '\0';
				INFO("Device name updated: %s", it->second.name);
			}
			xSemaphoreGive(discoveredDevicesMutex_);
		}
	} else {
		WARN("BT Remote name read failed for device: %s, status: %d",
			addrStr.c_str(), param->read_rmt_name.stat);
	}
}

void BTRemote::handleGapBTAclConnComplete(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->acl_conn_cmpl_stat.bda);

	INFO("BT ACL connection complete: %s, status: %d, handle: 0x%04x",
		addrStr.c_str(), param->acl_conn_cmpl_stat.stat, param->acl_conn_cmpl_stat.handle);

	if (param->acl_conn_cmpl_stat.stat == ESP_BT_STATUS_SUCCESS) {
		INFO("ACL connection established successfully for: %s", addrStr.c_str());
	} else {
		ERROR("ACL connection failed for device: %s, status: %d",
			addrStr.c_str(), param->acl_conn_cmpl_stat.stat);
	}
}

void BTRemote::handleGapBTAclDisconnComplete(esp_bt_gap_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->acl_disconn_cmpl_stat.bda);

	INFO("BT ACL disconnection complete: %s, reason: %d, handle: 0x%04x",
		addrStr.c_str(), param->acl_disconn_cmpl_stat.reason, param->acl_disconn_cmpl_stat.handle);

	// Log human-readable disconnect reason
	const char *reason_str = "Unknown";
	switch (param->acl_disconn_cmpl_stat.reason) {
		case 0x05:
			reason_str = "Authentication Failure";
			break;
		case 0x08:
			reason_str = "Connection Timeout";
			break;
		case 0x13:
			reason_str = "Remote User Terminated Connection";
			break;
		case 0x14:
			reason_str = "Remote Device Terminated Connection (Low Resources)";
			break;
		case 0x15:
			reason_str = "Remote Device Terminated Connection (Power Off)";
			break;
		case 0x16:
			reason_str = "Connection Terminated By Local Host";
			break;
		case 0x22:
			reason_str = "LMP Response Timeout";
			break;
		case 0x3B:
			reason_str = "Unacceptable Connection Parameters";
			break;
		default:
			break;
	}
	INFO("Disconnect reason: 0x%02X (%s)", param->acl_disconn_cmpl_stat.reason, reason_str);
}

// ============================================================================
// HID Host Event Handlers
// ============================================================================

void BTRemote::handleHIDHostInit(esp_hidh_cb_param_t *param) {
	if (param->init.status == ESP_HIDH_OK) {
		INFO("HID Host initialized successfully (ESP_HIDH_INIT_EVT received)");
	} else {
		ERROR("HID Host initialization failed, status: %d", param->init.status);
	}
}

void BTRemote::handleHIDHostOpen(esp_hidh_cb_param_t *param) {
	std::string addrStr = bdAddrToString(param->open.bd_addr);
	const char *conn_state_str = "UNKNOWN";
	switch (param->open.conn_status) {
		case ESP_HIDH_CONN_STATE_CONNECTED:
			conn_state_str = "CONNECTED";
			break;
		case ESP_HIDH_CONN_STATE_CONNECTING:
			conn_state_str = "CONNECTING";
			break;
		case ESP_HIDH_CONN_STATE_DISCONNECTED:
			conn_state_str = "DISCONNECTED";
			break;
		case ESP_HIDH_CONN_STATE_DISCONNECTING:
			conn_state_str = "DISCONNECTING";
			break;
		default:
			break;
	}

	INFO("HID OPEN event: %s, handle: %d, status: %d, conn_status: %d (%s), is_orig: %d",
		addrStr.c_str(), param->open.handle, param->open.status,
		param->open.conn_status, conn_state_str, param->open.is_orig);

	// Add debugging to identify who initiated the connection
	if (param->open.is_orig == 0) {
		INFO("This is an INCOMING connection (client reconnected to us)");
	} else {
		INFO("This is an OUTGOING connection (we initiated)");
	}

	// CRITICAL: Only process when device is FULLY CONNECTED, not just CONNECTING
	// ESP_HIDH_OPEN_EVT can fire multiple times during connection process
	if (param->open.status == ESP_HIDH_OK && param->open.conn_status == ESP_HIDH_CONN_STATE_CONNECTED) {

		// Update device state
		if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
			auto it = hidDevices_.find(addrStr);
			if (it != hidDevices_.end()) {
				it->second.state = HIDConnectionState::CONNECTED;
				it->second.handle = param->open.handle;
				INFO("HID device state updated to CONNECTED");
			} else {
				// Device not found, create new entry
				HIDDevice hidDev;
				memcpy(hidDev.address, param->open.bd_addr, sizeof(esp_bd_addr_t));
				strncpy(hidDev.name, addrStr.c_str(), sizeof(hidDev.name) - 1);
				hidDev.state = HIDConnectionState::CONNECTED;
				hidDev.handle = param->open.handle;
				hidDevices_[addrStr] = hidDev;
				INFO("New HID device added to connected list");
			}
			xSemaphoreGive(hidDevicesMutex_);
		}

		// Initialize gamepad state
		GamepadState state;
		memcpy(state.address, param->open.bd_addr, sizeof(esp_bd_addr_t));
		state.connected = true;
		state.timestamp = millis();
		state.buttons = 0;
		state.dpad = 8; // Center
		state.leftStickX = 0;
		state.leftStickY = 0;
		state.rightStickX = 0;
		state.rightStickY = 0;
		inputDevices_->registerGamepadState(addrStr, state);

		// CRITICAL: Do NOT set protocol mode here!
		// During reconnection, the L2CAP security handshake may not be complete yet.
		// The ESP32 needs remote device features cached before security can proceed.
		// Protocol mode will be set after ESP_HIDH_GET_DSCP_EVT when SDP query completes.
		INFO("Waiting for SDP query to complete before setting protocol mode...");

		// NOTE: Do NOT resume BLE advertising here!
		// At this point the L2CAP and SDP handshake is not complete yet.
		// If we resume BLE now, BLE traffic will starve the gamepad connection
		// and prevent it from completing.
		// Instead, we'll resume BLE after protocol mode is set (ESP_HIDH_SET_PROTO_EVT)
		// or if connection fails (ESP_HIDH_CLOSE_EVT).
		INFO("BLE advertising remains paused to allow HID connection to complete...");
	} else {
		std::string addrStr = bdAddrToString(param->open.bd_addr);
		ERROR("HID device connection failed: %s, status: %d",
			addrStr.c_str(), param->open.status);

		// Resume BLE advertising even if HID connection failed
		INFO("Resuming BLE advertising after failed HID connection...");
		esp_err_t ret = esp_ble_gap_start_advertising(&g_adv_params);
		if (ret != ESP_OK) {
			WARN("Failed to restart BLE advertising: %s", esp_err_to_name(ret));
		}
	}
}

void BTRemote::handleHIDHostClose(esp_hidh_cb_param_t *param) {
	INFO("HID device disconnected: handle=%d, reason=%d, conn_status=%d",
		param->close.handle, param->close.reason, param->close.conn_status);

	// Find device by handle and update state
	if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		for (auto &pair : hidDevices_) {
			if (pair.second.handle == param->close.handle) {
				pair.second.state = HIDConnectionState::DISCONNECTED;
				INFO("HID device state updated to DISCONNECTED: %s", pair.first.c_str());

				inputDevices_->updateGamepad(pair.first, [](std::string &macAddress, GamepadState &state) {
					state.connected = false;
					state.timestamp = millis();
					INFO("Gamepad marked as disconnected: %s", macAddress.c_str());
				});
			}
		}
		xSemaphoreGive(hidDevicesMutex_);
	}

	// Resume BLE advertising after HID disconnection
	INFO("HID device closed - resuming BLE advertising...");
	esp_err_t ret = esp_ble_gap_start_advertising(&g_adv_params);
	if (ret != ESP_OK) {
		WARN("Failed to restart BLE advertising: %s", esp_err_to_name(ret));
	} else {
		INFO("BLE advertising resumed successfully after HID disconnection");
	}
}

void BTRemote::handleHIDHostData(esp_hidh_cb_param_t *param) {
	DEBUG("HID data received: handle=%d, length=%d, proto_mode=%d",
		param->data_ind.handle, param->data_ind.len, param->data_ind.proto_mode);

	// Find device by handle to get address
	if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
		for (const auto &pair : hidDevices_) {
			if (pair.second.handle == param->data_ind.handle) {
				// Process gamepad report data (proto_mode: ESP_HIDH_REPORT_MODE = 0x01)
				if (param->data_ind.len > 0) {
					processGamepadReport(pair.second.address, param->data_ind.data, param->data_ind.len);
				}
				break;
			}
		}
		xSemaphoreGive(hidDevicesMutex_);
	}
}

void BTRemote::processGamepadReport(const esp_bd_addr_t address, const uint8_t *data, uint16_t length) {
	std::string addrStr = bdAddrToString(address);

	inputDevices_->updateGamepad(addrStr, [data, length](std::string &macAddress, GamepadState &state) {
		INFO("Got report with length %d", length);
		for (int i = 0; i < length; i++) {
			DEBUG("Byte[%d]: dec=%3d bin=%s hex=0x%02X", i, data[i],
				String(data[i], BIN).c_str(), data[i]);
		}

		state.timestamp = millis();

		INFO("Parsing gamepad report: VID=0x%04X PID=0x%04X", state.vendorId, state.productId);

		if (state.vendorId == 0x045E) {
			// We can only parse page 1 for now....
			if (data[0] == 1 && length == 16) {
				// 16-bit little-endian values (low byte first, then high byte)
				state.leftStickX = ((int16_t) data[2] * 256 + data[1]) - 32768;
				state.leftStickY = ((int16_t) data[4] * 256 + data[3]) - 32768;
				state.rightStickX = ((int16_t) data[6] * 256 + data[5]) - 32768;
				state.rightStickY = ((int16_t) data[8] * 256 + data[7]) - 32768;

				// Don't know where to get the dpad status from,
				// Also the shoulder buttons are somehow strange...

				state.buttons = ((int16_t) data[13] * 256 + data[14]);

				INFO("Standard Gamepad: Buttons=0x%02X, DPad=%d, Left Stick=(%d,%d), Right Stick=(%d,%d)",
					state.buttons, state.dpad,
					state.leftStickX, state.leftStickY,
					state.rightStickX, state.rightStickY);

			} else {
				WARN("Ignoring Page %d of report with size %d", data[0], length);
			}
		} else {
			WARN("No XInput data, expected vendor id 0x045E and length 16 bytes");
		}
	});
}

// ============================================================================
// Static Callback Dispatchers
// ============================================================================

void BTRemote::hidHostEventHandler(esp_hidh_cb_event_t event, esp_hidh_cb_param_t *param) {
	if (!g_btremote_instance) {
		return;
	}

	// CRITICAL: Do NOT block this callback - it runs in Bluedroid's task!
	// Queue the event for processing in dedicated task
	HIDEventItem item;
	item.event = event;
	item.param = *param; // Shallow copy param
	item.data_copy = nullptr;
	item.data_len = 0;

	// CRITICAL: Deep-copy data for events with pointers
	// Bluedroid will free the data after this callback returns
	if (event == ESP_HIDH_DATA_IND_EVT && param->data_ind.len > 0 && param->data_ind.data != nullptr) {
		// Deep copy HID input report data
		item.data_len = param->data_ind.len;
		item.data_copy = (uint8_t *) malloc(item.data_len);
		if (item.data_copy != nullptr) {
			memcpy(item.data_copy, param->data_ind.data, item.data_len);
			// Update param to point to our copy
			item.param.data_ind.data = item.data_copy;
		} else {
			ERROR("Failed to allocate %d bytes for HID data copy", item.data_len);
			return; // Drop event if allocation fails
		}
	} else if (event == ESP_HIDH_GET_RPT_EVT && param->get_rpt.len > 0 && param->get_rpt.data != nullptr) {
		// Deep copy HID get report response data
		item.data_len = param->get_rpt.len;
		item.data_copy = (uint8_t *) malloc(item.data_len);
		if (item.data_copy != nullptr) {
			memcpy(item.data_copy, param->get_rpt.data, item.data_len);
			// Update param to point to our copy
			item.param.get_rpt.data = item.data_copy;
		} else {
			ERROR("Failed to allocate %d bytes for GET_RPT data copy", item.data_len);
			return; // Drop event if allocation fails
		}
	}

	// Try to queue the event (don't block - drop if queue full)
	if (xQueueSend(g_btremote_instance->hidEventQueue_, &item, 0) != pdTRUE) {
		ERROR("HID event queue full - dropping event %d", event);
		// Free allocated data if queue send failed
		if (item.data_copy != nullptr) {
			free(item.data_copy);
		}
	}
}

// HID event processing task - runs independently from Bluedroid callbacks
void BTRemote::hidEventTask(void *pvParameters) {
	BTRemote *instance = static_cast<BTRemote *>(pvParameters);
	HIDEventItem item;

	INFO("HID event task started");

	while (true) {
		// Wait for events from queue (blocks until event available)
		if (xQueueReceive(instance->hidEventQueue_, &item, portMAX_DELAY) == pdTRUE) {
			instance->processHIDEvent(item);

			// CRITICAL: Free deep-copied data after processing
			// This was allocated in hidHostEventHandler for ESP_HIDH_DATA_IND_EVT and ESP_HIDH_GET_RPT_EVT
			if (item.data_copy != nullptr) {
				free(item.data_copy);
				item.data_copy = nullptr;
			}
		}
	}
}

// Process HID event (called from dedicated task, not callback)
void BTRemote::processHIDEvent(const HIDEventItem &item) {
	// Make a non-const copy of param so we can pass pointer to handlers
	esp_hidh_cb_param_t param = item.param;

	switch (item.event) {
		case ESP_HIDH_INIT_EVT:
			handleHIDHostInit(&param);
			break;
		case ESP_HIDH_OPEN_EVT:
			handleHIDHostOpen(&param);
			break;
		case ESP_HIDH_CLOSE_EVT:
			handleHIDHostClose(&param);
			break;
		case ESP_HIDH_DATA_IND_EVT:
			handleHIDHostData(&param);
			break;
		case ESP_HIDH_DATA_EVT:
			INFO("HID data sent confirmation");
			break;
		case ESP_HIDH_SET_PROTO_EVT:
			INFO("HID Set Protocol complete: handle=%d, status=%d",
				param.set_proto.handle, param.set_proto.status);
			if (param.set_proto.status == ESP_HIDH_OK) {
				INFO("Protocol mode successfully set - device should now send reports");

				// CRITICAL: Now that protocol mode is set, the HID connection is fully
				// established and ready. Resume BLE advertising now that the critical
				// phase of the HID connection is complete.
				INFO("HID connection complete - resuming BLE advertising...");
				esp_err_t ret = esp_ble_gap_start_advertising(&g_adv_params);
				if (ret != ESP_OK) {
					WARN("Failed to restart BLE advertising: %s", esp_err_to_name(ret));
				} else {
					INFO("BLE advertising resumed successfully after HID connection complete");
				}
			} else {
				ERROR("Failed to set protocol mode, status: %d", param.set_proto.status);

				// Resume BLE even on failure
				INFO("Resuming BLE advertising after HID protocol setup failure...");
				esp_ble_gap_start_advertising(&g_adv_params);
			}
			break;
		case ESP_HIDH_GET_IDLE_EVT:
			INFO("HID Get Idle: handle=%d, status=%d, idle_rate=%d",
				param.get_idle.handle, param.get_idle.status, param.get_idle.idle_rate);
			break;
		case ESP_HIDH_SET_IDLE_EVT:
			INFO("HID Set Idle: handle=%d, status=%d",
				param.set_idle.handle, param.set_idle.status);
			break;
		case ESP_HIDH_ADD_DEV_EVT:
			INFO("HID Add Device event: handle=%d, status=%d",
				param.add_dev.handle, param.add_dev.status);
			if (param.add_dev.status == ESP_HIDH_OK) {
				INFO("Device added successfully to HID Host database");
			}
			break;
		case ESP_HIDH_RMV_DEV_EVT:
			INFO("HID Remove Device event: handle=%d, status=%d",
				param.rmv_dev.handle, param.rmv_dev.status);
			break;
		case ESP_HIDH_VC_UNPLUG_EVT:
			INFO("HID Virtual Cable Unplug: handle=%d, status=%d",
				param.unplug.handle, param.unplug.status);
			break;
		case ESP_HIDH_SET_INFO_EVT:
			INFO("HID Set Info: handle=%d, status=%d",
				param.set_info.handle, param.set_info.status);
			break;
		case ESP_HIDH_GET_PROTO_EVT:
			INFO("HID Get Protocol: handle=%d, proto_mode=%d",
				param.get_proto.handle, param.get_proto.proto_mode);
			break;
		case ESP_HIDH_GET_RPT_EVT:
			INFO("HID Get Report complete: handle=%d, len=%d",
				param.get_rpt.handle, param.get_rpt.len);
			break;
		case ESP_HIDH_GET_DSCP_EVT:
			INFO("HID Descriptor received: handle=%d, status=%d, added=%d, VID=0x%04x, PID=0x%04x",
				param.dscp.handle, param.dscp.status, param.dscp.added,
				param.dscp.vendor_id, param.dscp.product_id);

			if (param.dscp.status == ESP_HIDH_OK) {
				// Note: param.dscp.dsc_list contains SDP attribute data, not the raw HID descriptor
				// The actual HID report descriptor is embedded within the SDP data and would
				// require complex SDP parsing to extract. For now, we'll use device-specific
				// parsing based on VID/PID instead.
				INFO("Device VID=0x%04X PID=0x%04X - use device-specific report parsing",
					param.dscp.vendor_id, param.dscp.product_id);

				// CRITICAL: SDP query complete! L2CAP security handshake is now done.
				// Remote device features are cached, so it's safe to set protocol mode.
				INFO("SDP query complete - L2CAP handshake successful, setting protocol mode to REPORT_MODE");

				// Find the device by handle to get its address and store VID/PID
				if (xSemaphoreTake(hidDevicesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
					for (auto &entry : hidDevices_) {
						if (entry.second.handle == param.dscp.handle) {
							INFO("Found device %s for handle %d", entry.first.c_str(), param.dscp.handle);

							// Store VID/PID in gamepad state for device-specific parsing
							inputDevices_->updateGamepad(entry.first, [param](std::string &macAddress, GamepadState &state) {
								state.vendorId = param.dscp.vendor_id;
								state.productId = param.dscp.product_id;
								INFO("Stored VID/PID for gamepad %s: VID=0x%04X PID=0x%04X",
									macAddress.c_str(), state.vendorId, state.productId);
							});

							// Set protocol mode - this should succeed now that SDP is complete
							INFO("Setting protocol mode to REPORT_MODE for %s", entry.first.c_str());
							esp_err_t ret = esp_bt_hid_host_set_protocol(entry.second.address, ESP_HIDH_REPORT_MODE);
							if (ret != ESP_OK) {
								ERROR("Failed to set protocol mode: %s", esp_err_to_name(ret));
							} else {
								INFO("Protocol mode set command sent successfully for device: %s", entry.first.c_str());
							}
							break;
						}
					}
					xSemaphoreGive(hidDevicesMutex_);
				} else {
					ERROR("Failed to acquire hidDevicesMutex in ESP_HIDH_GET_DSCP_EVT");
				}
			} else {
				ERROR("Failed to retrieve HID descriptor, status: %d", param.dscp.status);
			}
			break;
		default:
			INFO("Unhandled HID Host event: %d", item.event);
			break;
	}
}

void BTRemote::gapBTEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
	if (!g_btremote_instance) {
		return;
	}

	switch (event) {
		case ESP_BT_GAP_DISC_RES_EVT:
			g_btremote_instance->handleGapBTDiscoveryResult(param);
			break;
		case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
			g_btremote_instance->handleGapBTDiscoveryStateChanged(param);
			break;
		case ESP_BT_GAP_AUTH_CMPL_EVT:
			g_btremote_instance->handleGapBTAuthComplete(param);
			break;
		case ESP_BT_GAP_PIN_REQ_EVT:
			g_btremote_instance->handleGapBTPinRequest(param);
			break;
		case ESP_BT_GAP_CFM_REQ_EVT:
			g_btremote_instance->handleGapBTCfmRequest(param);
			break;
		case ESP_BT_GAP_KEY_NOTIF_EVT:
			g_btremote_instance->handleGapBTKeyNotify(param);
			break;
		case ESP_BT_GAP_KEY_REQ_EVT:
			g_btremote_instance->handleGapBTKeyRequest(param);
			break;
		case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
			g_btremote_instance->handleGapBTReadRemoteName(param);
			break;
		case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
			g_btremote_instance->handleGapBTAclConnComplete(param);
			break;
		case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
			g_btremote_instance->handleGapBTAclDisconnComplete(param);
			break;
		default:
			// Event 21 and other unhandled events - log at debug level
			// These are typically non-critical informational events
			DEBUG("Unhandled BT GAP event: %d", event);
			break;
	}
}

// ============================================================================
// BLE Event Handlers (existing code)
// ============================================================================

void BTRemote::gattsEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
	esp_ble_gatts_cb_param_t *param) {
	if (g_btremote_instance) {
		switch (event) {
			case ESP_GATTS_REG_EVT:
				g_btremote_instance->handleGattsRegister(gatts_if, param);
				break;
			case ESP_GATTS_CREATE_EVT:
				g_btremote_instance->handleGattsCreate(param);
				break;
			case ESP_GATTS_ADD_CHAR_EVT:
				g_btremote_instance->handleGattsAddChar(param);
				break;
			case ESP_GATTS_ADD_CHAR_DESCR_EVT:
				g_btremote_instance->handleGattsAddCharDescr(param);
				break;
			case ESP_GATTS_CONNECT_EVT:
				g_btremote_instance->handleGattsConnect(param);
				break;
			case ESP_GATTS_DISCONNECT_EVT:
				g_btremote_instance->handleGattsDisconnect(param);
				break;
			case ESP_GATTS_WRITE_EVT:
				g_btremote_instance->handleGattsWrite(param);
				break;
			case ESP_GATTS_MTU_EVT:
				g_btremote_instance->handleGattsMTU(param);
				break;
			case ESP_GATTS_CONF_EVT:
				g_btremote_instance->handleGattsConfirm(param);
				break;
			default:
				break;
		}
	}
}

void BTRemote::gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
	switch (event) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			INFO("Advertising data set (standard), starting advertising");
			esp_ble_gap_start_advertising(&g_adv_params);
			break;
		case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
			INFO("Raw advertising data set, starting advertising");
			esp_ble_gap_start_advertising(&g_adv_params);
			break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			INFO("Scan response data set");
			// Scan response is optional, advertising should already be started by raw adv data event
			break;
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
			if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
				INFO("Advertising started successfully");
			} else {
				ERROR("Advertising start failed: %d", param->adv_start_cmpl.status);
			}
			break;
		default:
			break;
	}
}

void BTRemote::handleGattsRegister(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	if (param->reg.status != ESP_GATT_OK) {
		ERROR("GATTS register failed, status=%d", param->reg.status);
		return;
	}

	INFO("GATTS registered, app_id=%d, gatts_if=%d", param->reg.app_id, gatts_if);
	gatts_if_ = gatts_if;

	esp_gatt_srvc_id_t service_id = {
		.id = {
			   .uuid = stringToUUID128(SERVICE_UUID),
			   .inst_id = 0,
			   },
		.is_primary = true,
	};

	esp_err_t ret = esp_ble_gatts_create_service(gatts_if, &service_id, 20);
	if (ret) {
		ERROR("Create service failed: %s", esp_err_to_name(ret));
	}
}

void BTRemote::handleGattsCreate(esp_ble_gatts_cb_param_t *param) {
	if (param->create.status != ESP_GATT_OK) {
		ERROR("Create service failed, status=%d", param->create.status);
		return;
	}

	INFO("Service created, handle=%d", param->create.service_handle);
	handles_.service_handle = param->create.service_handle;

	esp_err_t ret = esp_ble_gatts_start_service(handles_.service_handle);
	if (ret) {
		ERROR("Start service failed: %s", esp_err_to_name(ret));
		return;
	}

	esp_bt_uuid_t char_uuid = stringToUUID128(REQUEST_CHAR_UUID);
	esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_WRITE;
	ret = esp_ble_gatts_add_char(handles_.service_handle, &char_uuid,
		ESP_GATT_PERM_WRITE, prop, nullptr, nullptr);
	if (ret) {
		ERROR("Add request char failed: %s", esp_err_to_name(ret));
	}
}

void BTRemote::handleGattsAddChar(esp_ble_gatts_cb_param_t *param) {
	if (param->add_char.status != ESP_GATT_OK) {
		ERROR("Add char failed, status=%d", param->add_char.status);
		return;
	}

	INFO("Characteristic added, handle=%d, uuid[0]=0x%02x",
		param->add_char.attr_handle, param->add_char.char_uuid.uuid.uuid128[0]);

	esp_bt_uuid_t req_uuid = stringToUUID128(REQUEST_CHAR_UUID);
	esp_bt_uuid_t resp_uuid = stringToUUID128(RESPONSE_CHAR_UUID);
	esp_bt_uuid_t evt_uuid = stringToUUID128(EVENT_CHAR_UUID);
	esp_bt_uuid_t ctrl_uuid = stringToUUID128(CONTROL_CHAR_UUID);

	esp_bt_uuid_t *added_uuid = &param->add_char.char_uuid;

	char uuid_str[64];
	snprintf(uuid_str, sizeof(uuid_str), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		added_uuid->uuid.uuid128[15], added_uuid->uuid.uuid128[14],
		added_uuid->uuid.uuid128[13], added_uuid->uuid.uuid128[12],
		added_uuid->uuid.uuid128[11], added_uuid->uuid.uuid128[10],
		added_uuid->uuid.uuid128[9], added_uuid->uuid.uuid128[8],
		added_uuid->uuid.uuid128[7], added_uuid->uuid.uuid128[6],
		added_uuid->uuid.uuid128[5], added_uuid->uuid.uuid128[4],
		added_uuid->uuid.uuid128[3], added_uuid->uuid.uuid128[2],
		added_uuid->uuid.uuid128[1], added_uuid->uuid.uuid128[0]);
	INFO("Characteristic UUID added: %s", uuid_str);

	if (memcmp(added_uuid->uuid.uuid128, req_uuid.uuid.uuid128, 16) == 0) {
		handles_.request_char_handle = param->add_char.attr_handle;
		INFO("Request char handle=%d (UUID: %s)", handles_.request_char_handle, REQUEST_CHAR_UUID);

		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_INDICATE;
		esp_ble_gatts_add_char(handles_.service_handle, &resp_uuid,
			ESP_GATT_PERM_READ, prop, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, resp_uuid.uuid.uuid128, 16) == 0) {
		handles_.response_char_handle = param->add_char.attr_handle;
		INFO("Response char handle=%d (UUID: %s)", handles_.response_char_handle, RESPONSE_CHAR_UUID);

		esp_bt_uuid_t descr_uuid;
		descr_uuid.len = ESP_UUID_LEN_16;
		descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
		esp_ble_gatts_add_char_descr(handles_.service_handle, &descr_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, evt_uuid.uuid.uuid128, 16) == 0) {
		handles_.event_char_handle = param->add_char.attr_handle;
		INFO("Event char handle=%d (UUID: %s)", handles_.event_char_handle, EVENT_CHAR_UUID);

		esp_bt_uuid_t descr_uuid;
		descr_uuid.len = ESP_UUID_LEN_16;
		descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
		esp_ble_gatts_add_char_descr(handles_.service_handle, &descr_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, ctrl_uuid.uuid.uuid128, 16) == 0) {
		handles_.control_char_handle = param->add_char.attr_handle;
		INFO("Control char handle=%d (UUID: %s)", handles_.control_char_handle, CONTROL_CHAR_UUID);

		esp_bt_uuid_t descr_uuid;
		descr_uuid.len = ESP_UUID_LEN_16;
		descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
		esp_ble_gatts_add_char_descr(handles_.service_handle, &descr_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, nullptr, nullptr);
	}
}

void BTRemote::handleGattsAddCharDescr(esp_ble_gatts_cb_param_t *param) {
	if (param->add_char_descr.status != ESP_GATT_OK) {
		ERROR("Add char descriptor failed, status=%d", param->add_char_descr.status);
		return;
	}

	INFO("Descriptor added, handle=%d", param->add_char_descr.attr_handle);

	if (handles_.response_cccd_handle == 0 && handles_.response_char_handle != 0) {
		handles_.response_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Response CCCD handle=%d", handles_.response_cccd_handle);

		esp_bt_uuid_t evt_uuid = stringToUUID128(EVENT_CHAR_UUID);
		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_INDICATE;
		esp_ble_gatts_add_char(handles_.service_handle, &evt_uuid,
			ESP_GATT_PERM_READ, prop, nullptr, nullptr);

	} else if (handles_.event_cccd_handle == 0 && handles_.event_char_handle != 0) {
		handles_.event_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Event CCCD handle=%d", handles_.event_cccd_handle);

		esp_bt_uuid_t ctrl_uuid = stringToUUID128(CONTROL_CHAR_UUID);
		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
		esp_ble_gatts_add_char(handles_.service_handle, &ctrl_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, prop, nullptr, nullptr);

	} else if (handles_.control_cccd_handle == 0 && handles_.control_char_handle != 0) {
		handles_.control_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Control CCCD handle=%d", handles_.control_cccd_handle);

		INFO("All GATT attributes created successfully");

		static uint8_t raw_adv_data[31];
		int idx = 0;

		raw_adv_data[idx++] = 2;
		raw_adv_data[idx++] = 0x01;
		raw_adv_data[idx++] = 0x06;

		raw_adv_data[idx++] = 17;
		raw_adv_data[idx++] = 0x07;
		esp_bt_uuid_t service_uuid = stringToUUID128(SERVICE_UUID);
		for (int i = 0; i < 16; i++) {
			raw_adv_data[idx++] = service_uuid.uuid.uuid128[i];
		}

		char hex_str[100];
		int hex_idx = 0;
		for (int i = 0; i < idx && hex_idx < 90; i++) {
			hex_idx += snprintf(hex_str + hex_idx, sizeof(hex_str) - hex_idx, "%02x ", raw_adv_data[i]);
		}
		INFO("Raw advertising data (%d bytes): %s", idx, hex_str);

		esp_err_t ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, idx);
		if (ret != ESP_OK) {
			ERROR("Set raw advertising data failed: %s", esp_err_to_name(ret));
		} else {
			INFO("Raw advertising data set successfully");
		}

		esp_ble_adv_data_t scan_rsp_data = {
			.set_scan_rsp = true,
			.include_name = true,
			.include_txpower = false,
			.min_interval = 0,
			.max_interval = 0,
			.appearance = 0,
			.manufacturer_len = 0,
			.p_manufacturer_data = nullptr,
			.service_data_len = 0,
			.p_service_data = nullptr,
			.service_uuid_len = 0,
			.p_service_uuid = nullptr,
			.flag = 0,
		};
		ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
		if (ret != ESP_OK) {
			ERROR("Set scan response data failed: %s", esp_err_to_name(ret));
		}
	}
}

void BTRemote::handleGattsConnect(esp_ble_gatts_cb_param_t *param) {
	readyForEvents_ = false;
	deviceConnected_ = true;
	connState_.connected = true;
	connState_.conn_id = param->connect.conn_id;
	connState_.gatt_if = gatts_if_;
	memcpy(connState_.remote_addr, param->connect.remote_bda, sizeof(esp_bd_addr_t));
	connState_.mtu = 23; // Default MTU, will be updated if negotiated

	mtu_ = 23;

	INFO("Client connected, conn_id=%d, remote_addr=%02x:%02x:%02x:%02x:%02x:%02x",
		connState_.conn_id,
		connState_.remote_addr[0], connState_.remote_addr[1],
		connState_.remote_addr[2], connState_.remote_addr[3],
		connState_.remote_addr[4], connState_.remote_addr[5]);

	// CRITICAL: Queue MTU notification for task context to avoid calling vTaskDelay from ISR
	MessageProcessorItem item = MessageProcessorItem::createMTUNotification();
	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("Failed to queue MTU notification");
	}
}

void BTRemote::handleGattsDisconnect(esp_ble_gatts_cb_param_t *param) {
	readyForEvents_ = false;
	deviceConnected_ = false;
	connState_.connected = false;

	// CRITICAL: Thread-safe clear of fragmentBuffers_
	if (xSemaphoreTake(fragmentBuffersMutex_, portMAX_DELAY) == pdTRUE) {
		fragmentBuffers_.clear();
		xSemaphoreGive(fragmentBuffersMutex_);
	}

	INFO("Client disconnected, conn_id=%d", param->disconnect.conn_id);

	esp_ble_gap_start_advertising(&g_adv_params);
}

void BTRemote::handleGattsMTU(esp_ble_gatts_cb_param_t *param) {
	mtu_ = param->mtu.mtu - 3;
	connState_.mtu = mtu_;
	INFO("MTU changed to: %d (notified with %d)", mtu_, param->mtu.mtu);

	// CRITICAL: Queue MTU notification for task context to avoid calling vTaskDelay from ISR
	MessageProcessorItem item = MessageProcessorItem::createMTUNotification();
	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("Failed to queue MTU notification");
	}
}

void BTRemote::handleGattsConfirm(esp_ble_gatts_cb_param_t *param) {
	DEBUG("Indication confirmed: status=%d, handle=%d", param->conf.status, param->conf.handle);

	if (param->conf.status == ESP_GATT_OK) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(indicationConfirmSemaphore_, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken) {
			portYIELD_FROM_ISR();
		}
	} else {
		WARN("Indication confirmation failed: status=%d", param->conf.status);
		// Signal anyway to prevent deadlock
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(indicationConfirmSemaphore_, &xHigherPriorityTaskWoken);
		if (xHigherPriorityTaskWoken) {
			portYIELD_FROM_ISR();
		}
	}
}

void BTRemote::handleGattsWrite(esp_ble_gatts_cb_param_t *param) {
	const uint8_t *data = param->write.value;
	size_t length = param->write.len;

	if (param->write.handle == handles_.response_cccd_handle || param->write.handle == handles_.event_cccd_handle || param->write.handle == handles_.control_cccd_handle) {

		if (length == 2) {
			uint16_t descr_value = data[0] | (data[1] << 8);
			const char *char_name = "Unknown";
			if (param->write.handle == handles_.response_cccd_handle) {
				char_name = "Response";
			} else if (param->write.handle == handles_.event_cccd_handle) {
				char_name = "Event";
			} else if (param->write.handle == handles_.control_cccd_handle) {
				char_name = "Control";
			}

			INFO("CCCD write for %s characteristic: 0x%04x (Notify=%s, Indicate=%s)",
				char_name, descr_value,
				(descr_value & 0x0001) ? "YES" : "NO",
				(descr_value & 0x0002) ? "YES" : "NO");
		}

		if (param->write.need_rsp) {
			esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
				param->write.trans_id, ESP_GATT_OK, nullptr);
		}
		return;
	}

	if (param->write.len < FRAGMENT_HEADER_SIZE) {
		sendControlMessage(ControlMessageType::NACK, 0);
		if (param->write.need_rsp) {
			esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
				param->write.trans_id, ESP_GATT_OK, nullptr);
		}
		return;
	}

	if (param->write.handle == handles_.request_char_handle) {
		handleFragment(data, length);
	} else if (param->write.handle == handles_.control_char_handle) {
		handleControlMessage(data, length);
	}

	if (param->write.need_rsp) {
		esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
			param->write.trans_id, ESP_GATT_OK, nullptr);
	}
}
