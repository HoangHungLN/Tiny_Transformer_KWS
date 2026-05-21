#include "feature_extraction.h"

void Task_FeatureExtraction(void *pvParameters) {
    // Ép kiểu ép bộ nhớ: Cấp phát mảng 2 chiều dạng static để nó nằm ở bộ nhớ tĩnh,
    // tuyệt đối không dùng mảng cục bộ thông thường để tránh tràn RAM của Task.
    static float mfcc_features[NUM_FRAMES][NUM_MFCC];

    while (1) {
        // 1. Khóa Task lại, chờ tín hiệu từ push_to_talk hoặc micro
        // portMAX_DELAY giúp CPU nghỉ ngơi 100% khi không có nút bấm
        if (xSemaphoreTake(sem_audio_ready, portMAX_DELAY) == pdTRUE) {
            
            // 2. Chạy giải thuật DSP (Chuyển 16000 mẫu int16 thành ma trận float 98x13)
            calculate_mfcc(g_audio_buffer, (float*)mfcc_features);

            // 3. Gửi ma trận MFCC qua hàng đợi cho tinyKWSTask
            if (xQueueSend(mfcc_queue, &mfcc_features, pdMS_TO_TICKS(10)) != pdPASS) {
                Serial.println("[DSP ERROR] Hàng đợi MFCC bị đầy, luồng AI xử lý không kịp!");
            }
        }
    }
}

// =========================================================================
// KHU VỰC DÀNH CHO THÀNH VIÊN HIỆN THỰC THUẬT TOÁN DSP
// =========================================================================
void calculate_mfcc(const int16_t* raw_audio, float* mfcc_output) {
    // Lưu ý: Do hạn chế phần cứng, nên tính toán trực tiếp trên mảng 1D thay vì tạo nhiều mảng 2D

    // 1. Pre-emphasis: 
    // y[t] = x[t] - 0.97 * x[t-1]

    // 2 & 3. Framing & Windowing (Hamming Window):
    // Cắt 400 mẫu (25ms), dịch 160 mẫu (10ms)

    // 4. FFT and Power Spectrum (NFFT = 512):
    // Khuyến nghị: Dùng thư viện ESP-DSP (hàm dsps_fft2r_fc32) để phần cứng S3 tự tăng tốc

    // 5. Mel Filter Banks (26 Filters):
    // Nhân phổ năng lượng với mảng trọng số Mel-filter, sau đó tính 20*log10()

    // 6. DCT (Discrete Cosine Transform):
    // Rút trích 13 hệ số đầu tiên (từ index 1 đến 13) để loại bỏ nhiễu nền

    // 7. Normalization/Scaling:
    // Trừ đi Mean (chuẩn hóa theo từng frame) và chia cho giá trị cực đại để scale về [-1, 1]
}