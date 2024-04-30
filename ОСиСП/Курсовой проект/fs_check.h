#pragma once
#include "fs_fun.h"

/**
 * Проверка загрузочного сектора
*/
int check_boot_sector();

/**
 * Вывод статистики о загрузочном секторе
*/
void print_filesystem_info();

void check_for_gaps(int fd, const BootSector *bs);

PAIR check_fat_table();