#include "i2csync.h"

SemaphoreHandle_t i2c_global_mutex = xSemaphoreCreateMutex();
