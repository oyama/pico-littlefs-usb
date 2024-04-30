#include "tests.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/regs/addressmap.h>

extern const struct lfs_config lfs_pico_flash_config;  // littlefs_driver.c
extern int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize);

static lfs_t fs;
static struct lfs_config test_config = {
    .read_size      = 1,
    .prog_size      = FLASH_PAGE_SIZE,
    .block_size     = FLASH_SECTOR_SIZE,
    .block_count    = 340 * 1024 / FLASH_SECTOR_SIZE,
    .cache_size     = FLASH_SECTOR_SIZE,
    .lookahead_size = 16,
    .block_cycles   = 500,
};

static void setup(void) {
    test_config.read = lfs_pico_flash_config.read;
    test_config.prog = lfs_pico_flash_config.prog;
    test_config.erase = lfs_pico_flash_config.erase;
    test_config.sync = lfs_pico_flash_config.sync;

    int err = lfs_format(&fs, (const struct lfs_config *)&test_config);
    assert(err == 0);
    err = lfs_mount(&fs, (const struct lfs_config *)&test_config);
    assert(err == 0);

}

static void reload(void) {
    lfs_unmount(&fs);
    int err = lfs_mount(&fs, &test_config);
    assert(err == 0);
}


static void cleanup(void) {
    lfs_unmount(&fs);
}

static void test_read_file(void) {
    uint8_t buffer[512];

    setup();

    create_file(&fs, "READ.TXT", "Hello World!\n");

    mimic_fat_init(&test_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 0, 0, buffer, sizeof(buffer));  // Boot sector
    tud_msc_read10_cb(0, 1, 0, buffer, sizeof(buffer));  // Allocation table
    uint8_t expected_allocation_table[512] = {0xf8, 0xff, 0xff, 0xff, 0x0f, 0x00};
    assert(memcmp(buffer, expected_allocation_table, 6) == 0);

    uint16_t fat_sectors = fat_sector_size(&test_config);
    uint32_t root_dir_sector = fat_sectors + 1;

    tud_msc_read10_cb(0, root_dir_sector, 0, buffer, sizeof(buffer));  // Root directory entry
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "READ    TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = strlen("Hello World!\n")},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, root) == 0);

    uint32_t test_file_sector = root_dir_sector + 1;
    tud_msc_read10_cb(0, test_file_sector, 0, buffer, sizeof(buffer));  // TEST.TXT
    assert(strcmp(buffer, "Hello World!\n") == 0);
    cleanup();
}

static void test_read_large_file(void) {
    setup();

    lfs_file_t f;
    int rc = lfs_file_open(&fs, &f, "LARGE.TXT", LFS_O_RDWR|LFS_O_CREAT);
    assert(rc == 0);
    uint8_t buffer[512];
    for (size_t i = 0; i < 340; i++) {
        memset(buffer, i & 0xFF, sizeof(buffer));
        lfs_ssize_t size = lfs_file_write(&fs, &f, buffer, sizeof(buffer));
        assert(size == sizeof(buffer));
    }
    rc = lfs_file_close(&fs, &f);
    assert(rc == 0);

    reload();

    mimic_fat_init(&test_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 0, 0, buffer, sizeof(buffer));  // Boot sector
    tud_msc_read10_cb(0, 1, 0, buffer, sizeof(buffer));  // Allocation table

    uint16_t fat_sectors = fat_sector_size(&test_config);
    uint32_t root_dir_sector = fat_sectors + 1;

    tud_msc_read10_cb(0, root_dir_sector, 0, buffer, sizeof(buffer));  // Root directory entry
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "LARGE   TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = 512*340},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, root) == 0);

    uint32_t first_sector = root_dir_sector + 1;
    for (size_t i = 0; i < 340; i++) {
        char expected[512];
        memset(expected, i & 0xFF, sizeof(expected));

        tud_msc_read10_cb(0, first_sector + i, 0, buffer, sizeof(buffer));
        assert(memcmp(expected, buffer, sizeof(expected)) == 0);
    }

    cleanup();
}


void test_change_storage_size(void) {
    printf("large size FAT..........");

    //test_read_file();
    test_read_large_file();

    printf("ok\n");
}
