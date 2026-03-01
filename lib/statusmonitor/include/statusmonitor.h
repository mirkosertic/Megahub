#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <Arduino.h>

#include <atomic>

enum StatusmonitorStatus {
	BOOTING,
	SD_ERROR,
	CONFIGURATION_ACTIVE,
	IDLE
};

class Statusmonitor {
  private:
	Statusmonitor();
	std::atomic<StatusmonitorStatus> status_;
	TaskHandle_t taskHandle_;

  public:
	static Statusmonitor* instance();
	~Statusmonitor();

	void setStatus(StatusmonitorStatus status);
	StatusmonitorStatus status();
};

#endif
