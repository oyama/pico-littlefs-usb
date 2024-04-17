/*
 * mimic FAT file system functions
 *
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "mimic_fat.h"


#define ANSI_RED "\e[31m"
#define ANSI_CLEAR "\e[0m"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c

static const int FAT_SHORT_NAME_MAX = 11;
static const int FAT_SHORT_8x3_MAX = 8 + 3 + 1;
static const int FAT_LONG_FILENAME_CHUNK_MAX = 13;

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

static uint8_t fat_table[DISK_BLOCK_SIZE] = {
    0xF8, 0xFF, 0xFF, 0x00, 0x00
};

static fat_dir_entry_t root_dir_entry[DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t)] = {
  { "littlefsUSB", 0x08, 0x00, 0x00, 0x0000, 0x0000, 0x0000, 0x0000, 0x4F6D, 0x6543, 0x0000, 0x00000000 },
};

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
        if (codepoint == 0xFFFF) {
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

static void print_dir_entry(void *buffer) {
    uint8_t pbuffer[11+1];
    fat_dir_entry_t *dir = (fat_dir_entry_t *)buffer;
    for (int i = 0; i < DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t); i++) {
        if (dir->DIR_Name[0] == '\0') {
            break;
        }
        if ((dir->DIR_Attr & 0x0F) != 0x0F) {
            memcpy(pbuffer, &dir->DIR_Name, 11);
            pbuffer[11] = '\0';
            printf("name='%s' attr=0x%02X timeTenth=%u, CrtDate=%u,%u writeDateTime=%u,%u LstAccDate=%u cluster=%u\n",
                   pbuffer,
                   dir->DIR_Attr,
                   dir->DIR_CrtTimeTenth,
                   dir->DIR_CrtDate, dir->DIR_CrtTime,
                   dir->DIR_WrtDate, dir->DIR_WrtTime,
                   dir->DIR_LstAccDate,
                   dir->DIR_FstClusLO);
        } else {
            fat_lfn_t *lfn = (fat_lfn_t *)dir;
            uint16_t utf16le[13 + 1];
            memcpy(utf16le, lfn->LDIR_Name1, 5*2);
            memcpy(utf16le + 5, lfn->LDIR_Name2, 6*2);
            memcpy(utf16le + 5 + 6, lfn->LDIR_Name3, 2*2);
            utf16le[13] = '\0';
            uint8_t utf8[13 + 1];
            utf16le_to_utf8(utf8, sizeof(utf8), utf16le, 13);
            printf("name='%s' attr=0x%02X ord=0x%02X cluster=%u\n", utf8, lfn->LDIR_Attr, lfn->LDIR_Ord, dir->DIR_FstClusLO);
        }
        dir++;
    }
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
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % DISK_BLOCK_SIZE;
    int16_t result = 0;
    if (cluster & 0x01) {
        result = (fat_table[offset] >> 4) | ((uint16_t)fat_table[offset + 1] << 4);
    } else {
        result = fat_table[offset] | ((uint16_t)(fat_table[offset + 1] & 0x0F) << 8);
    }
    return result;
}

static void print_fat_table(size_t l) {
    printf("FAT table-------\n");
    for (int i = 0; i < l; i++) {
        printf(" cluster=%d fat=%03x\n", i, fat_table_value(i));
    }
}

static void set_fat_short_filename(uint8_t *short_filename, const uint8_t *filename) {
    uint8_t buffer[LFS_NAME_MAX + 1];
    strncpy(buffer, filename, sizeof(buffer));

    uint8_t *basename = strtok(buffer, ".");
    uint8_t *ext = strtok(NULL, ".");
    sprintf(short_filename, "%-8s%-3s", basename, ext);
}

static void restore_from_short_filename(uint8_t *filename, const uint8_t *short_filename) {
    uint8_t buffer[FAT_SHORT_NAME_MAX + 1];
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
}

static void restore_from_short_dirname(uint8_t *filename, const uint8_t *short_dirname) {
    sprintf(filename, "%s", short_dirname);
    size_t length = FAT_SHORT_NAME_MAX;
    while (length > 0 && filename[length - 1] == ' ') {
        --length;
    }
    filename[length] = '\0';
}

static bool is_short_filename_file(uint8_t *filename) {
    uint8_t buffer[LFS_NAME_MAX + 1];
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
        if (isalpha(name[i]) > 0 && isupper(name[i]) == 0) {
            return false;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (ext[i] == '\0') {
            break;
        }
        if (isalpha(ext[i]) > 0 && isupper(ext[i]) == 0) {
            return false;
        }
        if ((isalnum(ext[i]) == 0) && (is_fat_sfn_symbol(ext[i]) == 0)) {
            return false;
        }
    }
    return true;
}

static bool is_short_filename_dir(uint8_t *filename) {
    uint8_t buffer[LFS_NAME_MAX + 1];
    strncpy(buffer, filename, sizeof(buffer));
    if (strlen(filename) > FAT_SHORT_NAME_MAX) {
        return false;
    }
    for (int i = 0; i < FAT_SHORT_NAME_MAX; i++) {
        if (filename[i] == '\0') {
            break;
        }
        if (isalnum(filename[i]) == 0 && is_fat_sfn_symbol(filename[i]) == 0) {
            return false;
        }
        if (isalpha(filename[i]) > 0 && isupper(filename[i]) == 0) {
            return false;
        }
    }
    return true;
}

static void create_shortened_short_filename(uint8_t *sfn, const uint8_t *long_filename) {
    uint8_t buffer[LFS_NAME_MAX + 1];
    uint8_t short_filename[FAT_SHORT_NAME_MAX + 1];
    strncpy(buffer, long_filename, sizeof(buffer));
    trim_and_upper(buffer);
    uint8_t *name = strtok(buffer, ".");
    name[6] = '~';
    name[7] = '1';
    name[8] = '\0';
    uint8_t *ext = strtok(NULL, ".");
    ext[3] = '\0';
    snprintf(short_filename, sizeof(short_filename), "%-8s%-3s", name, ext);
    memcpy(sfn, short_filename, FAT_SHORT_NAME_MAX);
}

static void create_shortened_short_filename_dir(uint8_t *sfn, const uint8_t *long_filename) {
    uint8_t buffer[LFS_NAME_MAX + 1];
    uint8_t short_filename[FAT_SHORT_NAME_MAX + 1];

    strncpy(buffer, long_filename, sizeof(buffer));
    trim_and_upper(buffer);
    snprintf(short_filename, sizeof(short_filename), "%-11s", buffer);
    memcpy(sfn, short_filename, FAT_SHORT_NAME_MAX);
}

static uint8_t filename_check_sum(const uint8_t *filename) {
    uint8_t i, sum;

    for (i = sum = 0; i < FAT_SHORT_NAME_MAX; i++) {
        sum = (sum >> 1) + (sum << 7) + filename[i];
    }
    return sum;
}

static void set_LFN_name123(fat_lfn_t *dir, const uint8_t *filename) {
    uint16_t utf16_buffer[FAT_LONG_FILENAME_CHUNK_MAX];
    size_t l;
    memset(utf16_buffer, 0, sizeof(utf16_buffer));
    l = ascii_to_utf16le(utf16_buffer, sizeof(utf16_buffer), filename, strlen(filename));

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
    if (l < FAT_LONG_FILENAME_CHUNK_MAX && l < 5) {
        dir->LDIR_Name1[l * 2]     = 0x00;
        dir->LDIR_Name1[l * 2 + 1] = 0x00;
    } else if (l < FAT_LONG_FILENAME_CHUNK_MAX && l < FAT_SHORT_NAME_MAX) {
        dir->LDIR_Name2[(l-5) * 2]     = 0x00;
        dir->LDIR_Name2[(l-5) * 2 + 1] = 0x00;
    } else if (l < FAT_LONG_FILENAME_CHUNK_MAX) {
        dir->LDIR_Name3[(l-5-6) * 2]     = 0x00;
        dir->LDIR_Name3[(l-5-6) * 2 + 1] = 0x00;
    }
}

/*
 * Search for files on littlefs corresponding to request_block
 *
 * *cluster: current position in the FAT file system to be searched
 */
