/*
 * mimic FAT file system functions
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "mimic_fat.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c

static uint8_t fat_disk_image[1][DISK_BLOCK_SIZE] = {
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

static void print_block(uint8_t *buffer, size_t l) {
    for (size_t i = 0; i < l; ++i) {
        if (isalnum(buffer[i])) {
            printf("'%c' ", buffer[i]);
        } else {
            printf("0x%02x", buffer[i]);
        }
        if (i % 16 == 15)
            printf("\n");
        else
            printf(", ");
    }
}

static size_t ascii_to_utf16le(uint16_t *dist, size_t dist_size, const uint8_t *src, size_t src_size) {
    size_t utf16le_pos = 0;

    for (size_t i = 0; i < src_size && src[i] != '\0'; ++i) {
        uint32_t codepoint = (uint32_t)src[i];

        if (utf16le_pos + 1 <= dist_size) {
            dist[utf16le_pos++] = (uint16_t)codepoint;
        } else {
            break;
        }
    }

    if (utf16le_pos < dist_size) {
        dist[utf16le_pos] = '\0';
    }
    return utf16le_pos;
}

static size_t utf16le_to_utf8(uint8_t *dist, size_t buffer_size, const uint16_t *src, size_t len) {
    size_t dist_len = 0;

    for (size_t i = 0; i < len; ++i) {
        uint32_t codepoint = src[i];
        if (codepoint == 0xFFFF || codepoint == 0x0000) {
            break;
        }

        if (codepoint <= 0x7F) {
            if (dist_len + 1 <= buffer_size) {
                dist[dist_len++] = (uint8_t)codepoint;
            } else {
                break;
            }
        } else if (codepoint <= 0x7FF) {
            if (dist_len + 2 <= buffer_size) {
                dist[dist_len++] = (uint8_t)(0xC0 | (codepoint >> 6));
                dist[dist_len++] = (uint8_t)(0x80 | (codepoint & 0x3F));
            } else {
                break;
            }
        } else if (codepoint <= 0xFFFF) {
            if (dist_len + 3 <= buffer_size) {
                dist[dist_len++] = (uint8_t)(0xE0 | (codepoint >> 12));
                dist[dist_len++] = (uint8_t)(0x80 | ((codepoint >> 6) & 0x3F));
                dist[dist_len++] = (uint8_t)(0x80 | (codepoint & 0x3F));
            } else {
                break;
            }
        } else {
            break;
        }
    }

    if (dist_len < buffer_size) {
        dist[dist_len] = '\0';
    }
    return dist_len;
}

static int is_fat_sfn_symbol(uint8_t c) {
    switch (c) {
    case '$':
    case '%':
    case '\'':
    case '-':
    case '_':
    case '@':
    case '~':
    case '`':
    case '!':
    case '(':
    case ')':
    case '{':
    case '}':
    case '^':
    case '#':
    case '&':
        return 1;
        break;
    default:
        return 0;
    }
}

static void trim_and_upper(uint8_t *name) {
    uint8_t *start = name;
    while (*start == '.' || *start == ' ') {
        ++start;
    }

    size_t length = 0;
    while (*start != '\0') {
        if (isalpha(*start)) {
            name[length++] = toupper(*start);
        } else {
            name[length++] = *start;
        }
        ++start;
    }
    name[length] = '\0';
}

static int fat_table_value(int cluster) {
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % 512;
    int16_t result = 0;
    if (cluster & 0x01) {
        result = (fat_table[offset] >> 4) | ((uint16_t)fat_table[offset + 1] << 4);
    } else {
        result = fat_table[offset] | ((uint16_t)(fat_table[offset + 1] & 0x0F) << 8);
    }
    return result;
}

static void set_fat_short_filename(uint8_t *short_filename, const uint8_t *filename) {
    uint8_t buffer[128];
    strncpy(buffer, filename, sizeof(buffer));

    uint8_t *basename = strtok(buffer, ".");
    uint8_t *ext = strtok(NULL, ".");
    sprintf(short_filename, "%-8s%-3s", basename, ext);
}

static void restore_from_short_filename(uint8_t *filename, const uint8_t *short_filename) {
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

static bool is_short_filename(uint8_t *filename) {
    uint8_t buffer[256];
    strncpy(buffer, filename, sizeof(buffer));
    uint8_t *name = (uint8_t *)strtok(buffer, ".");
    if (strlen(name) > 8) {
        return false;
    }
    uint8_t *ext = (uint8_t *)strtok(NULL, ".");
    if (strlen(ext) > 3) {
        return false;
    }

    for (int i = 0; i < 8; i++) {
        if (name[i] == '\0') {
            break;
        }
        if (isalnum(name[i]) == 0 && is_fat_sfn_symbol(name[i]) == 0) {
            return false;
        }
        if (isalpha(name[i]) == 1 && isupper(name[i]) == 0) {
            return false;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (ext[i] == '\0') {
            break;
        }
        if (isalpha(ext[i]) == 1 && isupper(ext[i]) == 0) {
            return false;
        }
        if ((isalnum(ext[i]) == 0) && (is_fat_sfn_symbol(ext[i]) == 0)) {
            return false;
        }
    }
    return true;
}

static void create_shortened_short_file_name(uint8_t *sfn, const uint8_t *long_filename) {
    uint8_t buffer[256];

    strncpy(buffer, long_filename, sizeof(buffer));
    trim_and_upper(buffer);
    uint8_t *name = strtok(buffer, ".");
    name[8] = '\0';
    uint8_t *ext = strtok(NULL, ".");
    ext[3] = '\0';
    snprintf(sfn, 11 + 1 + 1, "%s.%s", name, ext);
}

static uint8_t check_sum(const uint8_t *filename) {
    uint8_t i, sum;

    for (i = sum = 0; i < 11; i++) {
        sum = (sum >> 1) + (sum << 7) + filename[i];
    }
    return sum;
}

static void set_LFN_name123(fat_lfn_t *dir, const uint8_t *filename) {
    uint16_t utf16_buffer[13];
    size_t l;
    l = ascii_to_utf16le(utf16_buffer, sizeof(utf16_buffer), filename, strlen(filename));

    //printf("  '%s'\n", filename);

    memcpy(&dir->LDIR_Name1, utf16_buffer + 0, sizeof(uint16_t) * 5);
    memcpy(&dir->LDIR_Name2, utf16_buffer + 5, sizeof(uint16_t) * 6);
    memcpy(&dir->LDIR_Name3, utf16_buffer + 5+6, sizeof(uint16_t) * 2);
    for (int i = 0; i < 5*2; i += 2) {
        if (dir->LDIR_Name1[i] == 0x00 && dir->LDIR_Name1[i+1] == 0x00) {
            dir->LDIR_Name1[i] = 0xFF;
            dir->LDIR_Name1[i+1] = 0xFF;
        }
    }
    for (int i = 0; i < 6*2; i += 2) {
        if (dir->LDIR_Name2[i] == 0x00 && dir->LDIR_Name2[i+1] == 0x00) {
            dir->LDIR_Name2[i] = 0xFF;
            dir->LDIR_Name2[i+1] = 0xFF;
        }
    }
    for (int i = 0; i < 2*2; i += 2) {
        if (dir->LDIR_Name3[i] == 0x00 && dir->LDIR_Name3[i+1] == 0x00) {
            dir->LDIR_Name3[i] = 0xFF;
            dir->LDIR_Name3[i+1] = 0xFF;
        }
    }
    if (l < 13 && l < 5) {
        dir->LDIR_Name1[l * 2]     = 0x00;
        dir->LDIR_Name1[l * 2 + 1] = 0x00;
    } else if (l < 13 && l < 11) {
        dir->LDIR_Name2[(l-5) * 2]     = 0x00;
        dir->LDIR_Name2[(l-5) * 2 + 1] = 0x00;
    } else if (l < 13) {
        dir->LDIR_Name2[(l-5-6) * 2]     = 0x00;
        dir->LDIR_Name2[(l-5-6) * 2 + 1] = 0x00;
    }
    //print_block((uint8_t *)dir, 32);
}

static bool restore_lfs_filename(uint8_t *filename, int root_dir_id) {
    fat_lfn_t *lfn_entry;
    uint16_t utf16_filename[256];
    int pos = 0;

    // restore Short File Name
    restore_from_short_filename(filename, root_dir_entry[root_dir_id].DIR_Name);
    if ((root_dir_entry[root_dir_id - 1].DIR_Attr & 0x0F) != 0x0F) {  // is Short File Name
        return true;
    }

    // restore Long File Name
    for (int i = root_dir_id - 1; i > 0; i--) {
        int last_entry = root_dir_entry[i].DIR_Attr & 0xF0;
        int order = root_dir_entry[i].DIR_Attr & 0x0F;
        lfn_entry = (fat_lfn_t *)&root_dir_entry[i];
        memcpy((utf16_filename + pos), lfn_entry->LDIR_Name1, sizeof(lfn_entry->LDIR_Name1));
        pos += 5;
        memcpy((utf16_filename + pos), lfn_entry->LDIR_Name2, sizeof(lfn_entry->LDIR_Name2));
        pos += 6;
        memcpy((utf16_filename + pos), lfn_entry->LDIR_Name3, sizeof(lfn_entry->LDIR_Name3));
        pos += 2;
        if (last_entry) {
            break;
        }
    }
    for (int i = 0; i < sizeof(utf16_filename); i++) {
        if (utf16_filename[i] == 0xFFFF) {
            utf16_filename[i] = '\0';
            pos = i;
            break;
        }
    }
    utf16le_to_utf8(filename, 256, (const uint16_t *)utf16_filename, pos);
    return true;
}

static bool find_root_dir_entry(uint32_t fat_sector, uint8_t *lfs_filename, uint32_t *lfs_offset) {
    if (fat_sector < 3) {
      return false;
    }

    // test allocated FAT sector
    if (fat_table_value(fat_sector - 1) == 0) {
      return false;
    }

    printf("Search for Littlefs files corresponding to FAT sector #%u\n", fat_sector);
    int cluster = 1;
    for (int i = 1; i < 16; i++) {
      if (root_dir_entry[i].DIR_FstClusLO == fat_sector - 1) {
        // use single cluster
        *lfs_offset = 0;
        restore_lfs_filename(lfs_filename, i);
        printf("  FAT sector=%d -> littlefs '%s' offset=0\n", fat_sector, lfs_filename);
        return true;
      }
      else if ((root_dir_entry[i].DIR_FstClusLO > fat_sector - 1) || root_dir_entry[i].DIR_Name[0] == '\0' ) {
        // use multi cluster
        int offset = (fat_sector - 1) - (root_dir_entry[i - 1].DIR_FstClusLO);
        *lfs_offset = offset * 512;
        restore_lfs_filename(lfs_filename, i - 1);
        printf("  FAT sector=%d -> littlefs '%s'(pos=%u)\n", fat_sector, lfs_filename, lfs_offset);

        return true;
      }

      if (root_dir_entry[i].DIR_Name[0] == '\0' || root_dir_entry[i].DIR_Name[0] == '\0') {
        break;
      }
    }
    return false;
}

/*
 * Returns the boot sector of the FAT image when USB requests sector 0
 */
