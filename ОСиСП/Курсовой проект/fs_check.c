#include "fs_check.h"

#define MAX_FAT_SIZE_BYTES 1073741824

//extern int fd;                  // Файловый дескриптор проверяемого раздела
static BootSector bs;           // Копия загрузочного сектора (только первые 90 байт)
static uint32_t* fat;           // FAT-таблица


/**
 * Проверка всей файловой системы
*/
int check_all() {
    int bsErrors = 0;
    int fatErrors = 0;

    // Проверка загрузочного сектора
    bs = init_boot_sector(0);    
    bsErrors = check_boot_sector(true);
    if (bsErrors > 0) {
        if (!autoFixOpt) {
            printf("При проверке загрузочного сектора найдено %d ошибок.\n", bsErrors);
            printf("1 - Попытаться восстановить загрузочный сектор из резервной копии\n"
                "2 - Завершить работу утилиты\n");
        }
        if (autoFixOpt || input_int(1, 2) == 1) {               // Если пользователь выбирает попытку восстановления,
            off_t copyOffset = try_find_boot_sector_copy();
            if (copyOffset == -1) {   // Если не получилось найти копию загрузочного сектора
                printf("Не удалось обнаружить копию загрузочного сектора. Дальнейшая проверка файловой системы невозможна\n");
                return bsErrors;
            }
            else {                    // Если удалось найти резервную копию загрузочного сектора
                // Перезапись всего загрузочного сектора
                uint8_t* buffer = (uint8_t*)alloc(bs.bytesPerSector, sizeof(uint8_t));
                fs_read(copyOffset, bs.bytesPerSector, buffer);
                fs_write(0, bs.bytesPerSector, buffer);
                free(buffer);
                printf("Загрузочный сектор восстановлен\n");
            }
        }
        else {
            printf("Завершение работы утилиты\n");
            return bsErrors;
        }
    }

    if (viewInfoOpt)
        print_filesystem_info();

    // FAT-таблица (чтение)
    if (bs.extFlags & 0x0080)                               // Если 7-й бит == 1 (активна 1 FAT)
        fat = read_fat_table((bs.extFlags & 0x000F), &bs); 
    else
        fat = read_fat_table(0, &bs);                       // Если включено зеркалирование

    //Проверка FAT-таблицы
    fatErrors = check_fat_table(true);
    if (fatErrors > 0) {
        if (!autoFixOpt) {
            printf("Найдено %d ошибок при проверке FAT-таблицы.\n", fatErrors);
            printf("1 - Попытаться восстановить загрузочный сектор из резервной копии\n"
                "2 - Завершить работу утилиты\n");
        }
        if (autoFixOpt || input_int(1, 2) == 1) {                 // Если пользователь выбирает попытку восстановления
            if (!try_find_fat_copy()) {             // Если не получилось найти копию FAT-таблицы
                printf("Не удалось найти резервную копию основной FAT-таблицы. Дальнейшая проверка файловой системы невозможна\n");
                return fatErrors;
            }
            else {                                  // Если получилось найти копию FAT-таблицы
                // Перезапись FAT-таблицы
                write_fat_table(0, fat, bs.fatSz_32 * bs.bytesPerSector / FAT_RECORD_SIZE, &bs);
                bs.extFlags = 0x0080;               // Записать в флаги 0b 0000 0000 1000 0000 (выключить зеркалирование и активная таблица - 0)
                uint16_t temp = htole16(bs.extFlags);
                fs_write(40, sizeof(temp), &temp);  // 40 - смещение поля extFlags в boot sector
                fs_write(bs.bkBootSec * bs.bytesPerSector + 40, sizeof(temp), &temp);

                printf("FAT-таблица восстановлена\n");
            }
        }
        else {
            printf("Завершение работы утилиты\n");
            return fatErrors;
        }
    }

    free(fat);                      // Освободить память
    
    return bsErrors + fatErrors;
}

