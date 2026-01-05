#ifndef LEGODEVICE_H
#define LEGODEVICE_H

#include <Arduino.h>

#include "mode.h"
#include "protocolstate.h"
#include "serialio.h"

#include <memory>
#include <string>

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

	void setM1(bool status);
	void setM2(bool status);
	void setMotorSpeed(int speed);

	void setPinMode(int pin, int mode);
	int digitalRead(int pin);
	void digitalWrite(int pin, int value);

	void initialize();
	void loop();

	std::string name();
	int numModes();

private:
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
};

#endif // LEGODEVICE_H
