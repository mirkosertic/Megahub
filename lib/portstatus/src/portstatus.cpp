#include "portstatus.h"

#define STATUS_MESSAGE_SIZE 1024
#define STATUS_QUEUE_LENGTH 2

Portstatus *GLOBAL_PORTSTATUS_INSTANCE = nullptr;

Portstatus::Portstatus() {
	statusQueue_ = xQueueCreate(STATUS_QUEUE_LENGTH, STATUS_MESSAGE_SIZE);
}

void Portstatus::queue(String command) {
	const char *buffer = command.c_str();

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
	if (GLOBAL_PORTSTATUS_INSTANCE == nullptr) {
		GLOBAL_PORTSTATUS_INSTANCE = new Portstatus();
	}
	return GLOBAL_PORTSTATUS_INSTANCE;
}
