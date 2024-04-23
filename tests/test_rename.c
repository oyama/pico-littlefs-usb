#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);
extern int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;

#define MESSAGE  "please rename!\n"


static void setup(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);

    create_file(&fs, "ORIGINAL.TXT", MESSAGE);
}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_rename_file(void) {
    setup();

    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint8_t buffer[512];
    uint16_t cluster = 2;

    // update dir entry. The old filename is flagged for deletion and a new file entry
    // is added at the same time. Clusters have the same.
    fat_dir_entry_t root0[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "ORIGINALTXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen(MESSAGE)},
        {.DIR_Name = "RENAMED TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen(MESSAGE)},
    };
    root0[1].DIR_Name[0] = 0xE5;
    tud_msc_write10_cb(0, 2, 0, root0, sizeof(root0));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "RENAMED.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(MESSAGE));
    assert(strcmp(buffer, MESSAGE) == 0);
    lfs_file_close(&fs, &f);

    struct lfs_info finfo;
    err = lfs_stat(&fs, "ORIGINAL.TXT", &finfo);
    assert(err ==  LFS_ERR_NOENT);

    cleanup();
}

static void test_rename_file_windows11(void) {
    setup();

    mimic_fat_initialize_cache();

    // Update procedure from the USB layer
    uint8_t buffer[512];
    uint16_t cluster = 2;

    //  The file name of the directory entry is changed.
    fat_dir_entry_t root0[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "RENAMED TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen(MESSAGE)},
    };
    tud_msc_write10_cb(0, 2, 0, root0, sizeof(root0));  // update directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "RENAMED.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(MESSAGE));
    assert(strcmp(buffer, MESSAGE) == 0);
    lfs_file_close(&fs, &f);

    struct lfs_info finfo;
    err = lfs_stat(&fs, "ORIGINAL.TXT", &finfo);
    assert(err ==  LFS_ERR_NOENT);

    cleanup();
}

void test_rename(void) {
    printf("rename .................");

    test_rename_file();
    test_rename_file_windows11();

    printf("ok\n");
}
