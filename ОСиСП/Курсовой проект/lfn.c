#include "lfn.h"

/**
 * Функция для перевода строки из Unicode в UTF-8
 * @param[out] dest    Символьная строка для результата (в UTF-8)
 * @param[in]  src     Массив 2-байтных элементов - кодовых точек символов (Unicode)
 * @param[in]  size    Количество элементов массива src
 * @returns            На сколько байтов сместился
*/
static int encode_utf8(char* dest, uint16_t* src, int amount) {
    char* initial = 0;
    for (int i = 0; i < amount; i += 1) {
        dest[i] = 0;
        if (src[i] < 0x80) {
            // Один байт
            dest[0] = src[i];
            dest += 1;
        } else if (src[i] < 0x800) {
            // Два байта
            dest[0] = (char) (0xC0 | (src[i] >> 6));
            dest[1] = (char) (0x80 | (src[i] & 0x3F));
            dest += 2;
        } else if (src[i] != 0xFFFF){
            // Три байта
            dest[0] = (char) (0xE0 | (src[i] >> 12));
            dest[1] = (char) (0x80 | ((src[i] >> 6) & 0x3F));
            dest[2] = (char) (0x80 | (src[i] & 0x3F));
            dest += 3;
        } else {
            return dest - initial;
        }
    }
    return dest - initial;
}

/**
 * Скопировать содержимое длинной записи в строку UTF-8
 * @returns Количество байтов, записанных в строку (символ может содержать от 1 до 3 байтов)
*/
static int copy_lfn_part(char* dst, LfnEntry* lfn) {
    char* initial = dst;
    dst += encode_utf8(dst, (uint16_t*)lfn->name0_4, 5);
    dst += encode_utf8(dst, (uint16_t*)lfn->name5_10, 6);
    dst += encode_utf8(dst, (uint16_t*)lfn->name11_12, 2);
    return dst - initial;
}

/**
 * Считать строку из стека в строку (UTF-8), память под строку выделяется в функции
 * Стек должен быть проверенным (количество элементов == максимальный id - 1; все id записей уникальны;), иначе ошибка
 * @param[in] top   Указатель на вершину стека
 * @param[in] size  Размер стека
 * @returns         Строку с длинным именем файла
*/
char* put_name_from_stack(LfnStack** top, int size) {
    char* res = (char*)alloc(size * 3 * 13, sizeof(char));  // Длинное имя файла
    char* tmp = res;    // Для пробега по строке res для записи в res "длинных записей"
    for (int i = 0; top != NULL; i += 1) {
        LfnStack* stackNode = *top;

        // Поиск длиннной записи и запись её в строку
        if ((stackNode->entry.id & 0x3F) == i) {     // Если вершина подходит по индексу
            tmp += copy_lfn_part(tmp, &stackNode->entry);
            // Удаление из стека
            *top = (*top)->next;
            free(stackNode);
        }
        else {
            while (stackNode->next != NULL && (stackNode->next->entry.id & 0x3F) != i)   // Поиск длинной записи по индексу
                stackNode = stackNode->next;
            if (stackNode->next == NULL) {       // Если следующая запись не найдена, то ошибка
                pdie("Ошибка. Длинные записи неупорядочены (по индексам)\n");
            }
            tmp += copy_lfn_part(tmp, &stackNode->next->entry);
            // Удаление из стека
            LfnStack* tmp = stackNode->next;
            stackNode->next = stackNode->next->next;
            free(tmp);
        }
    }
    res = (char*)realloc(res, strlen(res) + 1);     // Обрезать память
    return res;
}