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

// Hàm chuyển đổi giữa Hz và Mel
static inline float hz_to_mel(float hz) {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
}

static inline float mel_to_hz(float mel) {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

void calculate_mfcc(const int16_t* raw_audio, float* mfcc_output) {
    static bool dsp_initialized = false;
    static float fft_buffer[NFFT * 2]; 
    static float window[FRAME_LENGTH];
    static float filter_banks[NUM_FILTERS];
    static float mel_pts_hz[NUM_FILTERS + 2];
    static int bin[NUM_FILTERS + 2];

    if (!dsp_initialized) {
        // 1. Khởi tạo cửa sổ Hamming
        dsps_wind_hamming_f32(window, FRAME_LENGTH);

        // 2. Tính toán ma trận trọng số Mel Filter Bank
        float low_mel = hz_to_mel(0.0f);
        float high_mel = hz_to_mel((float)SAMPLE_RATE / 2.0f);
        
        for (int m = 0; m < NUM_FILTERS + 2; m++) {
            float mel = low_mel + m * (high_mel - low_mel) / (NUM_FILTERS + 1);
            mel_pts_hz[m] = mel_to_hz(mel);
            bin[m] = (int)std::floor((NFFT + 1) * mel_pts_hz[m] / SAMPLE_RATE);
        }

        esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
        if (ret != ESP_OK) {
            Serial.println("[DSP ERROR] Khởi tạo bộ toán rfft thất bại!");
        }
        dsp_initialized = true;
    }

    int16_t prev_sample = (NUM_FRAMES > 0) ? raw_audio[0] : 0; 

    for (int f = 0; f < NUM_FRAMES; f++) {
        int start_sample = f * FRAME_STEP;

        for (int i = 0; i < NFFT; i++) {
            if (i < FRAME_LENGTH) {
                int16_t current_sample = raw_audio[start_sample + i];
                float emphasized = (float)current_sample - 0.97f * (float)prev_sample;
                prev_sample = current_sample;

                fft_buffer[i * 2] = emphasized * window[i];
                fft_buffer[i * 2 + 1] = 0.0f;
            } else {
                fft_buffer[i * 2] = 0.0f;
                fft_buffer[i * 2 + 1] = 0.0f;
            }
        }

        // 4. Fast Fourier Transform (FFT) & Power Spectrum
        dsps_fft2r_fc32(fft_buffer, NFFT);
        dsps_bit_rev_fc32(fft_buffer, NFFT);

        for (int i = 0; i <= NFFT / 2; i++) {
            float real = fft_buffer[i * 2];
            float imag = fft_buffer[i * 2 + 1];
            float power = (real * real + imag * imag) / (float)NFFT;
            fft_buffer[i] = power; 
        }

        // 5. Mel Filter Banks
        std::fill_n(filter_banks, NUM_FILTERS, 0.0f);

        for (int m = 1; m <= NUM_FILTERS; m++) {
            int f_m_minus = bin[m - 1];
            int f_m = bin[m];
            int f_m_plus = bin[m + 1];

            for (int k = f_m_minus; k < f_m; k++) {
                if (k <= NFFT / 2) {
                    float weight = (float)(k - bin[m - 1]) / (bin[m] - bin[m - 1]);
                    filter_banks[m - 1] += fft_buffer[k] * weight;
                }
            }
            for (int k = f_m; k < f_m_plus; k++) {
                if (k <= NFFT / 2) {
                    float weight = (float)(bin[m + 1] - k) / (bin[m + 1] - bin[m]);
                    filter_banks[m - 1] += fft_buffer[k] * weight;
                }
            }

            if (filter_banks[m - 1] < 1e-7f) {
                filter_banks[m - 1] = 1e-7f;
            }
            filter_banks[m - 1] = 20.0f * std::log10(filter_banks[m - 1]);
        }

        // 6. Discrete Cosine Transform
        float max_val = 1e-8f;
        float mean_val = 0.0f;
        float frame_mfccs[NUM_MFCC];

        for (int i = 0; i < NUM_MFCC; i++) {
            int dct_index = i + 1; 
            float sum = 0.0f;
            
            for (int j = 0; j < NUM_FILTERS; j++) {
                sum += filter_banks[j] * std::cos(M_PI * (float)dct_index * ((float)j + 0.5f) / (float)NUM_FILTERS);
            }
            
            float ortho = std::sqrt(2.0f / (float)NUM_FILTERS);
            frame_mfccs[i] = sum * ortho;
            
            mean_val += frame_mfccs[i];
        }
        mean_val /= (float)NUM_MFCC;

        // 7. Normalization / Scaling
        for (int i = 0; i < NUM_MFCC; i++) {
            frame_mfccs[i] -= mean_val;
            if (std::abs(frame_mfccs[i]) > max_val) {
                max_val = std::abs(frame_mfccs[i]);
            }
        }

        for (int i = 0; i < NUM_MFCC; i++) {
            mfcc_output[f * NUM_MFCC + i] = frame_mfccs[i] / max_val;
        }
    }
}