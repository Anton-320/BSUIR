#pragma once
#include "general_fun.h"
#include "structs.h"
#include "lfn.h"
#include "io.h"
#include <curses.h>

uint8_t autoFixOpt = 0;     // Чинить автоматически (1 - да, 0 - нет)
uint8_t viewInfoOpt = 0;    // Показать информацию о ФС
uint8_t showFileTree = 0;   // Вывести файловое дерево на экран

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
int check_fat_table(bool print);

/**
 * Проверка дерева каталогов
 * Начало обработки - первая дочерняя (от корневого каталога) запись
 * @param[in] offset    Смещение директории (файла с 32-байтными записями)
 * @param[in] path      Полный путь к директории (для корневого каталога == "/")
 * @param[in] depth     Глубина в дереве каталогов (для корневой директории == 0)
*/
void read_and_check_dir_tree(off_t offset, const char *path, int depth);

/**
 * Попытаться найти резервную копию загрузочного сектора.
 * Результат записывается в статическую переменную bs
 * @return В случае успеха возвращает смещение нормальной копии относительно начала раздела, иначе -1
*/
off_t try_find_boot_sector_copy();

/**
 * Ищет резервную копию FAT-таблицы, проверяет и записывает в 
 * @returns true (1), если удалось найти корректную резервную копию FAT-таблицы, иначе false(0)
*/
bool try_find_fat_copy();