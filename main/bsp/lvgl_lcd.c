#include "lvgl_lcd.h"
#include "core/lv_obj_style_gen.h"
#include "dog.c"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stdint.h>
#include "audio.h" // 添加对audio.h的包含，以使用i2c_bus_handle

#define TAG "display"
esp_lcd_panel_handle_t lcd_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_display_t *lvgl_disp = NULL;
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
static lv_obj_t *scr = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

extern const uint8_t dog_map[];
static lv_obj_t *img = NULL;    // 静态图像对象指针
static lv_obj_t *btn1 = NULL;   // 按钮1对象指针
static lv_obj_t *btn2 = NULL;   // 按钮2对象指针
static lv_obj_t *label1 = NULL; // 按钮1标签
static lv_obj_t *label2 = NULL; // 按钮2标签
static bool btn1_state = true;  // 按钮1状态
static bool btn2_state = true;  // 按钮2状态
lv_img_dsc_t img_dsc;           // 定义一个名为img_dsc的图像描述符
LV_FONT_DECLARE(font_puhui_16_4)
LV_FONT_DECLARE(lv_font_source_han_sans_sc_14_cjk)

LV_IMG_DECLARE(dog) // LVGL官网转换的图片，后缀加结构体说明

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
        lvgl_show_camera(fb->width, fb->height, fb->len,
                         fb->buf); // 使用LCD驱动显示camera数据
        // 释放帧缓冲区
        esp_camera_fb_return(fb);
        // 平衡流畅度和CPU负载
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void display_lcd_init(void)
{
    gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT,
                                    .pin_bit_mask = 1ULL << BK_LIGHT};
    // Initialize the GPIO of backlight
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    spi_bus_config_t buscfg = {
        .sclk_io_num = SPI_PCLK,
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 20 * EXAMPLE_LCD_H_RES * sizeof(uint16_t) // 缓冲区
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC,
        .cs_gpio_num = LCD_CS,
        .pclk_hz = LCD_PCLK,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10, // 降低深度
    };

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                             &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_handle));

    // Turn off backlight to avoid unpredictable display on the LCD screen while
    // initializing the LCD panel driver. (Different LCD screens may need
    // different levels)
    ESP_ERROR_CHECK(gpio_set_level(BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL));

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));

    // Turn on the screen
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, true));

    // Swap x and y axis (Different LCD screens may need different options)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, false));
    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, false));

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, false, false));
    // Turn on backlight (Different LCD screens may need different levels)
    ESP_ERROR_CHECK(gpio_set_level(BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
}

uint8_t code = 0;
void send_code()
{
    while (1)
    {
        if (xQueueReceive(btn_queue, &code, 0))
        {
            printf("code: %d\n", code);
            lvgl_port_lock(0);
            switch (code)
            {
            case 0: // 按键1：模拟点击按钮1
                lv_obj_send_event(btn1, LV_EVENT_CLICKED, NULL);
                break;
            case 1: // 按键2：模拟点击按钮2
                lv_obj_send_event(btn2, LV_EVENT_CLICKED, NULL);
                break;
                // 可以根据需要添加更多按键映射
            }
            lvgl_port_unlock();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static esp_err_t bsp_i2c_device_probe(uint8_t addr)
{
   // 使用新式 probe 函数（内部会发送 START + ADDR + STOP）
    return i2c_master_probe(i2c_bus_handle, addr, 1000); // 1000ms 超时
}

static esp_err_t app_touch_init(void)
{
    /* Initilize I2C */
    // const i2c_config_t i2c_conf = {
    //     .mode = I2C_MODE_MASTER,
    //     .sda_io_num = GPIO_NUM_17,
    //     .sda_pullup_en = GPIO_PULLUP_DISABLE,
    //     .scl_io_num = GPIO_NUM_18,
    //     .scl_pullup_en = GPIO_PULLUP_DISABLE,
    //     // .master.clk_speed = 400000
    // };
    // i2c_param_config(I2C_NUM_0, &i2c_conf);
    // i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0);

    /* Initialize touch HW */
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = 280,
        .rst_gpio_num = GPIO_NUM_NC, // Shared with LCD reset
        .int_gpio_num = IO_EXPANDER_PIN_NUM_4,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    if (ESP_OK == bsp_i2c_device_probe(ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS))
    {
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_CST816S_ADDRESS;
    }
    esp_err_t ret = esp_lcd_new_panel_io_i2c_v2(i2c_bus_handle, &tp_io_config, &tp_io_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize touch panel IO: %d", ret);
        return ret;
    }
    ret = esp_lcd_touch_new_i2c_cst816s(tp_io_handle, &tp_cfg, &touch_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize touch panel: %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Touch panel initialized successfully");
    return ESP_OK;
}

// TEST
esp_err_t display_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 4096,       /* LVGL task stack size */
        .task_affinity = 1,       /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG,
                        "LVGL port initialization failed");

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = lcd_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * 20 * sizeof(uint16_t),
        // .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .double_buffer = 1,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation =
            {
                .swap_xy = false,
                .mirror_x = true,
                .mirror_y = false,
            },
        .flags =
            {
                .buff_dma = true,
                .swap_bytes = false,
            },
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    // 添加触摸屏输入设备
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);
    // 创建LVGL显示标签

    scr = lv_scr_act();
    // 加锁
    /* Task lock */
    lvgl_port_lock(0);
    jpeg_queue = xQueueCreate(2, sizeof(camera_fb_t *)); // 队列保存3帧

    lv_obj_t *label = NULL;
    lv_obj_t *img = lv_image_create(lv_scr_act()); /* 创建图片部件 */
    lv_obj_set_style_pad_all(lv_scr_act(), 0, LV_PART_MAIN);
    lv_obj_set_size(img, lv_pct(1000), lv_pct(1000)); // 铺满内容区
    // // 文字显示
    // label = lv_label_create(scr);
    // lv_obj_set_width(label, EXAMPLE_LCD_H_RES);
    // lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    // lv_obj_set_style_text_font(label, &font_puhui_16_4, LV_PART_MAIN);
    // lv_label_set_text(label, "hi,乐鑫");
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 90);

    xTaskCreate(send_code, "send", 4096, NULL, 5, NULL);
    // 解锁
    lvgl_port_unlock();

    return ESP_OK;
}

void display_init(void)
{
    // LCD 初始化
    display_lcd_init();
    // LVGL 初始化
    app_touch_init();
    // 触摸屏初始化
    display_lvgl_init();
}

TaskHandle_t camera_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;

// 按钮1事件处理函数
static void btn1_event_handler(lv_event_t *e)
{
    // btn1_state = !btn1_state; // 切换状态
    printf("btn1_\n");
    lvgl_port_lock(0);
    if (btn1_state)
    {
        lv_label_set_text(label1, "状态1: ON");
        lv_obj_set_style_bg_color(btn1, lv_color_hex(0x00FF00), 0); // 绿色
        if (camera_task_handle == NULL && display_task_handle == NULL)
        {
            xTaskCreatePinnedToCore(camera_task, "camera_task", 4096, NULL, 4,
                                    &camera_task_handle, 1);
            xTaskCreatePinnedToCore(display_task, "display_task", 4096, NULL, 6,
                                    &display_task_handle, 1);
        }
    }
    lvgl_port_unlock();
}

// 按钮2事件处理函数
static void btn2_event_handler(lv_event_t *e)
{
    btn2_state = !btn2_state; // 切换状态
    lvgl_port_lock(0);
    if (btn2_state)
    {
        lv_label_set_text(label2, "状态2: ON");
        lv_obj_set_style_bg_color(btn2, lv_color_hex(0x00FF00), 0); // 绿色
        if (camera_task_handle != NULL)
        {
            vTaskDelete(camera_task_handle);
            camera_task_handle = NULL;
        }
        if (display_task_handle != NULL)
        {
            vTaskDelete(display_task_handle);
            display_task_handle = NULL;
        }
    }
    else
    {
        lv_label_set_text(label2, "状态2: OFF");
        lv_obj_set_style_bg_color(btn2, lv_color_hex(0xFF0000), 0); // 红色
    }
    lvgl_port_unlock();
}

// 创建控制按钮
void create_control_buttons(void)
{
    lvgl_port_lock(0);

    // 创建按钮1
    btn1 = lv_button_create(lv_scr_act());
    lv_obj_set_size(btn1, 120, 40);
    lv_obj_align(btn1, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn1, btn1_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0xFF0000), 0); // 初始红色
    lv_obj_set_style_text_color(btn1, lv_color_hex(0xFFFFFF), 0);

    // 创建按钮1标签
    label1 = lv_label_create(btn1);
    lv_obj_set_style_text_font(label1, &font_puhui_16_4, LV_PART_MAIN);
    lv_label_set_text(label1, "状态1: OFF");
    lv_obj_center(label1);

    // 创建按钮2
    btn2 = lv_button_create(lv_scr_act());
    lv_obj_set_size(btn2, 120, 40);
    lv_obj_align(btn2, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(btn2, btn2_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xFF0000), 0); // 初始红色
    lv_obj_set_style_text_color(btn2, lv_color_hex(0xFFFFFF), 0);

    // 创建按钮2标签
    label2 = lv_label_create(btn2);
    lv_obj_set_style_text_font(label2, &font_puhui_16_4, LV_PART_MAIN);
    lv_label_set_text(label2, "状态2: OFF");
    lv_obj_center(label2);

    lvgl_port_unlock();
}

