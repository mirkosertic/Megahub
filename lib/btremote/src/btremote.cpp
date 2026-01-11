#include "btremote.h"

#include "commands.h"
#include "logging.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
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

#define APP_REQUEST_TYPE_STOP_PROGRAM	 0x01
#define APP_REQUEST_TYPE_GET_ROJECT_FILE 0x02
#define APP_REQUEST_TYPE_PUT_ROJECT_FILE 0x03
#define APP_REQUEST_TYPE_DELETE_PROJECT	 0x04
#define APP_REQUEST_TYPE_SYNTAX_CHECK	 0x05
#define APP_REQUEST_TYPE_RUN_PROGRAM	 0x06
#define APP_REQUEST_TYPE_GET_PROJECTS	 0x07
#define APP_REQUEST_TYPE_GET_AUTOSTART	 0x08
#define APP_REQUEST_TYPE_PUT_AUTOSTART	 0x09

#define APP_EVENT_TYPE_LOG		  0x01
#define APP_EVENT_TYPE_PORTSTATUS 0x02
#define APP_EVENT_TYPE_COMMAND	  0x03

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
		WARN("Expected %d bytes, wrote %d bytes\n", size, written);
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

void btLogForwarderTask(void *param) {
	BTRemote *server = (BTRemote *) param;
	while (true) {
		server->publishLogMessages();
		server->publishCommands();
		server->publishPortstatus();
	}
}

BTRemote::BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuration)
	: loggingOutput_(loggingOutput)
	, configuration_(configuration)
	, hub_(hub)
	, deviceConnected_(false)
	, mtu_(23)
	, nextMessageId_(0) {
}

