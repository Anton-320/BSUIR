#pragma once
#include "general_fun.h"
#include <fcntl.h>      //для функции open() и другого
#include <unistd.h>


/**
 * Открыть дисковое устройство с файловой системой
 * Если rw == 0, то только для чтения, иначе для чтения и записи
*/
void fs_open(const char *path, int rw);

/**
 * Чтение данных из раздела
 * 
 * @param[in]   pos     Смещение в байтах относительно начала раздела, с которого считывать              
 * @param[in]   size    Количество байтов для чтения
 * @param[out]  data    Буффер для прочитанных данных
 */
void fs_read(off_t pos, size_t size, void *data);

/**
 * Сместиться на offset байтов (с проверкой)
*/ 
void ch_seek(off_t offset, int whence);

/**
 * Прочитать size байтов (с проверкой)
*/
void ch_read(size_t size, void *data);

int fs_test(off_t pos, int size);

/**
 * Записать в раздел
*/
void fs_write(off_t pos, size_t size, void *data);

/**
 * Закрыть устройство / файл с файловой системой
*/
void fs_close();