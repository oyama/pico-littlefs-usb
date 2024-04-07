/*
 * USB mass storage class driver that mimics littlefs to FAT12 file system.
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <math.h>
#include <bsp/board.h>
#include <tusb.h>
#include <lfs.h>

#define LITTLE_ENDIAN16(x) (x)
#define LITTLE_ENDIAN32(x) (x)


typedef struct {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} fat_dir_entry_t;

static void print_block(uint8_t *buffer, size_t l) {
    for (size_t i = 0; i < l; ++i) {
        printf("%02x", buffer[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(", ");
    }
}

extern const struct lfs_config lfs_pico_flash_config;

static bool ejected = false;
bool usb_device_enable = false;

enum {
  DISK_BLOCK_NUM  = 16, // 8KB is the smallest size that windows allow to mount
  DISK_BLOCK_SIZE = 512
};

uint8_t fat_disk_image[1][DISK_BLOCK_SIZE] = {
  //------------- Block0: Boot Sector -------------//
  {
      0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x01, 0x01, 0x00,
      0x01, 0x10, 0x00, 0x10, 0x00, 0xF8, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x29, 0x34, 0x12, 0x00, 0x00, 'l' , 'i' , 't' , 't' , 'l' ,
      'e' , 'f' , 's' , 'U' , 'S' , 'B' , 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x00, 0x00,
      // Zero up to 2 last bytes of FAT magic code
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
  }
};

static uint8_t fat_table[512] = {
    0xF8, 0xFF, 0xFF, 0xFF, 0x0F
};

static fat_dir_entry_t root_dir_entry[16] = {
  { "littlefsUSB", 0x08, 0x00, 0x00, 0x0000, 0x0000, 0x0000, 0x0000, 0x4F6D, 0x6543, 0x0000, 0x00000000 },
};

static void mimic_fat_table(void *buffer, uint32_t bufsize) {
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);

    //printf("mimic_fat_table\n");
    lfs_dir_t dir;
    lfs_dir_open(&fs, &dir, "/");
    uint16_t cluster = 1;
    while (true) {
        struct lfs_info info;
        int res = lfs_dir_read(&fs, &dir, &info);
        if (res < 0) {
            printf("lfs_dir_read: %d\n", res);
            break;
        } else if (res == 0) {
            break;
        }
        if (info.type == LFS_TYPE_REG) {
            uint16_t size = info.size;
            for (int16_t size = info.size; size > 0; size -= 512) {
                cluster++;

                uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % 512;
                //printf("  name=%s,size=%u, offset=%u\n", info.name, info.size, offset);

                uint16_t entry = size > 512 ? cluster + 1 : 0xFFF;
                if (cluster & 0x01) {
                    fat_table[offset] = (fat_table[offset] & 0x0F) | (entry << 4);
                    fat_table[offset + 1] = entry >> 4;
                    //print_block(fat_table, 16);
                } else {
                    fat_table[offset] = entry;
                    fat_table[offset + 1] = (fat_table[offset + 1] & 0xF0) | ((entry >> 8) & 0x0F);
                    //print_block(fat_table, 16);

                }
            }
        }
    }

    lfs_dir_close(&fs, &dir);
    lfs_unmount(&fs);
    memcpy(buffer, fat_table, bufsize);
}

static int lookup_fat_table(int cluster) {
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % 512;
    int16_t result = 0;
    if (cluster & 0x01) {
        result = (fat_table[offset] >> 4) | ((uint16_t)fat_table[offset + 1] << 4);
    } else {
        result = fat_table[offset] | ((uint16_t)(fat_table[offset + 1] & 0x0F) << 8);
    }
    return result;
}

static void fat_short_filename(uint8_t *short_filename, const uint8_t *filename) {
    uint8_t buffer[128];
    strncpy(buffer, filename, sizeof(buffer));

    uint8_t *basename = strtok(buffer, ".");
    uint8_t *ext = strtok(NULL, ".");
    sprintf(short_filename, "%-8s%-3s", basename, ext);
}

static void fls_filename(uint8_t *filename, const uint8_t *short_filename) {
    uint8_t buffer[12];
    strncpy(buffer, short_filename, sizeof(buffer));

    uint8_t basename[8 + 1];
    uint8_t fileext[3 + 1];
    for (int i = 0; i < 8; i++) {
      if (buffer[i] != ' ') {
        basename[i] = buffer[i];
      } else {
        basename[i] = '\0';
        break;
      }
    }
    basename[sizeof(basename)-1] = '\0';
    for (int i = 0; i < 3; i++) {
      if (buffer[8+i] != ' ') {
        fileext[i] = buffer[8+i];
      } else {
        fileext[i] = '\0';
        break;
      }
    }
    fileext[sizeof(fileext)-1] = '\0';

    sprintf(filename, "%s.%s", basename, fileext);
    //printf("short_filename='%s' -> filename='%s'\n", short_filename, filename);
}

static void mimic_root_dir_entry(void *buffer, uint32_t bufsize) {
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);

    lfs_dir_t dir;
    lfs_dir_open(&fs, &dir, "/");
    int num_entry = 0;
    while (true) {
        struct lfs_info info;
        int res = lfs_dir_read(&fs, &dir, &info);
        if (res < 0) {
            printf("lfs_dir_read: %d\n", res);
            break;
        } else if (res == 0) {
            break;
        }
        if (info.type == LFS_TYPE_REG) {
            num_entry++;
            fat_short_filename(root_dir_entry[num_entry].DIR_Name, info.name);
            root_dir_entry[num_entry].DIR_Attr = 0x20;
            root_dir_entry[num_entry].DIR_NTRes = 0;
            root_dir_entry[num_entry].DIR_CrtTimeTenth = 0xC6;
            root_dir_entry[num_entry].DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
            root_dir_entry[num_entry].DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
            root_dir_entry[num_entry].DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
            root_dir_entry[num_entry].DIR_FstClusHI = 0;
            root_dir_entry[num_entry].DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
            root_dir_entry[num_entry].DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
            root_dir_entry[num_entry].DIR_FstClusLO = LITTLE_ENDIAN16(num_entry + 1);
            root_dir_entry[num_entry].DIR_FileSize = LITTLE_ENDIAN32(info.size);
        }
        if (num_entry >= 16) {
            printf("This implementation can't mimic more than 15 files\n");
            break;
        }
    }

    lfs_dir_close(&fs, &dir);
    lfs_unmount(&fs);

    uint8_t const *addr = (uint8_t *)root_dir_entry;
    memcpy(buffer, addr, bufsize);
}

static bool find_root_dir_entry(uint32_t fat_sector, uint8_t *lfs_filename, uint32_t *lfs_offset) {
    if (fat_sector < 3) {
      return false;
    }

    // test allocated FAT sector
    if (lookup_fat_table(fat_sector - 1) == 0) {
      return false;
    }

    printf("find_root_dir_entry(%u)\n", fat_sector);
    int cluster = 1;
    for (int i = 1; i < 16; i++) {
      if (root_dir_entry[i].DIR_FstClusLO == fat_sector - 1) {
        // use single cluster
        printf("  fat_sector=%d -> %s, offset=%d\n", fat_sector, root_dir_entry[i].DIR_Name, 0);
        *lfs_offset = 0;
        fls_filename(lfs_filename, root_dir_entry[i].DIR_Name);
        return true;
      }
      else if ((root_dir_entry[i].DIR_FstClusLO > fat_sector - 1) || root_dir_entry[i].DIR_Name[0] == '\0' ) {
        // use multi cluster
        int offset = (fat_sector - 1) - (root_dir_entry[i - 1].DIR_FstClusLO);

        *lfs_offset = offset * 512;
        fls_filename(lfs_filename, root_dir_entry[i - 1].DIR_Name);
        return true;
      }

      if (root_dir_entry[i].DIR_Name[0] == '\0' || root_dir_entry[i].DIR_Name[0] == '\0') {
        break;
      }
    }
    return false;
}

static void mimic_file_entry(uint32_t fat_sector, void *buffer, uint32_t bufsize) {
    uint8_t dummy[512] = "";
    uint8_t filename[12];
    uint32_t offset = 0;

    if (!find_root_dir_entry(fat_sector, filename, &offset)) {
        return;
    }

    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    lfs_file_t f;
    lfs_file_open(&fs, &f, filename, LFS_O_RDONLY);
    uint8_t sector[512];
    lfs_file_seek(&fs, &f, offset, LFS_SEEK_SET);
    lfs_file_read(&fs, &f, sector, sizeof(sector));
    memcpy(buffer, sector, bufsize);
    lfs_unmount(&fs);
    return;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void)lun;

    const char vid[] = "littlefs";
    const char pid[] = "Mass Storage";
    const char rev[] = "1.0";

    memcpy(vendor_id  , vid, strlen(vid));
    memcpy(product_id , pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return usb_device_enable;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void)lun;

    *block_count = DISK_BLOCK_NUM;
    *block_size  = DISK_BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void)lun;
    (void)power_condition;

    if (load_eject) {
        if (start) {
            // load disk storage
        } else {
            // unload disk storage
            ejected = true;
        }
    }
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void) lun;
    if ( lba >= DISK_BLOCK_NUM ) return -1;

    if (lba == 0) { // read Boot sector
        uint8_t const *addr = fat_disk_image[lba];
        memcpy(buffer, addr, bufsize);
        return (int32_t)bufsize;
    } else if (lba == 1) {  // read FAT table
        mimic_fat_table(buffer, bufsize);
        return (int32_t)bufsize;
    } else if (lba == 2) { // read Root dir entry
        mimic_root_dir_entry(buffer, bufsize);
        return (int32_t)bufsize;
    } else { // data entry
        mimic_file_entry(lba, buffer, bufsize);
        return (int32_t)bufsize;
    }
}

bool tud_msc_is_writable_cb (uint8_t lun) {
    (void) lun;
    return false; // READ ONLY
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void) lun;
    (void) lba; (void) offset; (void) buffer;

    // out of ramdisk
    if ( lba >= DISK_BLOCK_NUM ) return -1;

    return (int32_t) bufsize;
}

int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    void const* response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
    default:
        // Set Sense = Invalid Command Operation
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        // negative means error -> tinyusb could stall and/or response with failed status
        resplen = -1;
        break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) resplen = bufsize;

    if (response && (resplen > 0)) {
        if(in_xfer) {
            memcpy(buffer, response, (size_t) resplen);
        } else {
            ; // SCSI output
        }
    }
    return (int32_t)resplen;
}
