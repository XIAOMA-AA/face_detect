/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_chip_info.h"
#include "esp_log.h"
// #include "esp_flash.h"
#include "esp_system.h"
#include "face/who_human_face_detection.hpp"
#include "face/who_ai_utils.hpp"
#include "bsp/audio.h"
#include "bsp/camera.h"
#include "bsp/lcd.h"
#include "sd_card/sd_card.h"
#include "bsp/adc_button.h"
#include "nvs_flash.h"
#include "bsp/lvgl_lcd.h"

#define TAG "APP_MAIN"
void nvs_init()
{
    // ✅ 关键修复：初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle handle;
    esp_err_t err = nvs_open("fr", NVS_READWRITE, &handle);
    if (err == ESP_OK)
    {
        ESP_LOGI("NVS", "fr partition is successfully opened.");
    }
    else
    {
        ESP_LOGE("NVS", "Failed to open fr partition: %d", err);
    }
}


extern "C" void app_main(void)
{

    // audio_init();
    // LCD_ST7789_Init();
    // nvs_init();
    camera_init();
    adc_button_init();
    display_init();
    create_control_buttons();
    // test_lvgl_pic();
    // display_button();
    // lvgl_label_enroll_reconize();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    // app_camera_ai_lcd();
    // sd_card_init();

    // sd_card_fileList();

    // sd_card_write_test();
    // sd_card_read_test();
    // // sd_card_format();
    // sd_card_uninit();
   

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
