#include "btremote.h"

#include "commands.h"
#include "logging.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <algorithm>
#include <map>
#include <vector>

// UUIDs für Service und Characteristics
#define SERVICE_UUID	   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define REQUEST_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESPONSE_CHAR_UUID "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define EVENT_CHAR_UUID	   "d8de624e-140f-4a22-8594-e2216b84a5f2"
#define CONTROL_CHAR_UUID  "f78ebbff-c8b7-4107-93de-889a6a06d408"

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
const uint32_t INTER_FRAGMENT_DELAY_MS = 100;

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

// Structure to hold pending file transfer requests
struct PendingFileTransfer {
	uint8_t messageId;
	String project;
	String filename;
};

void btLogForwarderTask(void *param) {
	BTRemote *server = (BTRemote *) param;
	while (true) {
		server->publishLogMessages();
		server->publishCommands();
		server->publishPortstatus();
	}
}

void btResponseSenderTask(void *param) {
	BTRemote *server = (BTRemote *) param;
	while (true) {
		server->processResponseQueue();
	}
}

BTRemote::BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuration)
	: loggingOutput_(loggingOutput)
	, configuration_(configuration)
	, hub_(hub)
	, deviceConnected_(false)
	, readyForEvents_(false)
	, mtu_(23)
	, nextMessageId_(0)
	, responseQueue_(nullptr)
	, responseSenderTaskHandle_(nullptr) {
	// Create response queue (max 3 pending file transfers)
	responseQueue_ = xQueueCreate(3, sizeof(PendingFileTransfer));
}

void BTRemote::begin(const char *deviceName) {
	// NimBLE initialisieren
	NimBLEDevice::init(deviceName);

	// MTU setzen - NimBLE unterstützt bis zu 512
	NimBLEDevice::setMTU(517);

	// Server erstellen
	pServer_ = NimBLEDevice::createServer();
	pServer_->setCallbacks(this);

	// Service erstellen
	pService_ = pServer_->createService(SERVICE_UUID);

	// Request Characteristic (Write)
	pRequestChar_ = pService_->createCharacteristic(
		REQUEST_CHAR_UUID,
		NIMBLE_PROPERTY::WRITE);
	pRequestChar_->setCallbacks(this);

	// Response Characteristic (Indicate - provides automatic flow control)
	pResponseChar_ = pService_->createCharacteristic(
		RESPONSE_CHAR_UUID,
		NIMBLE_PROPERTY::INDICATE);

	// Event Characteristic (Indicate - provides automatic flow control)
	pEventChar_ = pService_->createCharacteristic(
		EVENT_CHAR_UUID,
		NIMBLE_PROPERTY::INDICATE);

	// Control Characteristic (Write/Notify)
	pControlChar_ = pService_->createCharacteristic(
		CONTROL_CHAR_UUID,
		NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
	pControlChar_->setCallbacks(this);

	// Service starten
	pService_->start();

	// Advertising konfigurieren und starten
	NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->enableScanResponse(true);
	pAdvertising->setPreferredParams(0x06, 0x12); // 22.5ms

	if (!NimBLEDevice::startAdvertising()) {
		ERROR("Failed to start advertising!");
	}

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

	// Logging forwarder task
	xTaskCreate(
		btLogForwarderTask,
		"BTLogForwarderTask",
		4096,
		(void *) this,
		1,
		&logforwarderTaskHandle_ // Store task handle for cancellation
	);

	// Response sender task - runs outside callback context
	xTaskCreate(
		btResponseSenderTask,
		"BTResponseSender",
		8192, // Larger stack for file operations
		(void *) this,
		2, // Higher priority than log forwarder
		&responseSenderTaskHandle_);

	INFO("BLE Server started as ID %s, waiting for connections...", SERVICE_UUID);
}

// Server Callbacks
void BTRemote::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
	readyForEvents_ = false;
	deviceConnected_ = true;
	mtu_ = connInfo.getMTU() - 3; // ATT Header abziehen

	INFO("Client connected, MTU: %d, was notified with MTU %d", mtu_, connInfo.getMTU());

	// MTU-Info an Client senden (kleine Verzögerung für Stabilität)
	vTaskDelay(pdMS_TO_TICKS(100));
	sendMTUNotification();
}

void BTRemote::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
	readyForEvents_ = false;
	deviceConnected_ = false;
	fragmentBuffers_.clear();

	INFO("Client disconnected");

	// Advertising neu starten
	if (!NimBLEDevice::startAdvertising()) {
		ERROR("Failed to restart advertising after disconnect!");
	}
}

void BTRemote::onMTUChange(uint16_t mtu, NimBLEConnInfo &connInfo) {
	mtu_ = max(12, mtu - 3); // ATT Header abziehen
	INFO("MTU changed to: %d, was notified with %d", mtu_, mtu);
	sendMTUNotification();
}

