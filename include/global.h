#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t xBinarySemaphoreMic;
extern QueueHandle_t featureQueue;
extern QueueHandle_t aiQueue;

#endif
