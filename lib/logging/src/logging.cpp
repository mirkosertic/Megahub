#include "logging.h"

#define LOGGING_MESSAGE_SIZE 512
#define LOGGING_QUEUE_LENGTH 10

LoggingOutput::~LoggingOutput() {
}

NoopLoggingOutput::~NoopLoggingOutput() {
}

void NoopLoggingOutput::log(const char *msg, va_list args) {
}

SerialLoggingOutput::SerialLoggingOutput(Print *serial) {
	serial_ = serial;
	logQueue_ = xQueueCreate(LOGGING_QUEUE_LENGTH, LOGGING_MESSAGE_SIZE);
}

SerialLoggingOutput::~SerialLoggingOutput() {
}

void SerialLoggingOutput::log(const char *msg, va_list args) {
	char buffer[LOGGING_MESSAGE_SIZE];
	vsnprintf(buffer, sizeof(buffer), msg, args);

	serial_->println(buffer);

	if (xQueueSend(logQueue_, buffer, 0) != pdTRUE) {
		// Queue full, message dropped
	}
}

String SerialLoggingOutput::waitForLogMessage(TickType_t ticksToWait) {
	char buffer[LOGGING_MESSAGE_SIZE];
	if (xQueueReceive(logQueue_, buffer, ticksToWait) == pdTRUE) {
		return String(buffer);
	}
	// Nothing logged in the given time
	return String();
}

Logging *Logging::instance() {
	static Logging instance(new NoopLoggingOutput());
	return &instance;
}

Logging::Logging(LoggingOutput *output) {
	output_ = output;
}

void Logging::genericLog(const char *msg, ...) {
	va_list arg;
	va_start(arg, msg);
	output_->log(msg, arg);
	va_end(arg);
}

void Logging::routeLoggingTo(LoggingOutput *output) {
	if (output_ != nullptr) {
		delete this->output_;
	}
	output_ = output;
}
