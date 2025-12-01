#ifndef __LVGL_LCD_H_
#define __LVGL_LCD_H_

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "stdio.h"

// #include "unity.h"
// #include "unity_test_runner.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "adc_button.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lvgl_port.h"
#include "../bsp/camera.h"
#include "lvgl.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "face/who_human_face_detection.hpp"

#define LCD_HOST SPI2_HOST
#define PARALLEL_LINES 16
#define ROTATE_FRAME 30
#define LCD_PCLK (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define SPI_MOSI GPIO_NUM_0
#define SPI_PCLK GPIO_NUM_1
#define LCD_CS GPIO_NUM_46
#define LCD_DC GPIO_NUM_2
#define EXAMPLE_PIN_NUM_RST -1
#define BK_LIGHT GPIO_NUM_45

#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 320
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
// 初始化 LCD 硬件
void display_lcd_init(void);
// 初始化 LVGL 及 UI 控件
esp_err_t display_lvgl_init(void);
extern const unsigned char gImage_XX[]; // 方法二图片数组
void display_button(void);
void create_control_buttons(void);

void display_init(void);
void test_lvgl_pic(void);
void lvgl_show_camera(size_t width, size_t height, size_t len,
                      uint8_t *image_data);
extern void lvgl_draw_pictures(int x_start, int y_start, int x_end, int y_end,
                               const void *gImage);
extern lv_obj_t *g_btn1; // 声明为全局变量
#ifdef __cplusplus
}
#endif
#endif
