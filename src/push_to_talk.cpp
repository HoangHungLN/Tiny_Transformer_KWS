#include "push_to_talk.h"
#include "micro.h"

void Task_PushToTalk(void *pvParameters) {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    while (1) {
        // Nút nhấn kéo xuống LOW khi được bấm
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            
            // Debounce
            vTaskDelay(pdMS_TO_TICKS(50));
            
            if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
                // 1. Gọi hàm ghi âm chính xác 16000 mẫu
                micro_record(g_audio_buffer, AUDIO_BUFFER_SIZE);

                int max_amplitude = 0;
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    // Lấy giá trị tuyệt đối để tìm biên độ
                    int current_val = g_audio_buffer[i] > 0 ? g_audio_buffer[i] : -g_audio_buffer[i];
                    if (current_val > max_amplitude) {
                        max_amplitude = current_val;
                    }
                }
                Serial.print("[TEST] Do lon am thanh (Max Amplitude): ");
                Serial.println(max_amplitude);

                // 2. Kích hoạt cờ Semaphore để báo cho feature_extraction biết data đã sẵn sàng
                xSemaphoreGive(xBinarySemaphoreMic);

                // 3. Khóa Task tại đây, đợi người dùng buông tay ra mới cho phép nhấn lại
                while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
        
        // Nghỉ 10ms tránh bị Watchdog Timer báo lỗi vì chiếm dụng CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
