#include "inputdevices.h"

#include "logging.h"

InputDevices::InputDevices() {
	gamepadStatesMutex_ = xSemaphoreCreateMutex();
}

bool InputDevices::getGamepadState(const char *macAddress, GamepadState &state) {
	std::string key(macAddress);
	bool found = false;

	if (xSemaphoreTake(gamepadStatesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		auto it = gamepadStates_.find(key);
		if (it != gamepadStates_.end()) {
			state = it->second;
			found = true;
		}
		xSemaphoreGive(gamepadStatesMutex_);
	} else {
		WARN("Failed to acquire mutex for gamepad states");
	}

	return found;
}

std::vector<GamepadState> InputDevices::getAllGamepadStates() {
	std::vector<GamepadState> states;

	if (xSemaphoreTake(gamepadStatesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		for (const auto &pair : gamepadStates_) {
			states.push_back(pair.second);
		}
		xSemaphoreGive(gamepadStatesMutex_);
	} else {
		WARN("Failed to acquire mutex for gamepad states");
	}

	return states;
}

void InputDevices::registerGamepadState(std::string macAddress, GamepadState state) {
	if (xSemaphoreTake(gamepadStatesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		gamepadStates_[macAddress] = state;
		INFO("Gamepad state initialized for device: %s", macAddress.c_str());
		xSemaphoreGive(gamepadStatesMutex_);
	}
}

void InputDevices::updateGamepad(std::string macAddress, std::function<void(std::string &macAddress, GamepadState &)> callback) {
	if (xSemaphoreTake(gamepadStatesMutex_, pdMS_TO_TICKS(100)) == pdTRUE) {
		auto it = gamepadStates_.find(macAddress);
		if (it != gamepadStates_.end()) {
			callback(macAddress, it->second);
		}
		xSemaphoreGive(gamepadStatesMutex_);
	}
}
