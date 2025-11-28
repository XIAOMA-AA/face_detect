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
#include "esp_chip_info.h"
// #include "esp_flash.h"
#include "esp_system.h"
#include "face/who_human_face_detection.hpp"
#include "face/who_ai_utils.hpp"
#include "bsp/audio.h"
#include "bsp/camera.h" 
#include "bsp/lcd.h"
#include "sd_card/sd_card.h"
#include "bsp/adc_button.h"

extern "C" void app_main(void)
{
    // audio_init();
    // camera_init();
    // LCD_ST7789_Init();
    // vTaskDelay(500 / portTICK_PERIOD_MS);
    // app_camera_ai_lcd();
    // sd_card_init();

    adc_button_init();
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
