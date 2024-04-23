/*
 * Copyright 2024, Hiroyuki OYAMA. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _TESTS_H_
#define _TESTS_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <lfs.h>
#include "mimic_fat.h"


extern const uint32_t FLASH_BASE;

void test_create(void);
void test_read(void);
void test_update(void);
void test_rename(void);
void test_move(void);
void test_delete(void);

void print_block(uint8_t *buffer, size_t l);
void print_dir_entry(void *buffer);

void create_file(lfs_t *fs, const unsigned char *path, const unsigned char *content);
void create_directory(lfs_t *fs, const unsigned char *path);
void update_fat_table(uint8_t *buffer, uint16_t cluster, uint16_t value);

int dirent_cmp(fat_dir_entry_t *a, fat_dir_entry_t *b);
int dirent_cmp_lfn(fat_dir_entry_t *a, fat_dir_entry_t *b);
void set_long_filename_entry(fat_lfn_t *ent, uint8_t *name, uint8_t order);

#endif

