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
      0xEB, 0x3C, 0x90, // BS_JmpBoot
      'M', 'S', 'D', 'O', 'S', '5', '.', '0', // BS_OEMName
      0x00, 0x02, // BPB_BytsPerSec
      0x01, // BPB_SecPerClus
      0x01, 0x00, // BPB_RsvdSecCnt
      0x01, // BPB_NumFATs
      0x10, 0x00, // BPB_RootEntCnt
      DISK_BLOCK_NUM & 0xFF, DISK_BLOCK_NUM >> 8, // BPB_TotSec16
      0xF8, // BPB_Media
      0x01, 0x00, // BPB_FATSz16
      0x01, 0x00, // BPB_SecPerTrk
      0x01, 0x00, // BPB_NumHeads
      0x00, 0x00, 0x00, 0x00, // BPB_HiddSec
      0x00, 0x00, 0x00, 0x00, // BPB_TotSec32
      0x80, // BS_DrvNum
      0x00, // BS_Reserved
      0x29, // BS_BootSig
      0x34, 0x12, 0x00, 0x00, // BS_VolID
      'l' , 'i' , 't' , 't' , 'l' , 'e' , 'f' , 's' , 'U' , 'S' , 'B' , // BS_VolLab
      0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, // BS_FilSysType
      0x00, 0x00,
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

