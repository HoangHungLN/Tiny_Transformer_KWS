#include "feature_extraction.h"

extern int16_t g_audio_buffer[];

void Task_FeatureExtraction(void *pvParameters) {
    // Ép kiểu ép bộ nhớ: Cấp phát mảng 2 chiều dạng static để nó nằm ở bộ nhớ tĩnh,
    // tuyệt đối không dùng mảng cục bộ thông thường để tránh tràn RAM của Task.
    static float mfcc_features[NUM_FRAMES][NUM_MFCC];

    while (1) {
        // 1. Khóa Task lại, chờ tín hiệu từ push_to_talk hoặc micro
        // portMAX_DELAY giúp CPU nghỉ ngơi 100% khi không có nút bấm
        if (xSemaphoreTake(xBinarySemaphoreMic, portMAX_DELAY) == pdTRUE) {
            
            // 2. Chạy giải thuật DSP (Chuyển 16000 mẫu int16 thành ma trận float 98x13)
            calculate_mfcc(g_audio_buffer, (float*)mfcc_features);

            // 3. Gửi ma trận MFCC qua hàng đợi cho tinyKWSTask
            if (xQueueSend(featureQueue, &mfcc_features, pdMS_TO_TICKS(10)) != pdPASS) {
                Serial.println("[DSP ERROR] Hàng đợi MFCC bị đầy, luồng AI xử lý không kịp!");
            }
        }
    }
}

void calculate_mfcc(const int16_t* raw_audio, float* mfcc_output) {
    static bool dsp_initialized = false;
    static float fft_buffer[NFFT * 2]; 
    static float pow_frames[NFFT / 2 + 1];
    static float dct_input[NUM_FILTERS];
    static float dct_output[NUM_FILTERS];

    if (!dsp_initialized) {
        esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
        if (ret != ESP_OK) {
            Serial.println("[DSP ERROR] Khởi tạo bộ toán rfft thất bại!");
        }
        dsp_initialized = true;
    }

    int16_t prev_sample = (NUM_FRAMES > 0) ? raw_audio[0] : 0; 

    for (int f = 0; f < NUM_FRAMES; f++) {
        int start_sample = f * FRAME_STEP;

        // Pre-emphasis & Khung hình & Nhân mảng Hamming tĩnh
        for (int i = 0; i < NFFT; i++) {
            if (i < FRAME_LENGTH) {
                int16_t current_sample = raw_audio[start_sample + i];
                float emphasized = (float)current_sample - 0.97f * (float)prev_sample;
                prev_sample = current_sample;
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
            dct_input[m] = 20.0f * std::log10f(mel_energy);
        }

        // Discrete Cosine Transform (DCT) tiêu chuẩn của ESP-DSP
        dsps_dct_f32(dct_input, dct_output, NUM_FILTERS);

        float max_val = 1e-8f;
        float mean_val = 0.0f;
        float frame_mfccs[NUM_MFCC];

        // Trích xuất hệ số từ 1 đến 13 và nhân bù tỷ lệ chuẩn hóa norm='ortho'
        for (int i = 0; i < NUM_MFCC; i++) {
            frame_mfccs[i] = dct_output[i + 1] * dct_scaling_factors[i];
            mean_val += frame_mfccs[i];
        }
        mean_val /= (float)NUM_MFCC;

        // Normalization / Scaling đưa khoảng giá trị về [-1, 1] cho mỗi Frame
        for (int i = 0; i < NUM_MFCC; i++) {
            frame_mfccs[i] -= mean_val;
            if (std::abs(frame_mfccs[i]) > max_val) {
                max_val = std::abs(frame_mfccs[i]);
            }
        }

        for (int i = 0; i < NUM_MFCC; i++) {
            mfcc_output[f * NUM_MFCC + i] = frame_mfccs[i] / max_val;
        }
        // padding 0 
        for (int f = NUM_FRAMES; f < TARGET_TIME_STEPS; f++) {
            for (int i = 0; i < NUM_MFCC; i++) {
                mfcc_output[f * NUM_MFCC + i] = 0.0f;
            }
        }
    }
}