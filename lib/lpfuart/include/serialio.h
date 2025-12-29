#ifndef SERIALIO_H
#define SERIALIO_H

#include <Arduino.h>

class SerialIO {
public:
	virtual ~SerialIO() { }

	virtual int available() = 0;
	virtual int readByte() = 0;
	virtual void sendByte(int byteData) = 0;
	virtual void switchToBaudrate(long serialSpeed) = 0;
	virtual void flush() = 0;

	virtual void setM1(bool status) = 0;
	virtual void setM2(bool status) = 0;

	virtual int digitalRead(int pin) = 0;
	virtual void digitalWrite(int pin, int value) = 0;
};

#endif // SERIALIO_H
