#include "sd_card.h"

#include "string.h"
#define MOUNT_POINT "/sdcard"
#define TAG "SD_CARD"
static sdmmc_card_t *card;

void sd_card_init(void)
{
    esp_err_t ret;
    // 配置SD卡连接
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    const char mount_point[] = MOUNT_POINT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 1;
    slot_config.clk = GPIO_NUM_15;
    slot_config.cmd = GPIO_NUM_7;
    slot_config.d0 = GPIO_NUM_4;

    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
    }
    sdmmc_card_print_info(stdout, card);
}

void sd_card_uninit(void)
{
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
}

void sd_card_write_test(void)
{
    // First create a file.
    esp_err_t ret;
    const char *file_hello = MOUNT_POINT "/hello.txt";
    char data[64];
    snprintf(data, sizeof(data), "%s %s!\n", "Hello", card->cid.name);
    // ret = s_example_write_file(file_hello, data);
    FILE *f = fopen(file_hello, "w");
    if (f == NULL)
    {
        ESP_LOGI(TAG, "Failed to open file for writing");
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");
}

void sd_card_read_test(void)
{
    // First create a file.
    esp_err_t ret;
    const char *file_hello = MOUNT_POINT "/hello.txt";
    char data[64];
    // ret = s_example_read_file(file_hello, data, sizeof(data));
    FILE *f = fopen(file_hello, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos)
    {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);
}

// 保存图像到SD卡
void save_face_to_sdcard(camera_fb_t *frame)
{
    // printf("save_face_to_sdcard\n");
    // 生成文件名
    static int file_counter = 0;
    char file_name[32];
    snprintf(file_name, sizeof(file_name), "/sdcard/face_%d.jpg", file_counter++);

    // 打开文件以写入图像数据
    FILE *file = fopen(file_name, "wb");
    if (file == NULL)
    {
        ESP_LOGI(TAG, "Failed to open file for writing");
        return;
    }

    // 写入图像数据
    size_t len = frame->len;
    size_t written = fwrite(frame->buf, 1, len, file);
    if (written != len)
    {
        ESP_LOGI(TAG, "Failed to write entire image data to file");
    }
    else
    {
        ESP_LOGI(TAG, "Saved face to %s", file_name);
    }

    // 关闭文件
    fclose(file);
}

/**
 * @brief 列出SD卡根目录下的所有文件
 *
 */
void sd_card_fileList(void)
{
    // 列出SD卡根目录下的所有文件
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL)
    {
        ESP_LOGI(TAG, "Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        ESP_LOGI(TAG, "File LIST: %s", entry->d_name);
    }

    closedir(dir);
}

/**
 * @brief 格式化SD卡
 *
 */
void sd_card_format(void)
{
    esp_err_t ret = esp_vfs_fat_sdcard_format(MOUNT_POINT, card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to format SD card (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "SD card formatted successfully");
    }
}
