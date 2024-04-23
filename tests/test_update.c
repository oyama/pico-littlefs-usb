#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;


static void setup(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);

    create_file(&fs, "UPDATE.TXT", "Please update!\n");
}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_update_file(void) {
    setup();

    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 3;  // unassigned cluster
    uint8_t buffer[512];
    const uint8_t message[] = "Hello World!\n";
    // write file blocks to unassigned clusters
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, message, sizeof(buffer));
    tud_msc_write10_cb(0, cluster + 1, 0, buffer, sizeof(buffer));  // write to anonymous cache

    // update File allocation table
    uint8_t fat_table[512] = {0xF8, 0xFF, 0xFF, 0xFF, 0x0F};  // With cluster 2 allocated
    update_fat_table(fat_table, cluster, 0xFFF);
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    // update dir entry. The cluster to which the file belongs is set to 0.
    fat_dir_entry_t root0[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "UPDATE  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
    };
    tud_msc_write10_cb(0, 2, 0, root0, sizeof(root0));  // update directory entry

    // update dir entry. The clusters written in step 1 are specified.
    fat_dir_entry_t root1[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "UPDATE  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = cluster, .DIR_FileSize = strlen(message)},
    };
    tud_msc_write10_cb(0, 2, 0, root1, sizeof(root1));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "UPDATE.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(message));
    assert(strcmp(buffer, message) == 0);
    lfs_file_close(&fs, &f);

    cleanup();
}

static void test_update_file_windows11(void) {
    setup();

    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint16_t cluster = 3;  // unassigned cluster
    uint8_t buffer[512];
    const uint8_t message[] = "Hello World!\n";

    // update dir entry. The cluster to which the file belongs is set to 0.
    fat_dir_entry_t root0[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "UPDATE  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
    };
    tud_msc_write10_cb(0, 2, 0, root0, sizeof(root0));  // update directory entry

    // update File allocation table
    uint8_t fat_table[512] = {0xF8, 0xFF, 0xFF, 0xFF, 0x0F};  // With cluster 2 allocated
    update_fat_table(fat_table, cluster, 0xFFF);
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    // update dir entry. The clusters written in step 2 are specified.
    fat_dir_entry_t root1[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "UPDATE  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = cluster, .DIR_FileSize = strlen(message)},
    };
    tud_msc_write10_cb(0, 2, 0, root1, sizeof(root1));  // update directory entry

    // write the file block to the cluster specified in step 3
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, message, sizeof(buffer));
    tud_msc_write10_cb(0, cluster + 1, 0, buffer, sizeof(buffer));

    // update File allocation table
    update_fat_table(fat_table, 2, 0x000);  // Release old allocated areas
    tud_msc_write10_cb(0, 1, 0, fat_table, sizeof(fat_table));  // update file allocated table

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "UPDATE.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(message));
    assert(strcmp(buffer, message) == 0);
    lfs_file_close(&fs, &f);

    cleanup();
}

void test_update(void) {
    printf("update .................");

    test_update_file();
    test_update_file_windows11();

    printf("ok\n");
}
