#ifndef __LCD_H
#define __LCD_H

#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
// #include "unity.h"
// #include "unity_test_runner.h"
#include "esp_lcd_panel_ops.h"  
#include "esp_lcd_panel_vendor.h"  

#include "esp_check.h"
#include "esp_err.h"
#include "string.h"
#ifdef __cplusplus
extern "C"
{
#endif
// Using SPI2 in the example, as it also supports octal modes on some targets
#define LCD_HOST SPI2_HOST
// To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many.
// More means more memory use, but less overhead for setting up / finishing transfers. Make sure 240
// is dividable by this.
#define PARALLEL_LINES 16
// The number of frames to show before rotate the graph
#define ROTATE_FRAME 30
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0 0 /*!< for 1-line SPI, this also refereed as MOSI */
#define EXAMPLE_PIN_NUM_PCLK 1
#define EXAMPLE_PIN_NUM_CS 46
#define EXAMPLE_PIN_NUM_DC 2
#define EXAMPLE_PIN_NUM_RST -1
#define EXAMPLE_PIN_NUM_BK_LIGHT 45

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 240
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8
// static SemaphoreHandle_t refresh_finish = NULL;
void LCD_ST7789_Init(void);
void lcd_draw_pictures(int x_start, int y_start, int x_end, int y_end, const void *gImage);


#ifdef __cplusplus
}

#endif
#endif

