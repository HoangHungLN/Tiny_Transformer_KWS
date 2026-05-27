#ifndef __MICRO_H__
#define __MICRO_H__

#include <driver/i2s.h>
#include "global.h"

#define I2S_PORT I2S_NUM_0
#define I2S_WS   8 // Word Select (L/R)
#define I2S_SCK  9 // Serial Clock
#define I2S_SD   10 // Serial Data

void micro_init();
void micro_record(int16_t *buffer, size_t num_samples);

#endif