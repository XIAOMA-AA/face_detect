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
QueueHandle_t jpeg_queue = NULL; // 队列保存JPEG数据
void camera_task(void *arg)
{
    esp_err_t err;
    camera_fb_t *fb = NULL;

    while (1)
    {
        // 获取摄像头帧
        fb = esp_camera_fb_get();
        // printf("camera_task\n");
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            continue;
        }
        // 发送帧到显示任务
        if (xQueueSend(jpeg_queue, &fb, portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to send frame to queue");
            esp_camera_fb_return(fb);
            continue;
        }

        // 短暂延迟以控制帧率
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void display_task(void *arg)
{
    // 声明变量
    camera_fb_t *fb = NULL;
    // 无限循环
    while (1)
    {
        // 从队列获取JPEG帧
        if (xQueueReceive(jpeg_queue, &fb, portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to receive frame from queue");
            continue;
        }
        // 直接使用摄像头输出的RGB565数据
        lvgl_show_camera(fb->width, fb->height, fb->len,fb->buf); // 使用LCD驱动显示camera数据
        // 释放帧缓冲区
        esp_camera_fb_return(fb);
        // 平衡流畅度和CPU负载
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void)
{
    jpeg_queue = xQueueCreate(2, sizeof(camera_fb_t *)); // 队列保存3帧

    // audio_init();
    // LCD_ST7789_Init();
    // nvs_init();
    camera_init();
    adc_button_init();
    display_init();
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
    xTaskCreatePinnedToCore(camera_task, "camera_task", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL, 6, NULL, 1);

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
