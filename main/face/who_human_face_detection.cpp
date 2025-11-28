#include "who_human_face_detection.hpp"
#include "esp_log.h"
#include "esp_camera.h"
// #include "dl_image.hpp"
#include "../components/esp-dl/include/image/dl_image.hpp"
// #include "human_face_detect_msr01.hpp"
// #include "human_face_detect_msr01.hpp"
#include "../components/esp-dl/include/model_zoo/human_face_detect_msr01.hpp"
// #include "human_face_detect_mnp01.hpp"
#include "../components/esp-dl/include/model_zoo/human_face_detect_mnp01.hpp"
#include "who_ai_utils.hpp"
#include "esp_camera.h"
#include "../bsp/lcd.h"
#include "../bsp/camera.h"
#include "../sd_card/sd_card.h"
// #include "esp32_s3_szp.h"

static const char *TAG = "human_face_detection";

static QueueHandle_t xQueueLCDFrame = NULL;
static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueSaveFrame = NULL;

static bool gEvent = true;
static bool gReturnFB = true;
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

int is_press;
// AI处理任务
static void task_process_ai(void *arg)
{
    camera_fb_t *frame = NULL;
    HumanFaceDetectMSR01 detector(0.3F, 0.3F, 10, 0.3F);
    HumanFaceDetectMNP01 detector2(0.4F, 0.3F, 10);

    while (true)
    {
        if (gEvent)
        {
            if (xQueueReceive(xQueueAIFrame, &frame, portMAX_DELAY))
            {
                std::list<dl::detect::result_t> &detect_candidates = detector.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3});
                std::list<dl::detect::result_t> &detect_results = detector2.infer((uint16_t *)frame->buf, {(int)frame->height, (int)frame->width, 3}, detect_candidates);

                if (detect_results.size() > 0)
                {
                    draw_detection_result((uint16_t *)frame->buf, frame->height, frame->width, detect_results);
                    print_detection_result(detect_results);
                    // xQueueSend(xQueueSaveFrame, &frame, portMAX_DELAY);
                    switch (is_press)
                    {
                    case ENROLL:
                        // 注册人脸
                        break;
                    case REOCONNIZE:
                        // 识别人脸
                        break;
                    default:
                        break;
                    }
                }
            }

            if (xQueueLCDFrame)
            {
                xQueueSend(xQueueLCDFrame, &frame, portMAX_DELAY);
            }
            else if (gReturnFB)
            {
                esp_camera_fb_return(frame);
            }
            else
            {
                free(frame);
            }
        }
    }
}

// lcd处理任务
static void task_process_lcd(void *arg)
{
    camera_fb_t *frame = NULL;

    while (true)
    {
        if (xQueueReceive(xQueueLCDFrame, &frame, portMAX_DELAY))
        {
            // lcd_draw_bitmap(0, 0, frame->width, frame->height, (uint16_t *)frame->buf);
            lcd_draw_pictures(0, 0, frame->width, frame->height, (uint16_t *)frame->buf);
            esp_camera_fb_return(frame);
        }
    }
}

// 摄像头处理任务
static void task_process_camera(void *arg)
{
    while (true)
    {
        camera_fb_t *frame = esp_camera_fb_get();
        if (frame)
        {
            xQueueSend(xQueueAIFrame, &frame, portMAX_DELAY);
        }
    }
}

static void task_save_face(void *arg)
{
    while (true)
    {
        camera_fb_t *frame = NULL;
        if (xQueueReceive(xQueueSaveFrame, &frame, portMAX_DELAY))
        {
            save_face_to_sdcard(frame);
            esp_camera_fb_return(frame);
        }
    }
}

// 人脸检测
void app_camera_ai_lcd(void)
{
    xQueueLCDFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueSaveFrame = xQueueCreate(2, sizeof(camera_fb_t *));

    xTaskCreatePinnedToCore(task_process_camera, "task_process_camera", 3 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_process_lcd, "task_process_lcd", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_process_ai, "task_process_ai", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_save_face, "task_save_face", 4 * 1024, NULL, 5, NULL, 0);
}
