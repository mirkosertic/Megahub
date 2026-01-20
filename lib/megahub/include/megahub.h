#ifndef MEGAHUB_H
#define MEGAHUB_H

#include "imu.h"
#include "inputdevices.h"
#include "legodevice.h"
#include "logging.h"
#include "lua.hpp"

#include <memory>

#define PORT1 1
#define PORT2 2
#define PORT3 3
#define PORT4 4

#define UART1_GP4 10000
#define UART1_GP5 10001
#define UART1_GP6 10002
#define UART1_GP7 10003
#define UART2_GP4 10004
#define UART2_GP5 10005
#define UART2_GP6 10006
#define UART2_GP7 10007

#define NEOPIXEL_TYPE 1000

#define FORMAT_SIMPLE 2000

#define PINMODE_INPUT		   3000
#define PINMODE_INPUT_PULLUP   3001
#define PINMODE_INPUT_PULLDOWN 3002
#define PINMODE_OUTPUT		   3003

#define GAMEPAD1		 4000
#define GAMEPAD_BUTTON_1 5000
#define GAMEPAD_LEFT_X	 6000
#define GAMEPAD_LEFT_Y	 6001
#define GAMEPAD_RIGHT_X	 6002
#define GAMEPAD_RIGHT_Y	 6003

struct LuaCheckResult {
	bool success;
	int parseTime;
	String errorMessage;
};

class Megahub {
public:
	Megahub(InputDevices *inputDevices, LegoDevice *device1, LegoDevice *device2, LegoDevice *device3, LegoDevice *device4, IMU *imu);
	virtual ~Megahub();

	void loop();

	LegoDevice *port(int num);
	IMU *imu();

	String deviceUid();
	String name();
	String manufacturer();
	String deviceType();
	String version();
	String serialnumber();

	LuaCheckResult checkLUACode(String luaCode);
	void executeLUACode(String luaCode);
	bool stopLUACode();

	void setPinMode(int pin, int mode);
	int digitalReadFrom(int pin);
	void digitalWriteTo(int pin, int value);

private:
	std::unique_ptr<InputDevices> inputdevices_;
	std::unique_ptr<LegoDevice> device1_;
	std::unique_ptr<LegoDevice> device2_;
	std::unique_ptr<LegoDevice> device3_;
	std::unique_ptr<LegoDevice> device4_;
	std::unique_ptr<IMU> imu_;

	lua_State *globalLuaState_;
	lua_State *currentprogramstate_;

	lua_State *newLuaState();
};

#endif // MEGAHUB_H
