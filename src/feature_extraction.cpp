#include "feature_extraction.h"

void Task_FeatureExtraction(void *pvParameters) {
    // Ép kiểu ép bộ nhớ: Cấp phát mảng 2 chiều dạng static để nó nằm ở bộ nhớ tĩnh,
    // không dùng mảng cục bộ thông thường để tránh tràn RAM của Task.
    static float mel_features[TARGET_TIME_STEPS][NUM_FILTERS];

    while (1) {
        // 1. Khóa Task lại, chờ tín hiệu từ push_to_talk
        // portMAX_DELAY giúp CPU nghỉ ngơi 100% khi không có nút bấm
        if (xSemaphoreTake(xBinarySemaphoreMic, portMAX_DELAY) == pdTRUE) {
            
            // 2. Chạy giải thuật DSP
            calculate_mel_spectrogram(g_audio_buffer, (float*)mel_features);

            // 3. Gửi ma trận Mel spectrogram qua hàng đợi cho tinyKWSTask
            if (xQueueSend(featureQueue, &mel_features, pdMS_TO_TICKS(10)) != pdPASS) {
                Serial.println("[DSP ERROR] Hàng đợi Mel spectrogram bị đầy");
            }
        }
    }
}

void calculate_mel_spectrogram(const int16_t* raw_audio, float* mel_output) {
    static bool dsp_initialized = false;
    static float fft_buffer[NFFT * 2];
    static float pow_frames[NFFT / 2 + 1];

    if (!dsp_initialized) {
        esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
        dsp_initialized = true;
    }

    for (int f = 0; f < NUM_FRAMES; f++) {
        int start_sample = f * FRAME_STEP;

        // Pre-emphasis & Khung hình & Nhân mảng Hamming tĩnh
        for (int i = 0; i < NFFT; i++) {
            if (i < FRAME_LENGTH) {
                int16_t current_sample = raw_audio[start_sample + i];
                // Lấy đúng mẫu liền trước trong mảng âm thanh gốc 1D
                int16_t previous_sample = (start_sample + i > 0) ? raw_audio[start_sample + i - 1] : raw_audio[0];
                float emphasized = (float)current_sample - 0.97f * (float)previous_sample;
                
                fft_buffer[i * 2] = emphasized * hamming_window[i];
                fft_buffer[i * 2 + 1] = 0.0f;
            } else {
                fft_buffer[i * 2] = 0.0f;
                fft_buffer[i * 2 + 1] = 0.0f;
            }
        }

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
            
            dsps_dotprod_f32(pow_frames, mel_filters[m], &mel_energy, NFFT / 2 + 1);

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