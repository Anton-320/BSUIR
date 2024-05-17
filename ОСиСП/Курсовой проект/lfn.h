#pragma once
#include <stdint.h>
#include "general_fun.h"
#include "structs.h"

typedef struct _LongFileNameEntry{
    uint8_t  id;			// Порядковый номер в последовательности записей длинных имѐн
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

char* put_name_from_stack(LfnStack** top, int size);