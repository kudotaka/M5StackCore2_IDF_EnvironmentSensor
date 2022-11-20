#pragma once
#include "stdint.h"

esp_err_t Speaker_Init();
esp_err_t Speaker_WriteBuff(uint8_t* buff, uint32_t len, uint32_t timeout);
esp_err_t Speaker_Deinit();
