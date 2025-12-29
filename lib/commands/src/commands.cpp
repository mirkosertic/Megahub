#include "commands.h"

#define COMMAND_MESSAGE_SIZE 512
#define COMMAND_QUEUE_LENGTH 50

Commands *GLOBAL_COMMANDS_INSTANCE = nullptr;

Commands::Commands() {
	commandQueue_ = xQueueCreate(COMMAND_QUEUE_LENGTH, COMMAND_MESSAGE_SIZE);
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
	if (GLOBAL_COMMANDS_INSTANCE == nullptr) {
		GLOBAL_COMMANDS_INSTANCE = new Commands();
	}
	return GLOBAL_COMMANDS_INSTANCE;
}