/**
 * Проверка загрузочного сектора (boot sector)
 * @param[in] print     Печатать сообщения об ошибках (1 - да, 0 - нет)
 * @returns             Общее количество предупреждений
*/
int check_boot_sector(bool print)
{
    int errors = 0;   // Общее число ошибок
    int warnings = 0; // Общее число предупреждений

    if (bs.jmpBoot[0] != 0xEB && bs.jmpBoot[0] != 0xE9) {
        if (print)
            fprintf(stdout, "Неисправность в Boot Sector по смещению 0: %hhu недопустимое значение для BS_jmpBoot "
                "(jump инструкция на начало загрузочного кода операционной системы)\n"
                "Допустимыми значениями являются 0xEB и 0xE9 (0xEB используется чаще)\n",
                bs.jmpBoot[0]);
        errors += 1;
    };
    
    if (bs.bytesPerSector != 512 && bs.bytesPerSector != 1024 && bs.bytesPerSector != 2048 && bs.bytesPerSector != 4096) {
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 11: %hu недопустимое количество байт в секторе для FAT32.\n"
                "Допустимыми значениями считаются 512, 1024, 2048, 4096\n", bs.bytesPerSector);
        errors += 1;
    }

    if (bs.sectorsPerCluster != 2 && bs.sectorsPerCluster != 4
        && bs.sectorsPerCluster != 8 && bs.sectorsPerCluster != 16
        && bs.sectorsPerCluster != 32 && bs.sectorsPerCluster != 64
        && bs.sectorsPerCluster != 128) {
        
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 13: %hhu недопустимое количество секторов в кластере.\n"
                "Допустимыми значениями считаются 2, 4, 8, 16, 32, 64, 128\n", bs.sectorsPerCluster);
        errors += 1;
    }
    else if (bs.sectorsPerCluster * bs.bytesPerSector > (32 * 1024)) {
        if (print)
            fprintf(stdout, "Предупреждение в загрузочном секторе (BIOS Parameter Block) по смещению 13: размер кластера %u, что больше, чем 32К.\n"
                "Это может привести к неправильной работе многих (в том числе установочных) программ\n",
                (unsigned int)(bs.sectorsPerCluster * bs.bytesPerSector));
        warnings += 1;
    }
    
    if (bs.resvdSectCount <= 0) {
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 14: %hu недопустимое количество секторов в Reserved Region\n"
                "Количество секторов в Reserved Region должно быть больше 0\n", bs.resvdSectCount);
        errors += 1;
    }

    if (bs.numFats == 0) {
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 16: %hhu недопустимое количество таблиц FAT.\n"
                "Количество таблиц FAT должно быть 2 для любой FAT. Хотя и допустимы и другие значения, "
                "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
                "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
                "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
                " но некоторые драйверы FAT могут работать неправильно.", bs.numFats);
        errors += 1;
    }
    else if (bs.numFats > 2) {
        if (print)
            fprintf(stdout, "Предупреждение в загрузочном секторе (BIOS Parameter Block) по смещению 16: количество %hhu таблиц FAT нежелательно.\n"
                "Количество таблиц FAT должно быть 2 для любой FAT. Хотя допустимы и другие значения, "
                "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
                "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
                "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
                " но некоторые драйверы FAT могут работать неправильно.", bs.numFats);
        warnings += 1;
    }

    if (!(bs.mediaType > 0xF8 && bs.mediaType < 0xFF) && bs.mediaType != 0xF0) {
        if (print)
            fprintf(stdout, "Предупреждение в загрузочном секторе (BIOS Parameter Block) по смещению 21: %hhu недопустимое значение типа диска\n"
                "Разрешѐнные значения: 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE и 0xFF.\n", bs.mediaType);
        warnings += 1;
    }

    if (bs.totalSectors_32 == 0) {
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 32: %u недопустимое значение количества секторов на диске\n"
                "Это поле включает общее количество секторов на диске. Количество секторов на диске должно быть не равно 0\n", bs.totalSectors_32);
        errors += 1;
    }
    
    if (bs.fatSz_32 > MAX_FAT_SIZE_BYTES / bs.bytesPerSector) {
        if (print)
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 36: %u слишком большой размер FAT-таблицы в секторах\n"
                "Максимально допустимый размер FAT-таблицы в байтах - %d, размер сектора в данной ФС - %hu. "
                "Следовательно максимально допустимый размер FAT в секторах - %u\n",
                bs.fatSz_32, MAX_FAT_SIZE_BYTES, bs.bytesPerSector, (uint32_t)(MAX_FAT_SIZE_BYTES / bs.bytesPerSector));
        errors += 1;
    }

    if (bs.checkSignature != 0xAA55) {
        if (print) {
            fprintf(stdout, "Неисправность в загрузочном секторе (BIOS Parameter Block) по смещению 510: "
                "По данному смещению должна располагаться сигнатура 0xAA55\n");
            if (autoFixOpt) {
                printf("1 - Добавить сигнатуру 0xAA55 по смещению 510\n");
                printf("2 - Оставить как есть\n");
            }
            if (autoFixOpt || input_int(1, 2) == 1) {
                bs.checkSignature = 0xAA55;
            }
            else errors += 1;
        }
        else
            errors += 1;
    }

    return errors;
}

