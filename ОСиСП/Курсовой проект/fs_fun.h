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
 * Прочитать из файловой системы
*/
void fs_read(off_t pos, int size, void *data);


int fs_test(off_t pos, int size);

/**
 * Записать на устройство в файловую систему
*/
void fs_write(off_t pos, int size, void *data);

/**
 * Закрыть устройство / файл с файловой системой
*/
void fs_close(int write);