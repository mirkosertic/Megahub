#include "commands.h"

#include "logging.h"

#define COMMAND_MESSAGE_SIZE 512
#define COMMAND_QUEUE_LENGTH 10

Commands::Commands() {
	commandQueue_ = xQueueCreate(COMMAND_QUEUE_LENGTH, COMMAND_MESSAGE_SIZE);
	if (!commandQueue_) {
		ERROR("Failed to initialize queue!");
		while(true);
	}
}

void Commands::queue(String command) {
	const char *buffer = command.c_str();

	if (xQueueSend(commandQueue_, buffer, 0) != pdTRUE) {
		// Queue full, message dropped
	}
}

String Commands::waitForCommand(TickType_t ticksToWait) {
	char buffer[COMMAND_MESSAGE_SIZE];
	if (xQueueReceive(commandQueue_, buffer, ticksToWait) == pdTRUE) {
		return String(buffer);
	}
	// Nothing logged in the given time
	return String();
}

Commands *Commands::instance() {
	static Commands instance;
	return &instance;
}