static lfs_t real_filesystem;


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
            printf("name='%s' attr=0x%02X timeTenth=%u, CrtDate=%u,%u"
                   " writeDateTime=%u,%u LstAccDate=%u Size=%u cluster=%u\n",
                   pbuffer,
                   dir->DIR_Attr,
                   dir->DIR_CrtTimeTenth,
                   dir->DIR_CrtDate, dir->DIR_CrtTime,
                   dir->DIR_WrtDate, dir->DIR_WrtTime,
                   dir->DIR_LstAccDate,
                   dir->DIR_FileSize,
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

static uint16_t fat_table_value(int cluster) {
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
    snprintf(short_filename, sizeof(buffer), "%-8s%-3s", basename, ext);
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

void set_volume_label_entry(fat_dir_entry_t *dir, const char *name) {
    uint8_t sfn_name[FAT_SHORT_NAME_MAX + 1];
    snprintf(sfn_name, sizeof(sfn_name), "%-11s", name);

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
    snprintf(sfn_name, sizeof(sfn_name), "%-11s", name);
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
    dir->DIR_Attr = 0x20;
    dir->DIR_NTRes = 0;
    dir->DIR_CrtTimeTenth = 0xC6;
    dir->DIR_CrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_CrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_LstAccDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusHI = 0;
    dir->DIR_WrtTime = LITTLE_ENDIAN16(0x526D);
    dir->DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
    dir->DIR_FstClusLO = info->size > 0 ? LITTLE_ENDIAN16(cluster) : 0;
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

/*
 * Save buffers sent by the host to LFS temporary files
 */
static bool save_temporary_file(uint32_t cluster, void *buffer) {
    printf("save_temporary_file: cluster=%u\n", cluster);

    struct lfs_info finfo;
    int err = lfs_stat(&real_filesystem, ".mimic", &finfo);
    if (err == LFS_ERR_NOENT) {
        err = lfs_mkdir(&real_filesystem, ".mimic");
        if (err != LFS_ERR_OK) {
            printf("can't create .mimic directory: err=%d\n", err);
            return false;
        }
    }

    char filename[LFS_NAME_MAX + 1];
    snprintf(filename, sizeof(filename), ".mimic/%04d", cluster);
    lfs_file_t f;
    err = lfs_file_open(&real_filesystem, &f, filename, LFS_O_RDWR|LFS_O_CREAT);
    if (err != LFS_ERR_OK) {
        printf("save_temporary_file: can't lfs_file_open '%s' err=%d\n", filename, err);
        return false;
    }

    lfs_file_write(&real_filesystem, &f, buffer, 512);
    lfs_file_close(&real_filesystem, &f);
    return 1;
}

static int read_temporary_file(uint32_t cluster, void *buffer) {
    lfs_file_t f;
    char filename[LFS_NAME_MAX + 1];

    snprintf(filename, sizeof(filename), ".mimic/%04d", cluster);
    int err = lfs_file_open(&real_filesystem, &f, filename, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("read_temporary_file: can't open '%s': err=%d\n", filename, err);
        return err;
    }

    lfs_ssize_t size = lfs_file_read(&real_filesystem, &f, buffer, 512);
    if (size != 512) {
        printf("read_temporary_file: can't read '%s': size=%u\n", filename, size);
        lfs_file_close(&real_filesystem, &f);
        return err;
    }

    lfs_file_close(&real_filesystem, &f);
    return LFS_ERR_OK;
}

static bool delete_temporary_file(uint32_t cluster) {
    printf("delete_temporary_file: cluster=%u\n", cluster);

    char filename[LFS_NAME_MAX + 1];
    snprintf(filename, sizeof(filename), ".mimic/%04d", cluster);
    int err = lfs_remove(&real_filesystem, filename);
    if (err != LFS_ERR_OK) {
        printf("delete_temporary_file: can't lfs_remove '%s' error=%d\n", filename, err);
        return false;
    }
    return true;
}

static fat_dir_entry_t *append_dir_entry_volume_label(fat_dir_entry_t *entry, const char *volume_label) {
    uint8_t name[FAT_SHORT_NAME_MAX + 1];

    snprintf(name, sizeof(name), "%-11s", volume_label);
    memcpy(entry->DIR_Name, name, FAT_SHORT_NAME_MAX);
    entry->DIR_Attr = 0x08;
    entry->DIR_NTRes = 0;
    entry->DIR_CrtTimeTenth = 0;
    entry->DIR_CrtTime = 0;
    entry->DIR_CrtDate = 0;
    entry->DIR_LstAccDate = 0;
    entry->DIR_FstClusHI = 0;
    entry->DIR_WrtTime = LITTLE_ENDIAN16(0x4F6D);
    entry->DIR_WrtDate = LITTLE_ENDIAN16(0x6543);
    entry->DIR_FstClusLO = 0;
    entry->DIR_FileSize = 0;
    entry++;
    return entry;
}

static fat_dir_entry_t *append_dir_entry_directory(fat_dir_entry_t *entry, struct lfs_info *finfo, uint32_t cluster) {
    printf("append_dir_entry_directory '%s'\n", finfo->name);

    uint8_t short_name[FAT_SHORT_NAME_MAX + 1];

    if (strcmp(finfo->name, ".") == 0 || strcmp(finfo->name, "..") == 0) {
        set_directory_entry(entry, finfo->name, cluster);
    }
    else if (is_short_filename_dir((uint8_t *)finfo->name)) {
        set_directory_entry(entry, finfo->name, cluster);
    } else {
        fat_dir_entry_t short_dir_entry;
        uint8_t buffer[FAT_SHORT_NAME_MAX + 1];

        set_directory_entry(&short_dir_entry, finfo->name, cluster);
        create_shortened_short_filename_dir(short_dir_entry.DIR_Name, finfo->name);
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, short_dir_entry.DIR_Name, FAT_SHORT_NAME_MAX);
        uint8_t check_sum = filename_check_sum(short_dir_entry.DIR_Name);
        int long_filename_num = floor((strlen(finfo->name) - 1) / FAT_LONG_FILENAME_CHUNK_MAX);

        for (int i = long_filename_num; i >= 0; i--) {
            uint8_t order = i + 1;
            uint8_t chunk[FAT_LONG_FILENAME_CHUNK_MAX + 1];
            uint8_t *head = (uint8_t *)&(finfo->name[i * FAT_LONG_FILENAME_CHUNK_MAX]);
            strncpy(chunk, head, FAT_LONG_FILENAME_CHUNK_MAX);
            chunk[FAT_LONG_FILENAME_CHUNK_MAX] = '\0';
            if (i == long_filename_num)
                order |= 0x40;
            set_long_file_entry(entry, chunk, order, check_sum);
            entry++;
        }
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, short_dir_entry.DIR_Name, FAT_SHORT_NAME_MAX);
        memcpy(entry, &short_dir_entry, sizeof(short_dir_entry));
    }
    entry++;
    return entry;
}

static fat_dir_entry_t *append_dir_entry_file(fat_dir_entry_t *entry, struct lfs_info *finfo, uint32_t cluster) {
    uint8_t short_name[FAT_SHORT_NAME_MAX + 1];

    if (is_short_filename_file((uint8_t *)finfo->name)) {
        set_file_entry(entry, finfo, cluster);
    } else {
        fat_dir_entry_t short_dir_entry;
        uint8_t buffer[FAT_SHORT_NAME_MAX + 1];

        set_file_entry(&short_dir_entry, finfo, cluster);
        create_shortened_short_filename(short_dir_entry.DIR_Name, finfo->name);
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, short_dir_entry.DIR_Name, FAT_SHORT_NAME_MAX);

        uint8_t check_sum = filename_check_sum(short_dir_entry.DIR_Name);
        int long_filename_num = floor((strlen(finfo->name) - 1) / FAT_LONG_FILENAME_CHUNK_MAX);
        for (int i = long_filename_num; i >= 0; i--) {
            uint8_t order = i + 1;
            uint8_t chunk[FAT_LONG_FILENAME_CHUNK_MAX + 1];
            uint8_t *head = (uint8_t *)&(finfo->name[i * FAT_LONG_FILENAME_CHUNK_MAX]);
            strncpy(chunk, head, FAT_LONG_FILENAME_CHUNK_MAX);
            chunk[FAT_LONG_FILENAME_CHUNK_MAX] = '\0';
            if (i == long_filename_num)
                order |= 0x40;
            set_long_file_entry(entry, chunk, order, check_sum);
            entry++;
        }
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, short_dir_entry.DIR_Name, FAT_SHORT_NAME_MAX);
        memcpy(entry, &short_dir_entry, sizeof(short_dir_entry));
    }

    entry++;
    return entry;
}


