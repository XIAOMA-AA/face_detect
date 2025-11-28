#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum
{
    ENROLL = 0,
    REOCONNIZE =1,
};



void app_camera_ai_lcd(void);
