#include "inputdevices.h"
#include "logging.h"
#include "megahub.h"

extern InputDevices *getInputDevicesRef(lua_State *L);

int gamepad_value(lua_State *luaState) {

	InputDevices *inputDevices = getInputDevicesRef(luaState);

	int gamepadIndex = lua_tointeger(luaState, 1);
	int value = lua_tointeger(luaState, 2);

	if (gamepadIndex == GAMEPAD1) {
		std::vector<GamepadState> knownpads = inputDevices->getAllGamepadStates();
		if (knownpads.size() == 1) {
			GamepadState state = knownpads[0];
			if (state.connected) {
				if (value == GAMEPAD_LEFT_X) {
					lua_pushnumber(luaState, state.leftStickX);
					return 1;
				}
				if (value == GAMEPAD_LEFT_Y) {
					lua_pushnumber(luaState, state.leftStickY);
					return 1;
				}
				if (value == GAMEPAD_RIGHT_X) {
					lua_pushnumber(luaState, state.leftStickX);
					return 1;
				}
				if (value == GAMEPAD_RIGHT_Y) {
					lua_pushnumber(luaState, state.leftStickY);
					return 1;
				}
				WARN("Unsupported value to retrive from gamepad %d : %d", gamepadIndex, value);
			}
		}
		WARN("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		WARN("Unsupported gamepad : %d", gamepadIndex);
	}

	// Number 0
	lua_pushnumber(luaState, 0);
	return 1;
}

int gamepad_connected(lua_State *luaState) {

	InputDevices *inputDevices = getInputDevicesRef(luaState);

	int gamepadIndex = lua_tointeger(luaState, 1);

	if (gamepadIndex == GAMEPAD1) {
		std::vector<GamepadState> knownpads = inputDevices->getAllGamepadStates();
		if (knownpads.size() == 1) {
			GamepadState state = knownpads[0];
			return state.connected;
		}

		WARN("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		WARN("Unsupported gamepad : %d", gamepadIndex);
	}

	// Not connected
	lua_pushboolean(luaState, 0);
	return 1;
}

int gamepad_buttonpressed(lua_State *luaState) {

	InputDevices *inputDevices = getInputDevicesRef(luaState);

	int gamepadIndex = lua_tointeger(luaState, 1);
	int button = lua_tointeger(luaState, 2);

	if (gamepadIndex == GAMEPAD1) {
		std::vector<GamepadState> knownpads = inputDevices->getAllGamepadStates();
		if (knownpads.size() == 1) {
			GamepadState state = knownpads[0];
			if (state.connected) {
				DEBUG("Curremt button state for gamepad %d is %d", gamepadIndex, state.buttons);
				if (button == GAMEPAD_BUTTON_1) {
					lua_pushboolean(luaState, (state.buttons & 0x01) > 0);
					return 1;
				}
				WARN("Unsupported button to retrive from gamepad %d : %d", gamepadIndex, button);
			}
		}
		WARN("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		WARN("Unsupported gamepad : %d", gamepadIndex);
	}

	// Not pressed
	lua_pushboolean(luaState, 0);
	return 1;
}

int gamepad_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{		 "value",		  gamepad_value},
		{	 "connected",	  gamepad_connected},
		{"buttonpressed", gamepad_buttonpressed},
		{		   NULL,				  NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
