#include "legodevice.h"

#include "i2csync.h"
#include "logging.h"
#include "waitingstate.h"

LegoDevice::LegoDevice(SerialIO *serialIO)
	: serialSpeed(2400)
	, numModes(-1)
	, deviceId(-1)
	, fwVersion("")
	, hwVersion("")
	, serialIO_(serialIO)
	, handshakeComplete(false)
	, lastKeepAliveCheck(0)
	, inDataMode(false) {
	protocolState = new WaitingState(this);

	modes = new Mode *[16];
	for (int i = 0; i < 16; i++) {
		modes[i] = new Mode();
	}
}

LegoDevice::~LegoDevice() {
	if (modes != nullptr) {
		for (int i = 0; i < numModes; i++) {
			delete modes[i];
		}
		delete[] modes;
	}
	if (protocolState != nullptr) {
		delete protocolState;
	}
}

bool LegoDevice::fullyInitialized() {
	return deviceId != -1 && numModes != -1;
}

bool LegoDevice::isHandshakeComplete() {
	return handshakeComplete;
}

void LegoDevice::markAsHandshakeComplete() {
	handshakeComplete = true;
}

void LegoDevice::parseIncomingData() {
	while (serialIO_->available() > 0) {
		int datapoint = serialIO_->readByte();
		ProtocolState *newState = protocolState->parse(datapoint);
		if (newState != protocolState) {
			delete protocolState;
			protocolState = newState;
		}
	}
}

void LegoDevice::setDeviceIdAndName(int deviceId, std::string &name) {
	this->deviceId = deviceId;
	this->name = name;
	INFO("Connected to device ID %d with name '%s'", deviceId, name.c_str());
}

void LegoDevice::initNumberOfModes(int numModes) {
	this->numModes = numModes;
	INFO("Device has %d modes", numModes);
}

void LegoDevice::setSerialSpeed(long serialSpeed) {
	this->serialSpeed = serialSpeed;
	INFO("Device requests serial speed %ld", serialSpeed);
}

void LegoDevice::setVersions(std::string &fwVersion, std::string &hwVersion) {
	this->fwVersion = fwVersion;
	this->hwVersion = hwVersion;
	INFO("Device firmware version: %s", fwVersion.c_str());
	INFO("Device hardware version: %s", hwVersion.c_str());
}

Mode *LegoDevice::getMode(int index) {
	return modes[index];
}

void LegoDevice::finishHandshake() {
	INFO("Finishing handshake and switching to %ld baud", serialSpeed);

	sendAck();
	delay(10);
	serialIO_->switchToBaudrate(serialSpeed);
	delay(10);
}

void LegoDevice::sendAck() {
	INFO("Sending ACK message to Lego device");
	serialIO_->sendByte(ProtocolState::LUMP_SYS_ACK);
	serialIO_->flush();
}

void LegoDevice::sendNack() {
	serialIO_->sendByte(ProtocolState::LUMP_SYS_NACK);
	serialIO_->flush();
}

void LegoDevice::sendSync() {
	serialIO_->sendByte(ProtocolState::LUMP_SYS_SYNC);
}

void LegoDevice::needsKeepAlive() {
	unsigned long now = millis();
	if (lastKeepAliveCheck == 0) {
		lastKeepAliveCheck = now;
		return;
	}

	if (now - lastKeepAliveCheck > 50) {
		lastKeepAliveCheck = now;
		DEBUG("Sending keep alive message to Lego device");
		sendNack();
	}
}

void LegoDevice::selectMode(int modeIndex) {
	INFO("Selecting mode %d", modeIndex);
	int command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_SELECT | ProtocolState::LUMP_MSG_SIZE_1;
	int checksum = 0xff ^ command ^ modeIndex;
	// int checksum = 0xff ^ modeIndex;
	serialIO_->sendByte(command);
	serialIO_->sendByte(modeIndex);
	serialIO_->sendByte(checksum);
	serialIO_->flush();
	INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, modeIndex, checksum);
}

void LegoDevice::selectSpeed(long speed) {
	INFO("Selecting speed %ld", speed);
	int command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_SPEED | ProtocolState::LUMP_MSG_SIZE_4;
	int byte0 = (speed >> 0) & 0xFF;
	int byte1 = (speed >> 8) & 0xFF;
	int byte2 = (speed >> 16) & 0xFF;
	int byte3 = (speed >> 24) & 0xFF;
	int checksum = 0xff ^ command ^ byte0 ^ byte1 ^ byte2 ^ byte3;

	serialIO_->sendByte(command);
	serialIO_->sendByte(byte0);
	serialIO_->sendByte(byte1);
	serialIO_->sendByte(byte2);
	serialIO_->sendByte(byte3);
	serialIO_->sendByte(checksum);
	serialIO_->flush();
	INFO("Sending select speed command");
}

bool LegoDevice::isInDataMode() {
	return inDataMode;
}
void LegoDevice::switchToDataMode() {
	inDataMode = true;
}

void LegoDevice::setM1(bool status) {
	i2c_lock();
	serialIO_->setM1(status);
	i2c_unlock();
}

void LegoDevice::setM2(bool status) {
	i2c_lock();
	serialIO_->setM2(status);
	i2c_unlock();
}

void LegoDevice::setMotorSpeed(int speed) {
	if (speed == 0) {
		setM1(false);
		setM2(false);
	} else if (speed > 0) {
		setM1(true);
		setM2(false);
	} else if (speed < 0) {
		setM1(false);
		setM2(true);
	}
}

void LegoDevice::initialize() {
	setM1(false);
	setM2(false);
}

void LegoDevice::loop() {
	parseIncomingData();

	if (isHandshakeComplete() && !isInDataMode()) {
		finishHandshake();
		selectMode(0);
		switchToDataMode();
	} else if (isInDataMode()) {
		needsKeepAlive();
	}
}

void LegoDevice::setPinMode(int pin, int mode) {
	i2c_lock();
	serialIO_->setPinMode(pin, mode);
	i2c_unlock();
}

int LegoDevice::digitalRead(int pin) {
	i2c_lock();
	int value = serialIO_->digitalRead(pin);
	i2c_unlock();
	return value;
}
void LegoDevice::digitalWrite(int pin, int value) {
	i2c_lock();
	serialIO_->digitalWrite(pin, value);
	i2c_unlock();
}
