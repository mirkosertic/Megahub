#ifndef BTREMOTE_H
#define BTREMOTE_H

#include "configuration.h"
#include "megahub.h"

#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <map>
#include <vector>

// Protocol Message Types (INTERN - für Fragmentierungs-Protokoll)
enum class ProtocolMessageType : uint8_t {
	REQUEST = 0x01,
	RESPONSE = 0x02,
	EVENT = 0x03,
	CONTROL = 0x04
};

// Control Message Types (INTERN - für Control-Channel)
enum class ControlMessageType : uint8_t {
	ACK = 0x01,
	NACK = 0x02,
	RETRY = 0x03,
	BUFFER_FULL = 0x04,
	RESET = 0x05,
	MTU_INFO = 0x06
};

// Konfiguration
const size_t FRAGMENT_HEADER_SIZE = 5;
const size_t MAX_BUFFER_SIZE = 65536; // 64KB max pro Message
const uint32_t FRAGMENT_TIMEOUT_MS = 50000;
const size_t MAX_CONCURRENT_MESSAGES = 3; // Memory Safety: Limit gleichzeitiger Nachrichten

// Fragment Buffer Struktur
struct FragmentBuffer {
	std::vector<uint8_t> data;
	uint32_t lastFragmentTime;
	uint16_t expectedFragments;
	uint16_t receivedFragments;
	ProtocolMessageType protocolType;
};

// BLE Server Klasse
class BTRemote : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
private:
	NimBLEServer *pServer_;
	NimBLEService *pService_;
	NimBLECharacteristic *pRequestChar_;
	NimBLECharacteristic *pResponseChar_;
	NimBLECharacteristic *pEventChar_;
	NimBLECharacteristic *pControlChar_;

	SerialLoggingOutput *loggingOutput_;
	Configuration *configuration_;
	Megahub *hub_;

	TaskHandle_t logforwarderTaskHandle_;

	bool deviceConnected_;
	bool readyForEvents_;
	size_t mtu_; // Default MTU

	std::map<uint8_t, FragmentBuffer> fragmentBuffers_;
	uint8_t nextMessageId_;

	QueueHandle_t responseQueue_;
	TaskHandle_t responseSenderTaskHandle_;

	// Callback-Funktionen
	// Callback: (appRequestType, messageId, payload)
	std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> onRequestCallback_;

	// Fragment verarbeiten
	void handleFragment(const uint8_t *data, size_t length);

	// Vollständige Message verarbeiten
	void processCompleteMessage(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data);

	// Control Message verarbeiten
	void handleControlMessage(const uint8_t *data, size_t length);

	// Fragmentierte Nachricht senden
	bool sendFragmented(NimBLECharacteristic *characteristic, ProtocolMessageType protocolType,
		uint8_t messageId, const std::vector<uint8_t> &data);

	// Response senden
	bool sendResponse(uint8_t messageId, const std::vector<uint8_t> &data);

	bool sendLargeResponse(uint8_t messageId, size_t totalSize, std::function<int(const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer)> chunkProvider);

	// MTU-Information an Client senden
	void sendMTUNotification();

	bool reqStopProgram(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqGetProjectFile(uint8_t messageId, const JsonDocument &requestDoc);
	bool reqPutProjectFile(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqDeleteProject(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqSyntaxCheck(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqRun(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqGetProjects(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqGetAutostart(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqPutAutostart(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqReadyForEvents(const JsonDocument &requestDoc, JsonDocument &responseDoc);

	// Control Message senden
	void sendControlMessage(ControlMessageType type, uint8_t messageId);

	// Request Callback registrieren
	// Callback-Signatur: void callback(uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t>& payload)
	void onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback);

	// Event senden
	bool sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data);

public:
	BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuragtion);

	void begin(const char *deviceName);

	// Server Callbacks
	void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;

	void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override;

	void onMTUChange(uint16_t mtu, NimBLEConnInfo &connInfo);

	// Characteristic Callbacks
	void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;

	// Events
	void publishLogMessages();
	void publishCommands();
	void publishPortstatus();

	// Process queued responses (called from background task)
	void processResponseQueue();

	// Timeout-Handler (sollte regelmäßig aufgerufen werden)
	void loop();

	bool isConnected() const;
	size_t getMTU() const;
};

#endif // BTREMOTE_H
