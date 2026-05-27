#include "neo_pixel.h"
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void Task_NeoPixel(void *pvParameters) {
    pixels.begin();
    pixels.setBrightness(50);
    pixels.show();
    
    int current_label = 2; // Default is 'other' label
    
    while(1) {
        // Get new result from AI
        int new_label;
        if (xQueueReceive(aiQueue, &new_label, 0) == pdPASS) {
            current_label = new_label;
        }
        
        // 0: down, 1: left, 2: other, 3: right, 4: up
        uint32_t color = pixels.Color(0, 0, 0);
        switch (current_label) {
            case 0: // down -> Red
                color = pixels.Color(255, 0, 0);
                break;
            case 1: // left -> Yellow
                color = pixels.Color(255, 255, 0);
                break;
            case 2: // other -> White
                color = pixels.Color(255, 255, 255);
                break;
            case 3: // right -> Blue
                color = pixels.Color(0, 0, 255);
                break;
            case 4: // up -> Blue
                color = pixels.Color(0, 0, 255);
                break;
            default:
                color = pixels.Color(0, 0, 0);
                break;
        }
        
        pixels.setPixelColor(0, color);
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(500));
        
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
