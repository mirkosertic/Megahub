#include "legodevice.h"

#include "i2csync.h"
#include "logging.h"
#include "motorpwmcontroller.h"

LegoDevice::LegoDevice(SerialIO *serialIO, uint8_t deviceIndex)
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
	, firstDataFrameReceived_(false)
	, lastReceivedDataInMillis_(0)
	, selectedMode_(-1)
	, lastParserStatsLog_(0)
	, deviceIndex_(deviceIndex)
	, pwmController_(nullptr) {

}

void LegoDevice::reset() {
	INFO("Performing a device reset");

	// Stop motor and unregister from PWM controller during reset
	if (deviceIndex_ != 255 && pwmController_ != nullptr) {
		pwmController_->setSpeed(deviceIndex_, 0);
		pwmController_->disableDevice(deviceIndex_);
	}

	numModes_ = 0;
	deviceId_ = -1;
	fwVersion_ = "";
	hwVersion_ = "";
	handshakeComplete_ = false;
	inDataMode_ = false;
	firstDataFrameReceived_ = false;

	parser_.reset();

	for (int i = 0; i < 16; i++) {
		modes_[i].reset();
	}

	serialSpeed_ = 2400;
	selectedMode_ = -1;
	serialIO_->switchToBaudrate(2400);

	// Re-register with PWM controller after reset
	if (deviceIndex_ != 255 && pwmController_ != nullptr) {
		pwmController_->enableDevice(deviceIndex_, this);
	}
}

