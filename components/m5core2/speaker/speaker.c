#include "freertos/FreeRTOS.h"
#include "speaker.h"
#include "driver/i2s.h"

#define I2S_BCK_PIN 12
#define I2S_LRCK_PIN 0
#define I2S_DATA_PIN 2
#define I2S_DATA_IN_PIN 34
#define SPEAKER_I2S_NUMBER I2S_NUM_0

esp_err_t Speaker_Init() {
  esp_err_t err = ESP_OK;

  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 6,
    .dma_buf_len          = 60,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
  };

ESP_ERROR_CHECK(i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL));

  i2s_pin_config_t tx_pin_config = {
    .bck_io_num           = I2S_BCK_PIN,
    .ws_io_num            = I2S_LRCK_PIN,
    .data_out_num         = I2S_DATA_PIN,
    .data_in_num          = I2S_DATA_IN_PIN,
  };
  err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);

  err += i2s_set_clk(SPEAKER_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  err += i2s_zero_dma_buffer(SPEAKER_I2S_NUMBER);
    if(err != ESP_OK){
        err = ESP_FAIL;
    }
    return err;
}

esp_err_t Speaker_WriteBuff(uint8_t* buff, uint32_t len, uint32_t timeout) {
    esp_err_t err = ESP_OK;
    size_t bytes_written = 0;
    err += i2s_write(SPEAKER_I2S_NUMBER, buff, len, &bytes_written, portMAX_DELAY);
    err += i2s_zero_dma_buffer(SPEAKER_I2S_NUMBER);
    if(err != ESP_OK){
        err = ESP_FAIL;
    }
    return err;
}

esp_err_t Speaker_Deinit() {
    esp_err_t err = ESP_OK;
    err += i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
    err += gpio_reset_pin(I2S_LRCK_PIN);
    err += gpio_reset_pin(I2S_DATA_PIN);
    err += gpio_reset_pin(I2S_BCK_PIN);
    err += gpio_reset_pin(I2S_DATA_IN_PIN);
    if(err != ESP_OK){
        err = ESP_FAIL;
    }
    return err;
}