#ifndef PORTSTATUS_H
#define PORTSTATUS_H

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class Portstatus {
private:
	QueueHandle_t statusQueue_;
	Portstatus();

public:
	static Portstatus *instance();

	String waitForStatus(TickType_t ticksToWait);
	void queue(String message);
};

#endif // PORTSTATUS_H
