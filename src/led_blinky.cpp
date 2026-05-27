#include "led_blinky.h"

void Task_LedBlinky(void *pvParameters) {
    pinMode(LED_PIN, OUTPUT);
    while(1) {
        digitalWrite(LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}