// Characteristic Callbacks
void BTRemote::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
	std::string value = pCharacteristic->getValue();

	if (value.length() < FRAGMENT_HEADER_SIZE) {
		sendControlMessage(ControlMessageType::NACK, 0);
		return;
	}

	const uint8_t *data = (const uint8_t *) value.data();
	size_t length = value.length();

	if (pCharacteristic == pRequestChar_) {
		handleFragment(data, length);
	} else if (pCharacteristic == pControlChar_) {
		handleControlMessage(data, length);
	}
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

		// Vollständige Message verarbeiten
		processCompleteMessage(protocolType, messageId, buffer.data);

		// Buffer freigeben
		fragmentBuffers_.erase(messageId);

		// ACK senden
		sendControlMessage(ControlMessageType::ACK, messageId);
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

		// Using indicate() instead of notify() - provides automatic flow control
		// The indicate() call blocks until client ACK is received
		pResponseChar_->setValue(fragment.data(), fragment.size());
		if (!pResponseChar_->indicate()) {
			ERROR("Failed to indicate fragment %d of %d", i + 1, totalFragments);
			return false;
		}

		// Note: indicate() already blocks until ACK, which may provide enough time
		// for browser's JS event loop. Only add delay if events are still dropped.
		// Uncomment if needed:
		if (i < totalFragments - 1) {
			vTaskDelay(pdMS_TO_TICKS(INTER_FRAGMENT_DELAY_MS));
		}
	}

	return true;
}

// Fragmentierte Nachricht senden
bool BTRemote::sendFragmented(NimBLECharacteristic *characteristic, ProtocolMessageType protocolType,
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

		// Fragment senden mit Error-Handling
		// Using indicate() instead of notify() - provides automatic flow control
		characteristic->setValue(fragment.data(), fragment.size());
		if (!characteristic->indicate()) {
			ERROR("Failed to indicate fragment %d of %d", i + 1, totalFragments);
			return false;
		}

		// Note: indicate() already blocks until ACK, which may provide enough time
		// for browser's JS event loop. Only add delay if events are still dropped.
		// Uncomment if needed:
		if (i < totalFragments - 1) {
			vTaskDelay(pdMS_TO_TICKS(INTER_FRAGMENT_DELAY_MS));
		}
	}

	return true;
}

// Response senden
bool BTRemote::sendResponse(uint8_t messageId, const std::vector<uint8_t> &data) {
	return sendFragmented(pResponseChar_, ProtocolMessageType::RESPONSE, messageId, data);
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

	return sendFragmented(pEventChar_, ProtocolMessageType::EVENT, nextMessageId_++, eventData);
}

// Control Message senden
void BTRemote::sendControlMessage(ControlMessageType type, uint8_t messageId) {
	if (!deviceConnected_) {
		return;
	}

	uint8_t data[2] = {(uint8_t) type, messageId};
	pControlChar_->setValue(data, 2);
	if (!pControlChar_->notify()) {
		ERROR("Failed to send control message type %d", (uint8_t) type);
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

	pControlChar_->setValue(data, 4);
	if (!pControlChar_->notify()) {
		ERROR("Failed to send MTU notification");
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
	String logMessage = loggingOutput_->waitForLogMessage(pdMS_TO_TICKS(5));
	while (logMessage.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {
			std::vector<uint8_t> response;
			createJsonResponse(response, [logMessage](JsonDocument &responseDoc) {
				responseDoc["message"] = logMessage;
			});

			sendEvent(APP_EVENT_TYPE_LOG, response);
		}

		logMessage = loggingOutput_->waitForLogMessage(pdMS_TO_TICKS(5));
	}
}

void BTRemote::publishCommands() {
	String command = Commands::instance()->waitForCommand(pdMS_TO_TICKS(5));
	while (command.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(command, response);

			sendEvent(APP_EVENT_TYPE_COMMAND, response);
		}

		command = Commands::instance()->waitForCommand(pdMS_TO_TICKS(5));
	}
}

void BTRemote::publishPortstatus() {
	String status = Portstatus::instance()->waitForStatus(pdMS_TO_TICKS(5));
	while (status.length() > 0) {
		if (deviceConnected_ && readyForEvents_) {

			std::vector<uint8_t> response;
			stringToVector(status, response);

			sendEvent(APP_EVENT_TYPE_PORTSTATUS, response);
		}

		status = Portstatus::instance()->waitForStatus(pdMS_TO_TICKS(5));
	}
}

bool BTRemote::reqStopProgram(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	return hub_->stopLUACode();
}

bool BTRemote::reqGetProjectFile(uint8_t messageId, const JsonDocument &requestDoc) {
	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();

	INFO("Queueing file transfer request for %s of project %s", filename.c_str(), project.c_str());

	// Queue the request instead of sending directly (to avoid blocking in callback)
	PendingFileTransfer transfer = {messageId, project, filename};
	if (xQueueSend(responseQueue_, &transfer, 0) != pdTRUE) {
		WARN("Response queue full, dropping file transfer request");
		return false;
	}

	return true;
}

void BTRemote::processResponseQueue() {
	PendingFileTransfer transfer;

	// Wait for a pending transfer request (blocks until available)
	if (xQueueReceive(responseQueue_, &transfer, portMAX_DELAY) == pdTRUE) {
		INFO("Processing queued file transfer for %s of project %s",
			transfer.filename.c_str(), transfer.project.c_str());

		File projectFile = configuration_->getProjectFile(transfer.project, transfer.filename);
		if (projectFile) {
			size_t projectSize = projectFile.size();
			INFO("Response has size %d", projectSize);

			// Now we're in a separate task context, safe to do long-running operations
			sendLargeResponse(transfer.messageId, projectSize, [&projectFile](const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer) {
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
			WARN("File not found: %s/%s", transfer.project.c_str(), transfer.filename.c_str());
		}
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
