#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "m5core2.h"


#if CONFIG_SOFTWARE_WIFI_SUPPORT
#include "wifi.h"

#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#endif

#endif

#if CONFIG_SOFTWARE_UI_SUPPORT
#include "ui.h"
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
#include "esp_sntp.h"
#endif

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
#include "m5unit.h"
#endif

#if CONFIG_SOFTWARE_SPEAKER_SUPPORT
#include "wav2_20.h"
#endif

static const char *TAG = "MY-MAIN";

#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
float g_temperature = 0.0;
float g_humidity = 0.0;
uint32_t g_pressure = 0.0;
#endif //CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
#if CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
float g_temperature = 0.0;
float g_humidity = 0.0;
float g_pressure = 0.0;
#endif //CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
#if CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT
//uint32_t g_pressure = 0.0;
float g_pressure = 0.0;
#endif //CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT
#if CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT
float g_pressure = 0.0;
#endif //CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT
#if ( CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT )
float g_temperature = 0.0;
float g_humidity = 0.0;
int g_co2 = 0;
#endif //( CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT )

#if CONFIG_SOFTWARE_RTC_SUPPORT
static char g_lastSyncDatetime[72] = {0};
#endif

#if CONFIG_SOFTWARE_CLOCK_SOUND_SUPPORT
uint8_t g_clockCount = 0;
uint8_t g_clockCurrent = 0;
uint8_t g_clockSoundEnabled = 1;
#endif

