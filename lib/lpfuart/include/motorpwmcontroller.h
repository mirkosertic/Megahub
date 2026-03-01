#ifndef MOTORPWMCONTROLLER_H
#define MOTORPWMCONTROLLER_H

#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class LegoDevice;

/**
 * Software PWM controller for LEGO motor speed control.
 *
 * Manages up to 4 motor devices using a single FreeRTOS task to minimize
 * memory overhead. Implements state caching to only send I2C commands when
 * motor state actually changes, reducing I2C bus traffic and electrical noise.
 *
 * Speed range: -127 to +127
 *   -127 = full speed reverse
 *   -64  = half speed reverse
 *   0    = stopped
 *   +64  = half speed forward
 *   +127 = full speed forward
 *
 * PWM frequency: 200Hz (5ms period)
 * Resolution: 1% duty cycle (100 steps)
 */
class MotorPWMController {
public:
	/**
	 * Constructor. Create one instance and inject into devices.
	 */
	MotorPWMController();

	/**
	 * Destructor. Stops the PWM task and cleans up resources.
	 */
	~MotorPWMController();

	/**
	 * Start the PWM task. Call once during system initialization.
	 */
	void start();

	/**
	 * Stop the PWM task and release resources.
	 */
	void stop();

	/**
	 * Register a device for PWM control.
	 *
	 * @param deviceIndex Device slot (0-3)
	 * @param device Pointer to LegoDevice instance
	 */
	void enableDevice(uint8_t deviceIndex, LegoDevice* device);

	/**
	 * Unregister a device from PWM control.
	 *
	 * @param deviceIndex Device slot (0-3)
	 */
	void disableDevice(uint8_t deviceIndex);

	/**
	 * Set motor speed for a device.
	 *
	 * @param deviceIndex Device slot (0-3)
	 * @param speed Speed value (-127 to +127)
	 */
	void setSpeed(uint8_t deviceIndex, int8_t speed);

	/**
	 * Get current speed setting for a device.
	 *
	 * @param deviceIndex Device slot (0-3)
	 * @return Current speed (-127 to +127)
	 */
	int8_t getSpeed(uint8_t deviceIndex) const;

private:
	// Prevent copying
	MotorPWMController(const MotorPWMController&) = delete;
	MotorPWMController& operator=(const MotorPWMController&) = delete;

	/**
	 * State information for each motor device.
	 */
	struct DeviceState {
		LegoDevice* device;      // Pointer to device instance
		int8_t targetSpeed;      // Desired speed (-127 to +127)
		bool currentM1;          // Cached M1 pin state
		bool currentM2;          // Cached M2 pin state
		bool enabled;            // Is this slot active?

		DeviceState()
			: device(nullptr)
			, targetSpeed(0)
			, currentM1(false)
			, currentM2(false)
			, enabled(false) {}
	};

	DeviceState devices_[4];          // State for all 4 device slots
	uint8_t cycleCounter_;            // PWM cycle counter (0-99)
	TaskHandle_t taskHandle_;         // FreeRTOS task handle
	std::atomic<bool> running_;       // Task running flag (atomic for cross-task access)

	/**
	 * FreeRTOS task function for PWM generation.
	 */
	static void pwmTask(void* parameter);

	/**
	 * Update motor state for a single device if needed.
	 * Only sends I2C commands if state changes.
	 *
	 * @param index Device index
	 * @param shouldBeOn Whether motor should be powered
	 * @param direction Motor direction (true = forward, false = reverse)
	 */
	void updateMotorState(uint8_t index, bool shouldBeOn, bool direction);
};

#endif // MOTORPWMCONTROLLER_H
