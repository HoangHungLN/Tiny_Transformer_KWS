#ifndef FEATURE_EXTRACTION_H
#define FEATURE_EXTRACTION_H

#include "global.h"
#include <Arduino.h>
#include "esp_dsp.h"
#include <cmath>
#include "feature_extraction_data.h"

// --- ÁNH XẠ THAM SỐ TỪ PYTHON SCRIPT ---
#define SAMPLE_RATE 16000
#define FRAME_LENGTH   400   // Tương đương 0.025s * 16000 (frame_size)
#define FRAME_STEP  160   // Tương đương 0.010s * 16000 (frame_stride)
#define NFFT        512
#define NUM_FILTERS 26    // nfilt
#define NUM_MFCC    13    // num_ceps

// Tính toán số frame: ceil((16000 - 400) / 160) = 98 frames (cho 1 giây audio)
// Ma trận đầu ra sẽ có kích thước:
#define NUM_FRAMES  98    
#define TOTAL_FEATURES (NUM_FRAMES * NUM_MFCC) // Tổng cộng 1274 phần tử

void Task_FeatureExtraction(void *pvParameters);

void calculate_mfcc(const int16_t* raw_audio, float* mfcc_output);

#endif