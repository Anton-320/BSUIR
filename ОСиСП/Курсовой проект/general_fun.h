#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <curses.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Завершить работу программы и вывести сообщение об ошибке
*/
void pdie(const char *msg, ...);

/**
 * Выделить память
 * Проверить, было ли выделение удачным
 * Важно! Выделенная память обнулена
*/
void *alloc(size_t amount, size_t size);

/**
 * Ввод челого числа в промежутке от min до max (включая)
 * @param[in] min   Минимальное значение интервала
 * @param[in] max   Максимальное значение интервала
 * @returns         Введённое число
*/
int input_int(int min, int max);

/**
 * Руководство по эксплуатации
*/
void usage(const char *progName);