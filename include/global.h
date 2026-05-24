#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define AUDIO_BUFFER_SIZE 16000 // 16000 samples -> 1s of audio at 16kHz

extern int16_t g_audio_buffer[AUDIO_BUFFER_SIZE];

extern SemaphoreHandle_t xBinarySemaphoreMic;
extern QueueHandle_t featureQueue;
extern QueueHandle_t aiQueue;

#endif
