#include <Arduino.h>

#include "statusmonitor.h"

#include "logging.h"

Statusmonitor *GLOBAL_STATUS_INSTANCE = nullptr;

#define STATUS_LED_PIN 2

void statusMonitorTask(void *parameter) {
	Statusmonitor *monitor = (Statusmonitor *) parameter;
	pinMode(STATUS_LED_PIN, OUTPUT);
	while (true) {

		static unsigned long lastHeapLog = 0;
		unsigned long currentMillis = millis();

		switch (monitor->status()) {
			case BOOTING:
				digitalWrite(STATUS_LED_PIN, LOW);
				vTaskDelay(pdMS_TO_TICKS(50));
				break;
			case SD_ERROR:
				digitalWrite(STATUS_LED_PIN, HIGH);
				vTaskDelay(pdMS_TO_TICKS(50));
				digitalWrite(STATUS_LED_PIN, LOW);
				vTaskDelay(pdMS_TO_TICKS(50));
				break;
			case CONFIGURATION_ACTIVE:
				digitalWrite(STATUS_LED_PIN, HIGH);
				vTaskDelay(pdMS_TO_TICKS(100));
				digitalWrite(STATUS_LED_PIN, LOW);
				vTaskDelay(pdMS_TO_TICKS(100));
				break;
			case IDLE:
				digitalWrite(STATUS_LED_PIN, HIGH);
				vTaskDelay(pdMS_TO_TICKS(500));
				digitalWrite(STATUS_LED_PIN, LOW);
				vTaskDelay(pdMS_TO_TICKS(500));
				break;
		}
	}
}

Statusmonitor::Statusmonitor() {
	status_ = BOOTING;

	xTaskCreate(
		statusMonitorTask,
		"StatusMonitor",
		3000,
		(void *) this,
		1,
		&taskHandle_ // Store task handle for cancellation
	);
}

Statusmonitor *Statusmonitor::instance() {
	if (GLOBAL_STATUS_INSTANCE == nullptr) {
		GLOBAL_STATUS_INSTANCE = new Statusmonitor();
	}
	return GLOBAL_STATUS_INSTANCE;
}

void Statusmonitor::setStatus(StatusmonitorStatus status) {
	status_ = status;
}

StatusmonitorStatus Statusmonitor::status() {
	return status_;
}
