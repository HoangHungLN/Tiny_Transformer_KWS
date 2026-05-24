#include "micro.h"

void micro_init() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 đọc chuẩn nhất ở 32bit
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 mono (chân L/R nối GND)
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    Serial.println("[MICRO] I2S INMP441 khoi tao thanh cong.");
}

void micro_record(int16_t *buffer, size_t num_samples) {
    size_t bytes_read;
    int32_t sample32 = 0;

    Serial.println("[MICRO] Bat dau ghi am 1 giay...");
    
    // Xóa rác trong DMA đệm trước khi ghi âm mới
    i2s_zero_dma_buffer(I2S_PORT); 
    
    for (size_t i = 0; i < num_samples; i++) {
        // Đọc từng mẫu 32-bit từ I2S
        i2s_read(I2S_PORT, &sample32, sizeof(int32_t), &bytes_read, portMAX_DELAY);
        
        // Dịch phải 16 bit để lấy data 16-bit chuẩn từ mic INMP441
        buffer[i] = (int16_t)(sample32 >> 16); 
    }
    
    Serial.println("[MICRO] Hoan tat ghi am.");
}