/**
 * Вывод информации о файловой системе
*/
void print_filesystem_info() {
    printf("Системный ID: %s\n", bs.OEMName);
    printf("Количество байт в секторе: %hu\n", bs.bytesPerSector);
    printf("Количество секторов в кластере: %hhu\n", bs.sectorsPerCluster);
    printf("Количество секторов в зарезервированном регионе: %hu\n", bs.resvdSectCount);
    printf("Количество таблиц FAT: %hhu\n", bs.numFats);
        
    printf("Тип носителя: ");
    if (bs.mediaType == 0xF0)
        printf("5.25\" or 3.5\" HD дискета\n");
    else if (bs.mediaType == 0xF8)
        printf("Жёсткий диск\n");
    else if (bs.mediaType == 0xF9)
        printf("3,5\" 720k дискета 2s/80tr/9sec or 5.25\" 1.2M floppy 2s/80tr/15sec\n");
    else if (bs.mediaType == 0xFA)
        printf("5.25\" 320k дискета 1s/80tr/8sec\n");
    else if (bs.mediaType == 0xFB)
        printf("3.5\" 640k дискета 2s/80tr/8sec\n");
    else if (bs.mediaType == 0xFC)
        printf("5.25\" 180k дискета 1s/40tr/9sec\n");
    else if (bs.mediaType == 0xFD)
        printf("5.25\" 360k дискета 2s/40tr/9sec\n");
    else if (bs.mediaType == 0xFE)
        printf("5.25\" 160k дискета 1s/40tr/8sec\n");
    else if (bs.mediaType == 0xFF)
        printf("5.25\" 320k дискета 2s/40tr/8sec\n");
    else
        printf("не удалось определить\n");

    printf("Количество секторов на дорожке: %hu\n", bs.sectorsPerTrack);
    printf("Количество головок: %hu\n", bs.numHeads);
    printf("Количество скрытых секторов, перед началом данного раздела диска: %u\n", bs.hiddenSectors);
    printf("Общее количество используемых секторов на диске: %hu\n", bs.totalSectors_32);
    printf("Количество секторов, выделенных под одну FAT-таблицу: %hu\n", bs.fatSz_32);
    printf("Номер первого кластера корневой директории: %hu\n", bs.rootClusterNum);
    if (bs.bkBootSec != 0)
        printf("Номер сектора в резервной области диска, где хранится копия boot сектора: %hu\n", bs.bkBootSec);
    printf("Имя диска: %s\n", bs.volumeLabel);
    printf("Серийный номер диска: %u\n", bs.volumeID);
}

