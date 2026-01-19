#include <Arduino.h>

#include "megahub.h"

#include "YDLidar.h"
#include "commands.h"
#include "gitrevision.h"
#include "i2csync.h"
#include "portstatus.h"

#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_mac.h>

#define MEGAHUBREF_NAME "MEGAHUBTHISREF"

SemaphoreHandle_t lua_global_mutex = xSemaphoreCreateMutex();

extern TaskHandle_t mainControlLoopTaskHandle;

TaskHandle_t statusReporterTaskHandle = NULL;

Megahub *getMegaHubRef(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, MEGAHUBREF_NAME);
	void **userdata = (void **) lua_touserdata(L, -1);
	lua_pop(L, 1); // Clean up stack
	return (Megahub *) (userdata ? *userdata : NULL);
}

void status_reporter_task(void *parameters) {
	Megahub *hub = (Megahub *) parameters;
	INFO("Starting task reporter task");
	while (true) {
		DEBUG("Sending Portstatus");

		static unsigned long lastHeapLog = 0;
		unsigned long currentMillis = millis();

		// Log stack utilization every 10 seconds
		if (currentMillis - lastHeapLog >= 10000) {
			INFO("Minimum Free Stack : %d", uxTaskGetStackHighWaterMark(NULL));
			lastHeapLog = currentMillis;
		}

		i2c_lock();
		JsonDocument status;

		// TODO: Update icons and other stuff here
		JsonArray ports = status["ports"].to<JsonArray>();

		LegoDevice *device1 = hub->port(PORT1);
		JsonObject port1 = ports.add<JsonObject>();
		port1["id"] = 1;
		if (device1->fullyInitialized()) {
			port1["connected"] = true;
			JsonObject device = port1["device"].to<JsonObject>();
			device["type"] = device1->name();
			device["deviceId"] = device1->getDeviceId();
			device["icon"] = "⚙️";
			JsonArray modes = device["modes"].to<JsonArray>();
			for (int i = 0; i < device1->numModes(); i++) {
				Mode *mode = device1->getMode(i);

				JsonObject singleMode = modes.add<JsonObject>();
				singleMode["id"] = i;				
				singleMode["name"] = String(mode->getName().c_str());
				singleMode["units"] = String(mode->getUnits().c_str());
				Format* format = mode->getFormat();
				if (format != nullptr) {
					singleMode["datasets"] = format->getDatasets();
					singleMode["figures"] = format->getFigures();
					singleMode["decimals"] = format->getDecimals();

					switch(format->getFormatType()) {
						case Format::FormatType::DATA8: {
							singleMode["type"] = "DATA8";
							break;
						}
						case Format::FormatType::DATA16: {
							singleMode["type"] = "DATA16";
							break;
						}
						case Format::FormatType::DATA32: {
							singleMode["type"] = "DATA32";
							break;
						}
						case Format::FormatType::DATAFLOAT: {
							singleMode["type"] = "DATAFLOAT";
							break;
						}
						default: {
							// Do nothing
						}
					}
				}
			}
		} else {
			port1["connected"] = false;
		}

		LegoDevice *device2 = hub->port(PORT2);
		JsonObject port2 = ports.add<JsonObject>();
		port2["id"] = 2;
		if (device2->fullyInitialized()) {
			port2["connected"] = true;
			JsonObject device = port2["device"].to<JsonObject>();
			device["type"] = device2->name();
			device["deviceId"] = device2->getDeviceId();
			device["icon"] = "⚙️";
			JsonArray modes = device["modes"].to<JsonArray>();
			for (int i = 0; i < device2->numModes(); i++) {
				Mode *mode = device2->getMode(i);

				JsonObject singleMode = modes.add<JsonObject>();
				singleMode["id"] = i;				
				singleMode["name"] = String(mode->getName().c_str());
				singleMode["units"] = String(mode->getUnits().c_str());
				Format* format = mode->getFormat();
				if (format != nullptr) {
					singleMode["datasets"] = format->getDatasets();
					singleMode["figures"] = format->getFigures();
					singleMode["decimals"] = format->getDecimals();

					switch(format->getFormatType()) {
						case Format::FormatType::DATA8: {
							singleMode["type"] = "DATA8";
							break;
						}
						case Format::FormatType::DATA16: {
							singleMode["type"] = "DATA16";
							break;
						}
						case Format::FormatType::DATA32: {
							singleMode["type"] = "DATA32";
							break;
						}
						case Format::FormatType::DATAFLOAT: {
							singleMode["type"] = "DATAFLOAT";
							break;
						}
						default: {
							// Do nothing
						}
					}
				}
			}
		} else {
			port2["connected"] = false;
		}

		LegoDevice *device3 = hub->port(PORT3);
		JsonObject port3 = ports.add<JsonObject>();
		port3["id"] = 3;
		if (device3->fullyInitialized()) {
			port3["connected"] = true;
			JsonObject device = port3["device"].to<JsonObject>();
			device["type"] = device3->name();
			device["deviceId"] = device3->getDeviceId();
			device["icon"] = "⚙️";
			JsonArray modes = device["modes"].to<JsonArray>();
			for (int i = 0; i < device3->numModes(); i++) {
				Mode *mode = device3->getMode(i);

				JsonObject singleMode = modes.add<JsonObject>();
				singleMode["id"] = i;
				singleMode["name"] = String(mode->getName().c_str());
				singleMode["units"] = String(mode->getUnits().c_str());
				Format* format = mode->getFormat();
				if (format != nullptr) {
					singleMode["datasets"] = format->getDatasets();
					singleMode["figures"] = format->getFigures();
					singleMode["decimals"] = format->getDecimals();

					switch(format->getFormatType()) {
						case Format::FormatType::DATA8: {
							singleMode["type"] = "DATA8";
							break;
						}
						case Format::FormatType::DATA16: {
							singleMode["type"] = "DATA16";
							break;
						}
						case Format::FormatType::DATA32: {
							singleMode["type"] = "DATA32";
							break;
						}
						case Format::FormatType::DATAFLOAT: {
							singleMode["type"] = "DATAFLOAT";
							break;
						}
						default: {
							// Do nothing
						}
					}
				}

			}
		} else {
			port3["connected"] = false;
		}

		LegoDevice *device4 = hub->port(PORT4);
		JsonObject port4 = ports.add<JsonObject>();
		port4["id"] = 4;
		if (device4->fullyInitialized()) {
			port4["connected"] = true;
			JsonObject device = port4["device"].to<JsonObject>();
			device["type"] = device4->name();
			device["deviceId"] = device4->getDeviceId();
			device["icon"] = "⚙️";
			JsonArray modes = device["modes"].to<JsonArray>();
			for (int i = 0; i < device4->numModes(); i++) {
				Mode *mode = device4->getMode(i);

				JsonObject singleMode = modes.add<JsonObject>();
				singleMode["id"] = i;				
				singleMode["name"] = String(mode->getName().c_str());
				singleMode["units"] = String(mode->getUnits().c_str());
				Format* format = mode->getFormat();
				if (format != nullptr) {
					singleMode["datasets"] = format->getDatasets();
					singleMode["figures"] = format->getFigures();
					singleMode["decimals"] = format->getDecimals();

					switch(format->getFormatType()) {
						case Format::FormatType::DATA8: {
							singleMode["type"] = "DATA8";
							break;
						}
						case Format::FormatType::DATA16: {
							singleMode["type"] = "DATA16";
							break;
						}
						case Format::FormatType::DATA32: {
							singleMode["type"] = "DATA32";
							break;
						}
						case Format::FormatType::DATAFLOAT: {
							singleMode["type"] = "DATAFLOAT";
							break;
						}
						default: {
							// Do nothing
						}
					}
				}
			}
		} else {
			port4["connected"] = false;
		}

		i2c_unlock();

		String strContent;
		serializeJson(status, strContent);

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

	// And also global functions
	lua_register(ls, "wait", global_wait);
	lua_register(ls, "print", global_print);
	lua_register(ls, "millis", global_millis);

	// The self reference
	void **userdata = (void **) lua_newuserdata(ls, sizeof(void *));
	*userdata = this;
	lua_setfield(ls, LUA_REGISTRYINDEX, MEGAHUBREF_NAME);

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

	// FastLED constants
	lua_pushinteger(ls, NEOPIXEL_TYPE);
	lua_setglobal(ls, "NEOPIXEL");

	// UI constants
	lua_pushinteger(ls, FORMAT_SIMPLE);
	lua_setglobal(ls, "FORMAT_SIMPLE");

	INFO("Finished creating new Lua state");

	return ls;
}

Megahub::Megahub(LegoDevice *device1, LegoDevice *device2, LegoDevice *device3, LegoDevice *device4, IMU *imu)
	: device1_(device1)
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

	if (mainControlLoopTaskHandle != NULL) {
		INFO("Canceling existing main control loop task");
		xTaskNotify(mainControlLoopTaskHandle, 1, eSetValueWithOverwrite);
		vTaskDelay(pdMS_TO_TICKS(100)); // Give some time to finish
	}

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

	if (mainControlLoopTaskHandle != NULL) {
		INFO("Canceling existing main control loop task");
		xTaskNotify(mainControlLoopTaskHandle, 1, eSetValueWithOverwrite);
		vTaskDelay(pdMS_TO_TICKS(100)); // Give some time to finish
		mainControlLoopTaskHandle = NULL;

		return true;
	}

	INFO("No Lua code execution to stop");
	return false;
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
