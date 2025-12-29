#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class Commands {
private:
	QueueHandle_t commandQueue_;
	Commands();

public:
	static Commands *instance();

	String waitForCommand(TickType_t ticksToWait);
	void queue(String message);
};

#endif // COMMANDS_H
