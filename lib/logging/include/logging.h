#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class LoggingOutput {
public:
	virtual ~LoggingOutput();
	virtual void log(const char *msg, va_list args) = 0;
};

class NoopLoggingOutput : public LoggingOutput {
public:
	virtual ~NoopLoggingOutput() override;
	virtual void log(const char *msg, va_list args) override;
};

class SerialLoggingOutput : public LoggingOutput {
private:
	Print *serial_;
	QueueHandle_t logQueue_;

public:
	SerialLoggingOutput(Print *serial);
	virtual ~SerialLoggingOutput() override;

	virtual void log(const char *msg, va_list args) override;
	String waitForLogMessage(TickType_t ticksToWait);
};

class Logging {
private:
	LoggingOutput *output_;

	Logging(LoggingOutput *output);

public:
	static Logging *instance();

	void genericLog(const char *msg, ...);

	void routeLoggingTo(LoggingOutput *output);
};

#define LOGGING_ENABLED

#ifdef LOGGING_ENABLED
	#define INFO(msg, ...) Logging::instance()->genericLog("[INFO] %lu %d %s#%d:%s() - " msg, millis(), xPortGetCoreID(), __FILE__, __LINE__, __func__, ##__VA_ARGS__);
	#define WARN(msg, ...) Logging::instance()->genericLog("[WARN] %lu %d %s#%d:%s() - " msg, millis(), xPortGetCoreID(), __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#endif

#ifdef DEBUG_LOGGING_ENABLED
	#define DEBUG(msg, ...) Serial.printf("[DEBUG] %lu %d %s#%d:%s() - " msg "\n", millis(), xPortGetCoreID(), __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#endif

#ifndef LOGGING_ENABLED
	#define INFO(msg, ...)
	#define WARN(msg, ...)
#endif

#ifndef DEBUG_LOGGING_ENABLED
	#define DEBUG(msg, ...)
#endif

#endif
