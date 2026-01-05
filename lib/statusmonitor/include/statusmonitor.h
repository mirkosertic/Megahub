#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <Arduino.h>

enum StatusmonitorStatus {
	BOOTING,
	SD_ERROR,
	CONFIGURATION_ACTIVE,
	IDLE
};

class Statusmonitor {
private:
	Statusmonitor();
	StatusmonitorStatus status_;
	TaskHandle_t taskHandle_;

public:
	static Statusmonitor *instance();

	void setStatus(StatusmonitorStatus status);
	StatusmonitorStatus status();
};

#endif
