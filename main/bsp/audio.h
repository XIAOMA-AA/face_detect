#ifndef  __AUDIO_H
#define  __AUDIO_H

#include "driver/i2c.h"
#include "esp_codec_dev.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "es8311.h"
#include "es8311_codec.h"
#include "esp_io_expander_tca9554.h"
#include "es7210.h"
#include "es7210_adc.h"
#include "driver/i2s.h"
#include "esp_codec_dev_defaults.h"
#include "driver/i2c_master.h"
#ifdef __cplusplus
extern "C"
{
#endif

void audio_init();
void audio_i2c_init();
void audio_codec_adc_init();
void audio_codec_dac_init();
void audio_i2s_init();
int audio_i2s_read(uint8_t *data, int len);
int audio_i2s_write(uint8_t *data,int len);

#ifdef __cplusplus
}
#endif
#endif