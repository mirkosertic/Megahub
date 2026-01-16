#include "btremote.h"

#include "commands.h"
#include "logging.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

// ESP-IDF Bluedroid includes
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"

// Arduino BT helper (handles controller init correctly)
#include "esp32-hal-bt.h"

// Global instance pointer for static callbacks
static BTRemote *g_btremote_instance = nullptr;

// UUIDs für Service und Characteristics
#define SERVICE_UUID	   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define REQUEST_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESPONSE_CHAR_UUID "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define EVENT_CHAR_UUID	   "d8de624e-140f-4a22-8594-e2216b84a5f2"
#define CONTROL_CHAR_UUID  "f78ebbff-c8b7-4107-93de-889a6a06d408"

// Helper: Convert UUID string to esp_bt_uuid_t (128-bit)
static esp_bt_uuid_t stringToUUID128(const char *uuid_str) {
	esp_bt_uuid_t uuid;
	uuid.len = ESP_UUID_LEN_128;

	// Parse UUID string: "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
	// Format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
	// ESP32 expects little-endian byte order

	uint8_t tmp[16];
	int idx = 0;

	for (int i = 0; i < strlen(uuid_str) && idx < 16; i++) {
		char c = uuid_str[i];
		if (c == '-')
			continue;

		char hex[3] = {uuid_str[i], uuid_str[i + 1], 0};
		tmp[idx++] = (uint8_t) strtol(hex, NULL, 16);
		i++; // Skip next char since we read 2
	}

	// Reverse to little-endian
	for (int i = 0; i < 16; i++) {
		uuid.uuid.uuid128[i] = tmp[15 - i];
	}

	return uuid;
}

// Advertising parameters
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

// Fragment Flags
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

#define APP_EVENT_TYPE_LOG		  0x01
#define APP_EVENT_TYPE_PORTSTATUS 0x02
#define APP_EVENT_TYPE_COMMAND	  0x03

// Delay zwischen Fragmenten (in ms)
// Small delay to allow browser's JS event loop to process events
// Much lower now that we're not blocking in callback context
const uint32_t INTER_FRAGMENT_DELAY_MS = 200;

// Task-Loop-Delay (in ms) - CPU-Zeit freigeben
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
	// Clear output buffer
	buffer.clear();

	// Measure required size
	size_t size = measureJson(doc);
	if (size == 0) {
		WARN("JSON document is empty or invalid");
		return false;
	}

	// Reserve space
	buffer.resize(size);

	// Serialize directly to buffer
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

	// Deserialize JSON from buffer
	DeserializationError error = deserializeJson(doc, (const char *) buffer.data(), buffer.size());

	if (error) {
		WARN("Deserialization error: %s", error.c_str());
	}

	return error;
}

/**
 * Helper: Create a JSON response and serialize it to vector
 *
 * @param buffer Output vector
 * @param fn Lambda function to populate the JsonDocument
 * @return true if successful
 *
 * Example:
 *   std::vector<uint8_t> response;
 *   createJsonResponse(response, [](JsonDocument& doc) {
 *       doc["status"] = "ok";
 *       doc["value"] = 42;
 *   });
 *   btRemote.sendResponse(messageId, response);
 */
template <typename Func>
inline bool createJsonResponse(std::vector<uint8_t> &buffer, Func fn) {
	JsonDocument doc;
	fn(doc);
	return serializeJsonToVector(doc, buffer);
}

/**
 * Helper: Parse JSON from vector and execute callback if successful
 *
 * @param buffer Input vector
 * @param fn Lambda function to process the JsonDocument
 * @return true if parsing and processing successful
 *
 * Example:
 *   parseJsonFromVector(payload, [](const JsonDocument& doc) {
 *       String command = doc["cmd"];
 *       int value = doc["value"];
 *       // Process data...
 *   });
 */
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

