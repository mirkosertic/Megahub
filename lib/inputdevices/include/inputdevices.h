#ifndef INPUTDEVICES_H
#define INPUTDEVICES_H

#include "esp_bt_defs.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

// Gamepad State - Standard HID gamepad layout
struct GamepadState {
	esp_bd_addr_t address;
	bool connected;
	uint32_t timestamp;
	uint16_t vendorId;
	uint16_t productId;

	uint16_t buttons;

	uint8_t dpad;

	// Analog sticks (16-bit signed, range: -32768 to 32767, center=0)
	int16_t leftStickX;
	int16_t leftStickY;
	int16_t rightStickX;
	int16_t rightStickY;
};

class InputDevices {
public:
	InputDevices();

	// Gamepad API
	void registerGamepadState(std::string macAddress, GamepadState state);
	void updateGamepad(std::string macAddress, std::function<void(std::string &macAddress, GamepadState &)> callback);
	bool getGamepadState(const char *macAddress, GamepadState &state);
	std::vector<GamepadState> getAllGamepadStates();

private:
	std::map<std::string, GamepadState> gamepadStates_;
	SemaphoreHandle_t gamepadStatesMutex_;
};

#endif // INPUTDEVICES_H
