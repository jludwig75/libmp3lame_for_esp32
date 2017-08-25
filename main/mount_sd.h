/*
 * mount_sd.h
 *
 *  Created on: Aug 25, 2017
 *      Author: jludwig
 */

#ifndef MAIN_MOUNT_SD_H_
#define MAIN_MOUNT_SD_H_


#include "esp_err.h"


esp_err_t mount_sd_card(const char *mount_path);
void unmount_sd_card();


#endif /* MAIN_MOUNT_SD_H_ */