// Message types for the unified message processor queue
enum class MessageProcessorItemType : uint8_t {
	INCOMING_REQUEST,  // Process incoming request (was blocking callback)
	FILE_TRANSFER      // Send file response (existing functionality)
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

// Unified message processor item
// IMPORTANT: This struct is trivially copyable (no destructor) because FreeRTOS queues
// use memcpy(), which bypasses C++ copy/move semantics. Memory must be freed manually
// in processMessageQueue() after processing.
struct MessageProcessorItem {
	MessageProcessorItemType type;
	void* dataPtr;  // Points to either IncomingRequest* or PendingFileTransfer*

	// Factory methods allocate memory that MUST be freed in processMessageQueue()
	static MessageProcessorItem createFileTransfer(uint8_t messageId, const String& project, const String& filename) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::FILE_TRANSFER;
		item.dataPtr = new PendingFileTransfer{messageId, project, filename};
		return item;
	}

	static MessageProcessorItem createRequest(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t>& data) {
		MessageProcessorItem item;
		item.type = MessageProcessorItemType::INCOMING_REQUEST;
		item.dataPtr = new IncomingRequest{protocolType, messageId, data};
		return item;
	}
};

// Unified message processor task - handles requests, file transfers, and event publishing
void btMessageProcessorTask(void *param) {
	BTRemote *server = (BTRemote *) param;
	while (true) {
		server->processMessageQueue();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

BTRemote::BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuration)
	: gatts_if_(ESP_GATT_IF_NONE)
	, app_id_(0x55)
	, loggingOutput_(loggingOutput)
	, configuration_(configuration)
	, hub_(hub)
	, deviceConnected_(false)
	, readyForEvents_(false)
	, mtu_(23)
	, nextMessageId_(0)
	, responseQueue_(nullptr)
	, responseSenderTaskHandle_(nullptr) {
	// Create unified message processor queue (handles both requests and file transfers)
	// Increased to 5 to handle concurrent requests + file transfers
	responseQueue_ = xQueueCreate(5, sizeof(MessageProcessorItem));

	// Initialize handles to invalid values
	memset(&handles_, 0, sizeof(handles_));
	memset(&connState_, 0, sizeof(connState_));
	connState_.connected = false;

	// Set global instance for static callbacks
	g_btremote_instance = this;
}

void BTRemote::begin(const char *deviceName) {
	esp_err_t ret;

	// 1. Initialize BT controller for dual-mode (BLE + Classic for future HID Host)
	// Use Arduino's btStart() helper which correctly handles dual-mode initialization
	// based on sdkconfig.defaults settings (CONFIG_BTDM_CTRL_MODE_BTDM=y)
	INFO("Starting Bluetooth controller in dual-mode (BTDM)...");
	if (!btStart()) {
		ERROR("Bluetooth controller start failed!");
		ERROR("Check that CONFIG_BT_ENABLED=y and CONFIG_BTDM_CTRL_MODE_BTDM=y in sdkconfig");
		return;
	}
	INFO("Bluetooth controller started successfully in dual-mode");

	// 2. Initialize Bluedroid stack
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

	// 5. Set device name
	ret = esp_ble_gap_set_device_name(deviceName);
	if (ret) {
		ERROR("Set device name failed: %s", esp_err_to_name(ret));
	}

	// 6. Set MTU
	ret = esp_ble_gatt_set_local_mtu(517);
	if (ret) {
		ERROR("Set local MTU failed: %s", esp_err_to_name(ret));
	}

	// 7. Register callbacks
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

	// 8. Register application profile
	ret = esp_ble_gatts_app_register(app_id_);
	if (ret) {
		ERROR("GATTS app register failed: %s", esp_err_to_name(ret));
		return;
	}

	INFO("Bluedroid BLE initialization started, waiting for registration...")

	// General Event handler for all incoming requests
	onRequest([this](uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t> &payload) {
		INFO("Processing BT message with id %d and appRequestType %d", messageId, appRequestType);

		parseJsonFromVector(payload, [this, messageId, appRequestType](const JsonDocument &requestDoc) {
			std::vector<uint8_t> response;
			if (appRequestType == APP_REQUEST_TYPE_GET_PROJECT_FILE) {
				// Special Case, responding with a large file
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

	// Unified message processor task - handles requests, file transfers, and event publishing
	xTaskCreate(
		btMessageProcessorTask,
		"BTMsgProcessor",
		8192, // Larger stack for file operations
		(void *) this,
		2, // Higher priority than log forwarder
		&responseSenderTaskHandle_);

	INFO("BLE Server started as ID %s, waiting for connections...", SERVICE_UUID);
}

// Fragment verarbeiten
void BTRemote::handleFragment(const uint8_t *data, size_t length) {
	ProtocolMessageType protocolType = (ProtocolMessageType) data[0];
	uint8_t messageId = data[1];
	uint16_t fragmentNum = (data[2] << 8) | data[3];
	uint8_t flags = data[4];

	DEBUG("Fragment received: ProtocolType=%d, ID=%d, Num=%d, Flags=0x%02X",
		(uint8_t) protocolType, messageId, fragmentNum, flags);

	// Memory Safety: Prüfe ob Limit erreicht
	if (fragmentBuffers_.size() >= MAX_CONCURRENT_MESSAGES && fragmentBuffers_.find(messageId) == fragmentBuffers_.end()) {
		WARN("Max concurrent messages reached (%d), rejecting message ID %d",
			MAX_CONCURRENT_MESSAGES, messageId);
		sendControlMessage(ControlMessageType::BUFFER_FULL, messageId);
		return;
	}

	// Buffer für diese Message ID holen oder erstellen
	auto &buffer = fragmentBuffers_[messageId];

	if (fragmentNum == 0) {
		// Erstes Fragment - Buffer initialisieren
		buffer.data.clear();
		buffer.lastFragmentTime = millis();
		buffer.receivedFragments = 0;
		buffer.protocolType = protocolType;
	}

	// Timeout prüfen
	if (millis() - buffer.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
		WARN("Fragment timeout for message ID %d", messageId);
		fragmentBuffers_.erase(messageId);
		sendControlMessage(ControlMessageType::NACK, messageId);
		return;
	}

	buffer.lastFragmentTime = millis();

	// Payload extrahieren und zum Buffer hinzufügen
	const uint8_t *payload = data + FRAGMENT_HEADER_SIZE;
	size_t payloadSize = length - FRAGMENT_HEADER_SIZE;

	// Overflow-Schutz
	if (buffer.data.size() + payloadSize > MAX_BUFFER_SIZE) {
		WARN("Buffer overflow for message ID %d", messageId);
		fragmentBuffers_.erase(messageId);
		sendControlMessage(ControlMessageType::BUFFER_FULL, messageId);
		return;
	}

	buffer.data.insert(buffer.data.end(), payload, payload + payloadSize);
	buffer.receivedFragments++;

	// Letztes Fragment?
	if (flags & FLAG_LAST_FRAGMENT) {
		DEBUG("Complete message received: ID=%d, Size=%d",
			messageId, buffer.data.size());

		// CRITICAL FIX: Queue the request for processing in separate task
		// This prevents deadlock where callback blocks waiting for indication ACK
		MessageProcessorItem item = MessageProcessorItem::createRequest(protocolType, messageId, buffer.data);

		if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
			ERROR("Message processor queue full, dropping request");
			sendControlMessage(ControlMessageType::NACK, messageId);
		} else {
			DEBUG("Request queued for processing in background task");
			// ACK sent immediately - processing happens async
			sendControlMessage(ControlMessageType::ACK, messageId);
		}

		// Buffer freigeben
		fragmentBuffers_.erase(messageId);
	}
}

// Vollständige Message verarbeiten
void BTRemote::processCompleteMessage(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data) {
	if (protocolType == ProtocolMessageType::REQUEST && data.size() > 0) {
		// Erstes Byte ist der Application Request Type
		uint8_t appRequestType = data[0];
		std::vector<uint8_t> payload(data.begin() + 1, data.end());

		DEBUG("Processing request AppRequestType=%d, Payload-Size=%d",
			appRequestType, payload.size());

		if (onRequestCallback_) {
			onRequestCallback_(appRequestType, messageId, payload);
		}
	}
}

// Control Message verarbeiten
void BTRemote::handleControlMessage(const uint8_t *data, size_t length) {
	if (length < 2) {
		return;
	}

	ControlMessageType ctrlType = (ControlMessageType) data[0];
	uint8_t messageId = data[1];

	DEBUG("Control message: Type=%d, ID=%d", (uint8_t) ctrlType, messageId);

	switch (ctrlType) {
		case ControlMessageType::RETRY:
			// Client fordert Retry an
			WARN("Client requested retry for message ID %d", messageId);
			break;
		case ControlMessageType::RESET:
			INFO("Client requested reset");
			fragmentBuffers_.clear();
			break;
		default:
			break;
	}
}
bool BTRemote::sendLargeResponse(uint8_t messageId, size_t totalSize, std::function<int(const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer)> chunkProvider) {
	if (!deviceConnected_) {
		WARN("Kein Client verbunden");
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

		// Header
		fragment.push_back((uint8_t) ProtocolMessageType::RESPONSE);
		fragment.push_back(messageId);
		fragment.push_back((i >> 8) & 0xFF);
		fragment.push_back(i & 0xFF);

		// Flags
		uint8_t flags = (i == totalFragments - 1) ? FLAG_LAST_FRAGMENT : 0x00;
		fragment.push_back(flags);

		int read = chunkProvider(i, payloadSize, fragment);
		DEBUG("Read %d bytes for chunk with index %d, fragment size is %d, flags is %d", read, i, fragment.size(), flags);

		// Send fragment using Bluedroid indicate
		esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
			handles_.response_char_handle, fragment.size(), fragment.data(), true); // true = need confirm (indicate)

		if (ret != ESP_OK) {
			ERROR("Failed to indicate fragment %d of %d: %s", i + 1, totalFragments, esp_err_to_name(ret));
			return false;
		}

		// Delay between fragments for browser JS event loop
		if (i < totalFragments - 1) {
			vTaskDelay(pdMS_TO_TICKS(INTER_FRAGMENT_DELAY_MS));
		}
	}

	return true;
}

// Fragmentierte Nachricht senden
bool BTRemote::sendFragmented(uint16_t char_handle, ProtocolMessageType protocolType,
	uint8_t messageId, const std::vector<uint8_t> &data) {
	if (!deviceConnected_) {
		WARN("Kein Client verbunden");
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

		// Header
		fragment.push_back((uint8_t) protocolType);
		fragment.push_back(messageId);
		fragment.push_back((i >> 8) & 0xFF);
		fragment.push_back(i & 0xFF);

		// Flags
		uint8_t flags = (i == totalFragments - 1) ? FLAG_LAST_FRAGMENT : 0x00;
		fragment.push_back(flags);

		// Payload
		size_t start = i * payloadSize;
		size_t end = std::min(start + payloadSize, data.size());

		if (start < data.size()) {
			fragment.insert(fragment.end(), data.begin() + start, data.begin() + end);
		}

		// Send fragment using Bluedroid indicate
		DEBUG("Sending indicate: gatts_if=%d, conn_id=%d, handle=%d, len=%d, protocolType=%d, msgId=%d",
			gatts_if_, connState_.conn_id, char_handle, fragment.size(), (uint8_t)protocolType, messageId);

		esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
			char_handle, fragment.size(), fragment.data(), true); // true = need confirm (indicate)

		if (ret != ESP_OK) {
			ERROR("Failed to indicate fragment %d of %d: %s", i + 1, totalFragments, esp_err_to_name(ret));
			return false;
		} else {
			DEBUG("Indicate sent successfully for fragment %d/%d", i + 1, totalFragments);
		}

		// Delay between fragments for browser JS event loop
		if (i < totalFragments - 1) {
			vTaskDelay(pdMS_TO_TICKS(INTER_FRAGMENT_DELAY_MS));
		}
	}

	return true;
}

// Response senden
bool BTRemote::sendResponse(uint8_t messageId, const std::vector<uint8_t> &data) {
	INFO("sendResponse: msgId=%d, handle=%d (response=%d, event=%d)",
		messageId, handles_.response_char_handle, handles_.response_char_handle, handles_.event_char_handle);
	return sendFragmented(handles_.response_char_handle, ProtocolMessageType::RESPONSE, messageId, data);
}

// Event senden
// Note: This is called from publishLogMessages/publishCommands/publishPortstatus
// which run in background task context (not callback), so direct sending is safe
bool BTRemote::sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data) {
	// Event-Daten: [Application Event Type][Payload...]
	std::vector<uint8_t> eventData;
	eventData.reserve(data.size() + 1);
	eventData.push_back(appEventType);
	eventData.insert(eventData.end(), data.begin(), data.end());

	return sendFragmented(handles_.event_char_handle, ProtocolMessageType::EVENT, nextMessageId_++, eventData);
}

// Control Message senden
void BTRemote::sendControlMessage(ControlMessageType type, uint8_t messageId) {
	if (!deviceConnected_) {
		return;
	}

	uint8_t data[2] = {(uint8_t) type, messageId};
	esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_handle, 2, data, false); // false = no confirm (notify)

	if (ret != ESP_OK) {
		ERROR("Failed to send control message type %d: %s", (uint8_t) type, esp_err_to_name(ret));
	}
}

// MTU-Information an Client senden
void BTRemote::sendMTUNotification() {
	if (!deviceConnected_) {
		return;
	}

	uint8_t data[4];
	data[0] = (uint8_t) ControlMessageType::MTU_INFO;
	data[1] = 0; // Reserved
	data[2] = (mtu_ >> 8) & 0xFF;
	data[3] = mtu_ & 0xFF;

	esp_err_t ret = esp_ble_gatts_send_indicate(gatts_if_, connState_.conn_id,
		handles_.control_char_handle, 4, data, false); // false = no confirm (notify)

	if (ret != ESP_OK) {
		ERROR("Failed to send MTU notification: %s", esp_err_to_name(ret));
	} else {
		INFO("MTU-Info sent: %d bytes", mtu_);
	}
}

// Request Callback registrieren
void BTRemote::onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback) {
	onRequestCallback_ = callback;
}