void lvgl_show_camera(size_t width, size_t height, size_t len,
                      uint8_t *image_data)
{

    lvgl_port_lock(0); // 锁定LVGL端口
                       // ✅ 关键修复：转换 RGB565 数据格式（大端 → 小端）
                       //   for (int i = 0; i < len; i += 2) {
                       //     uint8_t temp = image_data[i];      // 低字节
                       //     image_data[i] = image_data[i + 1]; // 高字节
                       //     image_data[i + 1] = temp;          // 低字节
                       //   }
    // img_dsc.header.always_zero = 0;           // 始终为零
    img_dsc.header.w = width;  // 图像宽度
    img_dsc.header.h = height; // 图像高度
    img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565; // 图像颜色格式
    img_dsc.data = image_data;                  // 图像数据
    img_dsc.data_size = len;
    img_dsc.header.stride = width * 2;

    if (img == NULL)
    {
        img = lv_img_create(lv_scr_act()); /* 首次创建图片部件 */
        // ✅ 新增：设置控件大小 = 图像尺寸（避免缩放！）
        lv_disp_get_hor_res(NULL);

        // 创建控制按钮
        create_control_buttons();
    }
    lv_img_set_src(img, &img_dsc);            /* 更新图片源 */
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0); /* 居中对齐 */

    // 确保按钮在图像上方
    if (btn1 != NULL && btn2 != NULL)
    {
        lv_obj_move_foreground(btn1);
        lv_obj_move_foreground(btn2);
    }

    lv_refr_now(NULL); // 立即刷新屏幕
    lvgl_port_unlock();
}

void lvgl_draw_pictures(int x_start, int y_start, int x_end, int y_end,
                        const void *gImage)
{
    // 计算图片的像素字节数
    size_t pixels_byte_size = (x_end - x_start) * (y_end - y_start) * 2;
    // 在SPIRAM中分配内存
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(
        pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    // 如果内存不足，则打印错误信息并返回
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, gImage, pixels_byte_size); // 把图片数据拷贝到内存
    esp_lcd_panel_draw_bitmap(lcd_handle, x_start, y_start, x_end, y_end,
                              (uint16_t *)pixels); // 显示整张图片数据
    heap_caps_free(pixels);
}