LegoDevice::~LegoDevice() {
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
	// Poll diagnostic counters (e.g. FIFO overrun) once per batch, not per byte.
	serialIO_->pollDiagnostics();
	// Read a maximum of 64 bytes per call to bound latency.
	// Increased from 32 to 64 to empty FIFO faster and reduce overrun probability
	// when motor noise generates spurious bytes.
	while (serialIO_->available() > 0 && count++ < 64) {
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
	return &modes_[index];
}

void LegoDevice::finishHandshake() {
	INFO("Finishing handshake and switching to %ld baud", serialSpeed_);

	sendAck();
	delay(10);
	serialIO_->switchToBaudrate(serialSpeed_);
	delay(10);
	parser_.clearBuffer();  // discard stale bytes from 2400-baud phase
}

void LegoDevice::sendAck() {
	DEBUG("Sending ACK message to Lego device");
	serialIO_->sendByte(lumpSysAck);
	serialIO_->flush();
}

void LegoDevice::sendNack() {
	serialIO_->sendByte(lumpSysNack);
	// No flush needed: WriteByte() already waited for TX FIFO space; the SC16IS752
	// transmits autonomously. Blocking here would only delay RX servicing.
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
	firstDataFrameReceived_ = false;
	lastReceivedDataInMillis_ = millis();  // reset timeout clock so startup window starts from data-mode entry

	// Send an immediate NACK to kick-start streaming.
	// Without this, lastKeepAliveCheck_ stays 0, the emergency-NACK guard
	// (lastKeepAliveCheck_ != 0) never fires, no keep-alive reaches the device,
	// and the device times out before any DATA frame arrives — creating a deadlock
	// where NACKs never start because no DATA frames arrive, and DATA frames never
	// arrive because no NACK was sent.
	sendNack();
	lastKeepAliveCheck_ = millis();
}

void LegoDevice::setMotorSpeed(int speed) {
	i2c_lock();
	INFO("Setting motor speed to %d", speed);
	if (speed == 0) {
		serialIO_->setM1(false);
		serialIO_->setM2(false);
	} else if (speed > 0) {
		serialIO_->setM1(false);
		serialIO_->setM2(true);
	} else if (speed < 0) {
		serialIO_->setM1(true);
		serialIO_->setM2(false);
	}
	i2c_unlock();
	/*

	// Check if PWM controller is injected
	if (pwmController_ == nullptr) {
		WARN("PWM controller not injected - cannot control motor speed");
		return;
	}

	// Check if this device has a valid index
	if (deviceIndex_ == 255) {
		WARN("Device not registered for PWM control (deviceIndex not set)");
		return;
	}

	// Clamp speed to valid range (-127 to +127)
	int8_t clampedSpeed = speed;
	if (clampedSpeed < -127) clampedSpeed = -127;
	if (clampedSpeed > 127) clampedSpeed = 127;

	INFO("Setting motor speed to %d (device index %d)", clampedSpeed, deviceIndex_);

	// Use the PWM controller for variable speed control
	pwmController_->setSpeed(deviceIndex_, clampedSpeed);

	*/
}

void LegoDevice::initialize() {
	// Register this device with the PWM controller if it has a valid index
	if (deviceIndex_ != 255 && pwmController_ != nullptr) {
		pwmController_->enableDevice(deviceIndex_, this);
	}

	// Initialize motor to stopped state
	setMotorSpeed(0);
}

void LegoDevice::setPWMController(MotorPWMController* controller) {
	pwmController_ = controller;
	INFO("PWM controller injected for device index %d", deviceIndex_);
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
	INFO("Parser: ok=%lu csErr=%lu discarded=%lu recoveries=%lu overflow=%lu unknownSys=%lu invalidSize=%lu uartOvr=%lu",
	     s.framesOk, s.checksumErrors, s.bytesDiscarded,
	     s.syncRecoveries, s.bufferOverflows,
	     s.unknownSysBytes, s.invalidSizeBytes,
	     serialIO_->uartOverrunCount());
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
		unsigned long now = millis();
		// Keep-alive: send NACK every 50 ms, but only when we are at least 4 ms past
		// the last received byte.  parseIncomingData() sets lastReceivedDataInMillis_
		// each time it reads bytes from the FIFO; when the FIFO was empty the value
		// is stale and (now - last) grows until the gap condition is met.  At 100 Hz
		// the inter-frame gap is ~9.5 ms, so a 4 ms threshold places the NACK near
		// the middle of the gap — far from both the just-finished and the upcoming
		// DATA frame.  Using available()==0 instead would be vacuous: parseIncomingData
		// always empties the FIFO just before this check.
		// Safety valve: force-send at 80 ms even if data is still flowing, to stay
		// within the device's ~100 ms keep-alive window.
		if (now - lastKeepAliveCheck_ >= 50) {
			bool inGap    = (now - lastReceivedDataInMillis_ >= 4);
			bool mustSend = (now - lastKeepAliveCheck_       >= 80);
			if (inGap || mustSend) {
				sendNack();
				lastKeepAliveCheck_ = now;
			}
		}

		// Two-tier timeout: generous window until first DATA frame, tight thereafter.
		// Devices can take 200–600 ms to start streaming after mode selection, especially
		// after a troubled handshake. Once streaming, 500 ms covers normal inter-frame gaps.
		unsigned long noDataTimeout = firstDataFrameReceived_ ? 500UL : 2000UL;
		if (now - lastReceivedDataInMillis_ > noDataTimeout) {
			// No data within timeout — assume device was unplugged
			WARN("Didn't receive data for some time, performing a device reset");
			reset();
			return;
		}

		// Log parser statistics every 5 seconds in data mode
		if (now - lastParserStatsLog_ >= 5000) {
			lastParserStatsLog_ = now;
			logParserStats();
		}

		// Monitor and report UART FIFO overruns to detect hardware issues.
		// Overruns indicate the ESP32 is not reading the SC16IS752 FIFO fast enough,
		// typically caused by motor-induced electrical noise generating spurious bytes.
		static uint32_t lastLoggedOverrunCount = 0;
		uint32_t currentOverruns = serialIO_->uartOverrunCount();
		if (currentOverruns != lastLoggedOverrunCount) {
			DEBUG("UART FIFO overruns detected: total=%lu (delta=+%lu since last check)",
			     currentOverruns, currentOverruns - lastLoggedOverrunCount);
			lastLoggedOverrunCount = currentOverruns;
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

SerialIO* LegoDevice::getSerialIO() {
	return serialIO_.get();
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

void LegoDevice::onDataFrameDispatched() {
	// Switches the no-data watchdog from the 2 s startup grace to the tight 500 ms
	// running timeout once the first DATA frame arrives.  NACK keep-alive is handled
	// entirely by the 50 ms timer in loop() — post-frame NACK would collide with
	// buffered frames still pending in the SC16IS752 FIFO.
	firstDataFrameReceived_ = true;
}