// Timeout-Handler (sollte regelmäßig aufgerufen werden)
void BTRemote::loop() {
	uint32_t now = millis();

	for (auto it = fragmentBuffers_.begin(); it != fragmentBuffers_.end();) {
		if (now - it->second.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
			WARN("Timeout: Message ID %d removed", it->first);
			sendControlMessage(ControlMessageType::NACK, it->first);
			it = fragmentBuffers_.erase(it);
		} else {
			++it;
		}
	}
}

bool BTRemote::isConnected() const {
	return deviceConnected_;
}

size_t BTRemote::getMTU() const {
	return mtu_;
}

void BTRemote::publishLogMessages() {
	// Non-blocking check (timeout = 0) to avoid latency accumulation
	String logMessage = loggingOutput_->waitForLogMessage(0);
	while (logMessage.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {
			std::vector<uint8_t> response;
			createJsonResponse(response, [logMessage](JsonDocument &responseDoc) {
				responseDoc["message"] = logMessage;
			});

			sendEvent(APP_EVENT_TYPE_LOG, response);
		}

		// Keep checking non-blocking until queue is empty
		logMessage = loggingOutput_->waitForLogMessage(0);
	}
}

void BTRemote::publishCommands() {
	// Non-blocking check (timeout = 0) to avoid latency accumulation
	String command = Commands::instance()->waitForCommand(0);
	while (command.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(command, response);

			sendEvent(APP_EVENT_TYPE_COMMAND, response);
		}

		// Keep checking non-blocking until queue is empty
		command = Commands::instance()->waitForCommand(0);
	}
}

