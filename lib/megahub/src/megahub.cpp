#include <Arduino.h>

#include "megahub.h"

#include "YDLidar.h"
#include "commands.h"
#include "gitrevision.h"
#include "i2csync.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <esp_mac.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define MEGAHUBREF_NAME		 "MEGAHUBTHISREF"
#define INPUTDEVICESREF_NAME "MEGAHUBINPUTDEVICESREF"

SemaphoreHandle_t lua_global_mutex = xSemaphoreCreateMutex();

TaskHandle_t statusReporterTaskHandle = NULL;

// Snapshot structures for status reporting - uses fixed-size arrays to avoid heap allocations
// during lock-held section for minimal i2c lock time
#define SNAPSHOT_MAX_MODES		 16
#define SNAPSHOT_NAME_MAX_LEN	 32
#define SNAPSHOT_UNITS_MAX_LEN	 16

struct ModeSnapshot {
	int id;
	char name[SNAPSHOT_NAME_MAX_LEN];
	char units[SNAPSHOT_UNITS_MAX_LEN];
	bool hasFormat;
	int datasets;
	int figures;
	int decimals;
	Format::FormatType formatType;
};

struct DeviceSnapshot {
	bool connected;
	char name[SNAPSHOT_NAME_MAX_LEN];
	int deviceId;
	int numModes;
	ModeSnapshot modes[SNAPSHOT_MAX_MODES];
};

struct HubSnapshot {
	DeviceSnapshot ports[4];
};

// Capture device state into snapshot (call with i2c lock held)
// Uses only stack operations - no heap allocations for minimal lock time
static void captureDeviceSnapshot(LegoDevice *device, DeviceSnapshot &snap) {
	snap.connected = device->fullyInitialized();
	if (!snap.connected) {
		snap.numModes = 0;
		return;
	}

	strncpy(snap.name, device->name().c_str(), SNAPSHOT_NAME_MAX_LEN - 1);
	snap.name[SNAPSHOT_NAME_MAX_LEN - 1] = '\0';
	snap.deviceId = device->getDeviceId();

	int numModes = device->numModes();
	snap.numModes = (numModes > SNAPSHOT_MAX_MODES) ? SNAPSHOT_MAX_MODES : numModes;

	for (int i = 0; i < snap.numModes; i++) {
		Mode *mode = device->getMode(i);
		ModeSnapshot &m = snap.modes[i];
		m.id = i;
		strncpy(m.name, mode->getName().c_str(), SNAPSHOT_NAME_MAX_LEN - 1);
		m.name[SNAPSHOT_NAME_MAX_LEN - 1] = '\0';
		strncpy(m.units, mode->getUnits().c_str(), SNAPSHOT_UNITS_MAX_LEN - 1);
		m.units[SNAPSHOT_UNITS_MAX_LEN - 1] = '\0';

		Format *format = mode->getFormat();
		m.hasFormat = (format != nullptr);
		if (m.hasFormat) {
			m.datasets = format->getDatasets();
			m.figures = format->getFigures();
			m.decimals = format->getDecimals();
			m.formatType = format->getFormatType();
		}
	}
}

// Convert format type enum to string
static const char *formatTypeToString(Format::FormatType type) {
	switch (type) {
		case Format::FormatType::DATA8:
			return "DATA8";
		case Format::FormatType::DATA16:
			return "DATA16";
		case Format::FormatType::DATA32:
			return "DATA32";
		case Format::FormatType::DATAFLOAT:
			return "DATAFLOAT";
		default:
			return nullptr;
	}
}

