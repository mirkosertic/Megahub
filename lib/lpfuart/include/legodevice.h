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

	int digitalRead(int pin);
	void digitalWrite(int pin, int value);

	void initialize();
	void loop();

private:
	long serialSpeed;
	Mode **modes;
	int numModes;
	std::string name;
	int deviceId;
	std::string fwVersion;
	std::string hwVersion;
	ProtocolState *protocolState;
	std::unique_ptr<SerialIO> serialIO_;
	bool handshakeComplete;
	unsigned long lastKeepAliveCheck;
	bool inDataMode;
};

#endif // LEGODEVICE_H