static bool find_lfs_by_request_block(lfs_t *fs,
                                      uint32_t request_block,
                                      uint16_t *cluster,
                                      const char *directory,
                                      uint8_t *lfs_filename,
                                      uint32_t *lfs_offset,
                                      bool *is_dir)
{
    bool result = false;
    lfs_dir_t dir;
    struct lfs_info finfo;

    int err = lfs_dir_open(fs, &dir, directory);
    if (err < 0) {
        printf("find_lfs_by_request_block('%s') error=%d\n", err);
        return false;
    }

    if (strcmp(directory, "") == 0) {
        // is root dir
        *cluster = 2; // set first cluster
    } else {
        // is subdir
        *cluster += 1; // add sub directory entry cluster
    }

    while (true) {
        uint8_t path[LFS_NAME_MAX + 1];

        err = lfs_dir_read(fs, &dir, &finfo);
        if (err < 0) {
            printf("find_lfs_by_request_block error=%d: '%s'\n", err, directory);
            return false;
        }
        if (err == 0) {
            break;
        }
        if (finfo.type == LFS_TYPE_DIR && (strcmp(finfo.name, ".") == 0 || strcmp(finfo.name, "..") == 0)) {
            continue;
        }
        if (finfo.type == LFS_TYPE_DIR && (strcmp(finfo.name, ".fat") == 0)) {
            continue;
        }

        if (finfo.type == LFS_TYPE_DIR) {
            sprintf(path, "%s/%s", directory, finfo.name);
            if (*cluster == request_block - 1) {
                strcpy(lfs_filename, path);
                *is_dir = true;
                result = true;
                break;
            }
            result = find_lfs_by_request_block(fs, request_block, cluster, path, lfs_filename, lfs_offset, is_dir);
            if (result == true) {
                break;
            }
        } else if (finfo.type == LFS_TYPE_REG) {
            if (*cluster == request_block - 1) {
                sprintf(lfs_filename, "%s/%s", directory, finfo.name);
                result = true;
                break;
            }

            int i = 0;
            for (int i = 0; finfo.size >= i * DISK_BLOCK_SIZE; i++) {
                if (*cluster == request_block - 1) {
                    printf("     find request_block=%u cluster=%u '%s'\n", request_block, *cluster, finfo.name);
                    sprintf(lfs_filename, "%s/%s", directory, finfo.name);
                    *lfs_offset = DISK_BLOCK_SIZE * i;
                    result = true;
                    break;
                }
                *cluster += 1;
            }

            if (result) {
                break;
            }
        }
    }
    lfs_dir_close(fs, &dir);
    return result;
}

static bool find_lfs_file(uint32_t request_block,
                          uint16_t *cluster,
                          uint8_t *lfs_filename,
                          uint32_t *lfs_offset,
                          bool *is_dir)
{
    if (request_block < 3) {
        return false;
    }
    if (fat_table_value(request_block - 1) == 0) {
        return false;
    }

    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    if (err < 0) {
        printf("find_lfs_file lfs_mount error=%d\n", err);
        return false;
    }

    //uint32_t cluster = 0;
    bool result = find_lfs_by_request_block(&fs, request_block, cluster, "", lfs_filename, lfs_offset, is_dir);
    lfs_unmount(&fs);
    return result;
}

void set_volume_label_entry(fat_dir_entry_t *dir, const char *name) {
    uint8_t sfn_name[FAT_SHORT_NAME_MAX + 1];
    sprintf(sfn_name, "%-11s", name);

    memcpy(dir->DIR_Name, sfn_name, FAT_SHORT_NAME_MAX);
    dir->DIR_Attr = 0x08;
    dir->DIR_NTRes = 0;
    dir->DIR_CrtTimeTenth = 0;
    dir->DIR_CrtTime = 0;
    dir->DIR_CrtDate = 0;
    dir->DIR_LstAccDate = 0;
    dir->DIR_FstClusHI = 0;
    dir->DIR_WrtTime = LITTLE_ENDIAN16(0x4F6D);
    dir->DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusLO = 0;
    dir->DIR_FileSize = 0;
}

