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
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_SK6812_SUPPORT )
#include "m5unit.h"
#endif

#if CONFIG_SOFTWARE_UI_SUPPORT
static lv_obj_t *active_screen;
static lv_obj_t *status_txtarea;

#if CONFIG_SOFTWARE_WIFI_SUPPORT
static lv_obj_t *wifi_label;
#endif

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT )
#define MOJI_DEGREESIGN  "°C"
static lv_obj_t *humidity_current;
static lv_obj_t *humidity_label;
static lv_obj_t *temperature_current;
static lv_obj_t *temperature_label;

static lv_obj_t *humidity_meter;
static lv_obj_t *temperature_meter;
static lv_style_t label_big_style;
static lv_style_t good_linemeter_style;
static lv_style_t over_linemeter_style;
static lv_style_t lower_linemeter_style;
static int32_t MAX_HUMIDITY_VALUE = 100;
static int32_t MIN_HUMIDITY_VALUE = 20;
static int32_t MAX_TEMPERATURE_VALUE = 45;
static int32_t MIN_TEMPERATURE_VALUE = -5;
static int32_t HUMIDITY_THRESHOLD_MAX = 70;
static int32_t HUMIDITY_THRESHOLD_MIN = 40;
static int32_t TEMPERATURE_THRESHOLD_MAX = 30;
static int32_t TEMPERATURE_THRESHOLD_MIN = 12;
#endif

#if CONFIG_SOFTWARE_RTC_SUPPORT
static lv_obj_t *datetime_txtarea;
#endif

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

#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT )
void ui_humidity_update(int32_t value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_HUMIDITY_VALUE) {
        value = MIN_HUMIDITY_VALUE;
    } else if (MAX_HUMIDITY_VALUE < value) {
        value = MAX_HUMIDITY_VALUE;
    }

    if (value < HUMIDITY_THRESHOLD_MIN) {
        lv_obj_add_style(humidity_meter, LV_LINEMETER_PART_MAIN, &lower_linemeter_style);
    } else if (HUMIDITY_THRESHOLD_MAX < value) {
        lv_obj_add_style(humidity_meter, LV_LINEMETER_PART_MAIN, &over_linemeter_style);
    } else {
        lv_obj_add_style(humidity_meter, LV_LINEMETER_PART_MAIN, &good_linemeter_style);
    }
    lv_label_set_text_fmt(humidity_current, "%d", value);
    lv_linemeter_set_value(humidity_meter, value);

    xSemaphoreGive(xGuiSemaphore);
}

void ui_temperature_update(int32_t value){
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    if (value < MIN_TEMPERATURE_VALUE) {
        value = MIN_TEMPERATURE_VALUE;
    } else if (MAX_TEMPERATURE_VALUE < value) {
        value = MAX_TEMPERATURE_VALUE;
    }

    if (value < TEMPERATURE_THRESHOLD_MIN) {
        lv_obj_add_style(temperature_meter, LV_LINEMETER_PART_MAIN, &lower_linemeter_style);
    } else if (TEMPERATURE_THRESHOLD_MAX < value) {
        lv_obj_add_style(temperature_meter, LV_LINEMETER_PART_MAIN, &over_linemeter_style);
    } else {
        lv_obj_add_style(temperature_meter, LV_LINEMETER_PART_MAIN, &good_linemeter_style);
    }
    lv_label_set_text_fmt(temperature_current, "%d", value);
    lv_linemeter_set_value(temperature_meter, value);

    xSemaphoreGive(xGuiSemaphore);
}
#endif


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


#if ( CONFIG_SOFTWARE_UNIT_ENV2_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV3_SUPPORT \
    || CONFIG_SOFTWARE_UNIT_ENV_SCD30_SUPPORT )
    // label_style
    lv_style_init(&label_big_style);
    lv_style_set_text_font(&label_big_style, LV_STATE_DEFAULT, &lv_font_montserrat_32);

    int32_t humidity_value = MIN_HUMIDITY_VALUE;
    humidity_meter = lv_linemeter_create(active_screen, NULL);
    lv_linemeter_set_range(humidity_meter, MIN_HUMIDITY_VALUE, MAX_HUMIDITY_VALUE);
    lv_linemeter_set_scale(humidity_meter, 240, 21);
    lv_obj_set_size(humidity_meter, 150, 150);
    lv_obj_align(humidity_meter, NULL, LV_ALIGN_CENTER, 80, 0);
    lv_linemeter_set_value(humidity_meter, humidity_value);
    humidity_current = lv_label_create(humidity_meter, NULL);
    lv_label_set_long_mode(humidity_current, LV_LABEL_LONG_DOT);
    lv_label_set_align(humidity_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(humidity_current, "%d", humidity_value);
    lv_obj_align(humidity_current, NULL, LV_ALIGN_CENTER, -16, -10);
    lv_obj_set_size(humidity_current, 60, 60);
    lv_obj_add_style(humidity_current, LV_STATE_DEFAULT, &label_big_style);
    humidity_label = lv_label_create(humidity_meter, NULL);
    lv_label_set_align(humidity_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(humidity_label, "%");
    lv_obj_align(humidity_label, NULL, LV_ALIGN_CENTER, 0, 40);

    int32_t temperature_value = MIN_TEMPERATURE_VALUE;
    temperature_meter = lv_linemeter_create(active_screen, NULL);
    lv_linemeter_set_range(temperature_meter, MIN_TEMPERATURE_VALUE, MAX_TEMPERATURE_VALUE);
    lv_linemeter_set_scale(temperature_meter, 240, 21);
    lv_obj_set_size(temperature_meter, 150, 150);
    lv_obj_align(temperature_meter, NULL, LV_ALIGN_CENTER, -80, 0);
    lv_linemeter_set_value(temperature_meter, temperature_value);
    temperature_current = lv_label_create(temperature_meter, NULL);
    lv_label_set_long_mode(temperature_current, LV_LABEL_LONG_DOT);
    lv_label_set_align(temperature_current, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text_fmt(temperature_current, "%d", temperature_value);
    lv_obj_align(temperature_current, NULL, LV_ALIGN_CENTER, -16, -10);
    lv_obj_set_size(temperature_current, 60, 60);
    lv_obj_add_style(temperature_current, LV_STATE_DEFAULT, &label_big_style);
    temperature_label = lv_label_create(temperature_meter, NULL);
    lv_label_set_align(temperature_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_text(temperature_label, MOJI_DEGREESIGN);
    lv_obj_align(temperature_label, NULL, LV_ALIGN_CENTER, 0, 40);

    // linemeter_style
    lv_style_init(&good_linemeter_style);
    lv_style_set_scale_grad_color(&good_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_set_line_color(&good_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_init(&over_linemeter_style);
    lv_style_set_scale_grad_color(&over_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    lv_style_set_line_color(&over_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    lv_style_init(&lower_linemeter_style);
    lv_style_set_scale_grad_color(&lower_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_CYAN);
    lv_style_set_line_color(&lower_linemeter_style, LV_STATE_DEFAULT, LV_COLOR_CYAN);
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

    xSemaphoreGive(xGuiSemaphore);
}
#endif