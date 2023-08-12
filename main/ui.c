/*
 * AWS IoT EduKit - Core2 for AWS IoT EduKit
 * Cloud Connected Blinky v1.4.1
 * .c
 * 
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "m5core2.h"
#include "ui.h"

#define MAX_TEXTAREA_LENGTH 128

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BME680_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD40_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ADT7410_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BMP280_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_QMP6988_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_MHZ19C_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT )
#include "m5unit.h"
#endif

#if CONFIG_SOFTWARE_UI_SUPPORT
static lv_obj_t *active_screen;
static lv_obj_t *status_txtarea;

#if CONFIG_SOFTWARE_WIFI_SUPPORT
static lv_obj_t *wifi_label;
#endif

static float MAX_TEMPERATURE_VALUE = 50.0;
static float MIN_TEMPERATURE_VALUE = -20.0;
static float MAX_HUMIDITY_VALUE = 99.0;
static float MIN_HUMIDITY_VALUE = 1.0;
static int32_t MAX_CO2_VALUE = 5000;
static int32_t MIN_CO2_VALUE = 400;
static int32_t MAX_PRESSURE_VALUE = 1100;
static int32_t MIN_PRESSURE_VALUE = 850;


#if CONFIG_SOFTWARE_RTC_SUPPORT
static lv_obj_t *datetime_txtarea;
#endif

#define MOJI_DEGREESIGN  "°C"
static lv_obj_t *temperature_txtarea;
static lv_obj_t *humidity_txtarea;
static lv_obj_t *pressure_txtarea;
static lv_obj_t *co2_txtarea;
static lv_obj_t *temperature_current;
static lv_obj_t *humidity_current;
static lv_obj_t *pressure_current;
static lv_obj_t *co2_current;
static lv_obj_t *temperature_label;
static lv_obj_t *humidity_label;
static lv_obj_t *pressure_label;
static lv_obj_t *co2_label;
static lv_style_t label_big_style;

#if ( CONFIG_SOFTWARE_BUTTON_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT )
static lv_obj_t *button_label;
#endif

static char *TAG = "MY-UI";

void ui_status_set(char *statusTxt) {
    if( statusTxt != NULL ) {
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

        lv_textarea_set_text(status_txtarea, statusTxt);

        xSemaphoreGive(xGuiSemaphore);
    } else {
        ESP_LOGE(TAG, "datetime dateTxt is NULL!");
    }
}

#if CONFIG_SOFTWARE_WIFI_SUPPORT
void ui_wifi_label_update(bool state){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (state == false) {
        lv_label_set_text(wifi_label, "");
    } else {
#ifdef CONFIG_LV_PREDEFINED_DISPLAY_M5STACKCORE2
        char buffer[25];
        sprintf (buffer, "#0000ff %s #", LV_SYMBOL_WIFI);
        lv_label_set_text(wifi_label, buffer);
#else
        lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
#endif
    }
    xSemaphoreGive(xGuiSemaphore);
}
#endif

#if ( CONFIG_SOFTWARE_BUTTON_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT )
void ui_button_label_update(bool state){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (state == false) {
        lv_label_set_text(button_label, "");
    } else {
        lv_label_set_text(button_label, LV_SYMBOL_OK);
    }
    xSemaphoreGive(xGuiSemaphore);
}
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
void ui_datetime_set(char *dateTxt) {
    if( dateTxt != NULL ) {
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        lv_textarea_set_text(datetime_txtarea, dateTxt);
        xSemaphoreGive(xGuiSemaphore);
    } else {
        ESP_LOGE(TAG, "datetime dateTxt is NULL!");
    }
}
#endif

void ui_temperature_update(float value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_TEMPERATURE_VALUE) {
        value = MIN_TEMPERATURE_VALUE;
    } else if (MAX_TEMPERATURE_VALUE < value) {
        value = MAX_TEMPERATURE_VALUE;
    }

    lv_label_set_text_fmt(temperature_current, "%4.1f", value);

    xSemaphoreGive(xGuiSemaphore);
}

void ui_humidity_update(float value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_HUMIDITY_VALUE) {
        value = MIN_HUMIDITY_VALUE;
    } else if (MAX_HUMIDITY_VALUE < value) {
        value = MAX_HUMIDITY_VALUE;
    }

    lv_label_set_text_fmt(humidity_current, "%4.1f", value);

    xSemaphoreGive(xGuiSemaphore);
}

void ui_co2_update(int32_t value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_CO2_VALUE) {
        value = MIN_CO2_VALUE;
    } else if (MAX_CO2_VALUE < value) {
        value = MAX_CO2_VALUE;
    }

    lv_label_set_text_fmt(co2_current, "%4d", value);

    xSemaphoreGive(xGuiSemaphore);
}

void ui_pressure_update(int32_t value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_PRESSURE_VALUE) {
        value = MIN_PRESSURE_VALUE;
    } else if (MAX_PRESSURE_VALUE < value) {
        value = MAX_PRESSURE_VALUE;
    }

    lv_label_set_text_fmt(pressure_current, "%4d", value);

    xSemaphoreGive(xGuiSemaphore);
}


void ui_init() {
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    ESP_LOGI(TAG, "ui_init() start. hor:%d, ver:%d", lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    active_screen = lv_scr_act();

    status_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(status_txtarea, 300, 50);
    lv_obj_align(status_txtarea, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 10);
    lv_textarea_set_max_length(status_txtarea, MAX_TEXTAREA_LENGTH);
    lv_textarea_set_text_sel(status_txtarea, false);
    lv_textarea_set_cursor_hidden(status_txtarea, true);
    lv_textarea_set_one_line(status_txtarea, true);
    lv_textarea_set_text(status_txtarea, "");

#if CONFIG_SOFTWARE_WIFI_SUPPORT
    wifi_label = lv_label_create(active_screen, NULL);
    lv_obj_align(wifi_label,NULL,LV_ALIGN_IN_TOP_RIGHT, 0, 4);
    lv_label_set_text(wifi_label, "");
#ifdef CONFIG_LV_PREDEFINED_DISPLAY_M5STACKCORE2
    lv_label_set_recolor(wifi_label, true);
#endif
#endif

#if ( CONFIG_SOFTWARE_BUTTON_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_BUTTON_SUPPORT )
    button_label = lv_label_create(active_screen, NULL);
    lv_obj_align(button_label, NULL, LV_ALIGN_IN_TOP_RIGHT, -18, 4);
    lv_label_set_text(button_label, "");
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
    datetime_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(datetime_txtarea, 175, 50);
    lv_obj_align(datetime_txtarea, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 6);
    lv_textarea_set_text_align(datetime_txtarea, LV_LABEL_ALIGN_LEFT);
    lv_textarea_set_max_length(datetime_txtarea, MAX_TEXTAREA_LENGTH);
    lv_textarea_set_text_sel(datetime_txtarea, false);
    lv_textarea_set_cursor_hidden(datetime_txtarea, true);
    lv_textarea_set_one_line(datetime_txtarea, true);
#endif

    // New UI
    lv_style_init(&label_big_style);
    lv_style_set_text_font(&label_big_style, LV_STATE_DEFAULT, &lv_font_montserrat_48);

    char * temperature_str = "----";
    temperature_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(temperature_txtarea, 150, 80);
    lv_obj_align(temperature_txtarea, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 40);
    lv_textarea_set_text_sel(temperature_txtarea, false);
    lv_textarea_set_cursor_hidden(temperature_txtarea, true);
    lv_textarea_set_text(temperature_txtarea, "");
    temperature_current = lv_label_create(temperature_txtarea, NULL);
    lv_obj_align(temperature_current, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);
    lv_obj_add_style(temperature_current, LV_STATE_DEFAULT, &label_big_style);
    lv_label_set_align(temperature_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(temperature_current, "%s", temperature_str);

    temperature_label = lv_label_create(temperature_txtarea, NULL);
    lv_obj_align(temperature_label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 11, 15);
    lv_label_set_align(temperature_label, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_text(temperature_label, MOJI_DEGREESIGN);

    char * humidity_str = "----";
    humidity_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(humidity_txtarea, 150, 80);
    lv_obj_align(humidity_txtarea, NULL, LV_ALIGN_IN_TOP_RIGHT, -10, 40);
    lv_textarea_set_text_sel(humidity_txtarea, false);
    lv_textarea_set_cursor_hidden(humidity_txtarea, true);
    lv_textarea_set_text(humidity_txtarea, "");
    humidity_current = lv_label_create(humidity_txtarea, NULL);
    lv_obj_align(humidity_current, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);
    lv_obj_add_style(humidity_current, LV_STATE_DEFAULT, &label_big_style);
    lv_label_set_align(humidity_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(humidity_current, "%s", humidity_str);

    humidity_label = lv_label_create(humidity_txtarea, NULL);
    lv_obj_align(humidity_label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 16, 15);
    lv_label_set_align(humidity_label, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_text(humidity_label, "%");

    char * co2_str = "----";
    co2_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(co2_txtarea, 150, 80);
    lv_obj_align(co2_txtarea, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 10, -40);
    lv_textarea_set_text_sel(co2_txtarea, false);
    lv_textarea_set_cursor_hidden(co2_txtarea, true);
    lv_textarea_set_text(co2_txtarea, "");
    co2_current = lv_label_create(co2_txtarea, NULL);
    lv_obj_align(co2_current, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);
    lv_obj_add_style(co2_current, LV_STATE_DEFAULT, &label_big_style);
    lv_label_set_align(co2_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(co2_current, "%s", co2_str);

    co2_label = lv_label_create(co2_txtarea, NULL);
    lv_obj_align(co2_label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -5, 15);
    lv_label_set_align(co2_label, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_text(co2_label, "ppm");

    char * pressure_str = "----";
    pressure_txtarea = lv_textarea_create(active_screen, NULL);
    lv_obj_set_size(pressure_txtarea, 150, 80);
    lv_obj_align(pressure_txtarea, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -10, -40);
    lv_textarea_set_text_sel(pressure_txtarea, false);
    lv_textarea_set_cursor_hidden(pressure_txtarea, true);
    lv_textarea_set_text(pressure_txtarea, "");
    pressure_current = lv_label_create(pressure_txtarea, NULL);
    lv_obj_align(pressure_current, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);
    lv_obj_add_style(pressure_current, LV_STATE_DEFAULT, &label_big_style);
    lv_label_set_align(pressure_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(pressure_current, "%s", pressure_str);

    pressure_label = lv_label_create(pressure_txtarea, NULL);
    lv_obj_align(pressure_label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 15);
    lv_label_set_align(pressure_label, LV_LABEL_ALIGN_RIGHT);
    lv_label_set_text(pressure_label, "hPa");


    xSemaphoreGive(xGuiSemaphore);
}
#endif