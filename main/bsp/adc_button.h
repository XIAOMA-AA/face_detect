#ifndef __ADC_BUTTON__
#define __ADC_BUTTON__
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_adc/adc_cali.h"
#include "iot_button.h"
#include "esp_adc/adc_oneshot.h"
#include "button_adc.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum
    {

        BSP_BUTTON_VOLUP = 0,
        BSP_BUTTON_VOLDOWN,
        BSP_BUTTON_SET,
        BSP_BUTTON_PLAY,
        BSP_BUTTON_MUTE,
        BSP_BUTTON_REC,
        BSP_BUTTON_MAIN,
        BSP_BUTTON_NUM,
    } bsp_button_t;

void adc_button_init(void);
extern QueueHandle_t btn_queue;
#ifdef __cplusplus
}
#endif

#endif

