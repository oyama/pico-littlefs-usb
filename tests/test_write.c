#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
extern int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;


static void setup(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void update_fat_table(uint8_t *buffer, uint16_t cluster, uint16_t value) {
    uint16_t offset = (uint16_t)floor((float)cluster + ((float)cluster / 2)) % DISK_BLOCK_SIZE;
    if (cluster & 0x01) {
        buffer[offset] = (buffer[offset] & 0x0F) | (value << 4);
        buffer[offset + 1] = value >> 4;
    } else {
        buffer[offset] = value;
        buffer[offset + 1] = (buffer[offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
    }
}

static void test_create_file(void) {
    uint8_t buffer[512];
    const uint8_t message[] = "Hello World\n";

    setup();
    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 2;
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, message, sizeof(buffer));
    tud_msc_write10_cb(0, cluster + 1, 0, buffer, sizeof(buffer));  // write to anonymous cache

    uint8_t fat_table[512] = {0xF8, 0xFF, 0xFF, 0x00, 0x00};
    update_fat_table(fat_table, cluster, 0xFFF);
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "TEST    TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = cluster, .DIR_FileSize = strlen(message)},
    };
    tud_msc_write10_cb(0, 2, 0, root, sizeof(root));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "TEST.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(message));
    assert(strcmp(buffer, message) == 0);
    lfs_file_close(&fs, &f);

    cleanup();
}

static void test_create_accross_blocksize(void) {
    uint8_t buffer[512];
    uint8_t read_buffer[1024];
    const uint8_t message[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n" \
                              "Hello World\n";  //512 byte + "Hello World\n"

    setup();
    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 2;
    memcpy(buffer, message, sizeof(buffer));
    tud_msc_write10_cb(0, cluster + 1, 0, buffer, sizeof(buffer));  // write to first block
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, message + 512, sizeof(buffer));
    tud_msc_write10_cb(0, cluster + 2, 0, buffer, sizeof(buffer));  // write to 2nd block

    uint8_t fat_table[512] = {0xF8, 0xFF, 0xFF, 0x00, 0x00};
    update_fat_table(fat_table, cluster, cluster + 1);  // point next cluster
    update_fat_table(fat_table, cluster + 1, 0xFFF);  // terminate allocate chain
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "TEST    TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = cluster, .DIR_FileSize = strlen(message)},
    };
    tud_msc_write10_cb(0, 2, 0, root, sizeof(root));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "TEST.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, read_buffer, sizeof(read_buffer));
    assert(size == strlen(message));
    assert(strcmp(read_buffer, message) == 0);
    lfs_file_close(&fs, &f);

    cleanup();
}


void test_write(void) {
    printf("write ..................");

    test_create_file();
    test_create_accross_blocksize();
    //test_update_file();

    printf("ok\n");
}
