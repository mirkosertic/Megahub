#ifndef BTREMOTE_H
#define BTREMOTE_H

#include "configuration.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_hidh_api.h"
#include "inputdevices.h"
#include "megahub.h"

#include <ArduinoJson.h>
#include <map>
#include <vector>

enum class ProtocolMessageType : uint8_t {
	REQUEST = 0x01,
	RESPONSE = 0x02,
	EVENT = 0x03,
	CONTROL = 0x04
};

enum class ControlMessageType : uint8_t {
	ACK = 0x01,
	NACK = 0x02,
	RETRY = 0x03,
	BUFFER_FULL = 0x04,
	RESET = 0x05,
	MTU_INFO = 0x06
};

const size_t FRAGMENT_HEADER_SIZE = 5;
const size_t MAX_BUFFER_SIZE = 65536;
const uint32_t FRAGMENT_TIMEOUT_MS = 50000;
const size_t MAX_CONCURRENT_MESSAGES = 3;

struct FragmentBuffer {
	std::vector<uint8_t> data;
	uint32_t lastFragmentTime;
	uint16_t expectedFragments;
	uint16_t receivedFragments;
	ProtocolMessageType protocolType;
};

struct BLEHandles {
	uint16_t service_handle;
	uint16_t request_char_handle;
	uint16_t request_char_value_handle;
	uint16_t response_char_handle;
	uint16_t response_char_value_handle;
	uint16_t response_cccd_handle;
	uint16_t event_char_handle;
	uint16_t event_char_value_handle;
	uint16_t event_cccd_handle;
	uint16_t control_char_handle;
	uint16_t control_char_value_handle;
	uint16_t control_cccd_handle;
};

struct ConnectionState {
	uint16_t conn_id;
	uint16_t gatt_if;
	esp_bd_addr_t remote_addr;
	uint16_t mtu;
	bool connected;
};

// Bluetooth Classic Device Types
enum class BTClassicDeviceType : uint8_t {
	UNKNOWN = 0x00,
	COMPUTER = 0x01,
	PHONE = 0x02,
	AUDIO = 0x03,
	PERIPHERAL = 0x04,
	IMAGING = 0x05,
	WEARABLE = 0x06,
	TOY = 0x07,
	HEALTH = 0x08,
	GAMEPAD = 0x09,
	KEYBOARD = 0x0A,
	MOUSE = 0x0B,
	JOYSTICK = 0x0C
};

// Bluetooth Classic Device Information
struct BTClassicDevice {
	esp_bd_addr_t address;
	char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
	BTClassicDeviceType deviceType;
	bool paired;
	int32_t rssi;
	uint32_t cod; // Class of Device
};

// HID Host Connection State
enum class HIDConnectionState : uint8_t {
	DISCONNECTED = 0x00,
	CONNECTING = 0x01,
	CONNECTED = 0x02,
	DISCONNECTING = 0x03
};

// HID Device Information
struct HIDDevice {
	esp_bd_addr_t address;
	HIDConnectionState state;
	char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
	uint32_t handle;
};

// HID Event Queue Item for async processing
struct HIDEventItem {
	esp_hidh_cb_event_t event;
	esp_hidh_cb_param_t param;
};

class BTRemote {
private:
	BLEHandles handles_;
	ConnectionState connState_;
	uint16_t gatts_if_;
	uint16_t app_id_;

	InputDevices *inputDevices_;
	SerialLoggingOutput *loggingOutput_;
	Configuration *configuration_;
	Megahub *hub_;

	TaskHandle_t logforwarderTaskHandle_;

	bool deviceConnected_;
	bool readyForEvents_;
	size_t mtu_;

	std::map<uint8_t, FragmentBuffer> fragmentBuffers_;
	uint8_t nextMessageId_;

	QueueHandle_t responseQueue_;
	TaskHandle_t responseSenderTaskHandle_;

	// CRITICAL: Semaphore for indication flow control (ESP_GATTS_CONF_EVT sync)
	SemaphoreHandle_t indicationConfirmSemaphore_;

	std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> onRequestCallback_;

	// Bluetooth Classic Discovery and Pairing
	std::map<std::string, BTClassicDevice> discoveredDevices_;
	SemaphoreHandle_t discoveredDevicesMutex_;
	uint32_t lastDeviceListPublishTime_;
	bool discoveryInProgress_;
	bool pairingInProgress_;
	esp_bd_addr_t pairingDeviceAddress_;
	bool suppressBLERestart_; // Prevents auto-restart of BLE advertising during HID pairing

	// HID Host
	std::map<std::string, HIDDevice> hidDevices_;
	SemaphoreHandle_t hidDevicesMutex_;

	// HID event processing queue and task
	QueueHandle_t hidEventQueue_;
	TaskHandle_t hidEventTaskHandle_;

