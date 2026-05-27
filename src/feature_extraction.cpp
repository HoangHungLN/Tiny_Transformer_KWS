#include "feature_extraction.h"

void Task_FeatureExtraction(void *pvParameters) {
    // Allocate static array in static memory to avoid Task RAM overflow
    static float mel_features[TARGET_TIME_STEPS][NUM_FILTERS];

    while (1) {
        // Wait for signal from push_to_talk
        if (xSemaphoreTake(xBinarySemaphoreMic, portMAX_DELAY) == pdTRUE) {
            
            // Run DSP algorithm
            calculate_mel_spectrogram(g_audio_buffer, (float*)mel_features);

            float* p_mel = (float*)mel_features;
            if (xQueueSend(featureQueue, &p_mel, pdMS_TO_TICKS(10)) != pdPASS) {
                Serial.println("[DSP ERROR] Mel spectrogram queue is full");
            } else {
                Serial.println("[DSP] Successfully pushed data to featureQueue.");
            }
        }
    }
}

void calculate_mel_spectrogram(const int16_t* raw_audio, float* mel_output) {
    static bool dsp_initialized = false;
    __attribute__((aligned(16))) static float fft_buffer[NFFT * 2];
    __attribute__((aligned(16))) static float pow_frames[NFFT / 2 + 1];

    if (!dsp_initialized) {
        esp_err_t ret = dsps_fft2r_init_fc32(NULL, NFFT);
        if (ret != ESP_OK) {
            Serial.println("[DSP ERROR] Failed to initialize rfft module!");
        }
        dsp_initialized = true;
    }

    for (int f = 0; f < NUM_FRAMES; f++) {
        int start_sample = f * FRAME_STEP;

        // Pre-emphasis & Framing & Multiply static Hamming array
        for (int i = 0; i < NFFT; i++) {
            if (i < FRAME_LENGTH) {
                int16_t current_sample = raw_audio[start_sample + i];
                // Get the exact previous sample in the 1D raw audio array
                int16_t previous_sample = (start_sample + i > 0) ? raw_audio[start_sample + i - 1] : raw_audio[0];
                float emphasized = (float)current_sample - 0.97f * (float)previous_sample;
                
                fft_buffer[i * 2] = emphasized * hamming_window[i];
                fft_buffer[i * 2 + 1] = 0.0f;
            } else {
                fft_buffer[i * 2] = 0.0f;
                fft_buffer[i * 2 + 1] = 0.0f;
            }
        }
        
        float debug_fft_before = fft_buffer[20];

        // Fast Fourier Transform (FFT) & Power Spectrum
        dsps_fft2r_fc32(fft_buffer, NFFT);
        dsps_bit_rev2r_fc32(fft_buffer, NFFT);

        for (int i = 0; i <= NFFT / 2; i++) {
            float real = fft_buffer[i * 2];
            float imag = fft_buffer[i * 2 + 1];
            pow_frames[i] = (real * real + imag * imag) / (float)NFFT; 
        }

        // Mel Filter Banks
        for (int m = 0; m < NUM_FILTERS; m++) {
            float mel_energy = 0.0f;
            
            // Manual dot product to avoid unaligned SIMD bugs
            for (int k = 0; k <= NFFT / 2; k++) {
                mel_energy += pow_frames[k] * mel_filters[m][k];
            }

            if (mel_energy < 1e-7f) {
                mel_energy = 1e-7f;
            }
            float log_mel_db = 20.0f * log10f(mel_energy);

            mel_output[f * NUM_FILTERS + m] = (log_mel_db - global_mean[m]) / (global_std[m] + 1e-8f);
        }
    }
    for (int f = NUM_FRAMES; f < TARGET_TIME_STEPS; f++) {
        for (int i = 0; i < NUM_FILTERS; i++) {
            mel_output[f * NUM_FILTERS + i] = 0.0f;
        }
    }
}