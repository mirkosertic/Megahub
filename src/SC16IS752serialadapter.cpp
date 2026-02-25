#include "SC16IS752serialadapter.h"

#include <Wire.h>

#include "logging.h"
#include "megahub.h"

SC16IS752SerialAdapter::SC16IS752SerialAdapter(SC16IS752 *hardwareserial, SC16IS752SerialAdapterChannel channel, int m1pin, int m2pin, uint8_t i2cAddress) {
	hardwareserial_ = hardwareserial;
	channel_ = channel;
	m1pin_ = m1pin;
	m2pin_ = m2pin;
	i2cAddress_ = i2cAddress;

	hardwareserial_->pinMode(m1pin, OUTPUT);
	hardwareserial_->pinMode(m2pin, OUTPUT);
	hardwareserial_->digitalWrite(m1pin, LOW);
	hardwareserial_->digitalWrite(m2pin, LOW);
	fifoOverrunCount_ = 0;

	INFO("Motor M1 %d + M2 %d set to low", m1pin, m2pin);
}

SC16IS752SerialAdapter::~SC16IS752SerialAdapter() {
}

void SC16IS752SerialAdapter::setM1(bool status) {
	hardwareserial_->digitalWrite(m1pin_, status ? HIGH : LOW);
}

void SC16IS752SerialAdapter::setM2(bool status) {
	hardwareserial_->digitalWrite(m2pin_, status ? HIGH : LOW);
}

int SC16IS752SerialAdapter::available() {
	return hardwareserial_->available(channel_ == CHANNEL_A ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B);
}

int SC16IS752SerialAdapter::readByte() {
	uint8_t ch = (channel_ == CHANNEL_A) ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B;
	return hardwareserial_->read(ch);
}

void SC16IS752SerialAdapter::pollDiagnostics() {
	uint8_t ch = (channel_ == CHANNEL_A) ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B;
	// Check LSR OE bit (bit 1) once per read-batch rather than per byte.
	// OE is sticky: set when any byte was overwritten in the RX FIFO since the
	// last LSR read. Checking here (called once per parseIncomingData() cycle)
	// keeps the diagnostic accurate without doubling I2C overhead per byte.
	if (readRegisterDirect(ch, SC16IS750_REG_LSR) & 0x02) {
		fifoOverrunCount_++;
	}
}

uint8_t SC16IS752SerialAdapter::readRegisterDirect(uint8_t channel, uint8_t reg_addr) {
	uint8_t result = 0;

	// Direct I2C read following the same pattern as SC16IS752::ReadRegister
	Wire.beginTransmission(i2cAddress_);
	Wire.write((reg_addr << 3 | channel << 1));
	Wire.endTransmission(0);
	Wire.requestFrom(i2cAddress_, (uint8_t)1);
	result = Wire.read();

	return result;
}

void SC16IS752SerialAdapter::sendByte(int byteData) {

	// INFO("Sending output : %s", this->formatByte(byteData).c_str());
	hardwareserial_->write(channel_ == CHANNEL_A ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B, byteData);
}

void SC16IS752SerialAdapter::switchToBaudrate(long serialSpeed) {
	hardwareserial_->flush(channel_ == CHANNEL_A ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B);
	if (channel_ == CHANNEL_A) {
		hardwareserial_->beginA(serialSpeed);
	} else {
		hardwareserial_->beginB(serialSpeed);
	}

	vTaskDelay(pdMS_TO_TICKS(10));

	// Basic verification: ensure the call didn't cause a crash and log success.
	INFO("Switched serial to %ld baud", serialSpeed);
}

void SC16IS752SerialAdapter::flush() {
	hardwareserial_->flush(channel_ == CHANNEL_A ? SC16IS752_CHANNEL_A : SC16IS752_CHANNEL_B);
}

uint32_t SC16IS752SerialAdapter::uartOverrunCount() {
	return fifoOverrunCount_;
}

int SC16IS752SerialAdapter::digitalRead(int pin) {
	return hardwareserial_->digitalRead(pin);
}

void SC16IS752SerialAdapter::digitalWrite(int pin, int value) {
	hardwareserial_->digitalWrite(pin, value);
}

void SC16IS752SerialAdapter::setPinMode(int pin, int mode) {
	switch (mode) {
		case PINMODE_INPUT:
			hardwareserial_->pinMode(pin, INPUT);
			break;
		case PINMODE_INPUT_PULLUP:
			hardwareserial_->pinMode(pin, INPUT_PULLUP);
			break;
		case PINMODE_INPUT_PULLDOWN:
			hardwareserial_->pinMode(pin, INPUT_PULLDOWN);
			break;
		case PINMODE_OUTPUT:
			hardwareserial_->pinMode(pin, OUTPUT);
			break;
		default:
			WARN("Unsupported pin mode %d for pin %d", mode, pin);
	}
}
