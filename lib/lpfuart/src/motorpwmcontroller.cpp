#include "motorpwmcontroller.h"

#include "i2csync.h"
#include "legodevice.h"
#include "logging.h"

// PWM configuration constants
static const uint32_t PWM_FREQUENCY_HZ = 200;                  // 200 Hz PWM frequency
static const uint32_t PWM_PERIOD_MS = 1000 / PWM_FREQUENCY_HZ; // 5 ms
static const uint8_t PWM_RESOLUTION_STEPS = 100;               // 100 steps (1% resolution)
static const uint32_t TASK_STACK_SIZE = 2048;                  // 2KB stack for PWM task
static const UBaseType_t TASK_PRIORITY = 2;                    // Lower than comms, higher than idle

MotorPWMController::MotorPWMController() : cycleCounter_(0), taskHandle_(nullptr), running_(false) {}

MotorPWMController::~MotorPWMController() {
	stop();
}

void MotorPWMController::start() {
	if (running_) {
		WARN("PWM controller already running");
		return;
	}

	INFO("Starting motor PWM controller at %lu Hz", PWM_FREQUENCY_HZ);

	// Create FreeRTOS task
	BaseType_t result = xTaskCreate(pwmTask, "MotorPWM", TASK_STACK_SIZE, this, TASK_PRIORITY, &taskHandle_);

	if (result != pdPASS) {
		ERROR("Failed to create PWM task");
		return;
	}

	running_ = true;
	INFO("PWM controller started successfully");
}

void MotorPWMController::stop() {
	if (!running_) {
		return;
	}

	INFO("Stopping motor PWM controller");
	running_ = false;

	if (taskHandle_ != nullptr) {
		vTaskDelete(taskHandle_);
		taskHandle_ = nullptr;
	}

	// Turn off all motors
	for (uint8_t i = 0; i < 4; i++) {
		if (devices_[i].enabled && devices_[i].device != nullptr) {
			updateMotorState(i, false, false);
		}
		devices_[i].enabled = false;
	}
}

void MotorPWMController::enableDevice(uint8_t deviceIndex, LegoDevice* device) {
	if (deviceIndex >= 4) {
		ERROR("Invalid device index: %d", deviceIndex);
		return;
	}

	if (device == nullptr) {
		ERROR("Cannot enable null device at index %d", deviceIndex);
		return;
	}

	INFO("Enabling PWM for device %d", deviceIndex);
	devices_[deviceIndex].device = device;
	devices_[deviceIndex].targetSpeed = 0;
	devices_[deviceIndex].currentM1 = false;
	devices_[deviceIndex].currentM2 = false;
	devices_[deviceIndex].enabled = true;

	// Ensure motor is off initially
	updateMotorState(deviceIndex, false, false);
}

void MotorPWMController::disableDevice(uint8_t deviceIndex) {
	if (deviceIndex >= 4) {
		ERROR("Invalid device index: %d", deviceIndex);
		return;
	}

	if (!devices_[deviceIndex].enabled) {
		return;
	}

	INFO("Disabling PWM for device %d", deviceIndex);

	// Turn off motor before disabling
	updateMotorState(deviceIndex, false, false);

	devices_[deviceIndex].enabled = false;
	devices_[deviceIndex].device = nullptr;
	devices_[deviceIndex].targetSpeed = 0;
}

void MotorPWMController::setSpeed(uint8_t deviceIndex, int8_t speed) {
	if (deviceIndex >= 4) {
		ERROR("Invalid device index: %d", deviceIndex);
		return;
	}

	if (!devices_[deviceIndex].enabled) {
		WARN("Device %d not enabled for PWM", deviceIndex);
		return;
	}

	// Clamp speed to valid range
	if (speed < -127) {
		speed = -127;
	}
	if (speed > 127) {
		speed = 127;
	}

	devices_[deviceIndex].targetSpeed = speed;
}

int8_t MotorPWMController::getSpeed(uint8_t deviceIndex) const {
	if (deviceIndex >= 4) {
		return 0;
	}
	return devices_[deviceIndex].targetSpeed;
}

void MotorPWMController::updateMotorState(uint8_t index, bool shouldBeOn, bool direction) {
	DeviceState& state = devices_[index];

	// Calculate desired M1/M2 states based on power and direction
	bool desiredM1, desiredM2;
	if (!shouldBeOn) {
		// Motor off: both pins LOW
		desiredM1 = false;
		desiredM2 = false;
	} else if (direction) {
		// Forward: M1=HIGH, M2=LOW
		desiredM1 = true;
		desiredM2 = false;
	} else {
		// Reverse: M1=LOW, M2=HIGH
		desiredM1 = false;
		desiredM2 = true;
	}

	// CRITICAL: Only send I2C commands if state actually changes
	if (desiredM1 == state.currentM1 && desiredM2 == state.currentM2) {
		// State unchanged - skip I2C communication entirely
		return;
	}

	// State changed - update motor via I2C
	i2c_lock();
	state.device->getSerialIO()->setM1(desiredM1);
	state.device->getSerialIO()->setM2(desiredM2);
	i2c_unlock();

	// Update cached state
	state.currentM1 = desiredM1;
	state.currentM2 = desiredM2;

	// Allow electrical transients to settle (same as original setMotorSpeed)
	delayMicroseconds(100);
}

void MotorPWMController::pwmTask(void* parameter) {
	MotorPWMController* controller = static_cast<MotorPWMController*>(parameter);
	const TickType_t tickPeriod = pdMS_TO_TICKS(PWM_PERIOD_MS);
	TickType_t lastWakeTime = xTaskGetTickCount();

	INFO("PWM task started, period=%lu ms", PWM_PERIOD_MS);

	while (controller->running_) {
		// Process all 4 devices in a single iteration
		for (uint8_t i = 0; i < 4; i++) {
			DeviceState& state = controller->devices_[i];

			// Skip disabled devices
			if (!state.enabled || state.device == nullptr) {
				continue;
			}

			int8_t speed = state.targetSpeed;

			// Calculate duty cycle (0-100%)
			uint8_t dutyCycle = (abs(speed) * 100) / 127;

			// Determine direction: speed >= 0 is forward
			bool direction = (speed >= 0);

			// Determine if motor should be ON during this PWM slice
			bool shouldBeOn = (controller->cycleCounter_ < dutyCycle) && (dutyCycle > 0);

			// Update motor state - this function handles state caching
			controller->updateMotorState(i, shouldBeOn, direction);
		}

		// Increment cycle counter (0-99)
		controller->cycleCounter_++;
		if (controller->cycleCounter_ >= PWM_RESOLUTION_STEPS) {
			controller->cycleCounter_ = 0;
		}

		// Wait for next cycle (200Hz = 5ms period)
		vTaskDelayUntil(&lastWakeTime, tickPeriod);
	}

	INFO("PWM task exiting");
	vTaskDelete(nullptr);
}
