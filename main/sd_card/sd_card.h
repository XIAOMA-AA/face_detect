#pragma once

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "diskio.h"
#include <dirent.h> 

#include "bsp/camera.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void sd_card_init(void);

    void sd_card_uninit(void);

    void sd_card_write_test(void);

    void sd_card_read_test(void);

    void save_face_to_sdcard(camera_fb_t *frame);

    void sd_card_fileList(void);

    void sd_card_format(void);

#ifdef __cplusplus
}
#endif