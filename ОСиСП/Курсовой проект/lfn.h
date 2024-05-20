#pragma once
#include <stdint.h>
#include "general_fun.h"
#include "structs.h"
#include <endian.h>
#include "io.h"

typedef struct _LongFileNameEntry{
    uint8_t  id;			// Порядковый номер записи длинного имени (Начинается с 1!!!)
    uint8_t  name0_4[10];	// Символы 1-5 длинного имени данного компонента
    uint8_t  attr;		    // Атрибуты – содержит ATTR_LFN
    uint8_t  reserved;		// Если 0, то запись является компонентом длинного имени
    uint8_t  aliasChecksum;	// Контрольная сумма короткого имени (которое располагается в конце списка)
    uint8_t  name5_10[12];	// Символы 6-11 длинного имени в данном компоненте
    uint16_t fstClusLowOff; // Должно быть 0
    uint8_t  name11_12[4];	// Символы 12-13 длинного имени в данном компоненте
} __attribute__ ((packed)) LfnEntry;

typedef struct _LfnStackNode{
    LfnEntry entry;
    struct _LfnStackNode* next;
} __attribute__ ((packed)) LfnStack;

/**
 * Очистить стек
*/
void clear_stack(LfnStack *top);

/**
 * Считать строку из стека в строку (UTF-8), память под строку выделяется в функции
 * Стек должен быть проверенным (количество элементов == максимальный id - 1; все id записей уникальны;), иначе ошибка
 * @param[in] top   Указатель на вершину стека
 * @param[in] size  Размер стека
 * @returns         Строку с длинным именем файла
*/
char *put_name_from_stack(LfnStack **top, int size);