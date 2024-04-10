/*
 * Raspberry Pi Pico littlefs USB Flash Memory Interface
 *
 * Every time you press the BOOTSEL button on the Raspberry Pi Pico,
 * append a log to the littlefs `SENSOR.TXT`. The host PC can mount
 * the Pico like a USB Mass storage class flash memory device and
 * read `SENSOR.TXT`.
 * Hold the button down for 10 seconds to format Pico's flash memory.
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdlib.h>
#include <string.h>
#include <bsp/board.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include <lfs.h>
#include "bootsel_button.h"

extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern bool usb_device_enable;                         // usb_msc_driver.c

#define FILENAME  "SENSOR.TXT"

#define README_TXT \
"Raspberry Pi Pico littlefs USB Flash Memory Interface\n" \
"\n" \
"Every time you press the BOOTSEL button on the Raspberry Pi Pico,\n" \
"append a log to the littlefs `SENSOR.TXT`. The host PC can mount\n" \
"the Pico like a USB Mass storage class flash memory device and\n" \
"read `SENSOR.TXT`.\n" \
"Hold the button down for 10 seconds to format Pico's flash memory.\n"


/*
 * Format the file system if it does not exist
 */
static void test_filesystem_and_format_if_necessary(bool force_format) {
    lfs_t fs;
    if ((lfs_mount(&fs, &lfs_pico_flash_config) != 0) || force_format) {
        printf("Format the onboard flash memory with littlefs\n");

        lfs_format(&fs, &lfs_pico_flash_config);

        lfs_mount(&fs, &lfs_pico_flash_config);
        lfs_file_t f;
        lfs_file_open(&fs, &f, "README.TXT", LFS_O_RDWR|LFS_O_CREAT);
        lfs_file_write(&fs, &f, README_TXT, strlen(README_TXT));
        lfs_file_close(&fs, &f);
    }
    lfs_unmount(&fs);
}

/*
 * Log clicks on the BOOTSEL button to a log file.
 * Press and hold the button for 10 seconds to initialize the file system.
 */
static void sensor_logging_task(void) {
    static bool last_status = false;
    static int count = 0;
    bool button = bb_get_bootsel_button();
    static uint64_t long_push = 0;

    if (last_status != button && button) {  // Push BOOTSEL button
        count += 1;
        lfs_t fs;
        printf("Update %s\n", FILENAME);
        int err = lfs_mount(&fs, &lfs_pico_flash_config);
        if (err) {
            printf("can't mount littlefs: err=%d\n", err);
            last_status = button;
            return;
        }

        lfs_file_t f;
        lfs_file_open(&fs, &f, FILENAME, LFS_O_RDWR|LFS_O_APPEND|LFS_O_CREAT);
        uint8_t buffer[512];
        snprintf(buffer, sizeof(buffer), "click=%d\n", count);
        lfs_file_write(&fs, &f, buffer, strlen(buffer));
        printf(buffer);
        lfs_file_close(&fs, &f);
        lfs_unmount(&fs);

        usb_device_enable = true;  // Enable USB device
    }
    last_status = button;

    if (button) {
        long_push++;
    } else {
        long_push = 0;
    }
    if (long_push > 125000) { // Long-push BOOTSEL button
        test_filesystem_and_format_if_necessary(true);
        count = 0;
        long_push = 0;
    }
}

int main(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);
    stdio_init_all();

    test_filesystem_and_format_if_necessary(false);
    while (true) {
        sensor_logging_task();
        tud_task();
    }
}
