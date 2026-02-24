#include "legodevice.h"

#include "i2csync.h"
#include "logging.h"

LegoDevice::LegoDevice(SerialIO *serialIO)
	: serialSpeed_(2400)
	, numModes_(-1)
	, deviceId_(-1)
	, fwVersion_("")
	, hwVersion_("")
	, parser_(this)
	, serialIO_(serialIO)
	, handshakeComplete_(false)
	, lastKeepAliveCheck_(0)
	, inDataMode_(false)
	, lastReceivedDataInMillis_(0)
	, selectedMode_(-1)
	, lastParserStatsLog_(0) {

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

	parser_.reset();

	for (int i = 0; i < 16; i++) {
		modes_[i]->reset();
	}

	serialSpeed_ = 2400;
	selectedMode_ = -1;
	serialIO_->switchToBaudrate(2400);
}

LegoDevice::~LegoDevice() {
	if (modes_ != nullptr) {
		for (int i = 0; i < 16; i++) {
			delete modes_[i];
		}
		delete[] modes_;
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
	int count = 0;
	// Read a maximum of 32 bytes per call to bound latency
	while (serialIO_->available() > 0 && count++ < 32) {
		lastReceivedDataInMillis_ = millis();
		parser_.feedByte((uint8_t)serialIO_->readByte());
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
	if (index < 0 || index >= 16) {
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
	DEBUG("Sending ACK message to Lego device");
	serialIO_->sendByte(lumpSysAck);
	serialIO_->flush();
}

void LegoDevice::sendNack() {
	serialIO_->sendByte(lumpSysNack);
	serialIO_->flush();
}

void LegoDevice::sendSync() {
	serialIO_->sendByte(lumpSysSync);
}

void LegoDevice::needsKeepAlive() {
	unsigned long now = millis();
	if (lastKeepAliveCheck_ == 0) {
		lastKeepAliveCheck_ = now;
		return;
	}

	unsigned long elapsed = now - lastKeepAliveCheck_;
	if (elapsed > 50) {
		lastKeepAliveCheck_ = now;
		DEBUG("Sending keep alive message to Lego device");
		sendNack();
	} else {
		DEBUG("No need to send keep alive, delay %lu", elapsed);
	}
}

void LegoDevice::selectMode(int modeIndex) {
	INFO("Selecting mode %d", modeIndex);
	// TODO: Check if this is a powered up device, and if this code is correct as it is!
	if (false) {
		if (modeIndex >= 8) {
			INFO("Using Ext-Mode 8");
			int command = lumpMsgTypeCmd | lumpCmdExtMode | lumpMsgSize1;
			int ext = lumpExtMode8;
			int checksum = 0xff ^ command ^ ext;
			serialIO_->sendByte(command);
			serialIO_->sendByte(ext);
			serialIO_->sendByte(checksum);
			serialIO_->flush();

			int idxToSend = modeIndex -= 8;
			command = lumpMsgTypeCmd | lumpCmdSelect | lumpMsgSize1;
			checksum = 0xff ^ command ^ idxToSend;
			serialIO_->sendByte(command);
			serialIO_->sendByte(idxToSend);
			serialIO_->sendByte(checksum);
			serialIO_->flush();

			INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, idxToSend, checksum);

		} else {
			INFO("Using Ext-Mode 0");
			int command = lumpMsgTypeCmd | lumpCmdExtMode | lumpMsgSize1;
			int ext = lumpExtMode0;
			int checksum = 0xff ^ command ^ ext;
			serialIO_->sendByte(command);
			serialIO_->sendByte(ext);
			serialIO_->sendByte(checksum);

			command = lumpMsgTypeCmd | lumpCmdSelect | lumpMsgSize1;
			checksum = 0xff ^ command ^ modeIndex;
			serialIO_->sendByte(command);
			serialIO_->sendByte(modeIndex);
			serialIO_->sendByte(checksum);
			serialIO_->flush();

			INFO("Sending select mode command: 0x%02X 0x%02X 0x%02X", command, modeIndex, checksum);
		}
	} else {
		INFO("Using standard CMD_SELECT for non powered up devices");
		int command = lumpMsgTypeCmd | lumpCmdSelect | lumpMsgSize1;
		int checksum = 0xff ^ command ^ modeIndex;
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
	int command = lumpMsgTypeCmd | lumpCmdSpeed | lumpMsgSize4;
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
	INFO("Setting motor speed to %d", speed);
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

int LegoDevice::getDefaultMode() {
	switch (deviceId_) {
		case DEVICEID_BOOST_COLOR_DISTANCE_SENSOR:
			return 8; // Mode SPEC_1, returns multiple values
		case DEVICEID_BOOST_INTERACTIVE_MOTOR:
			return 2; // Mode POS, absolute rotation angle since startup
		default:
			break;
	}
	return 0;
}

void LegoDevice::logParserStats() {
	const LumpParserStats &s = parser_.stats();
	INFO("Parser: ok=%lu csErr=%lu discarded=%lu recoveries=%lu overflow=%lu unknownSys=%lu invalidSize=%lu",
	     s.framesOk, s.checksumErrors, s.bytesDiscarded,
	     s.syncRecoveries, s.bufferOverflows,
	     s.unknownSysBytes, s.invalidSizeBytes);
}

void LegoDevice::loop() {
	parseIncomingData();

	if (isHandshakeComplete() && !isInDataMode()) {
		finishHandshake();
		int defaultMode = getDefaultMode();
		INFO("Setting default mode %d for deviceId 0x%04x", defaultMode, deviceId_);
		selectMode(getDefaultMode());
		switchToDataMode();
	} else if (isInDataMode()) {
		needsKeepAlive();

		unsigned long now = millis();
		if (now - lastReceivedDataInMillis_ > 200) {
			// No data for more than 200 ms — assume device was unplugged
			WARN("Didn't receive data for some time, performing a device reset");
			reset();
			return;
		}

		// Log parser statistics every 5 seconds in data mode
		if (now - lastParserStatsLog_ >= 5000) {
			lastParserStatsLog_ = now;
			logParserStats();
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

void LegoDevice::onDataFrame(int mode, const uint8_t *payload, int payloadSize) {
	Mode *m = getMode(mode);
	if (m == nullptr) {
		WARN("onDataFrame: invalid mode %d", mode);
		return;
	}
	m->processDataPacket(payload, payloadSize);
}

void LegoDevice::onCombiDataFrame(int mode, const uint8_t *payload, int payloadSize) {
	onDataFrame(mode, payload, payloadSize);
}