	void handleFragment(const uint8_t *data, size_t length);
	void processCompleteMessage(ProtocolMessageType protocolType, uint8_t messageId, const std::vector<uint8_t> &data);
	void handleControlMessage(const uint8_t *data, size_t length);
	bool sendFragmented(uint16_t char_handle, ProtocolMessageType protocolType,
		uint8_t messageId, const std::vector<uint8_t> &data);
	bool sendResponse(uint8_t messageId, const std::vector<uint8_t> &data);

	bool sendLargeResponse(uint8_t messageId, size_t totalSize, std::function<int(const int index, const size_t maxChunkSize, std::vector<uint8_t> &dataContainer)> chunkProvider);
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
	bool reqRequestPairing(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqRemovePairing(const JsonDocument &requestDoc, JsonDocument &responseDoc);
	bool reqStartDiscovery(const JsonDocument &requestDoc, JsonDocument &responseDoc);

	void sendControlMessage(ControlMessageType type, uint8_t messageId);
	void onRequest(std::function<void(uint8_t, uint8_t, const std::vector<uint8_t> &)> callback);
	bool sendEvent(uint8_t appEventType, const std::vector<uint8_t> &data);

	void handleGattsConnect(esp_ble_gatts_cb_param_t *param);
	void handleGattsDisconnect(esp_ble_gatts_cb_param_t *param);
	void handleGattsMTU(esp_ble_gatts_cb_param_t *param);
	void handleGattsWrite(esp_ble_gatts_cb_param_t *param);
	void handleGattsRegister(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
	void handleGattsCreate(esp_ble_gatts_cb_param_t *param);
	void handleGattsAddChar(esp_ble_gatts_cb_param_t *param);
	void handleGattsAddCharDescr(esp_ble_gatts_cb_param_t *param);
	void handleGattsConfirm(esp_ble_gatts_cb_param_t *param);

	// Bluetooth Classic GAP Event Handlers
	void handleGapBTAuthComplete(esp_bt_gap_cb_param_t *param);
	void handleGapBTDiscoveryResult(esp_bt_gap_cb_param_t *param);
	void handleGapBTDiscoveryStateChanged(esp_bt_gap_cb_param_t *param);
	void handleGapBTPinRequest(esp_bt_gap_cb_param_t *param);
	void handleGapBTCfmRequest(esp_bt_gap_cb_param_t *param);
	void handleGapBTKeyNotify(esp_bt_gap_cb_param_t *param);
	void handleGapBTKeyRequest(esp_bt_gap_cb_param_t *param);
	void handleGapBTReadRemoteName(esp_bt_gap_cb_param_t *param);

	// HID Host Event Handlers (called from HID event task, not callback)
	void handleHIDHostInit(esp_hidh_cb_param_t *param);
	void handleHIDHostOpen(esp_hidh_cb_param_t *param);
	void handleHIDHostClose(esp_hidh_cb_param_t *param);
	void handleHIDHostData(esp_hidh_cb_param_t *param);
	void processGamepadReport(const esp_bd_addr_t address, const uint8_t *data, uint16_t length);

	// HID event processing task
	static void hidEventTask(void *pvParameters);
	void processHIDEvent(const HIDEventItem &item);

	// Helper methods for Bluetooth Classic
	void stopDiscoveryInternal();
	BTClassicDeviceType classifyDevice(uint32_t cod);
	std::string bdAddrToString(const esp_bd_addr_t address);
	void stringToBdAddr(const std::string &str, esp_bd_addr_t address);

public:
	BTRemote(FS *fs, InputDevices *inputDevices, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuragtion);

	void begin(const char *deviceName);

	static void gattsEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
		esp_ble_gatts_cb_param_t *param);
	static void gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
	static void gapBTEventHandler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
	static void hidHostEventHandler(esp_hidh_cb_event_t event, esp_hidh_cb_param_t *param);

	void publishLogMessages();
	void publishCommands();
	void publishPortstatus();
	void publishBTClassicDevices();
	void processMessageQueue();
	void loop();

	bool isConnected() const;
	size_t getMTU() const;

	// Bluetooth Classic Discovery API (thread-safe)
	std::vector<BTClassicDevice> getDiscoveredDevices();
	bool isDiscoveryInProgress() const;
	void clearDiscoveredDevices();

	// Bluetooth Classic Pairing API
	bool removePairing(const char *macAddress);
	bool startPairing(const char *macAddress);
	bool isPairingInProgress() const;

	// HID Host API
	bool hidConnect(const char *macAddress);
	bool hidDisconnect(const char *macAddress);
	std::vector<HIDDevice> getConnectedHIDDevices();
};

#endif // BTREMOTE_H
