#ifndef LEGODEVICE_H
#define LEGODEVICE_H

#include <Arduino.h>

#include "mode.h"
#include "protocolstate.h"
#include "serialio.h"

#include <memory>
#include <string>

#define DEVICEID_EV3_COLOR_SENSOR			  29
#define DEVICEID_EV3_ULTRASONIC_SENSOR		  30
#define DEVICEID_EV3_GYRO_SENSOR			  32
#define DEVICEID_EV3_INFRARED_SENSOR		  33
#define DEVICEID_WEDO20_TILT				  34
#define DEVICEID_WEDO20_MOTION				  35
#define DEVICEID_BOOST_COLOR_DISTANCE_SENSOR  37
#define DEVICEID_BOOST_INTERACTIVE_MOTOR	  38
#define DEVICEID_TECHNIC_MEDIUM_MOTOR		  46
#define DEVICEID_TECHNIC_XL_MOTOR			  45
#define DEVICEID_SPIKE_MEDIUM_MOTOR			  48
#define DEVUCEID_SPIKE_LARGE_MOTOR			  49
#define DEVICEID_SPIKE_COLOR_SENSOR			  61
#define DEVICEID_SPIKE_ULTRASONIC_SENSOR	  62
#define DEVICEID_SPIKE_PRIME_FORCE_SENSOR	  63
#define DEVICEID_TECHNIC_COLOR_LIGHT_MATRIX	  64
#define DEVICEID_SPIKE_SMALL_MOTOR			  65
#define DEVICEID_TECHNIC_MEDIUM_ANGULAR_MOTOR 75
#define DEVICEID_TECHNIC_LARGE_ANGULAR_MOTOR  76

class LegoDevice {
public:
	LegoDevice(SerialIO *serialIO);
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

	void setPinMode(int pin, int mode);
	int digitalRead(int pin);
	void digitalWrite(int pin, int value);

	void initialize();
	void loop();

	std::string name();
	int numModes();

	void reset();

	int getDeviceId();

private:
	int getDefaultMode();

	long serialSpeed_;
	Mode **modes_;
	int numModes_;
	std::string name_;
	int deviceId_;
	std::string fwVersion_;
	std::string hwVersion_;
	ProtocolState *protocolState_;
	std::unique_ptr<SerialIO> serialIO_;
	bool handshakeComplete_;
	unsigned long lastKeepAliveCheck_;
	bool inDataMode_;
	unsigned long lastReceivedDataInMillis_;
	int selectedMode_;
};

#endif // LEGODEVICE_H
