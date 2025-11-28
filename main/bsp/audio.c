#include "audio.h"

#define I2C_NUM 0
#define I2S_NUM 0l

#define I2C_SCL_PIN 18
#define I2C_SDA_PIN 17

#define I2S_MCK_PIN 16     //  主时钟。MCK的频率 = 128或者256或者512 * 采样频率  (可选)
#define I2S_BCK_PIN 9      //  串行时钟(位时钟)  用于同步位数据传输。 BCK的频率 = 声道数 * 采样频率 * 采样位数
#define I2S_DATA_WS_PIN 45 // 字段选择信号, 用于切换左右声道
#define I2S_DATA_OUT_PIN 8 // 数据输出
#define I2S_DATA_IN_PIN 10 // 数据输入

#define PA_PIN 48

#define ES7210_I2C_ADDR (0x40)

#define TAG "audio"
i2s_chan_handle_t tx_handle = NULL; // I2S tx channel handler
i2s_chan_handle_t rx_handle = NULL; // I2S rx channel handler

audio_codec_data_if_t *record_data_if = NULL;
audio_codec_ctrl_if_t *record_ctrl_if = NULL;
audio_codec_if_t *record_codec_if = NULL;
esp_codec_dev_handle_t record_dev = NULL;

audio_codec_data_if_t *play_data_if = NULL;
audio_codec_ctrl_if_t *play_ctrl_if = NULL;
audio_codec_gpio_if_t *play_gpio_if = NULL;
audio_codec_if_t *play_codec_if = NULL;
esp_codec_dev_handle_t play_dev = NULL;

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

/**
 * @brief I2C初始化
 *
 */
void audio_i2c_init()
{
    // i2c_config_t conf = {
    //     .mode = I2C_MODE_MASTER,
    //     .sda_io_num = I2C_SDA_PIN,
    //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
    //     .scl_io_num = I2C_SCL_PIN,
    //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
    //     .master.clk_speed = 400000,
    // };
    // i2c_param_config(I2C_NUM, &conf);
    // i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);

    i2c_master_bus_config_t i2c_master_bus = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    esp_err_t err = i2c_new_master_bus(&i2c_master_bus, &i2c_bus_handle);
    if (err == ESP_OK)
    {
        printf("i2c_new_master_bus success\n");
    }
}

/**
 * @brief I2S初始化
 *
 */
void audio_i2s_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

    // i2s_std_config_t i2s_std = {
    //     .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    //     .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO),
    //     .gpio_cfg = {
    //         .bclk = I2S_BCK_PIN,
    //         .mclk = I2S_MCK_PIN,
    //         .din = I2S_DATA_IN_PIN,
    //         .dout = I2S_DATA_OUT_PIN,
    //         .ws = I2S_DATA_WS_PIN}};
    i2s_std_config_t i2s_std = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_MCK_PIN,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_DATA_WS_PIN,
            .dout = I2S_DATA_OUT_PIN,
            .din = I2S_DATA_IN_PIN,
        },
    };
    i2s_channel_init_std_mode(tx_handle, &i2s_std);
    i2s_channel_init_std_mode(rx_handle, &i2s_std);
    i2s_channel_enable(tx_handle);
    i2s_channel_enable(rx_handle);
}

/**
 * @brief 7210adc初始化
 *
 */
void audio_codec_adc_init()
{
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle};
    record_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES7210_CODEC_DEFAULT_ADDR,
        .port = I2C_NUM,
        .bus_handle = i2c_bus_handle,
    };
    record_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = record_ctrl_if,
        .mic_selected = ES7210_ADDRESS_01 // | ES7210_ADDRESS_01  
    };
    record_codec_if = es7210_codec_new(&es7210_cfg);

    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = record_codec_if,
        .data_if = record_data_if,
        .dev_type = ESP_CODEC_DEV_TYPE_IN};
    record_dev = esp_codec_dev_new(&dev_cfg);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,
        .channel = 1, // 1
        .bits_per_sample = 16};
    esp_codec_dev_set_in_gain(record_dev, 30);
    // esp_codec_dev_set_in_channel_gain(record_dev, 2, 30);
    esp_codec_dev_open(record_dev, &fs);
}

/**
 * @brief 8311初始化
 *
 */
void audio_codec_dac_init()
{
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle};
    play_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle};
    play_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    play_gpio_if = audio_codec_new_gpio();

    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .ctrl_if = play_ctrl_if,
        .gpio_if = play_gpio_if,
        .pa_pin = PA_PIN,
        .pa_reverted = false,                      // 不反转 PA 的控制逻辑
        .invert_mclk = false,
        .invert_sclk = false,
        .use_mclk = true,    // 使用主时钟
        .digital_mic = false, // 不使用数字麦克风(集成了 ADC 的麦克风)
    };
    play_codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = play_codec_if,
        .data_if = play_data_if,
        .dev_type = ESP_CODEC_DEV_TYPE_OUT};
    play_dev = esp_codec_dev_new(&dev_cfg);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .mclk_multiple = 256,
        .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0)
    };

    esp_codec_dev_set_out_vol(play_dev, 70);
    esp_codec_dev_open(play_dev, &fs);
}

/**
 * @brief I2S读取数据
 *
 * @param data 数据指针
 * @param len 数据长度
 * @return int 读取长度
 */

int audio_i2s_read(uint8_t *data, int len)
{
    return esp_codec_dev_read(record_dev, data, len);
}

/**
 * @brief I2S写入数据，播放音频
 *
 * @param data 数据指针
 * @param len 数据长度
 * @return int 写入长度
 */

int audio_i2s_write(uint8_t *data, int len)
{
    return esp_codec_dev_write(play_dev, data, len);
}

esp_io_expander_handle_t tca9554_handle;
void TCA9554_Init(void)
{
    // if (esp_io_expander_new_i2c_tca9554(I2C_NUM, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &tca9554_handle) != ESP_OK)
    // {
    //     printf("esp_io_expander_new_tca9554 failed\n");
    // }
    // else
    // {
    //     printf("esp_io_expander_new_tca9554 success\n");
    // }

    if (ESP_OK == (esp_io_expander_new_i2c_tca9554(i2c_bus_handle, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &tca9554_handle)))
    {
        printf("设备探测成功，无需额外操作\n");
    }
    else if (ESP_OK == (esp_io_expander_new_i2c_tca9554(i2c_bus_handle, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_001, &tca9554_handle)))
    {
        // esp_io_expander_new_i2c_tca9554()
        printf("尝试另一种地址成功，继续其他操�?\n");
        // 继续后续操作
    }
    else
    {
        printf("设备探测失败\n");
        return; // 最终返�?
    }

    esp_io_expander_set_dir(tca9554_handle, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_7, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_level(tca9554_handle, IO_EXPANDER_PIN_NUM_0, 1);
    esp_io_expander_set_level(tca9554_handle, IO_EXPANDER_PIN_NUM_1, 1);
    esp_io_expander_set_level(tca9554_handle, IO_EXPANDER_PIN_NUM_7, 1);
}

void audio_init()
{
    audio_i2c_init();
    audio_i2s_init();
    audio_codec_dac_init();
    audio_codec_adc_init();
    TCA9554_Init();
}