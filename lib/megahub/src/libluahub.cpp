#include "megahub.h"

struct HubThreadParams {
	lua_State *mainstate;
	lua_State *threadstate;
	int function_ref_index;
	Megahub *hub;
};

extern Megahub *getMegaHubRef(lua_State *L);

void hub_thread_task(void *parameters) {
	HubThreadParams *params = (HubThreadParams *) parameters;
	Megahub *hub = params->hub;

	lua_State *threadState = params->threadstate;
	INFO("Starting thread task");
	while (true) {

		unsigned long start = micros();

		// Check for cancelation
		uint32_t notificationValue = 0;
		if (xTaskNotifyWait(0, 0, &notificationValue, 0) == pdTRUE) {
			if (notificationValue == 1) {
				INFO("Thread task will be canceled");
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

		// TODO: Implement better statistics here		
		long duration = micros() - start;

		// Give the scheduler some time - this could also be done by including a wait() call into the Lua script....
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	lua_closethread(threadState, params->mainstate);
	delete params;
	INFO("Done with thread");
	vTaskDelete(NULL);
}

int hub_startthread(lua_State *luaState) {

	String threadName = lua_tostring(luaState, 1);
	int stackSize = lua_tointeger(luaState, 2);
	luaL_checktype(luaState, 3, LUA_TFUNCTION);

	Megahub *hub = getMegaHubRef(luaState);

	// Create new thread environment
	HubThreadParams *params = new HubThreadParams();
	params->mainstate = luaState;
	params->function_ref_index = luaL_ref(luaState, LUA_REGISTRYINDEX);
	params->threadstate = lua_newthread(luaState);
	params->hub = hub;

	INFO("Starting thread %s with stack size %d", threadName.c_str(), stackSize);

	TaskHandle_t taskHandle = NULL;

	// Create the task
	xTaskCreate(
		hub_thread_task,
		threadName.c_str(),
		stackSize,
		(void *) params,
		1,
		&taskHandle // Store task handle for cancellation
	);

	hub->registerThread(taskHandle);

	TaskHandle_t* udata = (TaskHandle_t*)lua_newuserdata(luaState, sizeof(TaskHandle_t*));
	*udata = taskHandle;
	return 1;
}

int hub_stopthread(lua_State *luaState) {

	INFO("Stopping thread");

 	TaskHandle_t* task_handle = (TaskHandle_t*) lua_touserdata(luaState, 1);
    
    if (*task_handle != NULL) {
		Megahub *hub = getMegaHubRef(luaState);
		hub->stopThread(*task_handle);
        *task_handle = NULL;
    }
    
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
		{	 "startthread",	hub_startthread},
		{	 "stopthread",	hub_stopthread},
		{		 "init",			 hub_init},
		{"setmotorspeed", hub_setmotorspeed},
		{		 "pinMode",	hub_set_pin_mode},
		{	 "digitalRead",	hub_digitalread},
		{ "digitalWrite",	 hub_digitalwrite},
		{		   NULL,			   NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
