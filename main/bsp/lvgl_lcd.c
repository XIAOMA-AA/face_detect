#include "lvgl_lcd.h"
#include "dog.c"
#define TAG "display"
esp_lcd_panel_handle_t lcd_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_display_t *lvgl_disp = NULL;
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
static lv_obj_t *scr = NULL;

extern const uint8_t dog_map[];

LV_FONT_DECLARE(font_puhui_16_4)
LV_FONT_DECLARE(lv_font_source_han_sans_sc_14_cjk)

LV_IMG_DECLARE(dog) // LVGL官网转换的图片，后缀加结构体说明

void display_lcd_init(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_handle));

    // Turn off backlight to avoid unpredictable display on the LCD screen while initializing
    // the LCD panel driver. (Different LCD screens may need different levels)
    ESP_ERROR_CHECK(gpio_set_level(BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL));

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));

    // Turn on the screen
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_handle, true));

    // Swap x and y axis (Different LCD screens may need different options)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, true));
    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(lcd_handle, false));

    ESP_ERROR_CHECK(esp_lcd_panel_mirror(lcd_handle, true, false));
    // Turn on backlight (Different LCD screens may need different levels)
    ESP_ERROR_CHECK(gpio_set_level(BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
}
lv_obj_t *g_btn1 = NULL; // 全局变量
static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("Clicked\n");
        LV_LOG_USER("Clicked");
        // lvgl_port_lock(0);
        // lv_obj_clean(scr);
        // lvgl_port_unlock();
        // app_camera_ai_lcd();
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
        LV_LOG_USER("Toggled");
    }
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
            if (code == 0)
            {
                lv_obj_send_event(g_btn1, LV_EVENT_CLICKED, NULL);
            }
            else if (code == 1)
            {
                lv_obj_send_event(g_btn1, LV_EVENT_VALUE_CHANGED, NULL);
            }
            lvgl_port_unlock();
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

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
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

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
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    // 创建LVGL显示标签

    scr = lv_scr_act();
    // 加锁
    /* Task lock */
    lvgl_port_lock(0);

    lv_obj_t *label = NULL;
    lv_obj_t *img = lv_image_create(lv_scr_act()); /* 创建图片部件 */
    // 获取当前屏幕的实际分辨率（动态获取，最可靠！）
    // lv_coord_t scr_w = lv_disp_get_hor_res(NULL); // 屏幕宽度
    // lv_coord_t scr_h = lv_disp_get_ver_res(NULL); // 屏幕高度
    // lv_obj_set_size(img, scr_w, scr_h);
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

void display_button(void)
{
    lvgl_port_lock(0);
    lv_obj_t *label1;

    g_btn1 = lv_button_create(scr);
    lv_obj_add_event_cb(g_btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(g_btn1, LV_ALIGN_CENTER, 0, -40);
    lv_obj_remove_flag(g_btn1, LV_OBJ_FLAG_PRESS_LOCK);

    label1 = lv_label_create(g_btn1);
    lv_label_set_text(label1, "Button");
    lv_obj_center(label1);

    lv_obj_t *btn2 = lv_button_create(scr);
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label1 = lv_label_create(btn2);
    lv_label_set_text(label1, "Toggle");
    lv_obj_center(label1);
    lvgl_port_unlock();
}

void display_init(void)
{
    // LCD 初始化
    display_lcd_init();
    // LVGL 初始化
    display_lvgl_init();
}

void test_lvgl_pic(void)
{
    // 锁定LVGL端口
    lvgl_port_lock(0);
    lv_obj_t *img = lv_img_create(lv_scr_act()); /* 创建图片部件 */
    lv_img_set_src(img, &dog);                   /* 设置图片源 */
    lvgl_port_unlock();                          // 解锁LVGL端口
}

static lv_obj_t *img = NULL; // 静态图像对象指针
lv_img_dsc_t img_dsc;        // 定义一个名为img_dsc的图像描述符

void lvgl_show_camera(size_t width, size_t height, size_t len, uint8_t *image_data)
{

    lvgl_port_lock(0); // 锁定LVGL端口
                       // ✅ 关键修复：转换 RGB565 数据格式（大端 → 小端）
    for (int i = 0; i < len; i += 2)
    {
        uint8_t temp = image_data[i];      // 低字节
        image_data[i] = image_data[i + 1]; // 高字节
        image_data[i + 1] = temp;          // 低字节
    }
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
    }
    lv_img_set_src(img, &img_dsc);            /* 更新图片源 */
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0); /* 居中对齐 */
    lv_refr_now(NULL);                        // 立即刷新屏幕
    lvgl_port_unlock();
}

void lvgl_draw_pictures(int x_start, int y_start, int x_end, int y_end, const void *gImage)
{
    // 计算图片的像素字节数
    size_t pixels_byte_size = (x_end - x_start) * (y_end - y_start) * 2;
    // 在SPIRAM中分配内存
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    // 如果内存不足，则打印错误信息并返回
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, gImage, pixels_byte_size);                                                  // 把图片数据拷贝到内存
    esp_lcd_panel_draw_bitmap(lcd_handle, x_start, y_start, x_end, y_end, (uint16_t *)pixels); // 显示整张图片数据
    heap_caps_free(pixels);
}