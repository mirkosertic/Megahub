#ifndef BTREMOTE_H
#define BTREMOTE_H

#include "configuration.h"
#include "megahub.h"

#include <ArduinoJson.h>
#include <map>
#include <vector>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"

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

class BTRemote {
private:
	BLEHandles handles_;
	ConnectionState connState_;
	uint16_t gatts_if_;
	uint16_t app_id_;

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

public:
	BTRemote(FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuragtion);

	void begin(const char *deviceName);

	static void gattsEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
		esp_ble_gatts_cb_param_t *param);
	static void gapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

	void publishLogMessages();
	void publishCommands();
	void publishPortstatus();
	void processMessageQueue();
	void loop();

	bool isConnected() const;
	size_t getMTU() const;
};

#endif // BTREMOTE_H
