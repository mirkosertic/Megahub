#include "portstatus.h"

#include "logging.h"

#define STATUS_MESSAGE_SIZE 4096
#define STATUS_QUEUE_LENGTH 2

Portstatus::Portstatus() {
	statusQueue_ = xQueueCreate(STATUS_QUEUE_LENGTH, STATUS_MESSAGE_SIZE);
	if (!statusQueue_) {
		ERROR("Failed to initialize queue!");
		while (true)
			;
	}
}

void Portstatus::queue(String command) {
	const char *buffer = command.c_str();

	if (buffer == nullptr) {
		WARN("Portstatus is NULL!");
		return;
	}

	if (xQueueSend(statusQueue_, buffer, 0) != pdTRUE) {
		// Queue full, message dropped
	}
}

String Portstatus::waitForStatus(TickType_t ticksToWait) {
	char buffer[STATUS_MESSAGE_SIZE];
	if (xQueueReceive(statusQueue_, buffer, ticksToWait) == pdTRUE) {
		return String(buffer);
	}
	// Nothing logged in the given time
	return String();
}

Portstatus *Portstatus::instance() {
	static Portstatus instance;
	return &instance;
}