// Convert device snapshot to JSON (call without i2c lock)
static void deviceSnapshotToJson(const DeviceSnapshot &snap, int portId, JsonArray &ports) {
	JsonObject port = ports.add<JsonObject>();
	port["id"] = portId;

	if (!snap.connected) {
		port["connected"] = false;
		return;
	}

	port["connected"] = true;
	JsonObject device = port["device"].to<JsonObject>();
	device["type"] = snap.name;
	device["deviceId"] = snap.deviceId;
	device["icon"] = "⚙️";

	JsonArray modes = device["modes"].to<JsonArray>();
	for (int i = 0; i < snap.numModes; i++) {
		const ModeSnapshot &m = snap.modes[i];
		JsonObject singleMode = modes.add<JsonObject>();
		singleMode["id"] = m.id;
		singleMode["name"] = m.name;
		singleMode["units"] = m.units;

		if (m.hasFormat) {
			singleMode["datasets"] = m.datasets;
			singleMode["figures"] = m.figures;
			singleMode["decimals"] = m.decimals;
			const char *typeStr = formatTypeToString(m.formatType);
			if (typeStr) {
				singleMode["type"] = typeStr;
			}
		}
	}
}

Megahub *getMegaHubRef(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, MEGAHUBREF_NAME);
	void **userdata = (void **) lua_touserdata(L, -1);
	lua_pop(L, 1); // Clean up stack
	return (Megahub *) (userdata ? *userdata : NULL);
}

InputDevices *getInputDevicesRef(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, INPUTDEVICESREF_NAME);
	void **userdata = (void **) lua_touserdata(L, -1);
	lua_pop(L, 1); // Clean up stack
	return (InputDevices *) (userdata ? *userdata : NULL);
}

