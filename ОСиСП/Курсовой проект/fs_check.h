#pragma once
#include "fs_fun.h"

/**
 * Проверка загрузочного сектора
*/
int check_boot_sector(const BootSector *bs);

/**
 * Вывод статистики о загрузочном секторе
*/
void print_stat_boot_sector(const BootSector *bs);

void check_for_gaps(int fd, const BootSector *bs, uint32_t totalClusters);

PAIR check_fatTable();