/*
 * Create a directory entry cache corresponding to the base file system
 *
 * Recursively traverse the specified base file system directory and update cache and allocation tables.
 */
static int create_dir_entry_cache(unsigned char *path, uint32_t parent_cluster, uint32_t *allocated_cluster) {
    printf("create_dir_entry_cache('%s', %u, %u)\n", path, parent_cluster, *allocated_cluster);
    uint32_t current_cluster = *allocated_cluster;
    fat_dir_entry_t *entry, dir_entry[DISK_BLOCK_SIZE / sizeof(fat_dir_entry_t)];
    lfs_dir_t dir;
    struct lfs_info finfo;
    unsigned char directory_path[LFS_NAME_MAX + 1];
    memset(dir_entry, 0, sizeof(dir_entry));
    entry = dir_entry;

    if (parent_cluster == 0) {
        entry = append_dir_entry_volume_label(entry, "littlefsUSB");
        current_cluster = 1;
    }
    update_fat_table(current_cluster, 0xFFF);

    int err = lfs_dir_open(&real_filesystem, &dir, path);
    if (err != LFS_ERR_OK) {
        printf("create_dir_entry_cache: lfs_dir_open('%s') error=%d\n", path, err);
        return err;
    }

    while (true) {
        err = lfs_dir_read(&real_filesystem, &dir, &finfo);
        if (err == 0)
            break;
        if (err < 0) {
            printf("create_dir_entry_cache: lfs_dir_read('%s') error=%d\n", path, err);
            break;
        }

        if (finfo.type == LFS_TYPE_DIR && parent_cluster == 0 && strcmp(finfo.name, ".mimic") == 0)
            continue;

        if (finfo.type == LFS_TYPE_DIR && parent_cluster == 0
            && (strcmp(finfo.name, ".") == 0 || strcmp(finfo.name, "..") == 0))
        {
            continue;
        }
        if (finfo.type == LFS_TYPE_DIR  && strcmp(finfo.name, ".") == 0) {
            entry = append_dir_entry_directory(entry, &finfo, current_cluster);
            continue;
        }
        if (finfo.type == LFS_TYPE_DIR  && strcmp(finfo.name, "..") == 0) {
            if (parent_cluster == 0)
                entry = append_dir_entry_directory(entry, &finfo, 0);
            else
                entry = append_dir_entry_directory(entry, &finfo, parent_cluster);
            continue;
        }

        if (finfo.type == LFS_TYPE_DIR) {
            *allocated_cluster += 1;
            update_fat_table(*allocated_cluster, 0xFFF);
            entry = append_dir_entry_directory(entry, &finfo, *allocated_cluster);
            if (parent_cluster == 0)
                strncpy(directory_path, finfo.name, sizeof(directory_path));
            else
                snprintf(directory_path, sizeof(directory_path), "%s/%s", path, finfo.name);

            err = create_dir_entry_cache(directory_path, current_cluster, allocated_cluster);
            if (err < 0) {
                lfs_dir_close(&real_filesystem, &dir);
                return err;
            }

        } else if (finfo.type == LFS_TYPE_REG) {
            uint32_t file_cluster = *allocated_cluster + 1;
            if (finfo.size > 0) {
                for (int i = ceil((double)finfo.size / DISK_BLOCK_SIZE); i > 0; i--) {
                    *allocated_cluster += 1;
                    if (i == 1) {
                        update_fat_table(*allocated_cluster, 0xFFF);
                    } else {
                        update_fat_table(*allocated_cluster, *allocated_cluster + 1);
                    }
                }
            }
            entry = append_dir_entry_file(entry, &finfo, file_cluster);
        }
    }
    lfs_dir_close(&real_filesystem, &dir);
    save_temporary_file(current_cluster, dir_entry);
    //print_dir_entry(dir_entry);
    return 0;
}

