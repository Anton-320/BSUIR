/**
 * fs_fun.h - Промежуточные операции в файловой системе (чтение FAT таблицы, загрузочного сектора и т.п.) 
 * PS LBA - Logic Block Addressing - метод адресации секторов на диске (начиная с 0)
*/


#pragma once
#include "io.h"
#include "structs.h"
#include "lfn.h"
#include <endian.h>

//Размер записи таблицы FAT
#define FAT_RECORD_SIZE 4



/**
 * Считать основные поля загрузочного сектора
 * @param[in] offset    Смещение от начала раздела, по которому считывать загрузочный сектор (первые 90 байт)
*/
BootSector init_boot_sector(off_t offset);

/**
 * Записать загрузочный сектор
*/
void write_boot_sector(const BootSector *bs, off_t offset);

/**
 * Вычислить смещение таблицы FAT32
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
*/
off_t get_fat_offset(int num, const BootSector* bs);

/**
 * Прочитать FAT-таблицу 
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
*/
uint32_t* read_fat_table(int num, const BootSector* bs);

/**
 * Записать FAT-таблицу из буфера в раздел
 * @param[in] num Номер таблицы, начиная с 0
 * @param[in] fat Буфер с FAT-таблицей
 * @param[in] entCount Количество входов FAT-таблицы
 * @param[in] bs указатель на структуру загрузочного сектора
*/
void write_fat_table(int num, uint32_t *fat, uint32_t entCount, const BootSector *bs);

void end_program() {
    printf("Завершение работы утилиты\n");
    system("pause");
}

/**
 * Поиск номера сектора (начиная с 0), с которого начинается кластер, по номеру кластера
 * @param [in]  N   номер кластера
 * @param [in]  bs  указатель на структуру загрузочного сектора
 * @returns     LBA (logic block address) - номер сектора, начиная с 0
*/
uint64_t get_fileEnt_lba(uint32_t N, const BootSector *bs);