void mimic_fat_boot_sector(void *buffer, uint32_t bufsize) {
    uint8_t const *addr = fat_disk_image[0];
    memcpy(buffer, addr, bufsize);
}

/*
 * Return the FAT table when USB requests sector 1.
 * Build a FAT table based on littlefs files.
 */
void mimic_fat_table(void *buffer, uint32_t bufsize) {
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
                printf("  name=%s,size=%u, offset=%u\n", info.name, info.size, offset);

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

/*
 * Return the FAT root directory entry when USB request sector 2.
 * Explore the littlefs root directory and build file information.
 */
void mimic_fat_root_dir_entry(void *buffer, uint32_t bufsize) {
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);

    lfs_dir_t dir;
    lfs_dir_open(&fs, &dir, "/");
    int num_entry = 0;
    int file_entry = 0;
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
            file_entry++;
            if (is_short_filename((uint8_t *)info.name)) {
                printf("create Short File Name entry '%s' -> '%s'\n", info.name, info.name);
                set_fat_short_filename(root_dir_entry[num_entry].DIR_Name, info.name);
                root_dir_entry[num_entry].DIR_Attr = 0x20;
                root_dir_entry[num_entry].DIR_NTRes = 0;
                root_dir_entry[num_entry].DIR_CrtTimeTenth = 0xC6;
                root_dir_entry[num_entry].DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
                root_dir_entry[num_entry].DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
                root_dir_entry[num_entry].DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
                root_dir_entry[num_entry].DIR_FstClusHI = 0;
                root_dir_entry[num_entry].DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
                root_dir_entry[num_entry].DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
                root_dir_entry[num_entry].DIR_FstClusLO = LITTLE_ENDIAN16(file_entry + 1);
                root_dir_entry[num_entry].DIR_FileSize = LITTLE_ENDIAN32(info.size);
            } else {  // Long file name
                fat_dir_entry_t sfn_dir;
                uint8_t short_filename[11 + 1 + 1];

                printf("create Long File Name entry '%s'\n", info.name);
                create_shortened_short_file_name(short_filename, info.name);
                set_fat_short_filename(sfn_dir.DIR_Name, short_filename);
                sfn_dir.DIR_Attr = 0x20;
                sfn_dir.DIR_NTRes = 0;
                sfn_dir.DIR_CrtTimeTenth = 0xC6;
                sfn_dir.DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
                sfn_dir.DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
                sfn_dir.DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
                sfn_dir.DIR_FstClusHI = 0;
                sfn_dir.DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
                sfn_dir.DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
                sfn_dir.DIR_FstClusLO = LITTLE_ENDIAN16(file_entry + 1);
                sfn_dir.DIR_FileSize = LITTLE_ENDIAN32(info.size);

                int lfn_entry = floor((strlen(info.name) - 1) / 13);
                for (int i = lfn_entry; i >= 0; i--) {
                    uint8_t lfn_ord = i + 1;
                    uint8_t lfn_chunk[14];
                    uint8_t *head = (uint8_t *)&info.name[i * 13];
                    strncpy(lfn_chunk, head, 13);
                    lfn_chunk[13] = '\0';
                    if (i == lfn_entry) {
                        lfn_ord |= 0x40;
                    }

                    fat_lfn_t *root_long_dir_entry = (fat_lfn_t *)&root_dir_entry[num_entry];
                    set_LFN_name123(root_long_dir_entry, lfn_chunk);
                    root_long_dir_entry->LDIR_Ord = lfn_ord;
                    root_long_dir_entry->LDIR_Attr = 0x0F;
                    root_long_dir_entry->LDIR_Type = 0x00;
                    root_long_dir_entry->LDIR_Chksum = check_sum(sfn_dir.DIR_Name);
                    root_long_dir_entry->LDIR_FstClusLO[0] = 0x00;
                    root_long_dir_entry->LDIR_FstClusLO[1] = 0x00;
                    num_entry++;
                }
                printf("  create SFN linked to LFN '%s' -> '%s'\n", short_filename, info.name);
                memcpy(&root_dir_entry[num_entry], &sfn_dir, sizeof(sfn_dir));
            }
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

/*
 * Return a littlefs file block when USB request sector 3 or later.
 * Searches the littlefs file corresponding to the file block for the
 * file name and the offset of the data to be returned.
 */
void mimic_fat_file_entry(uint32_t fat_sector, void *buffer, uint32_t bufsize) {
    uint8_t dummy[512] = "";
    uint8_t filename[256];
    uint32_t offset = 0;

    if (!find_root_dir_entry(fat_sector, filename, &offset)) {
        return;
    }

    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    lfs_file_t f;
    err = lfs_file_open(&fs, &f, filename, LFS_O_RDONLY);
    if (err < 0) {
        printf("can't open littlefs '%s' rc=%d\n", filename, err);
        lfs_unmount(&fs);
        return;
    }

    uint8_t sector[512];
    lfs_file_seek(&fs, &f, offset, LFS_SEEK_SET);
    lfs_file_read(&fs, &f, sector, sizeof(sector));
    memcpy(buffer, sector, bufsize);
    lfs_unmount(&fs);
    return;
}