/**
 * Проверка FAT таблицы
 * @param[in] print Выводить ли сообщения об ошибках (true - да, false - нет)
 * @returns         Общее количество ошибок (циклов, разрывов и т.д.)
*/
int check_fat_table(bool print) {
    
    uint16_t gaps = 0;       // Количество разрывов
    uint16_t cycles = 0;     // Количество циклов
    int warns = 0;           // Количество предупреждений

    uint32_t entCount = bs.fatSz_32 * bs.bytesPerSector / FAT_RECORD_SIZE;    // Количество записей в FAT-таблице (вместимость)
    uint8_t* visitedClusters = (uint8_t*)alloc(entCount, sizeof(uint8_t));    // Элемент массива == 1, если кластер посещён, == 0, если нет
    
    for (uint32_t i = 2; i < entCount; i += 1) {
        
        if (visitedClusters[i] || (fat[i] & 0x0FFFFFFF) == 0x0FFFFFF7 || (fat[i] & 0x0FFFFFFF) == 0)         // Если уже посещён или BAD CLUSTER или пустой
            continue;
 
        for (uint32_t j = i; (fat[j] & 0x0FFFFFFF) < 0x0FFFFFF8; j = (fat[j] & 0x0FFFFFFF)) {
            visitedClusters[j] = 1;                 // Пометить кластер как уже пройденный

            if ((fat[j] & 0x0FFFFFFF) == 0 || (fat[j] & 0x0FFFFFFF) > entCount) { // Если следующий кластер пустой или за таблицей FAT, то разрыв
                if (print) {
                    printf("Неисправность в FAT: Обнаружен разрыв в цепочке кластеров по смещению %u\n", j);
                    if ((fat[j] & 0x0FFFFFFF) == 0)
                        printf("Кластер %u является пустым кластером (равен 0), что недопустимо в цепочке кластеров\n", j);
                    else
                        printf("Кластер %u указывает на значение %u, которое находится за пределами FAT\n", j, fat[j] & 0x0FFFFFFF);
                    if (!autoFixOpt) {
                        printf("1 - Пометить кластер %u значением EOC\n", j);
                        printf("2 - Оставить как есть\n");
                    }
                    if (autoFixOpt || input_int(1, 2) == 1) {
                        fat[j] = 0x0FFFFFFF;
                        fs_write(get_fat_offset(0, &bs) + FAT_RECORD_SIZE * j, FAT_RECORD_SIZE, (fat + j));
                    }
                }
                gaps += 1;
                break;
            }
            else if (visitedClusters[fat[j] & 0x0FFFFFFF]) {     // Если следующее значение уже проходили, то цикл в цепочек кластеров
                if (print) {
                    printf("Неисправность в FAT: Обнаружен цикл в цепочке кластеров\n"
                        "Кластер %u указывает на кластер %u, который находится перед кластером %u или в другой цепочке кластеров\n", j, fat[j], j);
                    if (!autoFixOpt) {
                        printf("1 - Пометить кластер %u значением EOC\n", j);
                        printf("2 - Оставить как есть\n");
                    }
                    if (autoFixOpt || input_int(1, 2) == 1) {
                        fat[j] = 0x0FFFFFFF;
                        fs_write(get_fat_offset(0, &bs) + FAT_RECORD_SIZE * j, FAT_RECORD_SIZE, (fat + j));
                    }
                }
                cycles += 1;
                break;
            }
        }
    }
    free(visitedClusters);  // Освободить память
    return gaps + cycles;
}



static off_t dataRegOff;     // Смещение региона данных

/**
 * Размер директории в кластерах
*/
static int get_dir_size(off_t offset) {
    int cluster = (offset - dataRegOff) / (bs.sectorsPerCluster * bs.bytesPerSector);   // Номер кластера в Data Region (начиная с 0)
    int counter = 0;
    for (; (cluster & 0x0FFFFFFF) < 0x0FFFFFF8; counter += 1)
        cluster = fat[cluster + 2];
    return counter;
}

static void delete_entry_from_dir(off_t lfnOffset, off_t sfnOffset) {
    uint8_t tmp = 0xE5;
    while (lfnOffset < sfnOffset) {
        fs_write(lfnOffset, sizeof(uint8_t), &tmp);
        lfnOffset += sizeof(LfnEnt);
    }
    fs_write(sfnOffset, sizeof(uint8_t), &tmp);
}

/**
 * Получить номер первого кластера файла в регионе данных
*/
static uint32_t get_first_cluster_num(const DirEntry* sfn) {
    return ((sfn->firstClusterHigh * 65536) + sfn->firstClusterLow);
}

