#pragma once
#include "structs.h"
#include "lfn.h"
#include "io.h"



/**
 * Проверка всей файловой системы
*/
void check_all();

/**
 * Проверка загрузочного сектора
 * @returns количество неисправностей
*/
int check_boot_sector(bool print);

/**
 * Вывод информации о файловой системе
*/
void print_filesystem_info();

/**
 * Проверка FAT таблицы
 * @param[in] print Выводить ли сообщения об ошибках (true - да, false - нет)
 * @returns         Общее количество ошибок (циклов, разрывов и т.д.)
*/
bool check_fats_are_same(uint32_t fatEntCount, int activeFatInd);

/**
 * Проверка неиспользованных кластеров FAT
 * @param[in] fatEntCount Количество записей таблицы FAT
*/
void check_unused_fat_clusters(uint32_t fatEntCount);

/**
 * Проверка дерева каталогов
 * Начало обработки - первая дочерняя (от корневого каталога) запись
 * @param[in] offset    Смещение директории (файла с 32-байтными записями)
 * @param[in] path      Полный путь к директории (для корневого каталога == "/")
 * @param[in] depth     Глубина в дереве каталогов (для корневой директории == 0)
 * @note	После считывания длинных записей смещение offset обновляется сразу, а после считывания
 * короткой записи offset обновляется (становится в конец короткой записи) только в конце цикла for
*/
void read_and_check_dir_tree(off_t offset, const char *path, int depth);

/**
 * Попытаться найти резервную копию загрузочного сектора.
 * Результат записывается в статическую переменную bs
 * @return В случае успеха возвращает смещение нормальной копии относительно начала раздела, иначе -1
*/
off_t try_find_boot_sector_copy();