static void set_directory_entry(fat_dir_entry_t *dir, const char *name, int cluster) {
    uint8_t sfn_name[FAT_SHORT_NAME_MAX + 1];
    sprintf(sfn_name, "%-11s", name);
    memcpy(dir->DIR_Name, sfn_name, FAT_SHORT_NAME_MAX);
    dir->DIR_Attr = 0x10; // directory
    dir->DIR_NTRes = 0;
    dir->DIR_CrtTimeTenth = 0xC6;
    dir->DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusHI = 0;
    dir->DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusLO = cluster;
    dir->DIR_FileSize = 0;
}

static void set_file_entry(fat_dir_entry_t *dir, struct lfs_info *info, int cluster) {
    set_fat_short_filename(dir->DIR_Name, info->name);
    dir->DIR_Attr = 0x00;  //0x20;
    dir->DIR_NTRes = 0;
    dir->DIR_CrtTimeTenth = 0xC6;
    dir->DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusHI = 0;
    dir->DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusLO = LITTLE_ENDIAN16(cluster);
    dir->DIR_FileSize = LITTLE_ENDIAN32(info->size);
}

static void set_long_file_entry(fat_dir_entry_t *dir, uint8_t *lfn_chunk, uint8_t lfn_order, uint8_t check_sum) {
    fat_lfn_t *long_dir_entry = (fat_lfn_t *)dir;
    set_LFN_name123(long_dir_entry, lfn_chunk);
    long_dir_entry->LDIR_Ord = lfn_order;
    long_dir_entry->LDIR_Attr = 0x0F;
    long_dir_entry->LDIR_Type = 0x00;
    long_dir_entry->LDIR_Chksum = check_sum;
    long_dir_entry->LDIR_FstClusLO[0] = 0x00;
    long_dir_entry->LDIR_FstClusLO[1] = 0x00;
}

static uint16_t number_of_subdirectory_clusters(lfs_t *fs, const uint8_t *path) {
    uint16_t result = 0;
    lfs_dir_t dir;
    lfs_dir_open(fs, &dir, path);
    while (true) {
        struct lfs_info info;
        int res = lfs_dir_read(fs, &dir, &info);
        if (res == 0) {
            break;
        }
        if (info.type == LFS_TYPE_REG) {
            for (int16_t size = info.size; size > 0; size -= DISK_BLOCK_SIZE) {
                result++;
            }
        } else if (info.type == LFS_TYPE_DIR) {
            if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
                continue;
            }
            uint8_t dir_name[LFS_NAME_MAX + 1];
            sprintf(dir_name, "%s/%s", path, info.name);
            result++;
            result += number_of_subdirectory_clusters(fs, dir_name);
        }
    }
    lfs_dir_close(fs, &dir);
    printf("    number_of_subdirectory_clusters(%s) -> %u\n", path, result);
    return result;
}

