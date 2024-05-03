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
    .block_count    = (1.8 * 1024 * 1024) / FLASH_SECTOR_SIZE,
    .cache_size     = FLASH_SECTOR_SIZE,
    .lookahead_size = 16,
    .block_cycles   = 500,
};

static void setup(void) {
    test_config.read = lfs_pico_flash_config.read;
    test_config.prog = lfs_pico_flash_config.prog;
    test_config.erase = lfs_pico_flash_config.erase;
    test_config.sync = lfs_pico_flash_config.sync;

    int err = lfs_format(&fs, (const struct lfs_config *)&lfs_pico_flash_config);
    assert(err == 0);
    err = lfs_mount(&fs, (const struct lfs_config *)&lfs_pico_flash_config);
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

static uint32_t xor_rand(uint32_t *seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}


static void test_read_large_file(size_t file_size) {
    setup();

    lfs_file_t f;
    int rc = lfs_file_open(&fs, &f, "LARGE.TXT", LFS_O_RDWR|LFS_O_CREAT);
    assert(rc == 0);
    uint8_t buffer[512];

    uint32_t counter = file_size;
    xor_rand(&counter);

    size_t remind = file_size;
    size_t total_written = 0;
    size_t segment_size = file_size / 10;
    size_t next_segment_threshold = segment_size;
    while (remind > 0) {
        size_t chunk = remind % sizeof(buffer) ? remind % sizeof(buffer) : sizeof(buffer);
        uint32_t *b = (uint32_t *)buffer;
        for (size_t i = 0; i < (chunk / 4); i++) {
            b[i] = xor_rand(&counter);
        }
        lfs_ssize_t size = lfs_file_write(&fs, &f, buffer, chunk);
        assert(size == chunk);
        remind = remind - size;

        total_written += size;
        if (total_written >= next_segment_threshold || remind == 0) {
            printf(".");
            next_segment_threshold += segment_size;
        }
    }
    rc = lfs_file_close(&fs, &f);
    assert(rc == 0);

    reload();

    mimic_fat_init(&lfs_pico_flash_config);
    mimic_fat_create_cache();

    tud_msc_read10_cb(0, 0, 0, buffer, sizeof(buffer));  // Boot sector
    tud_msc_read10_cb(0, 1, 0, buffer, sizeof(buffer));  // Allocation table

    uint16_t fat_sectors = fat_sector_size(&lfs_pico_flash_config);
    uint32_t root_dir_sector = fat_sectors + 1;

    tud_msc_read10_cb(0, root_dir_sector, 0, buffer, sizeof(buffer));  // Root directory entry
    fat_dir_entry_t root[16] = {
        {.DIR_Name = "littlefsUSB", .DIR_Attr = 0x08, .DIR_FstClusLO = 0, .DIR_FileSize = 0},
        {.DIR_Name = "LARGE   TXT", .DIR_Attr = 0x20, .DIR_FstClusLO = 2, .DIR_FileSize = file_size},
    };
    assert(dirent_cmp((fat_dir_entry_t *)buffer, root) == 0);

    counter = file_size;  // reset random seed
    xor_rand(&counter);
    uint32_t file_sector = root_dir_sector + 1;
    remind = file_size;
    total_written = 0;
    segment_size = file_size / 10;
    next_segment_threshold = segment_size;
    while (remind > 0) {
        size_t chunk = remind % sizeof(buffer) ? remind % sizeof(buffer) : sizeof(buffer);
        char expected[512] = {0};
        uint32_t *b = (uint32_t *)expected;
        for (size_t i = 0; i < (chunk / 4); i++) {
            b[i] = xor_rand(&counter);
        }
        tud_msc_read10_cb(0, file_sector, 0, buffer, sizeof(buffer));
        file_sector++;
        remind = remind - chunk;
        assert(memcmp(expected, buffer, chunk) == 0);

        total_written += chunk;
        if (total_written >= next_segment_threshold || remind == 0) {
            printf(".");
            next_segment_threshold += segment_size;
        }
    }

    cleanup();
}


void test_large_file(void) {
    printf("read 512 bytes file .");
    test_read_large_file(512);
    printf(" ok\n");

    printf("read 1024 bytes file ");
    test_read_large_file(1024);
    printf(" ok\n");

    // NOTE: FAT12 tests whether sector boundaries are handled correctly because FAT is 12 bits wide.
    printf("read 174080 bytes file ");
    test_read_large_file(174080);
    printf(" ok\n");

    printf("read 174592 bytes file ");
    test_read_large_file(174592);
    printf(" ok\n");

    printf("read 348160 bytes file ");
    test_read_large_file(348160);
    printf(" ok\n");

    printf("read 348672 bytes file ");
    test_read_large_file(348672);
    printf(" ok\n");

    printf("read 522240 bytes file ");
    test_read_large_file(522240);
    printf(" ok\n");

    printf("read 522752 bytes file ");
    test_read_large_file(522752);
    printf(" ok\n");
}
