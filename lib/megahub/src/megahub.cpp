#include <Arduino.h>

#include "megahub.h"

#include "YDLidar.h"
#include "commands.h"
#include "gitrevision.h"
#include "i2csync.h"

#include <ArduinoJson.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define MEGAHUBREF_NAME "MEGAHUBTHISREF"
#define FASTLEDREF_NAME "FASTLEDREF"

SemaphoreHandle_t lua_global_mutex = xSemaphoreCreateMutex();

struct MainControlLoopTaskParams {
	lua_State *mainstate;
	lua_State *threadstate;
	int function_ref_index;
};

TaskHandle_t mainControlLoopTaskHandle = NULL;

void main_control_loop_task(void *parameters) {
	MainControlLoopTaskParams *params = (MainControlLoopTaskParams *) parameters;

	lua_State *threadState = params->threadstate;
	INFO("Starting main control loop wask");
	while (true) {
		// Check for cancelation
		uint32_t notificationValue = 0;
		if (xTaskNotifyWait(0, 0, &notificationValue, 0) == pdTRUE) {
			if (notificationValue == 1) {
				INFO("Main control loop will be canceled");
				break;
			}
		}

		// To be called function is argument index 1, so we push it onto the stack
		lua_rawgeti(threadState, LUA_REGISTRYINDEX, params->function_ref_index);

		// Invoke the function, 0 arguments, 0 return values
		int result = lua_pcall(threadState, 0, 0, 0);

		// Do some sanity checking
		if (result != LUA_OK) {
			const char *error_msg = lua_tostring(threadState, -1);
			WARN("Error processing Lua function : %s", error_msg);
			lua_pop(threadState, 1);

			break;
		}

		// Give the scheduler some time - this could also be done by including a wait() call into the Lua script....
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	lua_closethread(threadState, params->mainstate);
	delete params;
	INFO("Done with main control loop");
	vTaskDelete(NULL);
}

int hub_register_main_control_loop(lua_State *luaState) {

	luaL_checktype(luaState, 1, LUA_TFUNCTION);

	// Create new thread environment
	MainControlLoopTaskParams *params = new MainControlLoopTaskParams();
	params->mainstate = luaState;
	params->function_ref_index = luaL_ref(luaState, LUA_REGISTRYINDEX);
	params->threadstate = lua_newthread(luaState);

	// Create the task
	xTaskCreate(
		main_control_loop_task,
		"MainControlLoop",
		4096,
		(void *) params,
		1,
		&mainControlLoopTaskHandle // Store task handle for cancellation
	);

	return 0;
}

int hub_init(lua_State *luaState) {
	INFO("Starting initialization block");

	// To be called function is argument index 1, so we push it onto the stack
	luaL_checktype(luaState, 1, LUA_TFUNCTION);

	// Invoke the function, 0 arguments, 0 return values
	int result = lua_pcall(luaState, 0, 0, 0);

	// Do some sanity checking
	if (result != LUA_OK) {
		const char *error_msg = lua_tostring(luaState, -1);
		WARN("Error processing Lua function : %s", error_msg);
		lua_pop(luaState, 1);

		return 0;
	}

	INFO("Finished with initialization block");

	return 0;
}

Megahub *getMegaHubRef(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, MEGAHUBREF_NAME);
	void **userdata = (void **) lua_touserdata(L, -1);
	lua_pop(L, 1); // Clean up stack
	return (Megahub *) (userdata ? *userdata : NULL);
}

int hub_setmotorspeed(lua_State *luaState) {
	// To be called function is argument index 1, so we push it onto the stack
	int port = lua_tointeger(luaState, 1);
	int speed = lua_tointeger(luaState, 2);

	DEBUG("Setting motor speed of port %d to %d", port, speed);

	Megahub *megahub = getMegaHubRef(luaState);
	LegoDevice *device = megahub->port(port);
	device->setMotorSpeed(speed);

	DEBUG("Finished with setting motor speed");

	return 0;
}

int hub_digitalread(lua_State *luaState) {

	int pin = lua_tointeger(luaState, 1);

	DEBUG("Reading digital pin %d", pin);

	Megahub *megahub = getMegaHubRef(luaState);
	int value = megahub->digitalReadFrom(pin);

	lua_pushinteger(luaState, value);
	DEBUG("Digital read from pin %d returned value %d", pin, value);

	return 1;
}

int hub_digitalwrite(lua_State *luaState) {

	int pin = lua_tointeger(luaState, 1);
	int value = lua_tointeger(luaState, 2);

	DEBUG("Writing digital pin %d with value %d", pin, value);

	Megahub *megahub = getMegaHubRef(luaState);
	megahub->digitalWriteTo(pin, value);

	return 0;
}

int hub_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"main_control_loop", hub_register_main_control_loop},
		{			 "init",					   hub_init},
		{	 "setmotorspeed",			  hub_setmotorspeed},
		{		 "digitalRead",				hub_digitalread},
		{	 "digitalWrite",				 hub_digitalwrite},
		{			   NULL,						   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}

int fastled_show(lua_State *luaState) {
	DEBUG("FastLED show");

	FastLED.show();

	return 0;
}

int fastled_clear(lua_State *luaState) {
	DEBUG("FastLED clear");

	FastLED.clear();

	return 0;
}