/**
 * Начало обработки - первая дочерняя (от корневого каталога) запись
*/
static void read_and_check_dir_tree(off_t offset, const char* path) {
    LfnEnt tmpLfn = {};     // Временная переменная для длинной записи
    DirEntry shortEntry = {};   // Временная переменная для короткой записи 
    char* fileName = NULL;  // Имя файла
    off_t lfnOffset = 0;    // Для хранения смещения lfn
    off_t dirSize = get_dir_size(offset) * bs.sectorsPerCluster * bs.bytesPerSector;
    off_t finalOffset = offset + dirSize;                             // Смещение конца директории

    for (; offset < finalOffset; offset += sizeof(DirEntry)) {        //  Цикл по записям (коротким) директории
        fs_read(offset, sizeof(LfnEnt), &tmpLfn);

        if (tmpLfn.id == 0xE5 || tmpLfn.id == 0 || tmpLfn.id == 0x05) {
            offset += sizeof(DirEntry);
            continue;
        }

        lfnOffset = offset;
        if (tmpLfn.attr & ATTR_LFN && tmpLfn.reserved == 0) {   // Если LFN ("длинная запись")
            offset += sizeof(LfnEnt);                           // Сместить указатель на одну запись
            LfnStack* top = (LfnStack*)alloc(1, sizeof(LfnStack));  // Вершина стека с длинными записями
            top->entry = tmpLfn;    // Первая длинная запись
            int longEntryCount = 1; // Для подсчёта длинных записей
            
            // Цикл по считыванию с устройства в стек длинных записей одной короткой записи
            while (1) {
                LfnStack* stackNode = (LfnStack*)alloc(1, sizeof(LfnStack));
                fs_read(offset, sizeof(LfnEnt), &(stackNode->entry));
                
                if (stackNode->entry.id == 0xE5 || stackNode->entry.id == 0)
                    pdie("Разрыв в списке длинных записей");
                
                if (stackNode->entry.attr & ATTR_LFN && stackNode->entry.reserved == 0) {
                    stackNode->next = top;  // Если новая запись - длинная запись, сохранить в стеке
                    top = stackNode;
                    longEntryCount += 1;    // Подсчёт длинных записей
                }
                else {                      // Если новая запись - короткая запись
                    memcpy(&(shortEntry), &(stackNode->entry), sizeof(DirEntry));  // Скопировать в поле короткой записи из дерева каталогов
                    free(stackNode);        // Освободить память
                    stackNode = NULL;
                    break;                  // Выйти из цикла
                }
                offset += sizeof(LfnEnt);   // Шаг считывания записей
            }
            fileName = put_name_from_stack(&top, longEntryCount);   // Считывание из стека элементов длинного имени файла
        }
        else {  // Если короткая запись (нет перед ней длинных)
            memcpy(&shortEntry, &tmpLfn, sizeof(DirEntry));
            fileName = (char*)alloc(12, sizeof(char));
            memcpy(fileName, shortEntry.name, sizeof(shortEntry.name));
        }
        
        char* fullPath = (char*)alloc(strlen(path) + strlen(fileName) + 2, sizeof(char));
        strcpy(fullPath, path);
        strcat(fullPath, "/");
        strcat(fullPath, fileName);

        uint32_t fileFirstClusterNum = get_first_cluster_num(&shortEntry);      // Номер 1-ого кластера файла (относительно Data Region)
        uint32_t fileFirstClusterVal = fat[fileFirstClusterNum] & 0x0FFFFFFF;   // Считать значение
        if (fileFirstClusterVal == 0x0FFFFFF7) {
            fprintf(stderr, "Неисправность в Data Region: файл %s лежит в BAD CLUSTER\n", fileName);
            if (!autoFixOpt) {
                printf("1 - Удалить эту запись из директории\n");
                printf("2 - Оставить как есть\n");
            }
            if (autoFixOpt || input_int(1,2) == 1) {
                delete_entry_from_dir(lfnOffset, offset);
            }
            
        }
        else if (fileFirstClusterVal == 0) {
            fprintf(stderr, "Неисправность в Data Region: файл %s лежит в кластере, помеченном как свободный\n", fileName);
            printf("1 - Пометить кластер значением EOC\n");
            printf("2 - Удалить эту запись из директории\n");
            printf("3 - Оставить как есть\n");
            int option = input_int(1,2);
            switch(option) {
                case 1: {
                    fat[fileFirstClusterNum + 2] = 0x0FFFFFFF;
                    fs_write(lfnOffset, get_fat_offset(0, &bs) + fileFirstClusterNum * 4 + 2, (fat + fileFirstClusterNum + 2));
                    break;
                }
                case 2: {
                    delete_entry_from_dir(lfnOffset, offset);
                    break;
                }
            }
        }

        if (shortEntry.attributes & ATTR_DIR) {
            
            read_and_check_dir_tree(fileFirstClusterNum, fullPath);
            continue;
        }
        free(fileName);
        free(fullPath);
    }
}

