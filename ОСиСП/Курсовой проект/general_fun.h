#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Завершить работу программы и вывести сообщение об ошибке
*/
void pdie(const char *msg, ...);

/**
 * Выделить память
 * Проверить, было ли выделение удачным
*/
void* alloc(int size);