void BTRemote::begin(const char *deviceName) {
	BLEDevice::init(deviceName);
	BLEDevice::setMTU(517); // Try maximum MTU

	// Server erstellen
	pServer_ = BLEDevice::createServer();
	pServer_->setCallbacks(this);

	// Service erstellen
	pService_ = pServer_->createService(SERVICE_UUID);

	// Request Characteristic (Write)
	pRequestChar_ = pService_->createCharacteristic(
		REQUEST_CHAR_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	pRequestChar_->setCallbacks(this);

	// Response Characteristic (Notify)
	pResponseChar_ = pService_->createCharacteristic(
		RESPONSE_CHAR_UUID,
		BLECharacteristic::PROPERTY_NOTIFY);
	pResponseChar_->addDescriptor(new BLE2902());

	// Event Characteristic (Notify)
	pEventChar_ = pService_->createCharacteristic(
		EVENT_CHAR_UUID,
		BLECharacteristic::PROPERTY_NOTIFY);
	pEventChar_->addDescriptor(new BLE2902());

	// Control Characteristic (Write/Notify)
	pControlChar_ = pService_->createCharacteristic(
		CONTROL_CHAR_UUID,
		BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
	pControlChar_->setCallbacks(this);
	pControlChar_->addDescriptor(new BLE2902());

	// Service starten
	pService_->start();

	// Advertising starten
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();

	// General Event handler for all incoming requests
	onRequest([this](uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t> &payload) {
		INFO("Processing BTMessage with id %d and appRequestType %d", messageId, appRequestType);

		parseJsonFromVector(payload, [this, messageId, appRequestType](const JsonDocument &requestDoc) {
			std::vector<uint8_t> response;
			createJsonResponse(response, [this, appRequestType, requestDoc](JsonDocument &responseDoc) {
				bool result = false;
				switch (appRequestType) {
					case APP_REQUEST_TYPE_STOP_PROGRAM:
						result = reqStopProgram(requestDoc, responseDoc);
						break;
					case APP_REQUEST_TYPE_GET_ROJECT_FILE:
						result = reqGetProjectFile(requestDoc, responseDoc);
						break;
					case APP_REQUEST_TYPE_PUT_ROJECT_FILE:
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
					default:
						WARN("Not supported appRequestType : %d", appRequestType);
						responseDoc["error"] = "Not supported appRequestType!";
						break;
				}
				responseDoc["result"] = result;
			});

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

	INFO("BLE Server started as ID %s, waiting for Connections...", SERVICE_UUID);
}

// Server Callbacks
void BTRemote::onConnect(BLEServer *pServer) {
	deviceConnected_ = true;
	mtu_ = pServer->getPeerMTU(pServer->getConnId()) - 3; // ATT Header abziehen
	INFO("Client connected, MTU: %d", mtu_);

	// send MTU to client
	vTaskDelay(pdMS_TO_TICKS(200));
	sendMTUNotification();
}

void BTRemote::onDisconnect(BLEServer *pServer) {
	deviceConnected_ = false;
	fragmentBuffers_.clear();
	INFO("Client disconnected");
	BLEDevice::startAdvertising();
}

// Characteristic Callbacks
void BTRemote::onWrite(BLECharacteristic *pCharacteristic) {
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

	// Buffer für diese Message ID holen oder erstellen
	auto &buffer = fragmentBuffers_[messageId];

	if (fragmentNum == 0) {
		// Erste Fragment - Buffer initialisieren
		buffer.data.clear();
		buffer.lastFragmentTime = millis();
		buffer.receivedFragments = 0;
		buffer.protocolType = protocolType;
	}

	// Timeout prüfen
	if (millis() - buffer.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
		WARN("Fragment timeout for Message ID %d", messageId);
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
		WARN("Buffer overflow für Message ID %d", messageId);
		fragmentBuffers_.erase(messageId);
		sendControlMessage(ControlMessageType::BUFFER_FULL, messageId);
		return;
	}

	buffer.data.insert(buffer.data.end(), payload, payload + payloadSize);
	buffer.receivedFragments++;

	// Letztes Fragment?
	if (flags & FLAG_LAST_FRAGMENT) {
		DEBUG("Complete Message received: ID=%d, Size=%d",
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

		DEBUG("Processinf Request AppRequestType=%d, Payload-Size=%d",
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

	DEBUG("Control Message: Type=%d, ID=%d", (uint8_t) ctrlType, messageId);

	switch (ctrlType) {
		case ControlMessageType::RETRY:
			// Client fordert Retry an
			break;
		case ControlMessageType::RESET:
			fragmentBuffers_.clear();
			break;
		default:
			break;
	}
}

// Fragmentierte Nachricht senden
bool BTRemote::sendFragmented(BLECharacteristic *characteristic, ProtocolMessageType protocolType,
	uint8_t messageId, const std::vector<uint8_t> &data) {
	if (!deviceConnected_) {
		WARN("Kein Client verbunden");
		return false;
	}

	size_t payloadSize = mtu_ - FRAGMENT_HEADER_SIZE;
	size_t totalFragments = (data.size() + payloadSize - 1) / payloadSize;

	if (totalFragments == 0) {
		totalFragments = 1;
	}

	DEBUG("Sendiong fragmented: ProtocolType=%d, ID=%d, Size=%d, Fragments=%d",
		(uint8_t) protocolType, messageId, data.size(), totalFragments);

	for (size_t i = 0; i < totalFragments; i++) {
		std::vector<uint8_t> fragment;
		fragment.reserve(mtu_);

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

		// Fragment senden
		characteristic->setValue(fragment.data(), fragment.size());
		characteristic->notify();

		// Kleine Pause zwischen Fragmenten (wichtig!)
		delay(10);
	}

	return true;
}

// Response senden
bool BTRemote::sendResponse(uint8_t messageId, const std::vector<uint8_t> &data) {
	return sendFragmented(pResponseChar_, ProtocolMessageType::RESPONSE, messageId, data);
}

// Event senden
bool BTRemote::sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data) {
	// Event-Daten: [Application Event Type][Payload...]
	std::vector<uint8_t> eventData;
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
	pControlChar_->notify();
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
	pControlChar_->notify();

	INFO("MTU-Info sent: %d bytes", mtu_);
}

// Request Callback registrieren
// Callback-Signatur: void callback(uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t>& payload)
void BTRemote::onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback) {
	onRequestCallback_ = callback;
}

// Timeout-Handler (sollte regelmäßig aufgerufen werden)
void BTRemote::loop() {
	uint32_t now = millis();

	for (auto it = fragmentBuffers_.begin(); it != fragmentBuffers_.end();) {
		if (now - it->second.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
			WARN("Timeout: Message ID %d entfernt", it->first);
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
		if (deviceConnected_) {
			std::vector<uint8_t> response;
			createJsonResponse(response, [this, logMessage](JsonDocument &responseDoc) {
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
		if (deviceConnected_) {

			std::vector<uint8_t> response;
			stringToVector(command, response);

			sendEvent(APP_EVENT_TYPE_COMMAND, response);
		}

		command = Commands::instance()->waitForCommand(pdMS_TO_TICKS(5));
	}
}

void BTRemote::publishPortstatus() {
	String command = Portstatus::instance()->waitForStatus(pdMS_TO_TICKS(5));
	while (command.length() > 0) {
		if (deviceConnected_) {

			std::vector<uint8_t> response;
			stringToVector(command, response);

			sendEvent(APP_EVENT_TYPE_PORTSTATUS, response);
		}

		command = Portstatus::instance()->waitForStatus(pdMS_TO_TICKS(5));
	}
}

bool BTRemote::reqStopProgram(const JsonDocument &requestDoc, JsonDocument &responseDoc) {
	return hub_->stopLUACode();
}

bool BTRemote::reqGetProjectFile(const JsonDocument &requestDoc, JsonDocument &responseDoc) {

	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();

	responseDoc["content"] = configuration_->getProjectFileContentAsString(project, filename);
	return true;
}

bool BTRemote::reqPutProjectFile(const JsonDocument &requestDoc, JsonDocument &responseDoc) {

	String project = requestDoc["project"].as<String>();
	String filename = requestDoc["filename"].as<String>();
	String content = requestDoc["content"].as<String>();

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
	for (int i = 0; i < foundProjects.size(); i++) {
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
