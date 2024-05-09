#include "general_fun.h"

/**
 * Завершить работу программы и вывести сообщение об ошибке
*/
void pdie(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, ": %s\n", strerror(errno));
	exit(1);
}

void* alloc(size_t amount, size_t size)
{
	void* this;

	if ((this = calloc(amount, size)))
		return this;
	pdie("calloc");
		return NULL;		/* for GCC */
}

/**
 * Ввод челого числа в промежутке от min до max (включая)
 * @param[in] min   Минимальное значение интервала
 * @param[in] max   Максимальное значение интервала
 * @returns         Введённое число
*/
int input_int(int min, int max) {
    int result = 0;
	int scanfRes = 0;
    while (1) {
		rewind(stdin);
		scanfRes = scanf("%d", &result);
		if (!scanfRes)
			fprintf(stderr, "Неправильный формат ввода данных. Введите целое число в интервале от %d до %d\n", min, max);
		else if (result < min || result > max)
			fprintf(stderr, "Число выходит из границ допустимого. Введите целое число в интервале от %d до %d\n", min, max);
		else break;
	}
	return result;
}