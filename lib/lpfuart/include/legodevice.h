#ifndef LEGODEVICE_H
#define LEGODEVICE_H

#include <Arduino.h>

#include "lumpparser.h"
#include "mode.h"
#include "serialio.h"

#include <array>
#include <memory>
#include <string>

class MotorPWMController;

#define DEVICEID_EV3_COLOR_SENSOR             29
#define DEVICEID_EV3_ULTRASONIC_SENSOR        30
#define DEVICEID_EV3_GYRO_SENSOR              32
#define DEVICEID_EV3_INFRARED_SENSOR          33
#define DEVICEID_WEDO20_TILT                  34
#define DEVICEID_WEDO20_MOTION                35
#define DEVICEID_BOOST_COLOR_DISTANCE_SENSOR  37
#define DEVICEID_BOOST_INTERACTIVE_MOTOR      38
#define DEVICEID_TECHNIC_XL_MOTOR             45
#define DEVICEID_TECHNIC_MEDIUM_MOTOR         46
#define DEVICEID_SPIKE_MEDIUM_MOTOR           48
#define DEVUCEID_SPIKE_LARGE_MOTOR            49
#define DEVICEID_SPIKE_COLOR_SENSOR           61
#define DEVICEID_SPIKE_ULTRASONIC_SENSOR      62
#define DEVICEID_SPIKE_PRIME_FORCE_SENSOR     63
#define DEVICEID_TECHNIC_COLOR_LIGHT_MATRIX   64
#define DEVICEID_SPIKE_SMALL_MOTOR            65
#define DEVICEID_TECHNIC_MEDIUM_ANGULAR_MOTOR 75
#define DEVICEID_TECHNIC_LARGE_ANGULAR_MOTOR  76

class LegoDevice {
public:
	LegoDevice(SerialIO *serialIO, uint8_t deviceIndex = 255);
	~LegoDevice();

	void markAsHandshakeComplete();
	void parseIncomingData();
	void setDeviceIdAndName(int deviceId, std::string &name);
	void initNumberOfModes(int numModes);
	void setSerialSpeed(long serialSpeed);
	void setVersions(std::string &fwVersion, std::string &hwVersion);
	Mode *getMode(int index);
	int getSelectedModeIndex();
	void finishHandshake();
	void sendAck();
	void sendNack();
	void sendSync();
	bool isHandshakeComplete();
	void needsKeepAlive();
	void selectMode(int modeIndex);
	void selectSpeed(long speed);
	bool fullyInitialized();
	bool isInDataMode();
	void switchToDataMode();

	void setMotorSpeed(int speed);
	void setPWMController(MotorPWMController* controller);

	void setPinMode(int pin, int mode);
	int digitalRead(int pin);
	void digitalWrite(int pin, int value);

	void initialize();
	void loop();

	std::string name();
	int numModes();

	void reset();

	int getDeviceId();
	SerialIO* getSerialIO();

	// Called by LumpParser for every validated DATA frame.
	// mode = resolved mode index (0-15, after applying extModeOffset).
	// Default implementation looks up Mode* and calls mode->processDataPacket().
	virtual void onDataFrame(int mode, const uint8_t *payload, int payloadSize);

	// Called by LumpParser for combi-mode DATA bursts.
	// Default: delegates to onDataFrame().
	virtual void onCombiDataFrame(int mode, const uint8_t *payload, int payloadSize);

	// Called by LumpParser immediately after a validated DATA frame is dispatched.
	// Switches the no-data watchdog from the 2 s startup grace to the tight 500 ms
	// running timeout.  NACK keep-alive is handled by the 50 ms timer in loop().
	void onDataFrameDispatched();

private:
	int getDefaultMode();
	void logParserStats();

	long serialSpeed_;
	std::array<Mode, 16> modes_;
	int numModes_;
	std::string name_;
	int deviceId_;
	std::string fwVersion_;
	std::string hwVersion_;
	LumpParser parser_;
	std::unique_ptr<SerialIO> serialIO_;
	bool handshakeComplete_;
	unsigned long lastKeepAliveCheck_;
	bool inDataMode_;
	bool firstDataFrameReceived_;   // false until first DATA frame arrives after mode entry
	unsigned long lastReceivedDataInMillis_;
	int selectedMode_;
	unsigned long lastParserStatsLog_;
	uint8_t deviceIndex_;  // Device slot index (0-3) for PWM controller, 255 if unassigned
	MotorPWMController* pwmController_;  // Injected PWM controller instance

	// Protocol constants needed for outgoing messages
	// (retained here since protocolstate.h is removed)
	static const int lumpMsgTypeCmd  = 0x40;
	static const int lumpMsgSize1    = 0 << 3;
	static const int lumpMsgSize4    = 2 << 3;
	static const int lumpCmdSpeed    = 0x2;
	static const int lumpCmdSelect   = 0x3;
	static const int lumpCmdExtMode  = 0x06;
	static const int lumpSysSync     = 0x0;
	static const int lumpSysNack     = 0x2;
	static const int lumpSysAck      = 0x4;
	static const int lumpExtMode0    = 0x00;
	static const int lumpExtMode8    = 0x08;
};

#endif // LEGODEVICE_H
