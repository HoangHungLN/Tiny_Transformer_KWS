#include "global.h"

#include "tinyml_kws.h"
#include "micro.h"
#include "push_to_talk.h"
#include "feature_extraction.h"

void setup() {
  Serial.begin(115200);

  xTaskCreate(tinyKWSTask, "KWS Task", 4096, NULL, 2, NULL);
}

void loop() {

}