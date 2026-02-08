#include "legodevice.h"

#include "i2csync.h"
#include "logging.h"
#include "waitingstate.h"

LegoDevice::LegoDevice(SerialIO *serialIO)
	: serialSpeed_(2400)
	, numModes_(-1)
	, deviceId_(-1)
	, fwVersion_("")
	, hwVersion_("")
	, serialIO_(serialIO)
	, handshakeComplete_(false)
	, lastKeepAliveCheck_(0)
	, inDataMode_(false)
	, lastReceivedDataInMillis_(0)
	, selectedMode_(-1) {
	protocolState_ = new WaitingState(this);

	modes_ = new Mode *[16];
	for (int i = 0; i < 16; i++) {
		modes_[i] = new Mode();
	}
}

void LegoDevice::reset() {
	INFO("Performing a device reset");
	numModes_ = 0;
	deviceId_ = -1;
	fwVersion_ = "";
	hwVersion_ = "";
	handshakeComplete_ = false;
	inDataMode_ = false;

	delete protocolState_;
	protocolState_ = new WaitingState(this);

	for (int i = 0; i < 16; i++) {
		modes_[i]->reset();
	}

	serialSpeed_ = 2400;
	selectedMode_ = -1;
	serialIO_->switchToBaudrate(2400);
}

LegoDevice::~LegoDevice() {
	if (modes_ != nullptr) {
		for (int i = 0; i < numModes_; i++) {
			delete modes_[i];
		}
		delete[] modes_;
	}
	if (protocolState_ != nullptr) {
		delete protocolState_;
	}
}

bool LegoDevice::fullyInitialized() {
	return deviceId_ != -1 && numModes_ != -1;
}

bool LegoDevice::isHandshakeComplete() {
	return handshakeComplete_;
}

void LegoDevice::markAsHandshakeComplete() {
	handshakeComplete_ = true;
}

void LegoDevice::parseIncomingData() {
	while (serialIO_->available() > 0) {
		lastReceivedDataInMillis_ = millis();
		int datapoint = serialIO_->readByte();
		ProtocolState *newState = protocolState_->parse(datapoint);
		if (newState != protocolState_) {
			delete protocolState_;
			protocolState_ = newState;
		}
	}
}

void LegoDevice::setDeviceIdAndName(int deviceId, std::string &name) {
	this->deviceId_ = deviceId;
	this->name_ = name;
	INFO("Connected to device ID %d with name '%s'", deviceId, name.c_str());
}

void LegoDevice::initNumberOfModes(int numModes) {
	this->numModes_ = numModes;
	INFO("Device has %d modes", numModes);
}

void LegoDevice::setSerialSpeed(long serialSpeed) {
	this->serialSpeed_ = serialSpeed;
	INFO("Device requests serial speed %ld", serialSpeed);
}

void LegoDevice::setVersions(std::string &fwVersion, std::string &hwVersion) {
	this->fwVersion_ = fwVersion;
	this->hwVersion_ = hwVersion;
	INFO("Device firmware version: %s", fwVersion.c_str());
	INFO("Device hardware version: %s", hwVersion.c_str());
}

Mode *LegoDevice::getMode(int index) {
	if (index == -1) {
		return nullptr;
	}
	return modes_[index];
}

void LegoDevice::finishHandshake() {
	INFO("Finishing handshake and switching to %ld baud", serialSpeed_);

	sendAck();
	delay(10);
	serialIO_->switchToBaudrate(serialSpeed_);
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
	if (lastKeepAliveCheck_ == 0) {
		lastKeepAliveCheck_ = now;
		return;
	}

	if (now - lastKeepAliveCheck_ > 50) {
		lastKeepAliveCheck_ = now;
		DEBUG("Sending keep alive message to Lego device");
		sendNack();
	}
}

