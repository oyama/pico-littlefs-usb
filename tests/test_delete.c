#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;


#define MESSAGE  "Please delete me!\n"

static void setup_file(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);

    create_file(&fs, "DELETEME.TXT", MESSAGE);
}

static void setup_dir(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);

    create_directory(&fs, "DIR_DEL");
}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_delete_file(void) {
    setup_file();

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 2;
    uint8_t buffer[512];

    // update dir entry. Assign a delete flag to the filename
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "DELETEMETXT", .DIR_Attr = 0x20, .DIR_FstClusLO = cluster, .DIR_FileSize = strlen(MESSAGE)},
    };
    root[1].DIR_Name[0] = 0xE5;  // delete flag
    tud_msc_write10_cb(0, 2, 0, root, sizeof(root));  // update directory entry

    // update File allocation table
    uint8_t fat_table[512] = {0xF8, 0xFF, 0xFF, 0xFF, 0x0F};  // With cluster 2 allocated
    update_fat_table(fat_table, cluster, 0x000);
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    reload();

    // Test reflection on the littlefs layer
    struct lfs_info finfo;
    int err = lfs_stat(&fs, "DELETEME.TXT", &finfo);
    assert(err ==  LFS_ERR_NOENT);

    cleanup();
}

static void test_delete_dir(void) {
    setup_dir();

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 2;
    uint8_t buffer[512];

    // update dir entry. Assign a delete flag to the filename
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "DIR_DEL    ", .DIR_Attr = 0x10, .DIR_FstClusLO = cluster, .DIR_FileSize = 0},
    };
    root[1].DIR_Name[0] = 0xE5;  // delete flag
    tud_msc_write10_cb(0, 2, 0, root, sizeof(root));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    struct lfs_info finfo;
    int err = lfs_stat(&fs, "DIR_DEL", &finfo);
    assert(err ==  LFS_ERR_NOENT);

    cleanup();
}


void test_delete(void) {
    printf("delete .................");

    test_delete_file();
    test_delete_dir();

    printf("ok\n");
}