#if CONFIG_SOFTWARE_SPEAKER_SUPPORT
size_t i2s_bytes_write = 0;
TaskHandle_t xSpeaker;
#endif

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
TaskHandle_t xButton;
static void button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start button_task");

    while(1){
        if (Button_WasPressed(button_left)) {
            ESP_LOGI(TAG, "BUTTON LEFT PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
#if CONFIG_SOFTWARE_SPEAKER_SUPPORT
            if (g_clockSoundEnabled) {
                g_clockSoundEnabled = 0;
            } else {
                g_clockSoundEnabled = 1;
            }
            vTaskResume(xSpeaker);
#endif
        }
        if (Button_WasReleased(button_left)) {
            ESP_LOGI(TAG, "BUTTON LEFT RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_left, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON LEFT LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
#if CONFIG_SOFTWARE_BUZZER_SUPPORT
            vTaskResume(xBuzzer);
#endif
        }

        if (Button_WasPressed(button_middle)) {
            ESP_LOGI(TAG, "BUTTON MIDDLE PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_middle)) {
            ESP_LOGI(TAG, "BUTTON MIDDLE RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
            M5Core2_Display_SetBrightness(40);
#endif
        }
        if (Button_WasLongPress(button_middle, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON MIDDLE LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
            M5Core2_Display_SetBrightness(0);
#endif
        }

        if (Button_WasPressed(button_right)) {
            ESP_LOGI(TAG, "BUTTON RIGHT PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_right)) {
            ESP_LOGI(TAG, "BUTTON RIGHT RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_right, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON RIGHT LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
#if CONFIG_SOFTWARE_RTC_SUPPORT
            ui_status_set(g_lastSyncDatetime);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
TaskHandle_t xClock;

#if CONFIG_SOFTWARE_WIFI_SUPPORT
TaskHandle_t xRtc;
static bool g_timeInitialized = false;
const char servername[] = "ntp.jst.mfeed.ad.jp";

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    g_timeInitialized = true;
}

void vLoopRtcTask(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start vLoopRtcTask. TaskDelayTime");

    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    ESP_LOGI(TAG, "ServerName:%s", servername);
    sntp_setservername(0, servername);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    while (1) {
        if (wifi_isConnected() == ESP_OK) {
            sntp_init();
        } else {
            vTaskDelay( pdMS_TO_TICKS(60000) );
            continue;
        }

        ESP_LOGI(TAG, "Waiting for time synchronization with SNTP server");
        while (!g_timeInitialized)
        {
            vTaskDelay( pdMS_TO_TICKS(5000) );
        }

        time_t now = 0;
        struct tm timeinfo = {0};
        time(&now);
        localtime_r(&now, &timeinfo);
        sprintf(g_lastSyncDatetime,"NTP Update : %04d/%02d/%02d %02d:%02d", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
        ESP_LOGI(TAG, "%s", g_lastSyncDatetime);
#if CONFIG_SOFTWARE_UI_SUPPORT
        ui_status_set(g_lastSyncDatetime);
#endif

        rtc_date_t rtcdate;
        rtcdate.year = timeinfo.tm_year+1900;
        rtcdate.month = timeinfo.tm_mon+1;
        rtcdate.day = timeinfo.tm_mday;
        rtcdate.hour = timeinfo.tm_hour;
        rtcdate.minute = timeinfo.tm_min;
        rtcdate.second = timeinfo.tm_sec;
        PCF8563_SetTime(&rtcdate);

        g_timeInitialized = false;
        sntp_stop();

        vTaskDelay( pdMS_TO_TICKS(6000000) );
    }
}
#endif

void vLoopClockTask(void *pvParametes)
{
    //PCF8563
    ESP_LOGI(TAG, "start vLoopClockTask.");
 
    // Set timezone to Japan Standard Time
    setenv("TZ", "JST-9", 1);
    tzset();

    while (1) {
        rtc_date_t rtcdate;
        PCF8563_GetTime(&rtcdate);
#if CONFIG_SOFTWARE_CLOCK_SOUND_SUPPORT
        if (g_clockCurrent != rtcdate.hour) {
            if (rtcdate.hour == 0) { // 0:00
                g_clockCount = 12;
            } else if (rtcdate.hour > 12) { // 13:00-23:00
                g_clockCount = rtcdate.hour - 12;
            } else { // 1:00-12:00
                g_clockCount = rtcdate.hour;
            }
            g_clockCurrent = rtcdate.hour;
            vTaskResume(xSpeaker);
        }
#endif
        char str1[30] = {0};
        sprintf(str1,"%04d/%02d/%02d %02d:%02d:%02d", rtcdate.year, rtcdate.month, rtcdate.day, rtcdate.hour, rtcdate.minute, rtcdate.second);
#if CONFIG_SOFTWARE_UI_SUPPORT
        ui_datetime_set(str1);
#endif

        vTaskDelay( pdMS_TO_TICKS(990) );
    }
}
#endif

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT )
TaskHandle_t xUnitEnv2;
void vLoopUnitEnv2Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Sht3x");
    esp_err_t ret = ESP_OK;
#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT )
    ret = Sht3x_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sht3x_Init Error");
        return;
    }
    ESP_LOGI(TAG, "Sht3x_Init() is OK!");
#endif
#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
    ret = Bmp280_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bmp280_Init Error");
        return;
    }
    ESP_LOGI(TAG, "Bmp280_Init() is OK!");
#endif
#if CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
    ret = Qmp6988_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Qmp6988_Init Error");
        return;
    }
    ESP_LOGI(TAG, "Qmp6988_Init() is OK!");
#endif
    while (1) {
        ret = Sht3x_Read();
        if (ret == ESP_OK) {
            vTaskDelay( pdMS_TO_TICKS(100) );
#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT )
            g_temperature = Sht3x_GetTemperature();
            g_humidity = Sht3x_GetHumidity();
#endif
#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
            g_pressure = Bmp280_getPressure() / 100;
//            ESP_LOGI(TAG, "temperature:%f, humidity:%f, pressure:%d", g_temperature, g_humidity, g_pressure);
            ESP_LOGI(TAG, "temperature:%f, humidity:%f, pressure:%f", g_temperature, g_humidity, g_pressure);
#endif
#if CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
            g_pressure = Qmp6988_CalcPressure() / 100;
            ESP_LOGI(TAG, "temperature:%f, humidity:%f, pressure:%f", g_temperature, g_humidity, g_pressure);
#endif
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_temperature_update( Sht3x_GetIntTemperature() );
            ui_humidity_update( Sht3x_GetIntHumidity() );
#endif
        } else {
            ESP_LOGE(TAG, "Sht3x_Read() is error code:%d", ret);
            vTaskDelay( pdMS_TO_TICKS(10000) );
        }

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif

#if ( CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT )
TaskHandle_t xUnitBmp280;
void vLoopUnitBmp280Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Bmp280");
    esp_err_t ret = ESP_OK;
    ret = Bmp280_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bmp280_Init Error");
        return;
    }
    ESP_LOGI(TAG, "Bmp280_Init() is OK!");

    while (1) {
        g_pressure = Bmp280_getPressure() / 100;
        ESP_LOGI(TAG, "pressure:%f", g_pressure);

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif

#if CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT
TaskHandle_t xUnitQmp6988;
void vLoopUnitQmp6988Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Qmp6988");
    esp_err_t ret = ESP_OK;
    ret = Qmp6988_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Qmp6988_Init Error");
        return;
    }
    ESP_LOGI(TAG, "Qmp6988_Init() is OK!");

    while (1) {
        g_pressure = Qmp6988_CalcPressure() / 100;
        ESP_LOGI(TAG, "pressure:%f", g_pressure);

        vTaskDelay( pdMS_TO_TICKS(5000) );
    }
}
#endif

#if CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT
TaskHandle_t xUnitEnvScd30;
void vLoopUnitEnvScd30Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Scd30");
    esp_err_t ret = ESP_OK;
    ret = Scd30_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_I2CInit Error");
        return;
    }
    ret = Scd30_SetAutoSelfCalibration(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_SetAutoSelfCalibration() is Error");
    }
    ret = Scd30_SetTemperatureOffset(220); // 2.2*100
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd30_SetTemperatureOffset() is Error");
    }
    vTaskDelay( pdMS_TO_TICKS(200) );
    while (1) {
        float scd30_tmp[3] = { 0.0 };
        if (Scd30_IsAvailable() != false) {
            ret = Scd30_GetCarbonDioxideConcentration(scd30_tmp);
            if (ret == ESP_OK) {
                //scd30_tmp[0]; //scd30_co2Concentration
                //scd30_tmp[1]; //scd30_temperature
                //scd30_tmp[2]; //scd30_humidity
                g_co2 = scd30_tmp[0];
                g_temperature = scd30_tmp[1];
                g_humidity = scd30_tmp[2];
                ESP_LOGI(TAG, "temperature:%f, humidity:%f", g_temperature, g_humidity);
#if CONFIG_SOFTWARE_UI_SUPPORT
                char str1[25] = {0};
                sprintf(str1, "CO2 : %04d ppm", g_co2);
                ui_status_set( str1 );

                ui_temperature_update( (int)g_temperature );
                ui_humidity_update( (int)g_humidity );
#endif
            } else {
                ESP_LOGE(TAG, "Scd30_GetCarbonDioxideConcentration is error code:%d", ret);
                vTaskDelay( pdMS_TO_TICKS(10000) );
            }
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif

#if CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT
TaskHandle_t xUnitEnvScd40;
void vLoopUnitEnvScd40Task(void *pvParametes)
{
    ESP_LOGI(TAG, "start I2C Scd40");
    esp_err_t ret = ESP_OK;
    ret = Scd40_Init(I2C_NUM_0, PORT_A_SDA_PIN, PORT_A_SCL_PIN, PORT_A_I2C_STANDARD_BAUD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_I2CInit Error");
        return;
    }
    ret = Scd40_StopPeriodicMeasurement();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_StopPeriodicMeasurement() is Error code:%X", ret);
    }
/*
    ret = Scd40_SetTemperatureOffset(0.0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_SetTemperatureOffset() is Error code:%X", ret);
    }
*/
    ret = Scd40_SetAutoSelfCalibration(false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_SetAutoSelfCalibration() is Error code:%X", ret);
    }
    ret = Scd40_StartPeriodicMeasurement();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Scd40_StartPeriodicMeasurement() is Error code:%X", ret);
    }
    vTaskDelay( pdMS_TO_TICKS(5000) );
    while (1) {
        float scd40_tmp[3] = { 0.0 };
        if (Scd40_IsAvailable() != false) {
//            ret = Scd40_GetCarbonDioxideConcentration(scd40_tmp);
            ret = Scd40_ReadMeasurement(scd40_tmp);
            if (ret == ESP_OK) {
                //scd40_tmp[0]; //scd40_co2Concentration
                //scd40_tmp[1]; //scd40_temperature
                //scd40_tmp[2]; //scd40_humidity
                g_co2 = scd40_tmp[0];
                g_temperature = scd40_tmp[1];
                g_humidity = scd40_tmp[2];
                ESP_LOGI(TAG, "temperature:%f, humidity:%f, CO2:%04d", g_temperature, g_humidity, g_co2);
#if CONFIG_SOFTWARE_UI_SUPPORT
                char str1[25] = {0};
                sprintf(str1, "CO2 : %04d ppm", g_co2);
                ui_status_set( str1 );

                ui_temperature_update( (int)g_temperature );
                ui_humidity_update( (int)g_humidity );
#endif
            } else {
                ESP_LOGE(TAG, "Scd40_GetCarbonDioxideConcentration is error code:%X", ret);
                vTaskDelay( pdMS_TO_TICKS(10000) );
            }
        }

        vTaskDelay( pdMS_TO_TICKS(10000) );
    }
}
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
TaskHandle_t xMPU6886;
static void mpu6886_task(void* pvParameters) {
    ESP_LOGI(TAG, "start mpu6886_task");

    float ax, ay, az;
    while (1) {
        MPU6886_GetAccelData(&ax, &ay, &az);
        ESP_LOGI(TAG, "MPU6886 Acc x: %.2f, y: %.2f, z: %.2f", ax, ay, az);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_LED_SUPPORT
TaskHandle_t xLED;
static void led_task(void* pvParameters) {
    ESP_LOGI(TAG, "start led_task");

    while(1){
        Led_OnOff(led_a, true);
        vTaskDelay(pdMS_TO_TICKS(1000));

        Led_OnOff(led_a, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_SCREEN_DEMO_SUPPORT
TaskHandle_t xSCREEN;
static void screen_task(void* pvParameters) {
    ESP_LOGI(TAG, "start screen_task");

    while(1){
        Axp192_ScreenBreath(0);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (0)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenBreath(70);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (70)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenBreath(100);
        ESP_LOGI(TAG, "Axp192_ScreenBreath (100)");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenOnOff(false);
        ESP_LOGI(TAG, "Axp192_ScreenOff");
        vTaskDelay(pdMS_TO_TICKS(2000));

        Axp192_ScreenOnOff(true);
        ESP_LOGI(TAG, "Axp192_ScreenOn");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_LED_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalLED;
Led_t* led_ext1;
static void external_led_task(void* pvParameters) {
    ESP_LOGI(TAG, "start external_led_task");
    Led_Init();
    if (Led_Enable(GPIO_NUM_26) == ESP_OK) {
        led_ext1 = Led_Attach(GPIO_NUM_26);
    }
    while(1){
        Led_OnOff(led_ext1, true);
        vTaskDelay(pdMS_TO_TICKS(1000));

        Led_OnOff(led_ext1, false);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT
// SELECT GPIO_NUM_XX
TaskHandle_t xExternalButton;
Button_t* button_ext1;
static void external_button_task(void* pvParameters) {
    ESP_LOGI(TAG, "start external_button_task");

    // ONLY M5Stick C Plus
    M5Stick_Port_PinMode(GPIO_NUM_25, INPUT);

    Button_Init();
    if (Button_Enable(GPIO_NUM_36, ACTIVE_HIGH) == ESP_OK) {
        button_ext1 = Button_Attach(GPIO_NUM_36, ACTIVE_HIGH);
    }
    while(1){
        if (Button_WasPressed(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 PRESSED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(true);
#endif
        }
        if (Button_WasReleased(button_ext1)) {
            ESP_LOGI(TAG, "BUTTON EXT1 RELEASED!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }
        if (Button_WasLongPress(button_ext1, pdMS_TO_TICKS(1000))) { // 1Sec
            ESP_LOGI(TAG, "BUTTON EXT1 LONGPRESS!");
#if CONFIG_SOFTWARE_UI_SUPPORT
            ui_button_label_update(false);
#endif
        }

        vTaskDelay(pdMS_TO_TICKS(80));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT
TaskHandle_t xExternalRGBLedBlink;
void vexternal_LoopRGBLedBlinkTask(void *pvParametes)
{
    ESP_LOGI(TAG, "start vexternal_LoopRGBLedBlinkTask");

    pixel_settings_t px_ext1;
    uint8_t blink_count = 5;
    uint32_t colors[] = {SK6812_COLOR_BLUE, SK6812_COLOR_LIME, SK6812_COLOR_AQUA
                    , SK6812_COLOR_RED, SK6812_COLOR_MAGENTA, SK6812_COLOR_YELLOW
                    , SK6812_COLOR_WHITE};
    Sk6812_Init(&px_ext1, GPIO_NUM_26, RMT_CHANNEL_0, 1);
    while (1) {
        for (uint8_t c = 0; c < sizeof(colors)/sizeof(uint32_t); c++) {
            for (uint8_t i = 0; i < blink_count; i++) {
                Sk6812_SetAllColor(&px_ext1, colors[c]);
                Sk6812_Show(&px_ext1, RMT_CHANNEL_0);
                vTaskDelay(pdMS_TO_TICKS(1000));

                Sk6812_SetAllColor(&px_ext1, SK6812_COLOR_OFF);
                Sk6812_Show(&px_ext1, RMT_CHANNEL_0);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
#endif

#if ( CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
TaskHandle_t xDigitDisplay;
DigitDisplay_t* digitdisplay_1;
void vLoopUnitDigitDisplayTask(void *pvParametes)
{
    ESP_LOGI(TAG, "start Digit Display");

// Sample Rowdata
//  --0x01--
// |        |
//0x20     0x02
// |        |
//  --0x40- -
// |        |
//0x10     0x04
// |        |
//  --0x08--   0x80

    uint8_t anime[] = {0x00, 0x30, 0x38, 0x78, 0x79, 0x7f};
    uint8_t animeCurrent = 0;
    uint8_t animeMax = sizeof(anime)/sizeof(uint8_t);
    uint8_t animeDigitPosition = 1;
    uint8_t animeDigitMax = DIGIT_COUNT;
    uint8_t data = 0x00;

    Tm1637_Init();
    if (Tm1637_Enable(GPIO_NUM_33, GPIO_NUM_32) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(GPIO_NUM_33, GPIO_NUM_32, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    while(1) {
        Tm1637_ClearDisplay(digitdisplay_1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        for (animeDigitPosition = 1; animeDigitPosition < animeDigitMax+1; animeDigitPosition++) {
            for (animeCurrent = 0; animeCurrent < animeMax; animeCurrent++) {
                for (uint8_t position = DIGIT_COUNT; position > animeDigitPosition-1; position--) {
                    for (uint8_t digit = DIGIT_COUNT; digit > 0; digit--) {
                        data = 0x00;
                        if (digit == position) {
                            data += 0x80;
                        } else {
                            //data = 0x00;
                        }

                        if (digit == animeDigitPosition) {
                            data += anime[animeCurrent];
                        } else if (digit < animeDigitPosition) {
                            data = 0xff;
                        }

                        Tm1637_DisplayBitRowdata(digitdisplay_1, digit-1, data);
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
        }
    }
    vTaskDelete(NULL); // Should never get to here...

/*
// Sample number
    uint8_t listDisp_1[DIGIT_COUNT];
    uint8_t count = 0;
    Tm1637_Init();
    if (Tm1637_Enable(PORT_D9_PIN, PORT_D10_PIN) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(PORT_D9_PIN, PORT_D10_PIN, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    Tm1637_ClearDisplay(digitdisplay_1);
    while(1){
        if (count == UINT8_MAX) {
            count = 0;
        }
        if (count%2) {
            for (uint8_t i = 0; i < DIGIT_COUNT; i++) {
                listDisp_1[i] = i;
            }
        } else {
            for (uint8_t i = 0; i < DIGIT_COUNT; i++) {
                listDisp_1[i] = i+4;
            }
        }
        Tm1637_DisplayAll(digitdisplay_1, listDisp_1);
        count++;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    vTaskDelete(NULL); // Should never get to here...
*/
/*
// Sample message
    Tm1637_Init();
    if (Tm1637_Enable(PORT_D9_PIN, PORT_D10_PIN) == ESP_OK) {
        digitdisplay_1 = Tm1637_Attach(PORT_D9_PIN, PORT_D10_PIN, BRIGHT_TYPICAL);
    } else {
        ESP_LOGE(TAG, "Digit Display Tm1637_Enable Error");
        vTaskDelete(NULL);
    }
    while (1) {
        Tm1637_ClearDisplay(digitdisplay_1);
        vTaskDelay(pdMS_TO_TICKS(500));
        Tm1637_DisplayStr(digitdisplay_1, "HELL0-1234567890", 500);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelete(NULL); // Should never get to here...
*/
}
#endif

#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
TaskHandle_t xEspMqttClient;
esp_mqtt_client_handle_t mqttClient;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA, TOPIC=%s, DATA=%s", event->topic , event->data);
//        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .buffer_size = CONFIG_BROKER_BUFFER_SIZE,
        .client_id = CONFIG_BROKER_MY_DEVICE_ID,
        .lwt_qos = CONFIG_BROKER_LWT_QOS,
        .protocol_ver = CONFIG_MQTT_PROTOCOL_311,
    };

    mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (mqttClient == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init() is error.");
        return;
    }
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqttClient));
}

static void vLoopEspMqttClient_task(void* pvParameters) {
    ESP_LOGI(TAG, "start EspMqttClient_task");

    // connected wifi
    while (1) {
        if (wifi_isConnected() == ESP_OK) {
            mqtt_app_start();
            break;
        }
        vTaskDelay( pdMS_TO_TICKS(30000) );
    }

    int msg_id;
    char pubMessage[128] = {0};
    vTaskDelay( pdMS_TO_TICKS(10000) );

    while (1) {
    // connected wifi
    if (wifi_isConnected() == ESP_OK) {
#if CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
        sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"pressure\":%4d}", g_temperature, g_humidity, g_pressure);
#endif //CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT
#if CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
        sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"pressure\":%4.1f}", g_temperature, g_humidity, g_pressure);
#endif //CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT
#if ( CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT )
    #if ( CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT )
        sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"co2\":%4d,\"pressure\":%4.1f}", g_temperature, g_humidity, g_co2, g_pressure);
    #elif ( CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT )
       sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"co2\":%4d,\"pressure\":%4.1f}", g_temperature, g_humidity, g_co2, g_pressure);
    #else
        sprintf(pubMessage, "{\"temperature\":%4.1f,\"humidity\":%4.1f,\"co2\":%4d}", g_temperature, g_humidity, g_co2);
    #endif
#endif //( CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT )
        msg_id = esp_mqtt_client_publish(mqttClient, CONFIG_BROKER_MY_PUB_TOPIC, pubMessage, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d pubMessage=%s", msg_id, pubMessage);
    } else {
        ESP_LOGI(TAG, "wifi not Connect. wait for ...");
    }

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
    vTaskDelete(NULL); // Should never get to here...
}
#endif

#if CONFIG_SOFTWARE_SPEAKER_SUPPORT
void vLoopSpeaker_task(void *pvParametes)
{
    ESP_LOGI(TAG, "start Speaker");
    ESP_ERROR_CHECK(Speaker_Init());

    while (1) {
      vTaskSuspend(NULL);
      M5Core2_Speaker_Enable(true);

#if CONFIG_SOFTWARE_CLOCK_SOUND_SUPPORT
    if (g_clockSoundEnabled)
    {
        for (uint8_t count = 0; count < g_clockCount; count++) {
            Speaker_WriteBuff((uint8_t *)wav2_20, sizeof(wav2_20), portMAX_DELAY);
        }
    }
#endif

      vTaskDelay(pdMS_TO_TICKS(100));
      M5Core2_Speaker_Enable(false);
      vTaskDelay( pdMS_TO_TICKS(100) );
    }

    vTaskDelete(NULL); // Should never get to here...
}
#endif

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void app_main(void)
{
    ESP_LOGI(TAG, "app_main() start.");
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("MY-MAIN", ESP_LOG_INFO);
//    esp_log_level_set("MY-UI", ESP_LOG_INFO);
//    esp_log_level_set("MY-WIFI", ESP_LOG_INFO);
//    esp_log_level_set("MY-QMP6988", ESP_LOG_INFO);
//    esp_log_level_set("wifi", ESP_LOG_INFO);
//    esp_log_level_set("gpio", ESP_LOG_INFO);

    M5Core2_Init();

#if CONFIG_SOFTWARE_UI_SUPPORT
    ui_init();
#endif
#if CONFIG_SOFTWARE_WIFI_SUPPORT
    initialise_wifi();
#endif

#if CONFIG_SOFTWARE_BUTTON_SUPPORT
    // BUTTON
    xTaskCreatePinnedToCore(&button_task, "button_task", 4096 * 1, NULL, 2, &xButton, 1);
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
#if CONFIG_SOFTWARE_WIFI_SUPPORT
    // rtc
    xTaskCreatePinnedToCore(&vLoopRtcTask, "rtc_task", 4096 * 1, NULL, 2, &xRtc, 1);
#endif
    // clock
    xTaskCreatePinnedToCore(&vLoopClockTask, "clock_task", 4096 * 1, NULL, 2, &xClock, 1);
#endif

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT )
    // UNIT ENV2
    xTaskCreatePinnedToCore(&vLoopUnitEnv2Task, "unit_env2_task", 4096 * 1, NULL, 2, &xUnitEnv2, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT
    // UNIT BMP280
    xTaskCreatePinnedToCore(&vLoopUnitBmp280Task, "unit_bmp280_task", 4096 * 1, NULL, 2, &xUnitBmp280, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT
    // UNIT QMP6988
    xTaskCreatePinnedToCore(&vLoopUnitQmp6988Task, "unit_qmp6988_task", 4096 * 1, NULL, 2, &xUnitQmp6988, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT
    // UNIT ENV SCD30
    xTaskCreatePinnedToCore(&vLoopUnitEnvScd30Task, "unit_env_scd30_task", 4096 * 1, NULL, 2, &xUnitEnvScd30, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT
    // UNIT ENV SCD40
    xTaskCreatePinnedToCore(&vLoopUnitEnvScd40Task, "unit_env_scd40_task", 4096 * 1, NULL, 2, &xUnitEnvScd40, 1);
#endif

#if CONFIG_SOFTWARE_MPU6886_SUPPORT
    // MPU6886
    xTaskCreatePinnedToCore(&mpu6886_task, "mpu6886_task", 4096 * 1, NULL, 2, &xMPU6886, 1);
#endif

#if CONFIG_SOFTWARE_LED_SUPPORT
    // INTERNAL LED
    xTaskCreatePinnedToCore(&led_task, "led_task", 4096 * 1, NULL, 2, &xLED, 1);
#endif

#if CONFIG_SOFTWARE_SCREEN_DEMO_SUPPORT
    // SCREEN DEMO
    xTaskCreatePinnedToCore(&screen_task, "screen_task", 4096 * 1, NULL, 2, &xSCREEN, 1);
#endif

#if CONFIG_SOFTWARE_BUZZER_SUPPORT
    // BUZZER
    xTaskCreatePinnedToCore(&vLoopBuzzerTask, "buzzer_task", 4096 * 1, NULL, 2, &xBuzzer, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_LED_SUPPORT
    // EXTERNAL LED
    xTaskCreatePinnedToCore(&external_led_task, "external_led_task", 4096 * 1, NULL, 2, &xExternalLED, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT
    // EXTERNAL BUTTON
    xTaskCreatePinnedToCore(&external_button_task, "external_button_task", 4096 * 1, NULL, 2, &xExternalButton, 1);
#endif

#if CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT
    // EXTERNAL RGB LED BLINK
    xTaskCreatePinnedToCore(&vexternal_LoopRGBLedBlinkTask, "external_rgb_led_blink_task", 4096 * 1, NULL, 2, &xExternalRGBLedBlink, 1);
#endif

#if ( CONFIG_SOFTWARE_UNIT_4DIGIT_DISPLAY_SUPPORT || CONFIG_SOFTWARE_UNIT_6DIGIT_DISPLAY_SUPPORT )
    // DIGIT DISPLAY
    xTaskCreatePinnedToCore(&vLoopUnitDigitDisplayTask, "vLoopUnitDigitDisplayTask", 4096 * 1, NULL, 2, &xDigitDisplay, 1);
#endif

#if CONFIG_SOFTWARE_ESP_MQTT_SUPPORT
    // ESP_MQTT
    xTaskCreatePinnedToCore(&vLoopEspMqttClient_task, "vLoopEspMqttClient_task", 4096 * 1, NULL, 2, &xEspMqttClient, 1);
#endif

#if CONFIG_SOFTWARE_SPEAKER_SUPPORT
    // SPEAKER
    xTaskCreatePinnedToCore(&vLoopSpeaker_task, "vLoopSpeaker_task", 4096 * 1, NULL, 2, &xSpeaker, 1);
#endif

}
