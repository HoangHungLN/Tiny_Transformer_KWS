#ifndef __TINYML_KWS_H__
#define __TINYML_KWS_H__

#include "global.h"
#include "kws_model.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

void setupKWS();
void tinyKWSTask(void *pvParameters);

#endif