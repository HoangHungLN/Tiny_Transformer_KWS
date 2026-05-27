#include "tinyml_kws.h"
#include "feature_extraction.h"

namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 80 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
    
    const char* KWS_LABELS[] = {"down", "left", "other", "right", "up"};
    bool is_model_allocated = false;
}

void setupKWS(){
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(kws_model);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    
    is_model_allocated = true;
    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void Task_TinyKWS(void *pvParameters){
    setupKWS();
    while(1){
        if (!is_model_allocated) {
            Serial.println("[AI ERROR] Model memory not allocated. Please check Model!");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        float* p_mel_features;
        // Wait for data from featureQueue
        if (xQueueReceive(featureQueue, &p_mel_features, portMAX_DELAY) == pdPASS) {
            Serial.println("[AI] Received features from DSP. Proceeding to Invoke...");
            
            // Re-fetch input tensor to be safe
            input = interpreter->input(0);
            
            // Copy data to Model Input (INT8)
            // Get scale and zero_point parameters for quantization
            float scale = input->params.scale;
            int zero_point = input->params.zero_point;
            // Quantize back to int8
            for (int i = 0; i < TARGET_TIME_STEPS * NUM_FILTERS; i++) {
                float val = roundf(p_mel_features[i] / scale) + zero_point;
                // Limit values between -128 and 127
                if (val > 127.0f) val = 127.0f;
                if (val < -128.0f) val = -128.0f;
                input->data.int8[i] = (int8_t)val;
            }
            
            // Run Model
            interpreter->ResetVariableTensors();
            uint32_t start_time = millis();
            TfLiteStatus invoke_status = interpreter->Invoke();
            if (invoke_status != kTfLiteOk) {
                Serial.println("[AI ERROR] Invoke failed!");
                continue;
            }
            uint32_t end_time = millis();
            
            // Re-fetch output tensor to be safe
            output = interpreter->output(0);
            
            // Find label with highest probability
            int num_classes = output->dims->data[output->dims->size - 1];
            int predicted_class = 0;
            float max_prob = -1.0f;
            
            for (int i = 0; i < num_classes; i++) {
                // Model is purely INT8 quantized
                float prob = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
                
                if (prob > max_prob) {
                    max_prob = prob;
                    predicted_class = i;
                }
            }
            
            Serial.printf("[AI] => PREDICTION RESULT (Time: %lu ms): %s (confidence: %.2f%%)\n", end_time - start_time, KWS_LABELS[predicted_class], max_prob * 100.0f);
            
            // Push result to aiQueue
            if (xQueueSend(aiQueue, &predicted_class, pdMS_TO_TICKS(10)) != pdPASS) {
                Serial.println("[AI ERROR] aiQueue is full!");
            }
        }
    }
}