#ifndef I2CSYNC_H
#define I2CSYNC_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t i2c_global_mutex;

#define i2c_lock()    xSemaphoreTake(i2c_global_mutex, portMAX_DELAY)
#define i2c_unlock()  xSemaphoreGive(i2c_global_mutex)

#endif // I2CSYNC_H