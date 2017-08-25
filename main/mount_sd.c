/*
 * mount_sd.c
 *
 *  Created on: Aug 25, 2017
 *      Author: jludwig
 */
#include "mount_sd.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"


#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

static const char* TAG = "mount_sd";


esp_err_t mount_sd_card(const char *mount_path)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // host.flags = SDMMC_HOST_FLAG_1BIT;

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and formatted
    // in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_path, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return ret;
    }

    return ESP_OK;
}

void unmount_sd_card()
{
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(TAG, "Card unmounted");
}




