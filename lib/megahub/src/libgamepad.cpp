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
				if (value == GAMEPAD_DPAD) {
					lua_pushnumber(luaState, state.dpad);
					return 1;
				}
				WARN("Unsupported value to retrive from gamepad %d : %d", gamepadIndex, value);
			}
		}
		DEBUG("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		DEBUG("Unsupported gamepad : %d", gamepadIndex);
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

		DEBUG("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		DEBUG("Unsupported gamepad : %d", gamepadIndex);
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
				if (button == GAMEPAD_BUTTON_2) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 1)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_3) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 2)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_4) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 3)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_5) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 4)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_6) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 5)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_7) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 6)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_8) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 7)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_9) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 8)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_10) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 9)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_11) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 10)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_12) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 11)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_13) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 12)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_14) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 13)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_15) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 14)) > 0);
					return 1;
				}
				if (button == GAMEPAD_BUTTON_16) {
					lua_pushboolean(luaState, (state.buttons & (0x01 << 15)) > 0);
					return 1;
				}
				WARN("Unsupported button to retrive from gamepad %d : %d", gamepadIndex, button);
			}
		}
		DEBUG("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		DEBUG("Unsupported gamepad : %d", gamepadIndex);
	}

	// Not pressed
	lua_pushboolean(luaState, 0);
	return 1;
}

int gamepad_buttonraw(lua_State *luaState) {

	InputDevices *inputDevices = getInputDevicesRef(luaState);

	int gamepadIndex = lua_tointeger(luaState, 1);

	if (gamepadIndex == GAMEPAD1) {
		std::vector<GamepadState> knownpads = inputDevices->getAllGamepadStates();
		if (knownpads.size() == 1) {
			GamepadState state = knownpads[0];
			if (state.connected) {
				DEBUG("Curremt button state for gamepad %d is %d", gamepadIndex, state.buttons);
				lua_pushnumber(luaState, state.buttons);
			}
		}
		DEBUG("Invalid number of connected gamepads %d", knownpads.size());
	} else {
		DEBUG("Unsupported gamepad : %d", gamepadIndex);
	}

	lua_pushnumber(luaState, 0);
	return 1;
}

int gamepad_library(lua_State *luaState) {
	const luaL_Reg hubfunctions[] = {
		{		 "value",		  gamepad_value},
		{	 "connected",	  gamepad_connected},
		{"buttonpressed", gamepad_buttonpressed},
		{	 "buttonsraw",	  gamepad_buttonraw},
		{		   NULL,				  NULL}
	};
	luaL_newlib(luaState, hubfunctions);
	return 1;
}