int fastled_set(lua_State *luaState) {
	DEBUG("FastLED set");

	int index = lua_tointeger(luaState, 1);
	int r = lua_tointeger(luaState, 2);
	int g = lua_tointeger(luaState, 3);
	int b = lua_tointeger(luaState, 4);

	lua_getfield(luaState, LUA_REGISTRYINDEX, FASTLEDREF_NAME);
	void **userdata = (void **) lua_touserdata(luaState, -1);
	lua_pop(luaState, 1);

	CRGB *leds = (CRGB *) (userdata ? *userdata : nullptr);
	if (leds != nullptr) {
		leds[index] = CRGB(r, g, b);
	} else {
		WARN("FastLED set called before addleds!");
	}

	return 0;
}

int fastled_addleds(lua_State *luaState) {
	DEBUG("FastLED addleds");

	int type = lua_tointeger(luaState, 1);
	int pin = lua_tointeger(luaState, 2);
	int numleds = lua_tointeger(luaState, 3);

	if (type == NEOPIXEL_TYPE) {
		CRGB *leds = new CRGB[numleds];
		switch (pin) {
			case GPIO_NUM_13:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_13>(leds, numleds);
				break;
			case GPIO_NUM_16:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_16>(leds, numleds);
				break;
			case GPIO_NUM_17:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_17>(leds, numleds);
				break;
			case GPIO_NUM_25:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_25>(leds, numleds);
				break;
			case GPIO_NUM_26:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_26>(leds, numleds);
				break;
			case GPIO_NUM_27:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_27>(leds, numleds);
				break;
			case GPIO_NUM_32:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_32>(leds, numleds);
				break;
			case GPIO_NUM_33:
				FastLED.addLeds<NEOPIXEL, GPIO_NUM_33>(leds, numleds);
				break;
			default:
				WARN("Unsupported pin %d for FastLED!");
				delete leds;
				return 0;
		}

		void **userdata = (void **) lua_newuserdata(luaState, sizeof(void *));
		*userdata = leds;
		lua_setfield(luaState, LUA_REGISTRYINDEX, FASTLEDREF_NAME);

	} else {
		WARN("FastLED addleds called with unknown type %d", type);
	}
	FastLED.clear();

	return 0;
}

int fastled_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{	 "show",	 fastled_show},
		{	 "clear",	  fastled_clear},
		{"addleds", fastled_addleds},
		{	 "set",		fastled_set},
		{	 NULL,			   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}

int imu_yaw(lua_State *luaState) {

	DEBUG("Getting IMU yaw");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getYaw());

	return 1;
}

int imu_pitch(lua_State *luaState) {

	DEBUG("Getting IMU pitch");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getPitch());

	return 1;
}

int imu_roll(lua_State *luaState) {

	DEBUG("Getting IMU roll");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getRoll());

	return 1;
}

int imu_accelerationx(lua_State *luaState) {

	DEBUG("Getting IMU acceleration x");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationX());

	return 1;
}

int imu_accelerationy(lua_State *luaState) {

	DEBUG("Getting IMU acceleration y");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationY());

	return 1;
}

int imu_accelerationz(lua_State *luaState) {

	DEBUG("Getting IMU acceleration z");

	Megahub *megahub = getMegaHubRef(luaState);
	IMU *imu = megahub->imu();

	lua_pushnumber(luaState, imu->getAccelerationZ());

	return 1;
}

int imu_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{		  "yaw",			imu_yaw},
		{		 "pitch",		  imu_pitch},
		{		 "roll",			 imu_roll},
		{"accelerationX", imu_accelerationx},
		{"accelerationY", imu_accelerationy},
		{"accelerationZ", imu_accelerationz},
		{		   NULL,			   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}

int debug_freeheap(lua_State *luaState) {

	long value = ESP.getFreeHeap();
	DEBUG("Free heap is %ld", value);

	lua_pushnumber(luaState, value);

	return 1;
}

int debug_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"freeHeap", debug_freeheap},
		{		 NULL,		   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}

int ui_show_value(lua_State *luaState) {

	DEBUG("UI show value called");
	JsonDocument doc;
	doc["type"] = "show_value";
	doc["label"] = lua_tostring(luaState, 1);

	int format = lua_tointeger(luaState, 2);
	if (format == FORMAT_SIMPLE) {
		doc["format"] = "simple";
	} else {
		doc["format"] = "unknown";
	}

	if (lua_isboolean(luaState, 3)) {
		bool boolvalue = lua_toboolean(luaState, 3);
		doc["value"] = boolvalue;
	} else if (lua_isnumber(luaState, 3)) {
		double numvalue = lua_tonumber(luaState, 3);
		doc["value"] = numvalue;
	} else if (lua_isstring(luaState, 3)) {
		const char *strvalue = lua_tostring(luaState, 3);
		doc["value"] = strvalue;
	} else {
		WARN("ui_show_value called with unsupported value type");
		doc["value"] = "unsupported";
	}

	String strCommand;
	serializeJson(doc, strCommand);

	DEBUG("UI show value command: %s", strCommand.c_str());

	// Enqueue command
	Commands::instance()->queue(strCommand);

	return 0;
}

int ui_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"showvalue", ui_show_value},
		{		 NULL,		   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}

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

	device1->initialize();
	device2->initialize();
	device3->initialize();
	device4->initialize();

	currentprogramstate_ = nullptr;
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
	esp_efuse_mac_get_default(chipId);

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
	return "Robotic Hub";
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
