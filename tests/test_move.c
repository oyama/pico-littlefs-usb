#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;

#define MESSAGE  "please move!\n"


static void setup(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);

    create_directory(&fs, "DIR_A");
    create_file(&fs, "MOVEME.TXT", MESSAGE);
}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_move_file(void) {
    setup();

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    // Update procedure from the USB layer
    uint8_t buffer[512];

    // update the origin dir entry. Attach deletion flag to filename.
    fat_dir_entry_t root0[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "DIR_A      ", .DIR_Attr = 0x10, .DIR_FstClusLO = 2, .DIR_FileSize = 0},
        {.DIR_Name = "MOVEME  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 3, .DIR_FileSize = strlen(MESSAGE)},
    };
    root0[2].DIR_Name[0] = 0xE5;  // deletion flag
    tud_msc_write10_cb(0, 2, 0, root0, sizeof(root0));  // update origin directory entry

    fat_dir_entry_t dir_a[16] = {
        {.DIR_Name = ".          ", .DIR_Attr = 0x10, .DIR_FstClusLO = 2, .DIR_FileSize = 0},
        {.DIR_Name = "..         ", .DIR_Attr = 0x10, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "MOVEME  TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 3, .DIR_FileSize = strlen(MESSAGE)},
    };
    tud_msc_write10_cb(0, 3, 0, dir_a, sizeof(dir_a));  // update destination directory entry

    reload();

    // Test reflection on the littlefs layer
    lfs_file_t f;
    int err = lfs_file_open(&fs, &f, "DIR_A/MOVEME.TXT", LFS_O_RDONLY);
    assert(err == LFS_ERR_OK);
    lfs_ssize_t size = lfs_file_read(&fs, &f, buffer, sizeof(buffer));
    assert(size == strlen(MESSAGE));
    assert(strcmp(buffer, MESSAGE) == 0);
    lfs_file_close(&fs, &f);

    struct lfs_info finfo;
    err = lfs_stat(&fs, "MOVEME.TXT", &finfo);
    assert(err ==  LFS_ERR_NOENT);

    cleanup();
}

void test_move(void) {
    printf("move   .................");

    test_move_file();

    printf("ok\n");
}