void status_reporter_task(void *parameters) {
	Megahub *hub = (Megahub *) parameters;
	INFO("Starting task reporter task");

	// Static to avoid stack overflow - HubSnapshot is ~4.5KB with fixed arrays
	// Safe because there's only one status_reporter_task instance
	static HubSnapshot snapshot;

	while (true) {
		DEBUG("Sending Portstatus");

		// Phase 1: Fast snapshot capture under i2c lock
		i2c_lock();
		captureDeviceSnapshot(hub->port(PORT1), snapshot.ports[0]);
		captureDeviceSnapshot(hub->port(PORT2), snapshot.ports[1]);
		captureDeviceSnapshot(hub->port(PORT3), snapshot.ports[2]);
		captureDeviceSnapshot(hub->port(PORT4), snapshot.ports[3]);
		i2c_unlock();

		// Phase 2: JSON construction without lock
		JsonDocument status;
		JsonArray ports = status["ports"].to<JsonArray>();

		for (int i = 0; i < 4; i++) {
			deviceSnapshotToJson(snapshot.ports[i], i + 1, ports);
		}

		String strContent;
		serializeJson(status, strContent);

		DEBUG("Port state JSON has length %d and is %s", strContent.length(), strContent.c_str());

		Portstatus::instance()->queue(strContent);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	INFO("Done with task reporter task");
	vTaskDelete(NULL);
}

extern int hub_library(lua_State *luaState);

extern int fastled_library(lua_State *luaState);

extern int imu_library(lua_State *luaState);

extern int debug_library(lua_State *luaState);

extern int ui_library(lua_State *luaState);

extern int lego_library(lua_State *luaState);

extern int gamepad_library(lua_State *luaState);

int global_wait(lua_State *luaState) {
	int delay = lua_tointeger(luaState, 1);

	DEBUG("Waiting for %d milliseconds", delay);

	vTaskDelay(pdMS_TO_TICKS(delay));

	return 0;
}

int global_print(lua_State *luaState) {
	const char *str = lua_tostring(luaState, 1);

	DEBUG("Printing data %s", str);

	INFO("%s", str);

	return 0;
}

int global_millis(lua_State *luaState) {
	long ms = millis();

	DEBUG("Current time is %ld milliseconds", ms);

	lua_pushinteger(luaState, ms);

	return 1;
}

lua_State *Megahub::newLuaState() {
	INFO("Creating new Lua state");
	lua_State *ls = luaL_newstate();

	INFO("Opening standard Lua libraries");
	luaL_openlibs(ls);

	INFO("Registering custom libraries and functions");
	// Register custom librariey
	luaL_requiref(ls, "hub", hub_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "fastled", fastled_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "imu", imu_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "ui", ui_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "deb", debug_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "lego", lego_library, 1);
	lua_pop(ls, 1); // remove lib from stack
	luaL_requiref(ls, "gamepad", gamepad_library, 1);
	lua_pop(ls, 1); // remove lib from stack

	// And also global functions
	lua_register(ls, "wait", global_wait);
	lua_register(ls, "print", global_print);
	lua_register(ls, "millis", global_millis);

	// The self reference
	void **userdata = (void **) lua_newuserdata(ls, sizeof(void *)); 
	*userdata = this;
	lua_setfield(ls, LUA_REGISTRYINDEX, MEGAHUBREF_NAME);

	// Input devices reference
	void **inputdevicesuserdata = (void **) lua_newuserdata(ls, sizeof(void *));
	*inputdevicesuserdata = inputdevices_.get();
	lua_setfield(ls, LUA_REGISTRYINDEX, INPUTDEVICESREF_NAME);

	// Globals
	lua_pushinteger(ls, PORT1);
	lua_setglobal(ls, "PORT1");
	lua_pushinteger(ls, PORT2);
	lua_setglobal(ls, "PORT2");
	lua_pushinteger(ls, PORT3);
	lua_setglobal(ls, "PORT3");
	lua_pushinteger(ls, PORT4);
	lua_setglobal(ls, "PORT4");

	lua_pushinteger(ls, GPIO_NUM_13);
	lua_setglobal(ls, "GPIO13");
	lua_pushinteger(ls, GPIO_NUM_16);
	lua_setglobal(ls, "GPIO16");
	lua_pushinteger(ls, GPIO_NUM_17);
	lua_setglobal(ls, "GPIO17");
	lua_pushinteger(ls, GPIO_NUM_25);
	lua_setglobal(ls, "GPIO25");
	lua_pushinteger(ls, GPIO_NUM_26);
	lua_setglobal(ls, "GPIO26");
	lua_pushinteger(ls, GPIO_NUM_27);
	lua_setglobal(ls, "GPIO27");
	lua_pushinteger(ls, GPIO_NUM_32);
	lua_setglobal(ls, "GPIO32");
	lua_pushinteger(ls, GPIO_NUM_33);
	lua_setglobal(ls, "GPIO33");
	lua_pushinteger(ls, GPIO_NUM_34);
	lua_setglobal(ls, "GPIO34");
	lua_pushinteger(ls, GPIO_NUM_35);
	lua_setglobal(ls, "GPIO35");
	lua_pushinteger(ls, GPIO_NUM_36);
	lua_setglobal(ls, "GPIO36");
	lua_pushinteger(ls, GPIO_NUM_39);
	lua_setglobal(ls, "GPIO39");

	lua_pushinteger(ls, UART1_GP4);
	lua_setglobal(ls, "UART1_GP4");
	lua_pushinteger(ls, UART1_GP5);
	lua_setglobal(ls, "UART1_GP5");
	lua_pushinteger(ls, UART1_GP6);
	lua_setglobal(ls, "UART1_GP6");
	lua_pushinteger(ls, UART1_GP7);
	lua_setglobal(ls, "UART1_GP7");

	lua_pushinteger(ls, UART2_GP4);
	lua_setglobal(ls, "UART2_GP4");
	lua_pushinteger(ls, UART2_GP5);
	lua_setglobal(ls, "UART2_GP5");
	lua_pushinteger(ls, UART2_GP6);
	lua_setglobal(ls, "UART2_GP6");
	lua_pushinteger(ls, UART2_GP7);
	lua_setglobal(ls, "UART2_GP7");

	lua_pushinteger(ls, PINMODE_INPUT);
	lua_setglobal(ls, "PINMODE_INPUT");
	lua_pushinteger(ls, PINMODE_INPUT_PULLUP);
	lua_setglobal(ls, "PINMODE_INPUT_PULLUP");
	lua_pushinteger(ls, PINMODE_INPUT_PULLDOWN);
	lua_setglobal(ls, "PINMODE_INPUT_PULLDOWN");
	lua_pushinteger(ls, PINMODE_OUTPUT);
	lua_setglobal(ls, "PINMODE_OUTPUT");

	// IMU
	lua_pushinteger(ls, YAW);
	lua_setglobal(ls, "YAW");
	lua_pushinteger(ls, PITCH);
	lua_setglobal(ls, "PITCH");
	lua_pushinteger(ls, ROLL);
	lua_setglobal(ls, "ROLL");
	lua_pushinteger(ls, ACCELERATION_X);
	lua_setglobal(ls, "ACCELERATION_X");
	lua_pushinteger(ls, ACCELERATION_Y);
	lua_setglobal(ls, "ACCELERATION_Y");
	lua_pushinteger(ls, ACCELERATION_Z);
	lua_setglobal(ls, "ACCELERATION_Z");

	// FastLED constants
	lua_pushinteger(ls, NEOPIXEL_TYPE);
	lua_setglobal(ls, "NEOPIXEL");

	// UI constants
	lua_pushinteger(ls, FORMAT_SIMPLE);
	lua_setglobal(ls, "FORMAT_SIMPLE");

	// Gamepad
	lua_pushinteger(ls, GAMEPAD1);
	lua_setglobal(ls, "GAMEPAD1");
	lua_pushinteger(ls, GAMEPAD_BUTTON_1);
	lua_setglobal(ls, "GAMEPAD_BUTTON_1");
	lua_pushinteger(ls, GAMEPAD_BUTTON_2);
	lua_setglobal(ls, "GAMEPAD_BUTTON_2");
	lua_pushinteger(ls, GAMEPAD_BUTTON_3);
	lua_setglobal(ls, "GAMEPAD_BUTTON_3");
	lua_pushinteger(ls, GAMEPAD_BUTTON_4);
	lua_setglobal(ls, "GAMEPAD_BUTTON_4");
	lua_pushinteger(ls, GAMEPAD_BUTTON_5);
	lua_setglobal(ls, "GAMEPAD_BUTTON_5");
	lua_pushinteger(ls, GAMEPAD_BUTTON_6);
	lua_setglobal(ls, "GAMEPAD_BUTTON_6");
	lua_pushinteger(ls, GAMEPAD_BUTTON_7);
	lua_setglobal(ls, "GAMEPAD_BUTTON_7");
	lua_pushinteger(ls, GAMEPAD_BUTTON_8);
	lua_setglobal(ls, "GAMEPAD_BUTTON_8");
	lua_pushinteger(ls, GAMEPAD_BUTTON_9);
	lua_setglobal(ls, "GAMEPAD_BUTTON_9");
	lua_pushinteger(ls, GAMEPAD_BUTTON_10);
	lua_setglobal(ls, "GAMEPAD_BUTTON_10");
	lua_pushinteger(ls, GAMEPAD_BUTTON_11);
	lua_setglobal(ls, "GAMEPAD_BUTTON_11");
	lua_pushinteger(ls, GAMEPAD_BUTTON_12);
	lua_setglobal(ls, "GAMEPAD_BUTTON_12");
	lua_pushinteger(ls, GAMEPAD_BUTTON_13);
	lua_setglobal(ls, "GAMEPAD_BUTTON_13");
	lua_pushinteger(ls, GAMEPAD_BUTTON_14);
	lua_setglobal(ls, "GAMEPAD_BUTTON_14");
	lua_pushinteger(ls, GAMEPAD_BUTTON_15);
	lua_setglobal(ls, "GAMEPAD_BUTTON_15");
	lua_pushinteger(ls, GAMEPAD_BUTTON_16);
	lua_setglobal(ls, "GAMEPAD_BUTTON_16");

	lua_pushinteger(ls, GAMEPAD_LEFT_X);
	lua_setglobal(ls, "GAMEPAD_LEFT_X");
	lua_pushinteger(ls, GAMEPAD_LEFT_Y);
	lua_setglobal(ls, "GAMEPAD_LEFT_Y");
	lua_pushinteger(ls, GAMEPAD_RIGHT_X);
	lua_setglobal(ls, "GAMEPAD_RIGHT_X");
	lua_pushinteger(ls, GAMEPAD_RIGHT_Y);
	lua_setglobal(ls, "GAMEPAD_RIGHT_Y");
	lua_pushinteger(ls, GAMEPAD_DPAD);
	lua_setglobal(ls, "GAMEPAD_DPAD");

	INFO("Finished creating new Lua state");

	return ls;
}

Megahub::Megahub(InputDevices *inputDevices, LegoDevice *device1, LegoDevice *device2, LegoDevice *device3, LegoDevice *device4, IMU *imu)
	: inputdevices_(inputDevices)
	, device1_(device1)
	, device2_(device2)
	, device3_(device3)
	, device4_(device4)
	, imu_(imu) {

	globalLuaState_ = newLuaState();

	device1_->initialize();
	device2_->initialize();
	device3_->initialize();
	device4_->initialize();

	currentprogramstate_ = nullptr;

	// Create the task
	xTaskCreate(
		status_reporter_task,
		"PortStatus",
		4096,
		(void *) this,
		1,
		&statusReporterTaskHandle);
}

Megahub::~Megahub() {
	lua_close(globalLuaState_);
}

void Megahub::loop() {
	i2c_lock();
	device1_->loop();
	device2_->loop();
	device3_->loop();
	device4_->loop();
	imu_->loop();
	i2c_unlock();
}

LegoDevice *Megahub::port(int num) {
	if (num == PORT1) {
		return device1_.get();
	}
	if (num == PORT2) {
		return device2_.get();
	}
	if (num == PORT3) {
		return device3_.get();
	}
	if (num == PORT4) {
		return device4_.get();
	}
	WARN("Unknown port number : %d", num);
	return nullptr;
}

IMU *Megahub::imu() {
	return imu_.get();
}

String Megahub::deviceUid() {
	uint8_t chipId[6];
	esp_read_mac(chipId, ESP_MAC_WIFI_STA);

	uint32_t serialNumber = 0;
	for (int i = 0; i < 6; i++) {
		serialNumber += (chipId[i] << (8 * i));
	}

	char serialStr[13];
	snprintf(serialStr, sizeof(serialStr), "%012X", serialNumber);

	return String(serialStr);
}

String Megahub::name() {
	return deviceUid() + " Megahub";
}

String Megahub::manufacturer() {
	return "Mirko Sertic";
}

String Megahub::deviceType() {
	return "Megahub";
}

String Megahub::version() {
	return String(gitRevShort);
}

String Megahub::serialnumber() {
	return deviceUid();
}

LuaCheckResult Megahub::checkLUACode(String luaCode) {
	INFO("Performing syntax check for Lua code of size %d", luaCode.length());
	LuaCheckResult result;

	long startTime = millis();

	INFO("Creating temporary Lua state for syntax check");
	lua_State *tempState = lua_newthread(globalLuaState_);

	INFO("Starting to parse Lua code (just compile, no execution)");
	int luaResult = luaL_loadstring(tempState, luaCode.c_str());

	long endTime = millis();

	result.parseTime = endTime - startTime;

	if (luaResult != LUA_OK) {
		const char *error_msg = lua_tostring(tempState, -1);
		result.success = false;
		result.errorMessage = String(error_msg);
		lua_pop(tempState, 1);
	} else {
		result.success = true;
		result.errorMessage = String("");
	}

	INFO("Closing temporary Lua state for syntax check");
	lua_closethread(tempState, globalLuaState_);

	INFO("Syntax check completed in %ld milliseconds with result %s", result.parseTime, result.success ? "success" : "failure");

	return result;
}

void Megahub::executeLUACode(String luaCode) {
	INFO("Executing Lua code of size %d", luaCode.length());

	stopRunningThreads();

	if (currentprogramstate_ != nullptr) {
		INFO("Closing existing Lua program state");
		lua_closethread(currentprogramstate_, globalLuaState_);
		currentprogramstate_ = nullptr;
	}

	long startTime = millis();

	INFO("Creating Lua state");
	currentprogramstate_ = lua_newthread(globalLuaState_);

	INFO("Executing Lua code");
	int luaResult = luaL_dostring(currentprogramstate_, luaCode.c_str());

	long time = millis() - startTime;

	if (luaResult != LUA_OK) {
		INFO("Lua execution failed with error %s", lua_tostring(currentprogramstate_, -1));
		lua_pop(currentprogramstate_, 1);
	}

	INFO("Execution completed in %ld milliseconds", time);
}

bool Megahub::stopLUACode() {
	INFO("Stopping Lua code execution");

	stopRunningThreads();

	return true;
}

int Megahub::digitalReadFrom(int pin) {
	int value = 0;
	switch (pin) {
		case UART1_GP4:
			value = device1_->digitalRead(4);
			break;
		case UART1_GP5:
			value = device1_->digitalRead(5);
			break;
		case UART1_GP6:
			value = device1_->digitalRead(6);
			break;
		case UART1_GP7:
			value = device1_->digitalRead(7);
			break;
		case UART2_GP4:
			value = device3_->digitalRead(4);
			break;
		case UART2_GP5:
			value = device3_->digitalRead(5);
			break;
		case UART2_GP6:
			value = device3_->digitalRead(6);
			break;
		case UART2_GP7:
			value = device3_->digitalRead(7);
			break;
		default:
			int value = digitalRead((gpio_num_t) pin);
			break;
	}

	DEBUG("Digital read from pin %d returned value %d", pin, value);
	return value;
}

void Megahub::digitalWriteTo(int pin, int value) {
	DEBUG("Digital write to pin %d with value %d", pin, value);
	switch (pin) {
		case UART1_GP4:
			device1_->digitalWrite(4, value);
			break;
		case UART1_GP5:
			device1_->digitalWrite(5, value);
			break;
		case UART1_GP6:
			device1_->digitalWrite(6, value);
			break;
		case UART1_GP7:
			device1_->digitalWrite(7, value);
			break;
		case UART2_GP4:
			device3_->digitalWrite(4, value);
			break;
		case UART2_GP5:
			device3_->digitalWrite(5, value);
			break;
		case UART2_GP6:
			device3_->digitalWrite(6, value);
			break;
		case UART2_GP7:
			device3_->digitalWrite(7, value);
			break;
		default:
			digitalWrite((gpio_num_t) pin, value);
			break;
	}
}

void Megahub::setPinMode(int pin, int mode) {
	DEBUG("Setting mode of pin %d to %d", pin, mode);
	switch (pin) {
		case UART1_GP4:
			device1_->setPinMode(4, mode);
			break;
		case UART1_GP5:
			device1_->setPinMode(5, mode);
			break;
		case UART1_GP6:
			device1_->setPinMode(6, mode);
			break;
		case UART1_GP7:
			device1_->setPinMode(7, mode);
			break;
		case UART2_GP4:
			device3_->setPinMode(4, mode);
			break;
		case UART2_GP5:
			device3_->setPinMode(5, mode);
			break;
		case UART2_GP6:
			device3_->setPinMode(6, mode);
			break;
		case UART2_GP7:
			device3_->setPinMode(7, mode);
			break;
		default:
			switch (mode) {
				case PINMODE_INPUT:
					pinMode(pin, INPUT);
					break;
				case PINMODE_INPUT_PULLUP:
					pinMode(pin, INPUT_PULLUP);
					break;
				case PINMODE_INPUT_PULLDOWN:
					pinMode(pin, INPUT_PULLDOWN);
					break;
				case PINMODE_OUTPUT:
					pinMode(pin, OUTPUT);
					break;
				default:
					WARN("Unsupported pin mode %d for pin %d", mode, pin);
			}
			break;
	}
}

void Megahub::registerThread(TaskHandle_t handle) {
	runningThreads_.push_back(handle);
}

void Megahub::stopRunningThreads() {
	for (int i = 0; i < runningThreads_.size(); i++) {
		TaskHandle_t handle = runningThreads_[i];

		INFO("Stopping thread #%d", i);
		xTaskNotify(handle, 1, eSetValueWithOverwrite);
		vTaskDelay(pdMS_TO_TICKS(100));
	}

	runningThreads_.clear();
}

void Megahub::stopThread(TaskHandle_t handle) {
	xTaskNotify(handle, 1, eSetValueWithOverwrite);
	vTaskDelay(pdMS_TO_TICKS(100));
	runningThreads_.erase(std::remove(runningThreads_.begin(), runningThreads_.end(), handle), runningThreads_.end());
}
