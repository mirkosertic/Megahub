#include "megahub.h"

struct MainControlLoopTaskParams {
	lua_State *mainstate;
	lua_State *threadstate;
	int function_ref_index;
};

extern Megahub *getMegaHubRef(lua_State *L);

TaskHandle_t mainControlLoopTaskHandle = NULL;

void main_control_loop_task(void *parameters) {
	MainControlLoopTaskParams *params = (MainControlLoopTaskParams *) parameters;
	Megahub *hub = getMegaHubRef(params->mainstate);

	lua_State *threadState = params->threadstate;
	INFO("Starting main control loop task");
	while (true) {

		unsigned long start = micros();

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

		long duration = micros() - start;
		hub->updateMainLoopStatistik(duration);

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

int hub_setmotorspeed(lua_State *luaState) {

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

int hub_set_pin_mode(lua_State *luaState) {

	int pin = lua_tointeger(luaState, 1);
	int value = lua_tointeger(luaState, 2);

	DEBUG("Setting pin mode of pin %d to %d", pin, value);

	Megahub *megahub = getMegaHubRef(luaState);
	megahub->setPinMode(pin, value);

	return 0;
}

int hub_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{"main_control_loop", hub_register_main_control_loop},
		{			 "init",					   hub_init},
		{	 "setmotorspeed",			  hub_setmotorspeed},
		{		  "pinMode",				hub_set_pin_mode},
		{		 "digitalRead",				hub_digitalread},
		{	 "digitalWrite",				 hub_digitalwrite},
		{			   NULL,						   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
