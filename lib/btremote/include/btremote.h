#ifndef BTREMOTE_H
#define BTREMOTE_H

#include "configuration.h"
#include "megahub.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
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
const uint32_t FRAGMENT_TIMEOUT_MS = 5000;

// Fragment Buffer Struktur
struct FragmentBuffer {
	std::vector<uint8_t> data;
	uint32_t lastFragmentTime;
	uint16_t expectedFragments;
	uint16_t receivedFragments;
	ProtocolMessageType protocolType;
};

// BLE Server Klasse
class BTRemote : public BLEServerCallbacks, public BLECharacteristicCallbacks {
private:
	BLEServer *pServer;
	BLEService *pService;
	BLECharacteristic *pRequestChar;
	BLECharacteristic *pResponseChar;
	BLECharacteristic *pEventChar;
	BLECharacteristic *pControlChar;

	bool deviceConnected;
	size_t mtu; // Default MTU

	std::map<uint8_t, FragmentBuffer> fragmentBuffers;
	uint8_t nextMessageId;

	// Callback-Funktionen
	// Callback: (appRequestType, messageId, payload)
	std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> onRequestCallback;

	// Fragment verarbeiten
	void handleFragment(const uint8_t *data, size_t length);

	// Vollständige Message verarbeiten
	void processCompleteMessage(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data);

	// Control Message verarbeiten
	void handleControlMessage(const uint8_t *data, size_t length);

	// Fragmentierte Nachricht senden
	bool sendFragmented(BLECharacteristic *characteristic, ProtocolMessageType protocolType,
		uint8_t messageId, const std::vector<uint8_t> &data);

	// Response senden
	bool sendResponse(uint8_t messageId, const std::vector<uint8_t> &data);

	// MTU-Information an Client senden
	void sendMTUNotification();

public:
	BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuragtion);

	void begin(const char *deviceName);

	// Server Callbacks
	void onConnect(BLEServer *pServer) override;

	void onDisconnect(BLEServer *pServer) override;

	// Characteristic Callbacks
	void onWrite(BLECharacteristic *pCharacteristic) override;

	// Event senden
	bool sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data);

	// Control Message senden
	void sendControlMessage(ControlMessageType type, uint8_t messageId);

	// Request Callback registrieren
	// Callback-Signatur: void callback(uint8_t appRequestType, uint8_t messageId, const std::vector<uint8_t>& payload)
	void onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback);

	// Timeout-Handler (sollte regelmäßig aufgerufen werden)
	void loop();

	bool isConnected() const;
	size_t getMTU() const;
};

#endif // BTREMOTE_H