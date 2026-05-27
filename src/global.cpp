#include "global.h"

int16_t g_audio_buffer[AUDIO_BUFFER_SIZE];

// Binary semaphore for synchronization between push_to_talk and feature_extraction
SemaphoreHandle_t xBinarySemaphoreMic = xSemaphoreCreateBinary();

// Queue to hold features, and AI result
QueueHandle_t featureQueue = xQueueCreate(1, sizeof(float*));
QueueHandle_t aiQueue = xQueueCreate(1, sizeof(int));