/**
 * 
*/
void check_data_region() {
    dataRegOff = bs.resvdSectCount * bs.bytesPerSector + bs.fatSz_32 * bs.bytesPerSector * bs.numFats;
    uint32_t rootLba = bs.resvdSectCount + (bs.fatSz_32 * 2) + ((bs.rootClusterNum - 2) * bs.sectorsPerCluster);    // LBA корневой директории
    
}

/**
 * Попытаться найти резервную копию загрузочного сектора.
 * Результат записывается в статическую переменную bs
 * @return В случае успеха возвращает смещение нормальной копии относительно начала раздела, иначе -1
*/
off_t try_find_boot_sector_copy() {

    // Если поле количество байтов в секторе не испорчено и соответствует спецификации
    if (bs.bytesPerSector == 512 || bs.bytesPerSector == 1024 || bs.bytesPerSector == 2048 || bs.bytesPerSector == 4096) {
        if (bs.bkBootSec > 0) {                    // Номер сектора, где хранится копия boot сектора не испорчен
            bs = init_boot_sector(bs.bkBootSec * bs.bytesPerSector);
            if (!check_boot_sector(false))         // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * bs.bytesPerSector;

            // 6 - номер сектора в резервной области диска, где хранится копия boot сектора согласно спецификации (и на практике)
            bs = init_boot_sector(6 * bs.bytesPerSector);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * bs.bytesPerSector;
            
            // 512 - количество байт в секторе (которое чаще всего используется)
            bs = init_boot_sector(bs.bkBootSec * 512);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * 512;

            bs = init_boot_sector(6 * 512);
            if (!check_boot_sector(false))
                return 6 * 512;
        }
        else {
            bs = init_boot_sector(6 * bs.bytesPerSector);
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * bs.bytesPerSector;
        
            bs = init_boot_sector(6 * 512);
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * 512;
        }
    }
    else {
        if (bs.bkBootSec > 0) {
            // 512 - количество байт в секторе (которое чаще всего используется)
            bs = init_boot_sector(bs.bkBootSec * 512);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * 512;

            bs = init_boot_sector(6 * 512);
            if (!check_boot_sector(false))
                return 6 * 512;
        }
        else {
            bs = init_boot_sector(6 * 512);
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * 512;        
        }
    }

    return -1;
}

/**
 * Ищет резервную копию FAT-таблицы, проверяет и записывает в буфер fat (статическая переменная)
 * @returns true (1), если удалось найти корректную резервную копию FAT-таблицы, иначе false(0)
*/
bool try_find_fat_copy() {
    if (bs.numFats < 2) {
        return false;
    }

    int checkedFatNum = 0;          // Номер проверенной FAT
    if (bs.extFlags & 0x0080)       // Если выключено зеркалирование (только 1 активная FAT)
        checkedFatNum = (bs.extFlags & 0x000F);
    else                            // Если включено зеркалирование
        checkedFatNum = 0;

    // Цикл поиска корректной резервной копии FAT-таблицы
    for (int i = 0; i < 8; i += 1) {
        if (i == checkedFatNum)
            continue;

        fs_read(get_fat_offset(i, &bs), bs.fatSz_32, fat);    // Считываем (предполагаемую) таблицу FAT в буфер fat (размер fat не меняем, т.к. boot sector правильный), не read_fat_table(), чтобы без лишнего выделения паямяти
        if (!check_fat_table(false)) // Если нет ошибок
            return true;
    }

    return false;
}