#include <tusb.h>

/*
 * Rebuild the directory entry cache.
 *
 * Execute when USB is connected.
 */
void mimic_fat_initialize_cache(void) {
    printf(ANSI_RED "Mount: mimic_fat_initialize_cache()\n" ANSI_CLEAR);

    lfs_unmount(&real_filesystem);
    int err = lfs_mount(&real_filesystem, &lfs_pico_flash_config);
    if (err < 0) {
        printf("mimic_fat_initialize_cache: lfs_mount error=%d\n", err);
        return;
    }
    /*
    uint8_t buffer[512];
    for (int i = 1; i < 128; i++) {
        save_temporary_file(i, buffer);
        tud_task();
    }
    printf("done\n");
    */

    uint32_t allocated_cluster = 1;
    create_dir_entry_cache("", 0, &allocated_cluster);
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

    memcpy(buffer, fat_table, bufsize);
    //print_fat_table(10);
}

/*
 * Check if the cluster of request_block is a directory
 */
static int test_is_dir_entry(uint32_t request_block, uint32_t target, bool *is_directory) {
    fat_dir_entry_t current[16];

    while (!*is_directory) {
        if (read_temporary_file(request_block - 1, current) != 0) {
            printf("is_dir_entry: file not found cluster=%u\n", request_block - 1);
            return -1;
        }
        //print_dir_entry(current);

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
                *is_directory = true;
                break;
            } else if ((dir->DIR_FstClusLO == target) && !(dir->DIR_Attr & 0x10)) {
                *is_directory = false;
                return 0;
            } else if (dir->DIR_Attr & 0x10) {
                int err = test_is_dir_entry(dir->DIR_FstClusLO + 1, target, is_directory);
                if (err != 0) {
                    return err;
                }
                if (*is_directory) {
                    return 0;
                }
            }
        }
        break;
    }
    return 0;
}

/*
 * Restore the *result_filename of the file_cluster_id file belonging to directory_cluster_id.
 */
