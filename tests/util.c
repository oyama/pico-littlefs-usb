#include <math.h>
#include "tests.h"


static const int FAT_SHORT_NAME_MAX = 11;
static const int FAT_SHORT_8x3_MAX = 8 + 3 + 1;
static const int FAT_LONG_FILENAME_CHUNK_MAX = 13;


void print_block(uint8_t *buffer, size_t l) {
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

void print_dir_entry(void *buffer) {
    uint8_t pbuffer[11+1];
    fat_dir_entry_t *dir = (fat_dir_entry_t *)buffer;
    printf("--------\n");
    for (int i = 0; i < DISK_SECTOR_SIZE / sizeof(fat_dir_entry_t); i++) {
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
            uint8_t utf8[13 * 4 + 1];
            utf16le_to_utf8(utf8, sizeof(utf8), utf16le, 13);
            printf("name='%s' attr=0x%02X ord=0x%02X cluster=%u\n", utf8, lfn->LDIR_Attr, lfn->LDIR_Ord, dir->DIR_FstClusLO);
        }
        dir++;
    }
}

void update_fat_table(uint8_t *buffer, uint16_t cluster, uint16_t value) {
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % DISK_SECTOR_SIZE;
    if (cluster & 0x01) {
        buffer[offset] = (buffer[offset] & 0x0F) | (value << 4);
        buffer[offset + 1] = value >> 4;
    } else {
        buffer[offset] = value;
        buffer[offset + 1] = (buffer[offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
    }
}

uint16_t fat_sector_size(struct lfs_config *c) {
    uint64_t storage_size = c->block_count * c->block_size;
    uint32_t cluster_size = storage_size / (DISK_SECTOR_SIZE * 1);
    return ceil((double)cluster_size / DISK_SECTOR_SIZE);
}

void create_file(lfs_t *fs, const unsigned char *path, const unsigned char *content) {
    lfs_file_t f;
    int err = lfs_file_open(fs, &f, path, LFS_O_RDWR|LFS_O_CREAT);
    assert(err == 0);
    size_t size = lfs_file_write(fs, &f, content, strlen(content));
    assert(size == strlen(content));
    lfs_file_close(fs, &f);
}

void create_directory(lfs_t *fs, const unsigned char *path) {
    int err = lfs_mkdir(fs, path);
    assert(err == 0);
}

void set_long_filename_entry(fat_lfn_t *ent, uint8_t *name, uint8_t order) {
    ent->LDIR_Ord = order;
    ent->LDIR_Attr = 0x0F;
    ent->LDIR_Name1;

    uint16_t chunk[13];
    size_t l = utf8_to_utf16le(chunk, sizeof(chunk), name, 13);
    memcpy(ent->LDIR_Name1, chunk + 0, sizeof(uint16_t) * 5);
    memcpy(ent->LDIR_Name2, chunk + 5, sizeof(uint16_t) * 6);
    memcpy(ent->LDIR_Name3, chunk + 5 + 6, sizeof(uint16_t) * 2);
    for (int i = 0; i < 5*2; i += 2) {
        if (ent->LDIR_Name1[i] == 0x00 && ent->LDIR_Name1[i+1] == 0x00) {
            ent->LDIR_Name1[i] = 0xFF;
            ent->LDIR_Name1[i+1] = 0xFF;
        }
    }
    for (int i = 0; i < 6*2; i += 2) {
        if (ent->LDIR_Name2[i] == 0x00 && ent->LDIR_Name2[i+1] == 0x00) {
            ent->LDIR_Name2[i] = 0xFF;
            ent->LDIR_Name2[i+1] = 0xFF;
        }
    }
    for (int i = 0; i < 2*2; i += 2) {
        if (ent->LDIR_Name3[i] == 0x00 && ent->LDIR_Name3[i+1] == 0x00) {
            ent->LDIR_Name3[i] = 0xFF;
            ent->LDIR_Name3[i+1] = 0xFF;
        }
    }
    if (l < 13 && l < 5) {
        ent->LDIR_Name1[l * 2]     = 0x00;
        ent->LDIR_Name1[l * 2 + 1] = 0x00;
    } else if (l < FAT_LONG_FILENAME_CHUNK_MAX && l < FAT_SHORT_NAME_MAX) {
        ent->LDIR_Name2[(l-5) * 2]     = 0x00;
        ent->LDIR_Name2[(l-5) * 2 + 1] = 0x00;
    } else if (l < FAT_LONG_FILENAME_CHUNK_MAX) {
        ent->LDIR_Name3[(l-5-6) * 2]     = 0x00;
        ent->LDIR_Name3[(l-5-6) * 2 + 1] = 0x00;
    }
}

int dirent_cmp(fat_dir_entry_t *a, fat_dir_entry_t *b) {
    for (int i = 0; i < 16; i++) {
        if (memcmp((a + i)->DIR_Name, (b + i)->DIR_Name, 11) != 0)
            return -1;
        if ((a + i)->DIR_Attr != (b + i)->DIR_Attr)
            return -1;
        if ((a + i)->DIR_FstClusLO != (b + i)->DIR_FstClusLO)
            return -1;
        if ((a + i)->DIR_FileSize != (b + i)->DIR_FileSize)
            return -1;
    }
    return 0;
}

int dirent_cmp_lfn(fat_dir_entry_t *a, fat_dir_entry_t *b) {
    bool is_long = false;
    for (int i = 0; i < 16; i++) {
        if ((a + i)->DIR_Attr == 0x0F) {
            fat_lfn_t *al = (fat_lfn_t *)(a + i);
            fat_lfn_t *bl = (fat_lfn_t *)(b + i);
            if (memcmp(al->LDIR_Name1, bl->LDIR_Name1, sizeof(uint16_t) * 5) != 0)
                return -1;
            if (memcmp(al->LDIR_Name2, bl->LDIR_Name2, sizeof(uint16_t) * 6) != 0)
                return -1;
            if (memcmp(al->LDIR_Name3, bl->LDIR_Name3, sizeof(uint16_t) * 2) != 0)
                return -1;
            if (al->LDIR_Attr != bl->LDIR_Attr)
                return -1;
            if (al->LDIR_Ord != bl->LDIR_Ord)
                return -1;
            is_long = true;
        } else {
            if (is_long == false && memcmp((a + i)->DIR_Name, (b + i)->DIR_Name, 11) != 0)
                return -1;
            if ((a + i)->DIR_Attr != (b + i)->DIR_Attr)
                return -1;
            if ((a + i)->DIR_FstClusLO != (b + i)->DIR_FstClusLO)
                return -1;
            if ((a + i)->DIR_FileSize != (b + i)->DIR_FileSize)
                return -1;
            is_long = false;
        }
    }
    return 0;
}
