#ifndef TESTLOGGING_H
#define TESTLOGGING_H

#include "logging.h"

#include <unity.h>

class UnityLoggingOutput : public LoggingOutput {
public:
	virtual ~UnityLoggingOutput() {
	}

	virtual void log(const char *msg, va_list args) override {
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), msg, args);

		TEST_MESSAGE(buffer);
	}
};

#endif // TESTLOGGING_H