static void restore_file_from(uint8_t *result_filename, uint32_t directory_cluster_id, uint32_t file_cluster_id) {
    printf("restore_file_from(directory_cluster_id=%u, file_cluster_id=%u)\n", directory_cluster_id, file_cluster_id);
    assert(file_cluster_id >= 2);

    if (directory_cluster_id == 0) {
        directory_cluster_id = 1;
    }

    int cluster_id = directory_cluster_id;
    int parent = 1;
    int target = file_cluster_id;

    fat_dir_entry_t dir[16];
    uint8_t result[LFS_NAME_MAX + 1];
    memset(result, 0, sizeof(result));
    if (directory_cluster_id == 0 && file_cluster_id == 0) {
        printf("  this is initial cluster\n");
        //return;
    }

    uint32_t self = 0;
    while (cluster_id >= 0) {
        printf("restore_file_from: cluster_id=%u, parent=%u, target=%u\n", cluster_id, parent, target);
        if ((cluster_id == 0 || cluster_id == 1) && read_temporary_file(1, &dir[0]) != 0) {
            printf("temporary file '.mimic/%04d' not found\n", 1);
            break;
        } else if (read_temporary_file(cluster_id, &dir[0]) != 0) {
            printf("temporary file '.mimic/%04d' not found\n", cluster_id);
            break;
        }

        printf("restore_file_from---\n");
        print_dir_entry(&dir);
        printf("--------------------\n");
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
                self = dir[i].DIR_FstClusLO;
                continue;
            }
            if (memcmp(dir[i].DIR_Name, "..         ", 11) == 0) {
                parent = dir[i].DIR_FstClusLO;
                if (parent == 0) {
                    /* NOTE: According to the FAT specification, the reference to the root
                     * directory is `cluster==0`, but the actual state of the root directory
                     * is `cluster==1`, so it needs to be corrected.
                     */
                    parent = 1;  // Actual root directory
                }
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

                if (dir[i].DIR_FstClusLO == target) {
                    strcpy(child_filename, result);
                    snprintf(result, sizeof(result), "%s/%s", filename, child_filename);
                    break;
                }

                is_long_filename = false;
                continue;
            } else if (dir[i].DIR_Attr & 0x20 || dir[i].DIR_Attr == 0x00) { // is file
                if (is_long_filename) {
                    utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
                } else {
                    restore_from_short_filename(filename, dir[i].DIR_Name);
                }

                if (dir[i].DIR_FstClusLO == target) {
                    strcpy(result, filename);
                    target = cluster_id;
                    break;

                }
                is_long_filename = false;
            } else {
                printf("  unknown DIR_Attr=0x%02X\n", dir[i].DIR_Attr);
            }
        }

        cluster_id = parent;
        target = self;
    }

    strncpy(result_filename, result, LFS_NAME_MAX + 1);
}

/*
 * Return the FAT root directory entry when USB request sector 2.
 * Explore the littlefs root directory and build file information.
 */