static void update_fat_table(uint16_t cluster, uint16_t value) {
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % DISK_BLOCK_SIZE;
    if (cluster & 0x01) {
        fat_table[offset] = (fat_table[offset] & 0x0F) | (value << 4);
        fat_table[offset + 1] = value >> 4;
    } else {
        fat_table[offset] = value;
        fat_table[offset + 1] = (fat_table[offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
    }

}

void create_fat_dir_entry(const uint8_t *path, uint16_t cluster, uint32_t request_block, void *buffer, uint32_t bufsize) {
    printf("create_fat_dir_entry for request_block=%u, path=%s\n", request_block, path);
    fat_dir_entry_t *entry, dir_entry[DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t)];
    memset(dir_entry, 0, sizeof(dir_entry));
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);

    int num_entry = 1;
    if (strcmp(path, "/") == 0) {
        // root dir entry
        entry = root_dir_entry;
        set_volume_label_entry(&entry[0], "littlefsUSB");
    } else {
        entry = dir_entry;
        set_directory_entry(&entry[0], ".", request_block - 1); // current dir
        set_directory_entry(&entry[1], "..", 0); // Root dir
        num_entry = 2;
    }
    lfs_dir_t dir;
    lfs_dir_open(&fs, &dir, path);
    while (true) {
        struct lfs_info info;
        int res = lfs_dir_read(&fs, &dir, &info);
        if (res < 0) {
            printf("create_fat_dir_entry lfs_dir_read: error=%d\n", res);
            break;
        } else if (res == 0) {
            break;
        }

        if (info.type == LFS_TYPE_DIR) {
            if (info.type == LFS_TYPE_DIR && (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)) {
                continue;
            }
            if (info.type == LFS_TYPE_DIR && (strcmp(info.name, ".fat") == 0)) {
                continue;
            }

            if (is_short_filename_dir((uint8_t *)info.name)) {
                printf("  create Short File Name dir entry '%s' -> '%s' cluster=%u\n", info.name, info.name, cluster);
                set_directory_entry(&entry[num_entry], info.name, cluster);
            } else {
                fat_dir_entry_t sfn_dir;
                uint8_t buffer[FAT_SHORT_NAME_MAX + 1];

                printf("  create Long File Name dir entry '%s'\n", info.name);
                set_directory_entry(&sfn_dir, info.name, cluster);
                create_shortened_short_filename_dir(sfn_dir.DIR_Name, info.name);
                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, sfn_dir.DIR_Name, FAT_SHORT_NAME_MAX);

                uint8_t check_sum = filename_check_sum(sfn_dir.DIR_Name);
                int lfn_entry = floor((strlen(info.name) - 1) / FAT_LONG_FILENAME_CHUNK_MAX);
                for (int i = lfn_entry; i >= 0; i--) {
                    uint8_t lfn_ord = i + 1;
                    uint8_t lfn_chunk[FAT_LONG_FILENAME_CHUNK_MAX + 1];
                    uint8_t *head = (uint8_t *)&info.name[i * FAT_LONG_FILENAME_CHUNK_MAX];
                    strncpy(lfn_chunk, head, FAT_LONG_FILENAME_CHUNK_MAX);
                    lfn_chunk[FAT_LONG_FILENAME_CHUNK_MAX] = '\0';
                    if (i == lfn_entry) {
                        lfn_ord |= 0x40;
                    }
                    set_long_file_entry(&entry[num_entry], lfn_chunk, lfn_ord, check_sum);
                    num_entry++;
                }
                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, sfn_dir.DIR_Name, FAT_SHORT_NAME_MAX);
                printf("  create SFN linked to LFN '%s' -> '%s' cluster=%u\n", buffer, info.name, cluster);
                memcpy(&entry[num_entry], &sfn_dir, sizeof(sfn_dir));
            }
            uint16_t num_cluster = number_of_subdirectory_clusters(&fs, info.name);
            num_entry += num_cluster;
            cluster += num_cluster + 1;

        } else if (info.type == LFS_TYPE_REG) {
            if (is_short_filename_file((uint8_t *)info.name)) {
                printf("  create Short File Name file entry '%s' -> '%s' cluster=%u\n", info.name, info.name, cluster);
                set_fat_short_filename(entry[num_entry].DIR_Name, info.name);
                set_file_entry(&entry[num_entry], &info, cluster);
            } else {  // Long file name
                fat_dir_entry_t sfn_dir;
                uint8_t buffer[FAT_SHORT_NAME_MAX + 1];

                // SET UNIQUE SHORT FILE NAME
                set_file_entry(&sfn_dir, &info, cluster);
                create_shortened_short_filename(sfn_dir.DIR_Name, info.name);

                printf("  create Long File Name file entry '%s'\n", info.name);
                uint8_t check_sum = filename_check_sum(sfn_dir.DIR_Name);
                int lfn_entry = floor((strlen(info.name) - 1) / FAT_LONG_FILENAME_CHUNK_MAX);
                for (int i = lfn_entry; i >= 0; i--) {
                    uint8_t lfn_ord = i + 1;
                    uint8_t lfn_chunk[FAT_LONG_FILENAME_CHUNK_MAX + 1];
                    uint8_t *head = (uint8_t *)&info.name[i * FAT_LONG_FILENAME_CHUNK_MAX];
                    strncpy(lfn_chunk, head, FAT_LONG_FILENAME_CHUNK_MAX);
                    lfn_chunk[FAT_LONG_FILENAME_CHUNK_MAX] = '\0';
                    if (i == lfn_entry) {
                        lfn_ord |= 0x40;
                    }
                    set_long_file_entry(&entry[num_entry], lfn_chunk, lfn_ord, check_sum);
                    num_entry++;
                }
                memset(buffer, 0, sizeof(buffer));
                memcpy(buffer, sfn_dir.DIR_Name, FAT_SHORT_NAME_MAX);
                printf("  create SFN linked to LFN '%s' -> '%s' cluster=%u\n", buffer, info.name, cluster);
                memcpy(&entry[num_entry], &sfn_dir, sizeof(sfn_dir));
            }

            num_entry++;
            for (int16_t size = info.size; size > 0; size -= DISK_BLOCK_SIZE) {
                cluster++;
            }

        }
        if (num_entry >= DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t)) {
            printf("Cannot create directory entries that exceed the block size > %u\n",
                   DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t));
            break;
        }
    }

    lfs_dir_close(&fs, &dir);
    lfs_unmount(&fs);

    memcpy(buffer, entry, bufsize);
}

/*
 * Returns the boot sector of the FAT image when USB requests sector 0
 */
