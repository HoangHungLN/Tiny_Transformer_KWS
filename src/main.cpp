#include "global.h"

#include "tinyml_kws.h"
#include "micro.h"
#include "push_to_talk.h"
#include "feature_extraction.h"
#include "led_blinky.h"
#include "neo_pixel.h"

void setup() {
  Serial.begin(115200);

  // Initialize Micro
  micro_init();

  // Initialize Tasks
  xTaskCreate(Task_PushToTalk, "PTT Task", 4096, NULL, 5, NULL);
  xTaskCreate(Task_FeatureExtraction, "DSP Task", 8192, NULL, 4, NULL);
  xTaskCreate(Task_TinyKWS, "KWS Task", 16384, NULL, 3, NULL);
  xTaskCreate(Task_NeoPixel, "NeoPixel Task", 4096, NULL, 2, NULL);
  xTaskCreate(Task_LedBlinky, "LED Task", 4096, NULL, 1, NULL);
}

void loop() {

}