#include "tests.h"


extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;


static void setup(void) {
    int err = lfs_format(&fs, &lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, &lfs_pico_flash_config);
    assert(err == 0);
}

static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_read_file(void) {
    uint8_t buffer[512];

    setup();

    create_file(&fs, "READ.TXT", "Hello World!\n");

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 0, 0, buffer, sizeof(buffer));  // Boot sector
    tud_msc_read10_cb(0, 1, 0, buffer, sizeof(buffer));  // Allocation table
    uint8_t expected_allocation_table[512] = {0xf8, 0xff, 0xff, 0xff, 0x0f, 0x00};
    assert(memcmp(buffer, expected_allocation_table, 6) == 0);

    tud_msc_read10_cb(0, 2, 0, buffer, sizeof(buffer));  // Root directory entry
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "READ    TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen("Hello World!\n")},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, root) == 0);

    tud_msc_read10_cb(0, 3, 0, buffer, sizeof(buffer));  // TEST.TXT
    assert(strcmp(buffer, "Hello World!\n") == 0);

    cleanup();
}

static void test_sub_directory(void) {
    uint8_t buffer[512];

    setup();

    create_directory(&fs, "DIR1");
    create_file(&fs, "DIR1/SUB.TXT", "directory 1\n");

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 0, 0, buffer, sizeof(buffer));  // Boot sector
    tud_msc_read10_cb(0, 1, 0, buffer, sizeof(buffer));  // Allocation table
    uint8_t expected_allocation_table[512] = {0xf8, 0xff, 0xff, 0xff, 0xff, 0xFF, 0x00};
    assert(memcmp(buffer, expected_allocation_table, 7) == 0);

    tud_msc_read10_cb(0, 2, 0, buffer, sizeof(buffer));  // Root directory entry
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "DIR1       ", .DIR_Attr = 0x10, .DIR_FstClusLO = 2, .DIR_FileSize = 0},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, root) == 0);

    tud_msc_read10_cb(0, 3, 0, buffer, sizeof(buffer));  // DIR1 directory entry
    fat_dir_entry_t dir1[16] = {
        {.DIR_Name = ".          ", .DIR_Attr = 0x10, .DIR_FstClusLO = 2, .DIR_FileSize = 0},
        {.DIR_Name = "..         ", .DIR_Attr = 0x10, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "SUB     TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 3, .DIR_FileSize = strlen("directory 1\n")},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, dir1) == 0);

    tud_msc_read10_cb(0, 4, 0, buffer, sizeof(buffer));  // DIR1/SUB.TXT
    assert(strcmp(buffer, "directory 1\n") == 0);

    cleanup();
}

static void test_long_filename(void) {
    uint8_t buffer[512];

    setup();

    create_file(&fs, "over 8x3 long filename.TXT", "long file name\n");

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 2, 0, buffer, sizeof(buffer));  // Root directory entry

    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {},
        {},
        {.DIR_Name = "FIL~4000TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen("long file name\n")},
    };
    set_long_filename_entry((fat_lfn_t *)&root[1], " filename.TXT", 0x42);
    set_long_filename_entry((fat_lfn_t *)&root[2], "over 8x3 long", 0x01);
    assert(dirent_cmp_lfn((fat_dir_entry_t *)buffer, root) == 0);

    cleanup();
}

void test_read(void) {
    printf("read ...................");

    test_read_file();
    test_sub_directory();
    test_long_filename();

    printf("ok\n");
}
