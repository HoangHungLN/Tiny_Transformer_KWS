#include "push_to_talk.h"
#include "micro.h"

int current_test_idx = 0;

void Task_PushToTalk(void *pvParameters) {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    while (1) {
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            
            vTaskDelay(pdMS_TO_TICKS(50));
            
            if (digitalRead(BOOT_BUTTON_PIN) == LOW) {

                // ===== Simulate microphone for inference on test data =====
                Serial.printf("\n[TEST EDGE] SIMULATED WORD: %s\n", test_samples[current_test_idx].label);
                
                // COPY DATA FROM WAV ARRAY TO MIC BUFFER
                for (int i = 0; i < 16000; i++) {
                    g_audio_buffer[i] = test_samples[current_test_idx].data[i];
                }

                // Increment counter to read the next word on next button press
                current_test_idx++;
                if (current_test_idx >= NUM_TEST_SAMPLES) {
                    current_test_idx = 0; 
                }


                // Call function to record 16000 samples
                // micro_record(g_audio_buffer, AUDIO_BUFFER_SIZE);

                // Trigger Semaphore flag to notify feature_extraction that data is ready
                xSemaphoreGive(xBinarySemaphoreMic);

                // Lock Task, wait for user to release button before allowing new recording
                while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
