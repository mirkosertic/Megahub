#include "btremote.h"

#include "logging.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <map>
#include <vector>
#include <ArduinoJson.h>

// UUIDs für Service und Characteristics
#define SERVICE_UUID	   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define REQUEST_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define RESPONSE_CHAR_UUID "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define EVENT_CHAR_UUID	   "d8de624e-140f-4a22-8594-e2216b84a5f2"
#define CONTROL_CHAR_UUID  "f78ebbff-c8b7-4107-93de-889a6a06d408"

// Fragment Flags
#define FLAG_LAST_FRAGMENT 0x01
#define FLAG_ERROR		   0x02

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
inline bool serializeJsonToVector(const JsonDocument& doc, std::vector<uint8_t>& buffer) {
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
    size_t written = serializeJson(doc, (char*)buffer.data(), size);
    
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
inline DeserializationError deserializeVectorToJson(const std::vector<uint8_t>& buffer, JsonDocument& doc) {
    if (buffer.empty()) {
        WARN("Buffer is empty");
        return DeserializationError::EmptyInput;
    }
    
    // Deserialize JSON from buffer
    DeserializationError error = deserializeJson(doc, (const char*)buffer.data(), buffer.size());
    
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
template<typename Func>
inline bool createJsonResponse(std::vector<uint8_t>& buffer, Func fn) {
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
template<typename Func>
inline bool parseJsonFromVector(const std::vector<uint8_t>& buffer, Func fn) {
    JsonDocument doc;
    DeserializationError error = deserializeVectorToJson(buffer, doc);
    
    if (error) {
        return false;
    }
    
    fn(doc);
    return true;
}

BTRemote::BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuragtion)
	: deviceConnected(false)
	, mtu(23)
	, nextMessageId(0) {
}

void BTRemote::begin(const char *deviceName) {
	BLEDevice::init(deviceName);
	BLEDevice::setMTU(517); // Maximale MTU anfragen

	// Server erstellen
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(this);

	// Service erstellen
	pService = pServer->createService(SERVICE_UUID);

	// Request Characteristic (Write)
	pRequestChar = pService->createCharacteristic(
		REQUEST_CHAR_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	pRequestChar->setCallbacks(this);

	// Response Characteristic (Notify)
	pResponseChar = pService->createCharacteristic(
		RESPONSE_CHAR_UUID,
		BLECharacteristic::PROPERTY_NOTIFY);
	pResponseChar->addDescriptor(new BLE2902());

	// Event Characteristic (Notify)
	pEventChar = pService->createCharacteristic(
		EVENT_CHAR_UUID,
		BLECharacteristic::PROPERTY_NOTIFY);
	pEventChar->addDescriptor(new BLE2902());

	// Control Characteristic (Write/Notify)
	pControlChar = pService->createCharacteristic(
		CONTROL_CHAR_UUID,
		BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
	pControlChar->setCallbacks(this);
	pControlChar->addDescriptor(new BLE2902());

	// Service starten
	pService->start();

	// Advertising starten
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();

    onRequest([this](uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t>& payload) {
		INFO("Processing BTMessage with id %d", messageId);

		parseJsonFromVector(payload, [this, messageId, payload](const JsonDocument& doc) {
			String name = doc["name"] | "default";
	
			std::vector<uint8_t> response;
			createJsonResponse(response, [this, name, messageId, payload](JsonDocument& doc) {
    			doc["messageId"] = messageId;
    			doc["payloadSize"] = payload.size();

				doc["name"] = name;
			});

			sendResponse(messageId, response);
		});
    });

	INFO("BLE Server started as ID %s, waiting for Connections...", SERVICE_UUID);
}

// Server Callbacks
void BTRemote::onConnect(BLEServer *pServer) {
	deviceConnected = true;
	mtu = pServer->getPeerMTU(pServer->getConnId()) - 3; // ATT Header abziehen
	INFO("Client connected, MTU: %d", mtu);

	// send MTU to client
	vTaskDelay(pdMS_TO_TICKS(200));
	sendMTUNotification();
}

void BTRemote::onDisconnect(BLEServer *pServer) {
	deviceConnected = false;
	fragmentBuffers.clear();
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

	if (pCharacteristic == pRequestChar) {
		handleFragment(data, length);
	} else if (pCharacteristic == pControlChar) {
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
	auto &buffer = fragmentBuffers[messageId];

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
		fragmentBuffers.erase(messageId);
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
		fragmentBuffers.erase(messageId);
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
		fragmentBuffers.erase(messageId);

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

		if (onRequestCallback) {
			onRequestCallback(appRequestType, messageId, payload);
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
			fragmentBuffers.clear();
			break;
		default:
			break;
	}
}

// Fragmentierte Nachricht senden
bool BTRemote::sendFragmented(BLECharacteristic *characteristic, ProtocolMessageType protocolType,
	uint8_t messageId, const std::vector<uint8_t> &data) {
	if (!deviceConnected) {
		WARN("Kein Client verbunden");
		return false;
	}

	size_t payloadSize = mtu - FRAGMENT_HEADER_SIZE;
	size_t totalFragments = (data.size() + payloadSize - 1) / payloadSize;

	if (totalFragments == 0) {
		totalFragments = 1;
	}

	DEBUG("Sendiong fragmented: ProtocolType=%d, ID=%d, Size=%d, Fragments=%d",
		(uint8_t) protocolType, messageId, data.size(), totalFragments);

	for (size_t i = 0; i < totalFragments; i++) {
		std::vector<uint8_t> fragment;
		fragment.reserve(mtu);

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
	return sendFragmented(pResponseChar, ProtocolMessageType::RESPONSE, messageId, data);
}

// Event senden
bool BTRemote::sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data) {
	// Event-Daten: [Application Event Type][Payload...]
	std::vector<uint8_t> eventData;
	eventData.push_back(appEventType);
	eventData.insert(eventData.end(), data.begin(), data.end());

	return sendFragmented(pEventChar, ProtocolMessageType::EVENT, nextMessageId++, eventData);
}

// Control Message senden
void BTRemote::sendControlMessage(ControlMessageType type, uint8_t messageId) {
	if (!deviceConnected) {
		return;
	}

	uint8_t data[2] = {(uint8_t) type, messageId};
	pControlChar->setValue(data, 2);
	pControlChar->notify();
}

// MTU-Information an Client senden
void BTRemote::sendMTUNotification() {
	if (!deviceConnected) {
		return;
	}

	uint8_t data[4];
	data[0] = (uint8_t) ControlMessageType::MTU_INFO;
	data[1] = 0; // Reserved
	data[2] = (mtu >> 8) & 0xFF;
	data[3] = mtu & 0xFF;

	pControlChar->setValue(data, 4);
	pControlChar->notify();

	INFO("MTU-Info sent: %d bytes", mtu);
}

// Request Callback registrieren
// Callback-Signatur: void callback(uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t>& payload)
void BTRemote::onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback) {
	onRequestCallback = callback;
}

// Timeout-Handler (sollte regelmäßig aufgerufen werden)
void BTRemote::loop() {
	uint32_t now = millis();

	for (auto it = fragmentBuffers.begin(); it != fragmentBuffers.end();) {
		if (now - it->second.lastFragmentTime > FRAGMENT_TIMEOUT_MS) {
			WARN("Timeout: Message ID %d entfernt", it->first);
			sendControlMessage(ControlMessageType::NACK, it->first);
			it = fragmentBuffers.erase(it);
		} else {
			++it;
		}
	}
}

bool BTRemote::isConnected() const {
	return deviceConnected;
}

size_t BTRemote::getMTU() const {
	return mtu;
}