void mimic_fat_root_dir_entry(void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=2 mimic_fat_root_dir_entry()\e[0m\n");

    read_temporary_file(1, buffer);
    return;
}


/*
 * Search for base cluster in the Allocation table
 *
 * Traverse the allocation table in reverse order and return the length of the allocation chain in offset.
 */
static uint32_t find_base_cluster_and_offset(uint32_t cluster, size_t *offset) {
    uint16_t value, next_cluster;
    bool is_exists = false;
    uint32_t find_limit = DISK_BLOCK_NUM;

    if (cluster > DISK_BLOCK_NUM) {
        return 0;
    }
    if (fat_table_value(cluster) == 0x00) {
        return 0;
    }

    *offset = 0;
    while (find_limit-- > 0) {
        is_exists = false;
        for (int i = 0; i < DISK_BLOCK_NUM; i++) {
            next_cluster = fat_table_value(i);
            if (next_cluster >= 0xFF8 || next_cluster == 0x00)
                continue;
            if (next_cluster == cluster) {
                cluster = i;  // i is the cluster number cvhained to cluster
                is_exists = true;
                *offset += 1;
                break;
            }
        }
        if (!is_exists)
            break;
    }
    return cluster;
}

typedef struct {
    bool is_found;
    uint8_t path[LFS_NAME_MAX + 1];
    uint32_t directory_cluster;
    bool is_directory;
} find_dir_entry_cache_result_t;

/*
 * Restore directory_cluster_id filename to *directory
 */
static void restore_directory_from(uint8_t *directory, uint32_t base_directory_cluster_id, uint32_t directory_cluster_id) {
    int cluster_id = base_directory_cluster_id;
    int parent = 0;
    int target = directory_cluster_id;

    fat_dir_entry_t dir[16];
    uint8_t result[LFS_NAME_MAX + 1];
    memset(result, 0, sizeof(result));

    while (cluster_id >= 0) {
        if ((cluster_id == 0 || cluster_id == 1) && read_temporary_file(1, &dir[0]) != 0) {
            printf("temporary file '.mimic/%04d' not found\n", 1);
            break;

        } else if (read_temporary_file(cluster_id, &dir[0]) != 0) {
            printf("temporary file '.mimic/%04d' not found\n", cluster_id);
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

                if (dir[i].DIR_FstClusLO == target) {
                    strcpy(child_filename, result);
                    if (strlen(child_filename) == 0) {
                        strncpy(result, filename, sizeof(result));
                    } else {
                        snprintf(result, sizeof(result), "%s/%s", filename, child_filename);
                    }
                    target = cluster_id;
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


static int find_dir_entry_cache(find_dir_entry_cache_result_t *result, uint32_t base_cluster, uint32_t target_cluster) {
    //printf("find_dir_entry_cache(base=%u, target=%u)\n", base_cluster, target_cluster);
    fat_dir_entry_t entry[16];

    int err = read_temporary_file(base_cluster, entry);
    if (err != LFS_ERR_OK) {
        printf("find_dir_entry_cache: read_temporary_file(cluster=%u) error=%d\n", base_cluster, err);
        return err;
    }

    for (int i = (base_cluster == 1 ? 1 : 2); i < 16; i++) {
        if (strncmp(entry[i].DIR_Name, "..         ", 11) == 0)
            continue;
        if (entry[i].DIR_Name[0] == 0)
            break;

        if (entry[i].DIR_FstClusLO == target_cluster) {
            result->is_found = true;
            result->directory_cluster = base_cluster;
            result->is_directory = (entry[i].DIR_Attr & 0x10) ? true : false;

            if (result->is_directory)
                restore_directory_from(result->path, base_cluster, target_cluster);
            else
                restore_file_from(result->path, base_cluster, target_cluster);
            return 1;
        }
        if ((entry[i].DIR_Attr & 0x10) == 0)
            continue;

        err = find_dir_entry_cache(result, entry[i].DIR_FstClusLO, target_cluster);
        if (err < 0) {
            return err;
        } else if (err == 1) {
            return 1;
        }
    }
    return 0;
}

/*
 */
void mimic_fat_read_cluster(uint32_t cluster, void *buffer, uint32_t bufsize) {
    printf("\e[36mRead block=%u mimic_fat_read_cluster()\e[0m\n", cluster + 1);

    size_t offset = 0;
    find_dir_entry_cache_result_t result;

    if (cluster == 1) {
        read_temporary_file(cluster, buffer);
        return;
    }

    uint16_t base_cluster = find_base_cluster_and_offset(cluster, &offset);
    if (base_cluster == 0) { // is not allocated
        printf("is not allocated\n");
        return;
    }

    memset(&result, 0, sizeof(result));
    int err = find_dir_entry_cache(&result, 1, base_cluster);
    if (err < 0) {
        return;
    }
    if (result.is_directory) {
        read_temporary_file(cluster, buffer);
        return;
    }

    printf("mimic_fat_read_cluster: result.path='%s'\n", result.path);

    lfs_file_t f;
    err = lfs_file_open(&real_filesystem, &f, result.path, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("mimic_fat_read_cluster: lfs_file_open('%s') error=%d\n", result.path, err);
        return;
    }

    lfs_file_seek(&real_filesystem, &f, offset * DISK_BLOCK_SIZE, LFS_SEEK_SET);
    lfs_file_read(&real_filesystem, &f, buffer, bufsize);
    lfs_file_close(&real_filesystem, &f);
}

static void difference_of_dir_entry(fat_dir_entry_t *orig, fat_dir_entry_t *new,
                                    fat_dir_entry_t *update,
                                    fat_dir_entry_t *delete)
{
    bool is_found = false;

    printf("difference_of_dir_entry-----\n");
    print_dir_entry(orig);
    printf("----------------------------\n");
    print_dir_entry(new);
    printf("----------------------------\n");
    for (int i = 0; i < 16; i++) {
        if (strncmp(new[i].DIR_Name, ".          ", 11) == 0
            || strncmp(new[i].DIR_Name, "..         ", 11) == 0
            || (new[i].DIR_Attr & 0x0F) == 0x0F
            || (new[i].DIR_Attr & 0x08) == 0x08) // volume label
        {
            continue;
        }

        if (new[i].DIR_Name[0] == 0xE5) {
            for (int j = 0; j < 16; j++) {
                if ((orig[j].DIR_Attr & 0x08) == 0x08) // volume label
                    continue;
                if (new[i].DIR_FstClusLO == orig[j].DIR_FstClusLO
                    && new[i].DIR_FileSize == orig[j].DIR_FileSize
                    && orig[j].DIR_Name[0] != 0xE5
                    && new[i].DIR_FileSize != 0)
                {
                    memcpy(delete, &orig[j], sizeof(fat_dir_entry_t));
                    delete++;
                    break;
                }
            }
            continue;
        }

        is_found = false;
        for (int j = 0; j < 16; j++) {
            if (new[i].DIR_Name[0] == 0xE5
               || strncmp(orig[j].DIR_Name, ".          ", 11) == 0
               || strncmp(orig[j].DIR_Name, "..         ", 11) == 0
               || (orig[j].DIR_Attr & 0x0F) == 0x0F
               || (orig[j].DIR_Attr & 0x08) == 0x08)  // volume label
            {
                continue;
            }
            if (strncmp(new[i].DIR_Name, orig[j].DIR_Name, 11) == 0 &&
                new[i].DIR_FstClusLO == orig[j].DIR_FstClusLO &&
                new[i].DIR_FileSize == orig[j].DIR_FileSize)
            {
                is_found = true;
                break;
            }
        }
        if (!is_found) {
            memcpy(update, &new[i], sizeof(fat_dir_entry_t));
            update++;
        }
    }
}

static int littlefs_mkdir(uint8_t *filename) {
    printf(ANSI_RED "littlefs_mkdir('%s') ..." ANSI_CLEAR, filename);
    struct lfs_info finfo;

    int err = lfs_stat(&real_filesystem, filename, &finfo);
    if (err == LFS_ERR_OK) {
        printf("ok\n");
        return LFS_ERR_OK;
    }

    err = lfs_mkdir(&real_filesystem, filename);
    if (err != LFS_ERR_OK && err != LFS_ERR_EXIST) {
        printf("ng: err=%d\n", err);
        return err;
    }

    printf("ok\n");

    return LFS_ERR_OK;
}

static int littlefs_write(uint8_t *filename, uint32_t cluster, size_t size) {
    printf(ANSI_RED "littlefs_write('%s', cluster=%u) ..." ANSI_CLEAR, filename, cluster);

    uint8_t buffer[512];

    if (strlen(filename) == 0) {
        printf(ANSI_RED "littlefs_write: filename not specified\n" ANSI_CLEAR);
        return -1;
    }

    lfs_file_t f;
    int err = lfs_file_open(&real_filesystem, &f, filename, LFS_O_RDWR|LFS_O_CREAT);
    if (err != LFS_ERR_OK) {
        printf("ng: lfs_file_open error=%d\n", err);
        return err;
    }

    size_t remind = size;
    while (true) {
        err = read_temporary_file(cluster,  buffer);
        if (err != LFS_ERR_OK) {
            printf("ng: read_temporary_file error=%d\n", err);
            lfs_file_close(&real_filesystem, &f);
            return err;
        }

        size_t s = lfs_file_write(&real_filesystem, &f, buffer, sizeof(buffer));
        if (s != 512) {
            printf("ng: lfs_file_write, %u < %u\n", s, 512);
            lfs_file_close(&real_filesystem, &f);
            return -1;
        }
        lfs_file_sync(&real_filesystem, &f);

        int next_cluster = fat_table_value(cluster);
        if (next_cluster >= 0xFF8) {  // eof
            break;
        }
        cluster = next_cluster;
    }
    err = lfs_file_truncate(&real_filesystem, &f, size);
    if (err != LFS_ERR_OK) {
        printf("ng: truncate err=%d\n", err);
        lfs_file_close(&real_filesystem, &f);
        return err;
    }
    lfs_file_close(&real_filesystem, &f);

    printf("ok\n");
    return 0;
}

static int littlefs_remove(uint8_t *filename) {
    printf(ANSI_RED "littlefs_remove('%s') ..." ANSI_CLEAR, filename);

    if (strlen(filename) == 0) {
        printf("ng: not allow brank filename\n");
        return LFS_ERR_INVAL;
    }
    int err = lfs_remove(&real_filesystem, filename);
    if (err != LFS_ERR_OK) {
        printf("ng: err=%d\n", err);
        return err;
    }

    printf("ok\n");
    return LFS_ERR_OK;
}

/*
 * Update a file or directory from the difference indicated by *src in dir_cluster_id
 *
 * *src is an array of differences created by diff_dir_entry()
 */
static void update_lfs_file_or_directory(fat_dir_entry_t *src, uint32_t dir_cluster_id) {
    printf("update_lfs_file_or_directory(dir_cluster_id=%u)\n", dir_cluster_id);
    uint8_t filename[LFS_NAME_MAX + 1];
    uint8_t directory[LFS_NAME_MAX + 1];
    uint16_t long_filename[LFS_NAME_MAX + 1];
    uint32_t current_directory = 0;

    memset(long_filename, 0, sizeof(long_filename));

    strcpy(directory, "");

    bool is_long_filename = false;
    for (int i = 0; i < 16; i++) {
        fat_dir_entry_t *dir = &src[i];
        if (dir->DIR_Name[0] == '\0') {
            break;
        }
        if (dir->DIR_Name[0] == 0xE5 || dir->DIR_Name[0] == 0x05) {
            continue;
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
        } else if (dir->DIR_Attr & 0x20 || dir->DIR_Attr == 0x00) { // is file
            if (is_long_filename) {
                utf16le_to_utf8(filename, sizeof(filename), long_filename, sizeof(long_filename));
            } else {
                restore_from_short_filename(filename, dir->DIR_Name);
            }

            if (dir->DIR_FstClusLO == 0) {
                printf(" Files not yet assigned cluster=0\n");
                break;
            }

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

static void delete_dir_entry_cache(fat_dir_entry_t *src, uint32_t dir_cluster_id) {
    uint8_t filename[LFS_NAME_MAX + 1];
    uint32_t next_cluster = 0;

    for (int i = 0; i < 16; i++) {
        fat_dir_entry_t *dir = &src[i];
        if (dir->DIR_Name[0] == '\0')
            break;

        if (dir->DIR_Attr & 0x10) {
            restore_directory_from(filename, dir_cluster_id, dir->DIR_FstClusLO);
        } else {
            restore_file_from(filename, dir_cluster_id, dir->DIR_FstClusLO);
        }
        littlefs_remove(filename);

        next_cluster = dir->DIR_FstClusLO;
        while (true) {
            delete_temporary_file(next_cluster);
            next_cluster = fat_table_value(next_cluster);
            if (next_cluster >= 0xFF8) {
                break;
            }
        }
        continue;
    }
}

static void update_dir_entry(uint32_t cluster, void *buffer) {
    fat_dir_entry_t orig[16];
    fat_dir_entry_t *new = buffer;
    fat_dir_entry_t dir_update[16];
    fat_dir_entry_t dir_delete[16];

    memset(orig, 0, sizeof(orig));
    if (read_temporary_file(cluster, orig) != 0) {
        printf("update_dir_entry: entry not found cluster=%d\n", cluster);
        return;
    }

    memset(dir_update, 0, sizeof(dir_update));
    memset(dir_delete, 0, sizeof(dir_delete));

    difference_of_dir_entry(orig, new, dir_update, dir_delete);
    delete_dir_entry_cache(dir_delete, cluster);

    save_temporary_file(cluster, buffer);
    update_lfs_file_or_directory(dir_update, cluster);
}

/*
 * Save request_blocks not associated with a resource in a temporary file
 */
static void update_file_entry(uint32_t request_block, void *buffer) {
    save_temporary_file(request_block - 1, buffer);
    //print_block(buffer, 512);
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
        fat_dir_entry_t dir_update[16];
        fat_dir_entry_t dir_delete[16];

        memset(dir_update, 0, sizeof(dir_update));
        memset(dir_delete , 0, sizeof(dir_delete));
        read_temporary_file(1, orig);

        difference_of_dir_entry(&orig[0], (fat_dir_entry_t *)buffer, dir_update, dir_delete);
        delete_dir_entry_cache(dir_delete, request_block - 1);

        save_temporary_file(1, buffer);
        save_temporary_file(0, buffer); // FIXME

        update_lfs_file_or_directory(dir_update, request_block - 1);

    } else { // data or directory entry
        bool is_directory = false;
        int err = test_is_dir_entry(2, request_block - 1, &is_directory);
        if (err != 0) {
            printf(ANSI_RED "mimic_fat_write: File update request with dir entry not updated\n" ANSI_CLEAR);
        }

        if (is_directory) {
            printf("update_dir_entry cluster=%u\n", request_block - 1);
            update_dir_entry(request_block - 1, buffer);
        } else {
            printf("update_file_entry cluster=%u\n", request_block - 1);
            update_file_entry(request_block, buffer);
        }
    }
}
