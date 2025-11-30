#include "adc_button.h"
#include "lvgl_lcd.h"
static const char *TAG = "BUTTON";

#define BUTTON_NUM 6
static button_handle_t g_btns[BUTTON_NUM] = {0};
QueueHandle_t btn_queue = NULL;

// 按钮事件回调函数
static void button_event_cb(void *arg, void *data)
{
    // 从按钮句柄获取事件状态
    button_event_t event = iot_button_get_event(arg);
    uint8_t btn_idx = data;
    // ESP_LOGI(TAG, "BTN[%d] %s", (int)data, iot_button_get_event_str(event));
    if (BUTTON_PRESS_DOWN == event)
    {
        printf("按键[%d] 按下\n", btn_idx);
        // 后续通过队列发送按键索引
        xQueueSend(btn_queue, &btn_idx, portMAX_DELAY);

        // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        // xQueueSendFromISR(btn_queue, &btn_idx, &xHigherPriorityTaskWoken);
        // if (xHigherPriorityTaskWoken)
        // {
        //     portYIELD_FROM_ISR();
        // }
    }
}

void adc_button_init(void)
{
    /** ESP32-S3-Korvo2 board */
    btn_queue = xQueueCreate(1, sizeof(uint8_t));
    const button_config_t btn_cfg = {0};
    button_adc_config_t btn_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = 4,
    };

    button_handle_t btns[6] = {NULL};

    const uint16_t vol[6] = {380, 820, 1180, 1570, 1980, 2410};
    for (size_t i = 0; i < 6; i++)
    {
        btn_adc_cfg.button_index = i;
        if (i == 0)
        {
            btn_adc_cfg.min = (0 + vol[i]) / 2;
        }
        else
        {
            btn_adc_cfg.min = (vol[i - 1] + vol[i]) / 2;
        }

        if (i == 5)
        {
            btn_adc_cfg.max = (vol[i] + 3000) / 2;
        }
        else
        {
            btn_adc_cfg.max = (vol[i] + vol[i + 1]) / 2;
        }

        esp_err_t ret = iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &btns[i]);
        iot_button_register_cb(btns[i], BUTTON_PRESS_DOWN, NULL, button_event_cb, (void *)i);
    }
}