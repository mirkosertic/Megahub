#ifndef H_SC16IS752_SERIALADAPTER_H
#define H_SC16IS752_SERIALADAPTER_H

#include <Arduino.h>

#include "SC16IS752.h"
#include "serialio.h"

enum SC16IS752SerialAdapterChannel {
	CHANNEL_A,
	CHANNEL_B
};

class SC16IS752SerialAdapter : public SerialIO {
public:
	SC16IS752SerialAdapter(SC16IS752 *hardwareserial, SC16IS752SerialAdapterChannel port, int m1pin, int m2pin);
	virtual ~SC16IS752SerialAdapter();

	virtual int available();
	virtual int readByte();
	virtual void sendByte(int byteData);
	virtual void switchToBaudrate(long serialSpeed);
	virtual void flush();

	virtual void setM1(bool status);
	virtual void setM2(bool status);

	virtual void setPinMode(int pin, int mode);
	virtual int digitalRead(int pin);
	virtual void digitalWrite(int pin, int value);

private:
	SC16IS752 *hardwareserial_;
	SC16IS752SerialAdapterChannel channel_;
	int m1pin_;
	int m2pin_;
};

#endif
