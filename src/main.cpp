#include "global.h"

#include "tinyml_kws.h"
#include "micro.h"
#include "push_to_talk.h"
#include "feature_extraction.h"

void setup() {
  Serial.begin(115200);

  // Khởi tạo Micro
  micro_init();

  // Khởi tạo các Task
  xTaskCreate(Task_PushToTalk, "PTT Task", 4096, NULL, 3, NULL);
  //xTaskCreate(Task_FeatureExtraction, "DSP Task", 8192, NULL, 2, NULL);
  //xTaskCreate(tinyKWSTask, "KWS Task", 16384, NULL, 1, NULL);
}

void loop() {

}