void LegoDevice::selectMode(int modeIndex) {
	INFO("Selecting mode %d", modeIndex);
	// TODO: Check if this is a powered up device, and if this code is correct as it is!
	if (false) {
		if (modeIndex >= 8) {
			INFO("Using Ext-Mode 8");
			int command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_EXT_MODE | ProtocolState::LUMP_MSG_SIZE_1;
			int ext = ProtocolState::LUMP_EXT_MODE_8;
			int checksum = 0xff ^ command ^ ext;
			serialIO_->sendByte(command);
			serialIO_->sendByte(ext);
			serialIO_->sendByte(checksum);
			serialIO_->flush();

			int idxToSend = modeIndex -= 8;
			command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_SELECT | ProtocolState::LUMP_MSG_SIZE_1;
			checksum = 0xff ^ command ^ idxToSend;
			// int checksum = 0xff ^ idxToSend;
			serialIO_->sendByte(command);
			serialIO_->sendByte(idxToSend);
			serialIO_->sendByte(checksum);
			serialIO_->flush();
			
			INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, idxToSend, checksum);

		} else {
			INFO("Using Ext-Mode 0");
			int command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_EXT_MODE | ProtocolState::LUMP_MSG_SIZE_1;
			int ext = ProtocolState::LUMP_EXT_MODE_0;
			int checksum = 0xff ^ command ^ ext;
			serialIO_->sendByte(command);
			serialIO_->sendByte(ext);
			serialIO_->sendByte(checksum);

			command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_SELECT | ProtocolState::LUMP_MSG_SIZE_1;
			checksum = 0xff ^ command ^ modeIndex;
			// int checksum = 0xff ^ modeIndex;
			serialIO_->sendByte(command);
			serialIO_->sendByte(modeIndex);
			serialIO_->sendByte(checksum);
			serialIO_->flush();
			
			INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, modeIndex, checksum);
		}
	} else {
		INFO("Using standard CMD_SELECT for non powered up devices");
		int command = ProtocolState::LUMP_MSG_TYPE_CMD | ProtocolState::LUMP_CMD_SELECT | ProtocolState::LUMP_MSG_SIZE_1;
		int checksum = 0xff ^ command ^ modeIndex;
		// int checksum = 0xff ^ modeIndex;
		serialIO_->sendByte(command);
		serialIO_->sendByte(modeIndex);
		serialIO_->sendByte(checksum);
		serialIO_->flush();
		INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, modeIndex, checksum);
	}

	selectedMode_ = modeIndex;
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
	return inDataMode_;
}
void LegoDevice::switchToDataMode() {
	inDataMode_ = true;
}

void LegoDevice::setMotorSpeed(int speed) {
	i2c_lock();
	if (speed == 0) {
		serialIO_->setM1(false);
		serialIO_->setM2(false);
	} else if (speed > 0) {
		serialIO_->setM1(true);
		serialIO_->setM2(false);
	} else if (speed < 0) {
		serialIO_->setM1(false);
		serialIO_->setM2(true);
	}
	i2c_unlock();
}

void LegoDevice::initialize() {
	setMotorSpeed(0);
}

void LegoDevice::loop() {
	parseIncomingData();

	if (isHandshakeComplete() && !isInDataMode()) {
		finishHandshake();
		// TODO: Set the default mode based on the detected device id!
		selectMode(0);
		switchToDataMode();
	} else if (isInDataMode()) {
		needsKeepAlive();

		unsigned long now = millis();
		if (now - lastReceivedDataInMillis_ > 200) {
			// We didn't receive data for more than 200 milliseconds, so we will perform a reset as we assume the device was unplugged
			INFO("Did't receive data for some time, performing a device reset");
			reset();
		}
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

std::string LegoDevice::name() {
	return name_;
}

int LegoDevice::numModes() {
	return numModes_;
}

int LegoDevice::getSelectedModeIndex() {
	return selectedMode_;
}

int LegoDevice::getDeviceId() {
	return deviceId_;
}
