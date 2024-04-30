#include "fs_check.h"

extern int fd;

static BootSector bs;

/**
 * Проверка загрузочного сектора (boot sector)
 * @returns общее количество неполадок и предупреждений
*/
int check_boot_sector()
{
    int errAndWrnCnt = 0;   //общее число ошибок и предупреждений

    if (bs.jmpBoot[0] != 0xEB && bs.jmpBoot[0] != 0xE9) {
        fprintf(stderr, "Неисправность: Boot Sector по смещению 0: %hhu недопустимое значение для BS_jmpBoot "
            "(jump инструкция на начало загрузочного кода операционной системы)\n"
            "Допустимыми значениями являются 0xEB и 0xE9 (0xEB используется чаще)\n",
            bs.jmpBoot[0]);
        errAndWrnCnt += 1;
    };
    
    if (bs.bytesPerSector != 512 && bs.bytesPerSector != 1024
     && bs.bytesPerSector != 2048 && bs.bytesPerSector != 4096) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 11: %hu недопустимое количество байт в секторе для FAT32.\n"
            "Допустимыми значениями считаются 512, 1024, 2048, 4096\n", bs.bytesPerSector);
        errAndWrnCnt += 1;
    }

    if (bs.sectorsPerCluster != 2 && bs.sectorsPerCluster != 4
        && bs.sectorsPerCluster != 8 && bs.sectorsPerCluster != 16
        && bs.sectorsPerCluster != 32 && bs.sectorsPerCluster != 64
        && bs.sectorsPerCluster != 128) {
        
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 13: %hhu недопустимое количество секторов в кластере.\n"
            "Допустимыми значениями считаются 2, 4, 8, 16, 32, 64, 128\n", bs.sectorsPerCluster);
        errAndWrnCnt += 1;
    }
    else if (bs.sectorsPerCluster * bs.bytesPerSector > (32 * 1024)) {
        fprintf(stderr, "Предупреждение: BIOS Parameter Block по смещению 13: размер кластера %u, что больше, чем 32К.\n"
            "Это может привести к неправильной работе многих (в том числе установочных) программ\n",
            (unsigned int)(bs.sectorsPerCluster * bs.bytesPerSector));
        errAndWrnCnt += 1;
    }
    
    
    if (bs.resvdSectCount <= 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 14: %hu недопустимое количество секторов в Reserved Region равно \n"
            "Количество секторов в Reserved Region должно быть больше 0\n", bs.resvdSectCount);
        errAndWrnCnt += 1;
    }

    if (bs.numFats == 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 16: %hhu недопустимое количество таблиц FAT.\n"
            "Количество таблиц FAT должно быть 2 для любой FAT. Хотя и допустимы и другие значения, "
            "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
            "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
            "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
            " но некоторые драйверы FAT могут работать неправильно.", bs.numFats);
        errAndWrnCnt += 1;
    }
    else if (bs.numFats != 2) {
        fprintf(stdout, "Предупреждение: BIOS Parameter Block по смещению 16: наличие %hhu таблиц FAT нежелательно.\n"
            "Количество таблиц FAT должно быть 2 для любой FAT. Хотя и допустимы и другие значения, "
            "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
            "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
            "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
            " но некоторые драйверы FAT могут работать неправильно.", bs.numFats);
        errAndWrnCnt += 1;
    }

    if (!(bs.mediaType > 0xF8 && bs.mediaType < 0xFF) && bs.mediaType != 0xF0) {
        fprintf(stderr, "Предупреждение: BIOS Parameter Block по смещению 21: %hhu недопустимое значение типа диска\n"
            "Разрешѐнные значения: 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE и 0xFF.\n", bs.mediaType);
        errAndWrnCnt += 1;
    }

    if (bs.totalSectors_32 == 0) {
        fprintf(stderr, "Неисправность: BIOS Parameter Block по смещению 32: %u недопустимое значение количества секторов на диске\n"
            "Это поле включает общее количество секторов на диске. Количество секторов на диске должно быть не равно 0\n", bs.totalSectors_32);
        errAndWrnCnt += 1;
    }
    
    return errAndWrnCnt;
}

void print_filesystem_info() {
    printf("Имя диска: %s\n", bs.volumeLabel);
    printf("Количество байт в секторе: %hu\n", bs.bytesPerSector);
    printf("Количество секторов в кластере: %hhu\n", bs.sectorsPerCluster);
    printf("Количество секторов в зарезервированном регионе: %hu\n", bs.resvdSectCount);
    printf("Количество таблиц FAT: %hhu\n", bs.numFats);
    printf("Количество секторов на дорожке: %hu\n", bs.sectorsPerTrack);
    printf("Количество головок: %hu\n", bs.numHeads);
    printf("Количество используемых секторов на диске: %hu\n", bs.totalSectors_32);
    printf("Количество секторов одной FAT: %hu\n", bs.fatSz_32);
    printf("Номер первого кластера корневой директории: %hu\n", bs.rootClusterNum);
    if (bs.bkBootSec != 0)
        printf("Номер сектора в резервной области диска, где хранится копия boot сектора: %hu\n", bs.bkBootSec);    
}

PAIR check_fat_table() {
    
    PAIR result = {};
    int errors = 0;     //количество ошибок
    int warns = 0;      //количество предупреждений

    uint32_t fatOffset = get_fatOffset(&bs);
    //TODO

    return result;
}

// Функция проверки наличия разрывов в цепочках кластеров
void check_for_gaps(int fd, const BootSector* bs) {
    uint32_t totalClusters = bs->totalSectors_32 / bs->sectorsPerCluster + 1;  // Общее количество кластеров
    FAT_Entry entry = {};
    for (uint32_t i = 2; i < totalClusters; i++) {
        entry = read_fat_entry(fd, i, bs);             // Считать записи 
        if (entry.nextCluster >= 0x0FFFFFFF) {         // Если кластер является последним в цепочке
            continue;
        }

        uint32_t nextCluster = entry.nextCluster;
        if (entry.nextCluster >= totalClusters) {
            printf("Обнаружен разрыв в цепочке кластеров: кластер %d points to invalid cluster %d\n", i, nextCluster);
        } else {
            FAT_Entry next_entry = read_fat_entry(fd, i, bs);   //TODO read
            if (next_entry.cluster != i) {
                printf("Обнаружен разрыв в цепочке кластеров: cluster %d points to cluster %d, but cluster %d does not point back\n", i, nextCluster, nextCluster);
            }
        }
    }
}
