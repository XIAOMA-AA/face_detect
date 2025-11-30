#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl_lcd.h"

// **新增状态定义**
typedef enum
{
    MODE_CAMERA_ONLY = 0, // 默认：仅显示
    MODE_REGISTER,        // 注册
    MODE_RECOGNIZE,       // 识别
} ai_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

void app_camera_ai_lcd(void);  // 函数声明

#ifdef __cplusplus
}
#endif
