#include "portstatus.h"

#include "logging.h"

#define STATUS_MESSAGE_SIZE 6144
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
	if (command.length() == 0) {
		WARN("Portstatus: empty message, dropping");
		return;
	}
	static char buf[STATUS_MESSAGE_SIZE];
	size_t len = command.length();
	if (len >= STATUS_MESSAGE_SIZE) {
		len = STATUS_MESSAGE_SIZE - 1;
	}
	memcpy(buf, command.c_str(), len);
	memset(buf + len, 0, STATUS_MESSAGE_SIZE - len);
	if (xQueueSend(statusQueue_, buf, 0) != pdTRUE) {
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