void mimic_fat_boot_sector(void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=0 mimic_fat_boot_sector()\e[0m\n");

    uint8_t const *addr = fat_disk_image[0];
    memcpy(buffer, addr, bufsize);
}

/*
 * Return the FAT table when USB requests sector 1.
 * Build a FAT table based on littlefs files.
 */
void mimic_fat_table(void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=1 mimic_fat_table()\e[0m\n");
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    if (err < 0) {
        printf("mimic_fat_table lfs_mount: error=%d\n", err);
        return;
    }

    lfs_dir_t dir;
    lfs_dir_open(&fs, &dir, "/");
    uint16_t cluster = 1;
    while (true) {
        struct lfs_info info;
        int res = lfs_dir_read(&fs, &dir, &info);
        if (res < 0) {
            printf("mimic_fat_table lfs_dir_read: error=%d\n", res);
            break;
        } else if (res == 0) {
            break;
        }
        if (info.type == LFS_TYPE_DIR && (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)) {
            continue;
        }
        if (info.type == LFS_TYPE_DIR && (strcmp(info.name, ".fat") == 0)) {
            continue;
        }

        if (info.type == LFS_TYPE_DIR) {
            cluster++;

            uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % DISK_BLOCK_SIZE;
            update_fat_table(cluster, 0xFFF);
            printf("  assign fat table cluster=%d dirname='%s'\n", cluster, info.name);
            uint16_t sub_clusters = number_of_subdirectory_clusters(&fs, info.name);
            for (int i = cluster + 1; i < cluster + sub_clusters + 1; i++) {
                printf("  assign fat table cluster=%d dirname='%s' + files\n", i, info.name);
                update_fat_table(i, 0xFFF);
            }
            cluster += sub_clusters;

        } else if (info.type == LFS_TYPE_REG) {
            uint16_t size = info.size;
            for (int16_t size = info.size; size >= 0; size -= DISK_BLOCK_SIZE) {
                cluster++;

                update_fat_table(cluster, size >= DISK_BLOCK_SIZE ? cluster + 1: 0xFFF);
                printf("  assign fat table cluster=%d filename='%s' size=%u\n", cluster, info.name, size);
            }
        }
    }

    lfs_dir_close(&fs, &dir);
    lfs_unmount(&fs);
    memcpy(buffer, fat_table, bufsize);

    print_fat_table(10);
}

/*
 * Return the FAT root directory entry when USB request sector 2.
 * Explore the littlefs root directory and build file information.
 */
void mimic_fat_root_dir_entry(void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=2 mimic_fat_root_dir_entry()\e[0m\n");
    create_fat_dir_entry("/", 2, 2, buffer, bufsize);
    print_dir_entry(buffer);
    //print_block(buffer, 512);
    return;
}

/*
 * Return a littlefs file block when USB request sector 3 or later.
 * Searches the littlefs file corresponding to the file block for the
 * file name and the offset of the data to be returned.
 */
void mimic_fat_file_entry(uint32_t request_block, void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=%u mimic_fat_file_entry()\e[0m\n", request_block);
    uint8_t dummy[DISK_BLOCK_SIZE] = "";
    uint8_t filename[LFS_NAME_MAX + 1];
    uint32_t offset = 0;
    bool is_dir = false;
    uint16_t cluster = 0;

    if (fat_table_value(request_block - 1) == 0) {
        printf("  not assign fat table: %u\n", request_block);
        return;
    }

    if (!find_lfs_file(request_block, &cluster, filename, &offset, &is_dir)) {
        printf("  mimic_fat_file_entry: request_block=%u cluster=%u File not found\n", request_block, request_block - 1);
        return;
    }
    if (is_dir) {
        printf("  mimic_fat_file_entry: request_block=%u  cluster=%u directory=\"%s\"\n", request_block, request_block - 1, filename);
        create_fat_dir_entry(filename, request_block, cluster, buffer, bufsize);
        print_dir_entry(buffer);
        return;
    }

    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    lfs_file_t f;
    printf("  mimic_fat_file_entry: request_block=%u cluster=%u file=\"%s\" offset=%u\n", request_block, request_block - 1, filename, offset);
    err = lfs_file_open(&fs, &f, filename, LFS_O_RDONLY);
    if (err < 0) {
        printf("can't open littlefs '%s' rc=%d\n", filename, err);
        lfs_unmount(&fs);
        return;
    }

    uint8_t sector[DISK_BLOCK_SIZE];
    lfs_file_seek(&fs, &f, offset, LFS_SEEK_SET);
    lfs_file_read(&fs, &f, sector, sizeof(sector));
    memcpy(buffer, sector, bufsize);
    lfs_unmount(&fs);
    return;
}

/*
 * Save buffers sent by the host to LFS temporary files
 */
static bool save_temporary_file(uint32_t request_block, void *buffer) {
    printf("save_temporary_file: cluster=%u\n", request_block - 1);
    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    if (err != LFS_ERR_OK) {
        printf("save_temporary_file: lfs_mount err=%d\n", err);
        return false;
    }

    struct lfs_info finfo;
    err = lfs_stat(&fs, "/.fat", &finfo);
    if (err == LFS_ERR_NOENT) {
        err = lfs_mkdir(&fs, "/.fat");
        if (err != LFS_ERR_OK) {
            printf("can't create .fat directory: err=%d\n", err);
            return false;
        }
    }

    char filename[LFS_NAME_MAX + 1];
    sprintf(filename, "/.fat/%04d", request_block - 1);
    lfs_file_t f;
    err = lfs_file_open(&fs, &f, filename, LFS_O_RDWR|LFS_O_CREAT);
    if (err != LFS_ERR_OK) {
        printf("save_temporary_file: can't lfs_file_open '%s' err=%d\n", filename, err);
        return false;
    }

    lfs_file_write(&fs, &f, buffer, 512);
    lfs_file_close(&fs, &f);
    lfs_unmount(&fs);
    return 1;
}

static int read_temporary_file(uint32_t request_block, void *buffer) {
    lfs_t fs;

    printf("read_temporary_file: cluster=%u\n", request_block - 1);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);

    char filename[LFS_NAME_MAX + 1];
    sprintf(filename, "/.fat/%04d", request_block - 1);
    lfs_file_t f;
    err = lfs_file_open(&fs, &f, filename, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("read_temporary_file: can't open '%s': err=%d\n", filename, err);
        lfs_unmount(&fs);
        return err;
    }

    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, 512);
    if (size != 512) {
        printf("read_temporary_file: can't read '%s': size=%u\n", filename, size);
        lfs_file_close(&fs, &f);
        lfs_unmount(&fs);
        return err;
    }

    lfs_file_close(&fs, &f);
    lfs_unmount(&fs);
    return LFS_ERR_OK;
}

/*
 * Recover FAT directory entries from LFS temporary files
 */
static bool load_lfs_fat_temporary_dir_entry(uint32_t request_block, fat_dir_entry_t *dir) {
    uint32_t cluster = request_block - 1;

    lfs_t fs;
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    struct lfs_info finfo;
    err = lfs_stat(&fs, "/.fat", &finfo);
    if (err == LFS_ERR_NOENT) {
        err = lfs_mkdir(&fs, "/.fat");
        if (err != LFS_ERR_OK) {
            printf("can't create .fat directory: err=%d\n", err);
            return false;
        }
    }

    char filename[LFS_NAME_MAX + 1];
    sprintf(filename, "/.fat/%04d", cluster);
    lfs_file_t f;
    err = lfs_file_open(&fs, &f, filename, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("can't open %s: err=%d\n", filename, err);
        return false;
    }

    lfs_file_read(&fs, &f, dir, 512);
    lfs_file_close(&fs, &f);
    lfs_unmount(&fs);
    return true;
}

static void diff_dir_entry(fat_dir_entry_t *orig,
                           fat_dir_entry_t *new,
                           fat_dir_entry_t *result_append,
                           fat_dir_entry_t *result_update)
{
    fat_dir_entry_t *result_append_backup = result_append;
    for (int i = 0; i < 16; i++) {
        if (memcmp(new[i].DIR_Name, ".          ", 11) == 0 || memcmp(new[i].DIR_Name, "..         ", 11) == 0) {
            continue;
        }

        bool is_exists = false;
        for (int j = 0; j < 16; j++) {
            if (memcmp(orig[j].DIR_Name, ".          ", 11) == 0 || memcmp(orig[j].DIR_Name, "..         ", 11) == 0) {
                continue;
            }

            if (memcmp(new[i].DIR_Name, orig[j].DIR_Name, 11) == 0) {
                is_exists = true;
                break;
            }
        }
        if (!is_exists) {
            printf("diff_dir_entry: + '%s' cluster=%u\n", new[i].DIR_Name, new[i].DIR_FstClusLO);
            memcpy(result_append, &new[i], sizeof(fat_dir_entry_t));
            result_append++;
        }

        bool is_update = false;
        for (int j = 0; j < 16; j++) {
            if (memcmp(orig[j].DIR_Name, ".          ", 11) == 0 || memcmp(orig[j].DIR_Name, "..         ", 11) == 0) {
                continue;
            }

            if (memcmp(new[i].DIR_Name, orig[j].DIR_Name, 11) == 0
                && memcmp(&new[i], &orig[j], sizeof(fat_dir_entry_t)) != 0)
            {
                printf("diff_dir_entry: ! '%s' cluster=%u\n", new[i].DIR_Name, new[i].DIR_FstClusLO);
                memcpy(result_update, &new[i], sizeof(fat_dir_entry_t));
                result_update++;
            }
        }
    }
}

/*
 * Restore the *result_filename of the file_cluster_id file belonging to directory_cluster_id.
 */
static void restore_file_from(uint8_t *result_filename, uint32_t directory_cluster_id, uint32_t file_cluster_id) {
    int cluster_id = directory_cluster_id;
    int parent = 0;
    int current = file_cluster_id;

    fat_dir_entry_t dir[16];
    uint8_t result[LFS_NAME_MAX + 1];
    memset(result, 0, sizeof(result));
    if (directory_cluster_id == 0 && file_cluster_id == 0) {
        printf("  this is initial cluster\n");
        //return;
    }

    while (cluster_id >= 0) {
        printf("restore_file_from: while dir cluster_id=%u, base directory_cluster_id=%u file_cluster_id=%u\n", cluster_id, directory_cluster_id, file_cluster_id);
        if ((cluster_id == 0 || cluster_id == 1) && !load_lfs_fat_temporary_dir_entry(1, &dir[0])) {
            printf("temporary file '/.fat/%04d' not found\n", 0);
            break;
        } else if (!load_lfs_fat_temporary_dir_entry(cluster_id + 1, &dir[0])) {
            printf("temporary file '/.fat/%04d' not found\n", cluster_id);
            break;
        }

        //print_dir_entry(&dir);

        uint8_t child_filename[LFS_NAME_MAX + 1];
        uint8_t filename[LFS_NAME_MAX + 1];
        uint16_t long_filename[LFS_NAME_MAX + 1];

        bool is_long_filename = false;
        for (int i = 0; i < 16; i++) {
            if (dir[i].DIR_Attr == 0x08) {
                parent = -1;
                continue;
            }
            if (dir[i].DIR_Name[0] == '\0') {
                break;
            }
            if (memcmp(dir[i].DIR_Name, ".          ", 11) == 0) {
                continue;
            }
            if (memcmp(dir[i].DIR_Name, "..         ", 11) == 0) {
                parent = dir[i].DIR_FstClusLO;
                continue;
            }

            if (dir[i].DIR_Attr & 0x0F == 0x0F) {
                fat_lfn_t *long_file = (fat_lfn_t *)&dir[i];
                if (long_file->LDIR_Ord & 0x40) {
                    memset(long_filename, 0xFF, sizeof(long_filename));
                    is_long_filename = true;
                }
                int offset = (long_file->LDIR_Ord & 0x0F) - 1;
                memcpy(&long_filename[offset * 13 + 0], long_file->LDIR_Name1, sizeof(uint16_t) * 5);
                memcpy(&long_filename[offset * 13 + 5], long_file->LDIR_Name2, sizeof(uint16_t) * 6);
                memcpy(&long_filename[offset * 13 + 5 + 6], long_file->LDIR_Name3, sizeof(uint16_t) * 2);
                continue;
            }

            printf("restore_file_from: test dir[%d], cluster_id=%u, DIR_FstClusLO=%u\n", i, cluster_id, dir[i].DIR_FstClusLO);
            if (dir[i].DIR_Attr & 0x10) { // is directory
                if (is_long_filename) {
                    utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
                } else {
                    restore_from_short_dirname(filename, dir[i].DIR_Name);
                }

                if (dir[i].DIR_FstClusLO == current) {
                    strcpy(child_filename, result);
                    sprintf(result, "%s/%s", filename, child_filename);
                    break;
                }

                is_long_filename = false;
                continue;
            } else if (dir[i].DIR_Attr & 0x20) { // is file
                if (is_long_filename) {
                    utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
                } else {
                    restore_from_short_filename(filename, dir[i].DIR_Name);
                }

                if (dir[i].DIR_FstClusLO == file_cluster_id) {
                    strcpy(result, filename);
                    current = cluster_id;
                    break;

                }
                is_long_filename = false;
            } else {
                printf("  unknown DIR_Attr=0x%02X\n", dir[i].DIR_Attr);
            }
        }

        cluster_id = parent;
    }

    strncpy(result_filename, result, LFS_NAME_MAX + 1);
}

/*
 * Restore directory_cluster_id filename to *directory
 */
static void restore_directory_from(uint8_t *directory, uint32_t base_directory_cluster_id, uint32_t directory_cluster_id) {
    int cluster_id = base_directory_cluster_id;
    int parent = 0;
    int current = directory_cluster_id;

    fat_dir_entry_t dir[16];
    uint8_t result[LFS_NAME_MAX + 1];
    memset(result, 0, sizeof(result));

    while (cluster_id >= 0) {
        if ((cluster_id == 0 || cluster_id == 1) && !load_lfs_fat_temporary_dir_entry(1, &dir[0])) {
            printf("temporary file '/.fat/%04d' not found\n", 0);
            break;

        } else if (!load_lfs_fat_temporary_dir_entry(cluster_id + 1, &dir[0])) {
            printf("temporary file '/.fat/%04d' not found\n", cluster_id);
            break;
        }

        uint8_t child_filename[LFS_NAME_MAX + 1];
        uint8_t filename[LFS_NAME_MAX + 1];
        uint16_t long_filename[LFS_NAME_MAX + 1];

        bool is_long_filename = false;
        for (int i = 0; i < 16; i++) {
            if (dir[i].DIR_Attr == 0x08) {
                parent = -1;
                continue;
            }
            if (dir[i].DIR_Name[0] == '\0') {
                break;
            }
            if (memcmp(dir[i].DIR_Name,    ".          ", 11) == 0) {
                continue;
            }
            if (memcmp(dir[i].DIR_Name, "..         ", 11) == 0) {
                parent = dir[i].DIR_FstClusLO;
                continue;
            }

            if (dir[i].DIR_Attr & 0x0F == 0x0F) {
                fat_lfn_t *long_file = (fat_lfn_t *)&dir[i];
                if (long_file->LDIR_Ord & 0x40) {
                    memset(long_filename, 0xFF, sizeof(long_filename));
                    is_long_filename = true;
                }
                int offset = (long_file->LDIR_Ord & 0x0F) - 1;
                memcpy(&long_filename[offset * 13 + 0], long_file->LDIR_Name1, sizeof(uint16_t) * 5);
                memcpy(&long_filename[offset * 13 + 5], long_file->LDIR_Name2, sizeof(uint16_t) * 6);
                memcpy(&long_filename[offset * 13 + 5 + 6], long_file->LDIR_Name3, sizeof(uint16_t) * 2);
                continue;
            }
            if (dir[i].DIR_Attr & 0x10) { // is directory
                if (is_long_filename) {
                    utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
                } else {
                    restore_from_short_dirname(filename, dir[i].DIR_Name);
                }

                if (dir[i].DIR_FstClusLO == current) {
                    strcpy(child_filename, result);
                    if (strlen(child_filename) == 0) {
                        strcpy(result, filename);
                    } else {
                        sprintf(result, "%s/%s", filename, child_filename);
                    }
                    current = cluster_id;
                    break;
                }

                is_long_filename = false;
                continue;
            } else {
                is_long_filename = false;
            }
        }

        cluster_id = parent;
    }

    strncpy(directory, result, LFS_NAME_MAX + 1);
}


static int littlefs_mkdir(uint8_t *filename) {
    lfs_t fs;

    printf(ANSI_RED "littlefs_mkdir('%s') ..." ANSI_CLEAR, filename);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    if (err != LFS_ERR_OK) {
        printf("ng: err=%d\n", err);
        return err;
    }
    err = lfs_mkdir(&fs, filename);
    if (err != LFS_ERR_OK && err != LFS_ERR_EXIST) {
        printf("ng: err=%d\n", err);
        lfs_unmount(&fs);
        return err;
    }
    lfs_unmount(&fs);

    printf("ok\n");

    return LFS_ERR_OK;
}

static int littlefs_write(uint8_t *filename, uint32_t cluster, size_t size) {
    lfs_t fs;
    uint8_t buffer[512];

    printf(ANSI_RED "littlefs_write('%s', cluster=%u) ..." ANSI_CLEAR, filename, cluster);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    if (err != LFS_ERR_OK) {
        printf("ng: lfs_mount err=%d\n", err);
        return err;
    }

    lfs_file_t f;
    lfs_file_open(&fs, &f, filename, LFS_O_RDWR|LFS_O_CREAT);
    size_t remind = size;

    while (true) {
        err = read_temporary_file(cluster + 1,  buffer);
        if (err != LFS_ERR_OK) {
            printf("ng: read_temporary_file err=%d\n", err);
            lfs_file_close(&fs, &f);
            lfs_unmount(&fs);
            return err;
        }
        size_t s = lfs_file_write(&fs, &f, buffer, sizeof(buffer));
        if (s != 512) {
            printf("ng: lfs_file_write, %u < %u\n", s, 512);
            lfs_file_close(&fs, &f);
            lfs_unmount(&fs);
            return -1;
        }
        int next_cluster = fat_table_value(cluster);
        if (next_cluster >= 0xFF8) {  // eof
            break;
        }
        cluster = next_cluster;
    }
    err = lfs_file_truncate(&fs, &f, size);
    if (err != LFS_ERR_OK) {
        printf("ng: truncate err=%d\n", err);
        lfs_file_close(&fs, &f);
        lfs_unmount(&fs);
        return err;
    }
    lfs_file_close(&fs, &f);

    lfs_unmount(&fs);
    printf("ok\n");
    return 0;
}

/*
 * Update a file or directory from the difference indicated by *src in dir_cluster_id
 *
 * *src is an array of differences created by diff_dir_entry()
 */
static void create_lfs_file_or_directory(fat_dir_entry_t *src, uint32_t dir_cluster_id) {
    printf("create_lfs_file_or_directory(dir_cluster_id=%u)\n", dir_cluster_id);
    uint8_t filename[LFS_NAME_MAX + 1];
    uint8_t directory[LFS_NAME_MAX + 1];
    uint16_t long_filename[LFS_NAME_MAX + 1];
    uint32_t current_directory = 0;

    memset(long_filename, 0, sizeof(long_filename));

    print_dir_entry(src);

    strcpy(directory, "");

    bool is_long_filename = false;
    for (int i = 0; i < 16; i++) {
        fat_dir_entry_t *dir = &src[i];
        if (dir->DIR_Name[0] == '\0') {
            break;
        }
        if (memcmp(dir->DIR_Name, "..         ", 11) == 0) {

            continue;
        }
        if (memcmp(dir->DIR_Name,    ".          ", 11) == 0) {
            current_directory = dir->DIR_FstClusLO;
            continue;
        }

        if ((dir->DIR_Attr & 0x0F) == 0x0F) {
            fat_lfn_t *long_file = (fat_lfn_t *)dir;
            if (long_file->LDIR_Ord & 0x40) {
                memset(long_filename, 0xFF, sizeof(long_filename));
                is_long_filename = true;
            }
            int offset = (long_file->LDIR_Ord & 0x0F) - 1;
            memcpy(&long_filename[offset * 13], long_file->LDIR_Name1, sizeof(uint16_t) * 5);
            memcpy(&long_filename[offset * 13 + 5], long_file->LDIR_Name2, sizeof(uint16_t) * 6);
            memcpy(&long_filename[offset * 13 + 5 + 6], long_file->LDIR_Name3, sizeof(uint16_t) * 2);
            continue;
        }
        if (dir->DIR_Attr & 0x10) { // is directory
            if (is_long_filename) {
                utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
            } else {
                restore_from_short_dirname(filename, dir->DIR_Name);
            }
            restore_directory_from(directory, dir_cluster_id, dir->DIR_FstClusLO);
            littlefs_mkdir(directory);

            is_long_filename = false;

            continue;
        } else if (dir->DIR_Attr & 0x20 || dir->DIR_Attr & 0xF0 == 0x00) { // is file
            if (is_long_filename) {
                utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
            } else {
                restore_from_short_filename(filename, dir->DIR_Name);
            }

            if (dir->DIR_FstClusLO == 0) {
                printf(" Files not yet assigned cluster=0\n");
                break;
            }

            printf("call restore_file_from(directory=%u, target=%u)\n", dir_cluster_id, dir->DIR_FstClusLO);
            restore_file_from(filename, dir_cluster_id,  dir->DIR_FstClusLO);
            littlefs_write(filename, dir->DIR_FstClusLO, dir->DIR_FileSize);
            is_long_filename = false;
            continue;
        } else {
            printf(" unknown DIR_Attr = 0x%02X\n", dir->DIR_Attr);
        }
        is_long_filename = false;
    }
}

/*
 * Check if the cluster of request_block is a directory
 */
static bool is_dir_entry(uint32_t request_block, uint32_t target) {
    fat_dir_entry_t current[16];
    bool is_directory = false;

    printf("is_dir_entry(cluster=%u, target=%u)\n", request_block - 1, target);
    while (!is_directory) {
        if (!load_lfs_fat_temporary_dir_entry(request_block, current)) {
            printf("is_dir_entry: file not found cluster=%u\n", request_block - 1);
            return false;
        }
        print_dir_entry(current);

        for (int i = 0; i < 16; i++) {
            fat_dir_entry_t *dir = &current[i];
            if (memcmp(dir->DIR_Name, ".          ", 11) == 0) {
                continue;
            }
            if (memcmp(dir->DIR_Name, "..         ", 11) == 0) {
                continue;
            }

            if (dir->DIR_Name[0] == '\0') {
                break;
            }
            if (dir->DIR_Attr & 0x0F == 0x0F) {
                continue;
            }
            if ((dir->DIR_FstClusLO == target) && (dir->DIR_Attr & 0x10)) {
                printf("is_dir_entry: hit dir entry cluster=%u\n", request_block - 1);
                is_directory = true;
                break;
            } else if ((dir->DIR_FstClusLO == target) && !(dir->DIR_Attr & 0x10)) {
                printf("is_dir_entry: hit file entry cluster=%u\n", request_block - 1);
                return false;
                break;
            } else if (dir->DIR_Attr & 0x10) {
                is_directory = is_dir_entry(dir->DIR_FstClusLO + 1, target);
                if (is_directory) {
                    return is_directory;
                }
            }
        }
        break;
    }
    return is_directory;
}


static void update_dir_entry(uint32_t request_block, void *buffer) {
    fat_dir_entry_t orig[16];
    fat_dir_entry_t *new = buffer;
    fat_dir_entry_t dir_append[16];
    fat_dir_entry_t dir_update[16];

    memset(orig, 0, sizeof(orig));
    if (!load_lfs_fat_temporary_dir_entry(request_block, orig)) {
        printf("update_dir_entry: entry not found cluster=%d\n", request_block - 1);
        return;
    }

    memset(dir_append, 0, sizeof(dir_append));
    memset(dir_update, 0, sizeof(dir_update));

    diff_dir_entry(orig, new, dir_append, dir_update);
    save_temporary_file(request_block, buffer);

    printf("update_dir_entry: + append dir entry--------\n");
    create_lfs_file_or_directory(dir_append, request_block - 1);

    printf("update_dir_entry: ! update dir entry-------\n");
    create_lfs_file_or_directory(dir_update, request_block - 1);
}

/*
 * Save request_blocks not associated with a resource in a temporary file
 */
static void update_file_entry(uint32_t request_block, void *buffer) {
    printf("save_temporary_file cluster=%u\n", request_block - 1);
    save_temporary_file(request_block, buffer);
    print_block(buffer, 512);
}


void mimic_fat_write(uint8_t lun, uint32_t request_block, uint32_t offset, void *buffer, uint32_t bufsize) {
    printf("\e[35mWrite block=%u offset=%u\e[0m\n", request_block, offset);

    if (request_block == 0) { // master boot record
        return;
    } else if (request_block == 1) { // FAT table
        memcpy(fat_table, buffer, sizeof(fat_table));
        printf("  mimic_fat_write: update_fat_table\n");
    } else if (request_block == 2) { // root dir entry
        printf("  mimic_fat_write: update root_dir_entry\n");

        fat_dir_entry_t orig[16];

        create_fat_dir_entry("/", request_block - 1, request_block, (void *)orig, 512);

        fat_dir_entry_t dir_append[16];
        fat_dir_entry_t dir_update[16];
        memset(dir_append, 0, sizeof(dir_append));
        memset(dir_update, 0, sizeof(dir_update));

        diff_dir_entry(&orig[0], (fat_dir_entry_t *)buffer, dir_append, dir_update);
        printf("create lfs file or directory-------\n");
        save_temporary_file(request_block, buffer);
        save_temporary_file(1, buffer); // FIXME

        create_lfs_file_or_directory(dir_append, request_block - 1);

        printf("updated file or directory--------\n");
        create_lfs_file_or_directory(dir_update, request_block - 1);

    } else { // data or directory entry
        if (is_dir_entry(2, request_block - 1)) {
            printf("update_dir_entry cluster=%u\n", request_block - 1);
            update_dir_entry(request_block, buffer);
        } else {
            printf("update_file_entry cluster=%u\n", request_block - 1);
            update_file_entry(request_block, buffer);
        }
    }
}
