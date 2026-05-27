#ifndef FEATURE_EXTRACTION_H
#define FEATURE_EXTRACTION_H

#include "global.h"
#include <Arduino.h>
#include "esp_dsp.h"
#include <cmath>
#include "feature_extraction_data.h"

#define SAMPLE_RATE 16000
#define FRAME_LENGTH   400   // Equivalent to 0.025s * 16000 (frame_size)
#define FRAME_STEP  160   // Equivalent to 0.010s * 16000 (frame_stride)
#define NFFT        512
#define NUM_FILTERS 32    // nfilt

// Calculate number of frames: ceil((16000 - 400) / 160) = 98 frames (for 1 second of audio)
// Output matrix will have size:
#define NUM_FRAMES  98 
#define TARGET_TIME_STEPS 100   

void Task_FeatureExtraction(void *pvParameters);
void calculate_mel_spectrogram(const int16_t* raw_audio, float* mel_output);

#endif