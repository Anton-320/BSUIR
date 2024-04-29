#include "fs_check.h"

extern int fd;

/**
 * Проверка загрузочного сектора (boot sector)
 * Возвращает
 * 0, если нет проблем
 * -1, если есть проблемы
*/
int check_boot_sector(const BootSector* bootSector)
{
    int errAndWrnCnt = 0;   //общее число ошибок и предупреждений

    if (bootSector->bytesPerSector != 512 && bootSector->bytesPerSector != 1024
     && bootSector->bytesPerSector != 2048 && bootSector->bytesPerSector != 4096) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 11: %hu недопустимое количество байт в секторе для FAT32.\n"
         "Допустимыми значениями считаются 512, 1024, 2048, 4096\n", bootSector->bytesPerSector);
        errAndWrnCnt += 1;
    }

    if (bootSector->sectorsPerCluster != 2 && bootSector->sectorsPerCluster != 4
     && bootSector->sectorsPerCluster != 8 && bootSector->sectorsPerCluster != 16
     && bootSector->sectorsPerCluster != 32 && bootSector->sectorsPerCluster != 64
     && bootSector->sectorsPerCluster != 128) {
        
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 13: %hhu недопустимое количество секторов в кластере.\n"
         "Допустимыми значениями считаются 2, 4, 8, 16, 32, 64, 128\n", bootSector->sectorsPerCluster);
        errAndWrnCnt += 1;
    }
    else if (bootSector->sectorsPerCluster * bootSector->bytesPerSector > (32 * 1024)) {
        fprintf(stderr, "Предупреждение: BIOS Parameter Block по смещению 13: размер кластера %u, что больше, чем 32К.\n"
         "Это может привести к неправильной работе многих (в том числе установочных) программ\n",
         (unsigned int)(bootSector->sectorsPerCluster * bootSector->bytesPerSector));
        errAndWrnCnt += 1;
    }
    
    
    if (bootSector->rsvdSectCount <= 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 14: %hu недопустимое количество секторов в Reserved Region равно \n"
         "Количество секторов в Reserved Region должно быть больше 0\n", bootSector->rsvdSectCount);
        errAndWrnCnt += 1;
    }

    if (bootSector->fatCount == 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 16: %hhu недопустимое количество таблиц FAT.\n"
         "Количество таблиц FAT должно быть 2 для любой FAT. Хотя и допустимы и другие значения, "
         "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
         "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
         "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
         " но некоторые драйверы FAT могут работать неправильно.", bootSector->fatCount);
        errAndWrnCnt += 1;
    }
    else if (bootSector->fatCount != 2) {
        fprintf(stdout, "Предупреждение: BIOS Parameter Block по смещению 16: наличие %hhu таблиц FAT нежелательно.\n"
         "Количество таблиц FAT должно быть 2 для любой FAT. Хотя и допустимы и другие значения, "
         "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
         "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
         "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
         " но некоторые драйверы FAT могут работать неправильно.", bootSector->fatCount);
        errAndWrnCnt += 1;
    }

    if (!(bootSector->mediaType > 0xF8 && bootSector->mediaType < 0xFF) && bootSector->mediaType != 0xF0) {
        fprintf(stderr, "Предупреждение: BIOS Parameter Block по смещению 21: %hhu недопустимое значение типа диска\n"
         "Разрешѐнные значения: 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE и 0xFF.\n", bootSector->mediaType);
        errAndWrnCnt += 1;
    }

    if (bootSector->totalSectors_32 == 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 32: %u недопустимое значение количества секторов на диске\n"
         "Это поле включает общее количество секторов на диске. Количество секторов на диске должно быть не равно 0\n", bootSector->totalSectors_32);
        errAndWrnCnt += 1;
    }
    
    return errAndWrnCnt;
}