void BTRemote::publishPortstatus() {
	// Non-blocking check (timeout = 0) to avoid latency accumulation
	String status = Portstatus::instance()->waitForStatus(0);
	while (status.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(status, response);

			sendEvent(APP_EVENT_TYPE_PORTSTATUS, response);
		}

		// Keep checking non-blocking until queue is empty
		status = Portstatus::instance()->waitForStatus(0);
	}
}

bool BTRemote::reqStopProgram(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	return hub_->stopLUACode();
}

bool BTRemote::reqGetProjectFile(uint8_t messageId, const JsonDocument &requestDoc) {
	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();

	INFO("Queueing file transfer request for %s of project %s", filename.c_str(), project.c_str());

	// Queue the file transfer for processing in background task
	MessageProcessorItem item = MessageProcessorItem::createFileTransfer(messageId, project, filename);
	if (xQueueSend(responseQueue_, &item, 0) != pdTRUE) {
		WARN("Message processor queue full, dropping file transfer request");
		return false;
	}

	return true;
}

// Unified message processor - handles both incoming requests and file transfers
void BTRemote::processMessageQueue() {
	MessageProcessorItem item;

	// Wait for a message with short timeout (10ms) for responsive event publishing
	if (xQueueReceive(responseQueue_, &item, 0) == pdTRUE) {

		if (item.type == MessageProcessorItemType::INCOMING_REQUEST) {
			// Process incoming request
			IncomingRequest* req = static_cast<IncomingRequest*>(item.dataPtr);
			INFO("Processing incoming request: msgId=%d, protocolType=%d, size=%d",
				req->messageId, (int)req->protocolType, req->data.size());

			// Call the original processing function (now safe, we're in separate task!)
			processCompleteMessage(req->protocolType, req->messageId, req->data);

			// CRITICAL: Manually free the allocated memory
			// (No destructor on MessageProcessorItem because FreeRTOS uses memcpy)
			delete req;

		} else if (item.type == MessageProcessorItemType::FILE_TRANSFER) {
			// Process file transfer
			PendingFileTransfer* transfer = static_cast<PendingFileTransfer*>(item.dataPtr);
			INFO("Processing queued file transfer for %s of project %s",
				transfer->filename.c_str(), transfer->project.c_str());

			File projectFile = configuration_->getProjectFile(transfer->project, transfer->filename);
			if (projectFile) {
				size_t projectSize = projectFile.size();
				INFO("Response has size %d", projectSize);

				// Now we're in a separate task context, safe to do long-running operations
				sendLargeResponse(transfer->messageId, projectSize, [&projectFile](const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer) {
					INFO("Reading chunk with index %d from file", index);
					char buffer[maxChunkSize];
					size_t read = projectFile.readBytes(&buffer[0], maxChunkSize);
					for (int i = 0; i < read; i++) {
						dataContainer.push_back(buffer[i]);
					}
					INFO("Read %d bytes from file for chunk %d", read, index);
					return read;
				});

				projectFile.close();
			} else {
				WARN("File not found: %s/%s", transfer->project.c_str(), transfer->filename.c_str());
			}

			// CRITICAL: Manually free the allocated memory
			// (No destructor on MessageProcessorItem because FreeRTOS uses memcpy)
			delete transfer;
		}
	}

	// After processing (or timeout), check for pending events and send them
	// This prevents race conditions - all BLE communication is serialized through this task
	if (isConnected() && readyForEvents_) {
		//publishLogMessages();
		publishCommands();
		publishPortstatus();
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
// This file contains the Bluedroid event handlers implementation
// Append this content to btremote.cpp after the existing methods

// ============================================================================
// STATIC EVENT HANDLERS
// ============================================================================

void BTRemote::gattsEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
	esp_ble_gatts_cb_param_t *param) {
	// Route to instance methods
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

// ============================================================================
// INSTANCE EVENT HANDLERS
// ============================================================================

void BTRemote::handleGattsRegister(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	if (param->reg.status != ESP_GATT_OK) {
		ERROR("GATTS register failed, status=%d", param->reg.status);
		return;
	}

	INFO("GATTS registered, app_id=%d, gatts_if=%d", param->reg.app_id, gatts_if);
	gatts_if_ = gatts_if;

	// Create service with proper service ID structure
	esp_gatt_srvc_id_t service_id = {
		.id = {
			.uuid = stringToUUID128(SERVICE_UUID),
			.inst_id = 0,
		},
		.is_primary = true,
	};

	esp_err_t ret = esp_ble_gatts_create_service(gatts_if, &service_id, 20); // 20 handles needed
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

	// Start service
	esp_err_t ret = esp_ble_gatts_start_service(handles_.service_handle);
	if (ret) {
		ERROR("Start service failed: %s", esp_err_to_name(ret));
		return;
	}

	// Add Request characteristic (WRITE)
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

	// Determine which characteristic was added by comparing UUIDs
	esp_bt_uuid_t req_uuid = stringToUUID128(REQUEST_CHAR_UUID);
	esp_bt_uuid_t resp_uuid = stringToUUID128(RESPONSE_CHAR_UUID);
	esp_bt_uuid_t evt_uuid = stringToUUID128(EVENT_CHAR_UUID);
	esp_bt_uuid_t ctrl_uuid = stringToUUID128(CONTROL_CHAR_UUID);

	esp_bt_uuid_t *added_uuid = &param->add_char.char_uuid;

	// Debug: Log the UUID being added
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

		// Add Response characteristic (INDICATE)
		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_INDICATE;
		esp_ble_gatts_add_char(handles_.service_handle, &resp_uuid,
			ESP_GATT_PERM_READ, prop, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, resp_uuid.uuid.uuid128, 16) == 0) {
		handles_.response_char_handle = param->add_char.attr_handle;
		INFO("Response char handle=%d (UUID: %s)", handles_.response_char_handle, RESPONSE_CHAR_UUID);

		// Add CCCD for indicate
		esp_bt_uuid_t descr_uuid;
		descr_uuid.len = ESP_UUID_LEN_16;
		descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
		esp_ble_gatts_add_char_descr(handles_.service_handle, &descr_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, evt_uuid.uuid.uuid128, 16) == 0) {
		handles_.event_char_handle = param->add_char.attr_handle;
		INFO("Event char handle=%d (UUID: %s)", handles_.event_char_handle, EVENT_CHAR_UUID);

		// Add CCCD for indicate
		esp_bt_uuid_t descr_uuid;
		descr_uuid.len = ESP_UUID_LEN_16;
		descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
		esp_ble_gatts_add_char_descr(handles_.service_handle, &descr_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, nullptr, nullptr);

	} else if (memcmp(added_uuid->uuid.uuid128, ctrl_uuid.uuid.uuid128, 16) == 0) {
		handles_.control_char_handle = param->add_char.attr_handle;
		INFO("Control char handle=%d (UUID: %s)", handles_.control_char_handle, CONTROL_CHAR_UUID);

		// Add CCCD for notify
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

	// Track CCCD handles (descriptors come after their characteristics)
	if (handles_.response_cccd_handle == 0 && handles_.response_char_handle != 0) {
		handles_.response_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Response CCCD handle=%d", handles_.response_cccd_handle);

		// Add Event characteristic (INDICATE)
		esp_bt_uuid_t evt_uuid = stringToUUID128(EVENT_CHAR_UUID);
		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_INDICATE;
		esp_ble_gatts_add_char(handles_.service_handle, &evt_uuid,
			ESP_GATT_PERM_READ, prop, nullptr, nullptr);

	} else if (handles_.event_cccd_handle == 0 && handles_.event_char_handle != 0) {
		handles_.event_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Event CCCD handle=%d", handles_.event_cccd_handle);

		// Add Control characteristic (WRITE + NOTIFY)
		esp_bt_uuid_t ctrl_uuid = stringToUUID128(CONTROL_CHAR_UUID);
		esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
		esp_ble_gatts_add_char(handles_.service_handle, &ctrl_uuid,
			ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, prop, nullptr, nullptr);

	} else if (handles_.control_cccd_handle == 0 && handles_.control_char_handle != 0) {
		handles_.control_cccd_handle = param->add_char_descr.attr_handle;
		INFO("Control CCCD handle=%d", handles_.control_cccd_handle);

		// All characteristics and descriptors created - start advertising
		INFO("All GATT attributes created successfully");

		// Build raw advertising data with 128-bit service UUID
		// Format: [Length][Type][Data]
		// For 128-bit UUID, we need ESP_BLE_AD_TYPE_128SRV_CMPL (0x07)
		static uint8_t raw_adv_data[31]; // Max BLE advertising data size
		int idx = 0;

		// Flags: General Discoverable + BR/EDR Not Supported
		raw_adv_data[idx++] = 2;    // Length
		raw_adv_data[idx++] = 0x01; // Type: Flags
		raw_adv_data[idx++] = 0x06; // Flags: General Discoverable (0x02) + BR/EDR Not Supported (0x04)

		// Complete 128-bit Service UUID
		raw_adv_data[idx++] = 17;   // Length (16 bytes UUID + 1 byte type)
		raw_adv_data[idx++] = 0x07; // Type: Complete List of 128-bit Service UUIDs
		// Add UUID in little-endian format
		esp_bt_uuid_t service_uuid = stringToUUID128(SERVICE_UUID);
		for (int i = 0; i < 16; i++) {
			raw_adv_data[idx++] = service_uuid.uuid.uuid128[i];
		}

		// Debug: Log the raw advertising data
		char hex_str[100];
		int hex_idx = 0;
		for (int i = 0; i < idx && hex_idx < 90; i++) {
			hex_idx += snprintf(hex_str + hex_idx, sizeof(hex_str) - hex_idx, "%02x ", raw_adv_data[i]);
		}
		INFO("Raw advertising data (%d bytes): %s", idx, hex_str);

		// Set raw advertising data
		esp_err_t ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, idx);
		if (ret != ESP_OK) {
			ERROR("Set raw advertising data failed: %s", esp_err_to_name(ret));
		} else {
			INFO("Raw advertising data set successfully");
		}

		// Also set scan response data with device name for better discoverability
		// This is optional but helps Web Bluetooth show the device name
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

	// Small delay for stability
	vTaskDelay(pdMS_TO_TICKS(100));
	sendMTUNotification();
}

void BTRemote::handleGattsDisconnect(esp_ble_gatts_cb_param_t *param) {
	readyForEvents_ = false;
	deviceConnected_ = false;
	connState_.connected = false;
	fragmentBuffers_.clear();

	INFO("Client disconnected, conn_id=%d", param->disconnect.conn_id);

	// Restart advertising
	esp_ble_gap_start_advertising(&g_adv_params);
}

void BTRemote::handleGattsMTU(esp_ble_gatts_cb_param_t *param) {
	mtu_ = param->mtu.mtu - 3; // Subtract ATT header
	connState_.mtu = mtu_;
	INFO("MTU changed to: %d (notified with %d)", mtu_, param->mtu.mtu);
	sendMTUNotification();
}

void BTRemote::handleGattsWrite(esp_ble_gatts_cb_param_t *param) {
	const uint8_t *data = param->write.value;
	size_t length = param->write.len;

	// Check if this is a CCCD write (2 bytes for enable/disable notifications/indications)
	if (param->write.handle == handles_.response_cccd_handle ||
		param->write.handle == handles_.event_cccd_handle ||
		param->write.handle == handles_.control_cccd_handle) {

		if (length == 2) {
			uint16_t descr_value = data[0] | (data[1] << 8);
			const char *char_name = "Unknown";
			if (param->write.handle == handles_.response_cccd_handle) char_name = "Response";
			else if (param->write.handle == handles_.event_cccd_handle) char_name = "Event";
			else if (param->write.handle == handles_.control_cccd_handle) char_name = "Control";

			INFO("CCCD write for %s characteristic: 0x%04x (Notify=%s, Indicate=%s)",
				char_name, descr_value,
				(descr_value & 0x0001) ? "YES" : "NO",
				(descr_value & 0x0002) ? "YES" : "NO");
		}

		// Send response if needed
		if (param->write.need_rsp) {
			esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
				param->write.trans_id, ESP_GATT_OK, nullptr);
		}
		return;
	}

	// Regular characteristic writes - must have at least fragment header
	if (param->write.len < FRAGMENT_HEADER_SIZE) {
		sendControlMessage(ControlMessageType::NACK, 0);
		// Send response if needed
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

	// Send response if needed
	if (param->write.need_rsp) {
		esp_ble_gatts_send_response(gatts_if_, param->write.conn_id,
			param->write.trans_id, ESP_GATT_OK, nullptr);
	}
}
