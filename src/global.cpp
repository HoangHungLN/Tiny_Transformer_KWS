#include "global.h"

SemaphoreHandle_t xBinarySemaphoreMic = xSemaphoreCreateBinary();

// Queue to hold features, Adjust the element size to match the output of the feature extraction.
QueueHandle_t featureQueue = xQueueCreate(1, sizeof(float) * 10);
QueueHandle_t aiQueue = xQueueCreate(1, sizeof(int));