#pragma once
#include "io.h"
#include "structs.h"


/**
 * Поиск входной точки кластера в таблице FAT по номеру кластера
 * @param [in]  N   номер кластера
 * @param [in]  bs  указатель на структуру загрузочного сектора
*/
uint64_t get_fatEnt_offset(uint64_t N, const BootSector *bs);

/**
 * 
*/
uint32_t get_fatOffset(const BootSector* bs);

/**
 * 
*/
FAT_Entry read_fat_entry(int fd, uint32_t cluster_number, const BootSector *bs);

//Размер записи таблицы FAT
#define FAT_ENTRY_SIZE 4