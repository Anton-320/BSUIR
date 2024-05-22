#include "fs_check.h"

#define MAX_FAT_SIZE_BYTES 1073741824
#define FAT_RECORD_SIZE 4       //Размер записи таблицы FAT

uint8_t viewInfoOpt = 0;    // Показать информацию о ФС
uint8_t showFileTree = 0;   // Вывести файловое дерево на экран

static BootSector bs;           // Копия загрузочного сектора (только первые 90 байт)
static uint32_t* fat;           // FAT-таблица
static off_t dataRegOff;     // Смещение региона данных

/**
 * Инициализировать структуру BootSector
*/
static void init_boot_sector(off_t offset) {
    fs_read(offset, sizeof(BootSector), &bs);

    // Перевод многобайтовых чисел (полей) из little-endian в формат хоста (т.е. нашего компьютера)
    
    bs.bytesPerSector = le16toh(bs.bytesPerSector);
    bs.resvdSectCount = le16toh(bs.resvdSectCount);
    bs.rootEntCnt_16 = le16toh(bs.rootEntCnt_16);
    bs.totalSectors_16 = le16toh(bs.totalSectors_16);
    bs.fatSz_16 = le16toh(bs.fatSz_16);
    bs.sectorsPerTrack = le16toh(bs.sectorsPerTrack);
    bs.numHeads = le16toh(bs.numHeads);
    bs.hiddenSectors = le32toh(bs.hiddenSectors);
    bs.totalSectors_32 = le32toh(bs.totalSectors_32);
    bs.fatSz_32 = le32toh(bs.fatSz_32);
    bs.extFlags = le16toh(bs.extFlags);
    bs.fsVersion = le16toh(bs.fsVersion);
    bs.rootClusterNum = le32toh(bs.rootClusterNum);
    bs.fsInfoSectorNumber = le16toh(bs.fsInfoSectorNumber);
    bs.bkBootSec = le16toh(bs.bkBootSec);
    bs.volumeID = le32toh(bs.volumeID);
    bs.checkSignature = le16toh(bs.checkSignature);
}

/**
 * Вычислить смещение таблицы FAT32
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
*/
static off_t get_fat_offset(int num) {
    return (bs.resvdSectCount * bs.bytesPerSector) + (num * bs.fatSz_32 * bs.bytesPerSector);
}

/**
 * Прочитать FAT-таблицу в массив (не забудьте потом освободить память!)
 * Замечание: старшие 4 бита тут не игнорируются, записи переводятся из little endian в формат хоста
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
 * @param[in]   _fatEntCount    Количество записей таблицы FAT
*/
static void read_fat(int num, uint32_t _fatEntCount) {
    uint32_t fatOffset = get_fat_offset(num);
    fs_read(fatOffset, _fatEntCount * FAT_RECORD_SIZE, fat);
    for (uint32_t i = 0; i < _fatEntCount; i += 1) {
        fat[i] = le32toh(fat[i]);     // Перевод из little endian в формат хоста
    }
}

/**
 * Записать FAT-таблицу
*/
void write_fat(int num, uint32_t entCount) {
    for (uint32_t i = 0; i < entCount; i += 1) {
        fat[i] = htole32(fat[i]);           // Перевод из формата хоста в little endian
    }
    uint32_t fatOffset = get_fat_offset(num);
    fs_write(fatOffset, entCount * FAT_RECORD_SIZE, fat);
}

/**
 * Проверка всей файловой системы
*/
void check_all() {
    int foundErrors = 0;

    // Проверка загрузочного сектора
    init_boot_sector(0);    
    foundErrors = check_boot_sector(true);
    if (foundErrors > 0) {
        printf("При проверке загрузочного сектора найдено %d ошибок.\n", foundErrors);
        printf("1 - Попытаться восстановить загрузочный сектор из резервной копии\n"
            "2 - Завершить работу утилиты\n");
        if (input_int(1, 2) == 1) {               // Если пользователь выбирает попытку восстановления,
            off_t copyOffset = try_find_boot_sector_copy();
            if (copyOffset == -1) {   // Если не получилось найти копию загрузочного сектора
                printf("Не удалось обнаружить копию загрузочного сектора. Дальнейшая проверка файловой системы невозможна\n");
                return;
            }
            else {                    // Если удалось найти резервную копию загрузочного сектора
                printf("Удалось найти резервную копию загрузочного сектора, которая прошла проверку на отсуствие недопустимых значений\n");
                printf("1 - Перезаписать загрузочный сектор из найденной резервной копии\n");
                printf("2 - Оставить как есть и не перезаписывать\n");            
                if (input_int(1,2) == 1) { // Перезапись всего загрузочного сектора
                    uint8_t* buffer = (uint8_t*)alloc(bs.bytesPerSector, sizeof(uint8_t));
                    fs_read(copyOffset, bs.bytesPerSector, buffer);
                    fs_write(0, bs.bytesPerSector, buffer);
                    free(buffer);
                    printf("Загрузочный сектор перезаписан из резервной копии\n");
                }
            }
        }
        else {
            printf("Завершение работы утилиты\n");
            return;
        }
    }

    if (viewInfoOpt)
        print_filesystem_info();

    // FAT-таблица (чтение)
    uint32_t fatEntCount = bs.fatSz_32 * bs.bytesPerSector / FAT_RECORD_SIZE; // Количество
    fat = (uint32_t*)alloc(fatEntCount, FAT_RECORD_SIZE);
    if (bs.extFlags & 0x0080)                          // Если 7-й бит == 1 (активна 1 FAT)
        read_fat((bs.extFlags & 0x000F), fatEntCount); 
    else
        read_fat(0, fatEntCount);                       // Если включено зеркалирование

    //Проверка FAT-таблицы
    foundErrors = check_fat_table(true);
    if (foundErrors > 0) {
       
        printf("Найдено %d ошибок при проверке FAT-таблицы.\nМожно попробовать восстановить таблицу FAT из резервной копии\n", foundErrors);
        printf("1 - Попытаться найти резервную копию таблицы FAT\n"
            "2 - Завершить работу утилиты\n");
        if (input_int(1, 2) == 1) {                 // Если пользователь выбирает попытку восстановления
            if (!try_find_fat_copy()) {             // Если не получилось найти копию FAT-таблицы
                printf("Не удалось найти резервную копию основной FAT-таблицы. Дальнейшая проверка файловой системы невозможна\n");
                return;
            }
            else {                                  // Если получилось найти копию FAT-таблицы
                printf("Найдена резервная копия таблицы FAT.\n"
                    "1 - Перезаписать резервную таблицу на место основной\n"
                    "2 - Оставить как есть и не перезаписывать\n");
                
                if (input_int(1, 2)) {                             // Перезапись FAT-таблицы
                    write_fat(0, bs.fatSz_32 * bs.bytesPerSector / FAT_RECORD_SIZE);
                    bs.extFlags = 0x0080;               // Записать в флаги 0b 0000 0000 1000 0000 (выключить зеркалирование и активная таблица - 0)
                    uint16_t temp = htole16(bs.extFlags);
                    fs_write(40, sizeof(temp), &temp);  // 40 - смещение поля extFlags в boot sector
                    fs_write(bs.bkBootSec * bs.bytesPerSector + 40, sizeof(temp), &temp);

                    printf("FAT-таблица перезаписана\n");
                }
            }
        }
        else {
            printf("Завершение работы утилиты\n");
            return;
        }
    }

    dataRegOff = bs.resvdSectCount * bs.bytesPerSector + bs.fatSz_32 * bs.bytesPerSector * bs.numFats;
    off_t rootOffset = dataRegOff + (bs.rootClusterNum - 2) * bs.sectorsPerCluster * bs.bytesPerSector;
    printf("\nПроверенные файлы:\n\n");
    read_and_check_dir_tree(rootOffset, "/", 0);            // Проверить регион данных

    free(fat);                      // Освободить память
    
    return;
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
                " но некоторые драйверы FAT могут работать неправильно.\n", bs.numFats);
        errors += 1;
    }
    else if (bs.numFats > 2) {
        if (print)
            fprintf(stdout, "Предупреждение в загрузочном секторе (BIOS Parameter Block) по смещению 16: количество %hhu таблиц FAT нежелательно.\n"
                "Количество таблиц FAT должно быть 2 для любой FAT. Хотя допустимы и другие значения, "
                "многие программы и системы не будут корректно работать. Значение 2 даѐт избыточность FAT структуры, "
                "при этом в случае потери сектора, данные не потеряются, потому что они дублированы. На не-дисковых носителях, "
                "например карта памяти FLASH, где избыточность не требуется, для экономии памяти может использоваться значение 1,"
                " но некоторые драйверы FAT могут работать неправильно.\n", bs.numFats);
        warnings += 1;
    }

    if (!(bs.mediaType >= 0xF8 && bs.mediaType <= 0xFF) && bs.mediaType != 0xF0) {
        if (print)
            fprintf(stdout, "Предупреждение в загрузочном секторе (BIOS Parameter Block) по смещению 21: %hhx недопустимое значение типа диска\n"
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
            printf("1 - Добавить сигнатуру 0xAA55 по смещению 510\n");
            printf("2 - Оставить как есть\n");
            if (input_int(1, 2) == 1) {
                bs.checkSignature = 0xAA55;
            }
        }
        errors += 1;
    }

    return errors;
}

/**
 * Вывод информации о файловой системе
*/
void print_filesystem_info() {
    printf("Параметры диска:\n");
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
    
    char tmp [12] = {};
    memcpy(tmp, bs.volumeLabel, sizeof(bs.volumeLabel));
    printf("Имя диска: %s\n", tmp);
    
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
                gaps += 1;
                if (print) {
                    printf("Неисправность в FAT: Обнаружен разрыв в цепочке кластеров по смещению %u\n", j);
                    if ((fat[j] & 0x0FFFFFFF) == 0)
                        printf("Кластер %u является пустым кластером (равен 0), что недопустимо в цепочке кластеров\n", j);
                    else
                        printf("Кластер %u указывает на значение %u, которое находится за пределами FAT\n", j, fat[j] & 0x0FFFFFFF);
                    
                    printf("1 - Пометить кластер %u значением EOC\n", j);
                    printf("2 - Оставить как есть\n");
                    if (input_int(1, 2) == 1) {
                        fat[j] = 0x0FFFFFFF;
                        fs_write(get_fat_offset(0) + FAT_RECORD_SIZE * j, FAT_RECORD_SIZE, (fat + j));
                        gaps -= 1;
                    }
                }
                break;
            }
            else if (visitedClusters[fat[j] & 0x0FFFFFFF]) {     // Если следующее значение уже проходили, то цикл в цепочек кластеров
                if (print) {
                    printf("Неисправность в FAT: Обнаружен цикл в цепочке кластеров\n"
                        "Кластер %u указывает на кластер %u, который находится перед кластером %u или в другой цепочке кластеров\n", j, fat[j], j);
                    printf("1 - Пометить кластер %u значением EOC\n", j);
                    printf("2 - Оставить как есть\n");
                    if (input_int(1, 2) == 1) {
                        fat[j] = 0x0FFFFFFF;
                        fs_write(get_fat_offset(0) + FAT_RECORD_SIZE * j, FAT_RECORD_SIZE, (fat + j));
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

/**
 * Удалить entAmount записей из директории, начиная со смещения startOff
*/
static void delete_entries_from_dir(off_t startOff, int entAmount) {
    uint8_t tmp = 0xE5;
    for (int i = 0; i < entAmount; i += 1, startOff += sizeof(DirEntry)) {
        fs_write(startOff, sizeof(uint8_t), &tmp);
    }
}

/**
 * Получить индекс первого кластера файла ! В ТАБЛИЦЕ FAT ! из записи директории
*/
static uint32_t get_first_cluster_num(const DirEntry* sfn) {
    return ((sfn->firstClusterHigh * 65536) + sfn->firstClusterLow);
}

/**
 * Проверка дерева каталогов
 * Начало обработки - первая дочерняя (от корневого каталога) запись
 * @param[in] offset    Смещение директории (файла с 32-байтными записями)
 * @param[in] path      Полный путь к директории (для корневого каталога == "/")
 * @param[in] depth     Глубина в дереве каталогов (для корневой директории == 0)
*/
void read_and_check_dir_tree(off_t offset, const char* path, int depth) {
    LfnEntry tmpLfn = {};       // Временная переменная для длинной записи
    DirEntry shortEntry = {};   // Временная переменная для короткой записи 
    char* fileName = NULL;      // Имя файла
    off_t lfnOffset = 0;        // Для хранения смещения lfn
    off_t dirSize = get_dir_size(offset) * bs.sectorsPerCluster * bs.bytesPerSector;    // Размер директории в байтах
    off_t finalOffset = offset + dirSize;                             // Смещение конца директории

    for (; offset < finalOffset; offset += sizeof(DirEntry)) {        //  Цикл по записям (коротким) директории
        fs_read(offset, sizeof(LfnEntry), &tmpLfn);
        if (tmpLfn.id == 0xE5 || tmpLfn.id == 0x05) {
            offset += sizeof(DirEntry);
            continue;
        }
        if (tmpLfn.id == 0)
            break;
        lfnOffset = offset;
        if (IS_LONG_ENT(tmpLfn.attr) && tmpLfn.reserved == 0) {       // Если LFN ("длинная запись")
            offset += sizeof(LfnEntry);                             // Сместить указатель на одну запись
            LfnStack* top = (LfnStack*)alloc(1, sizeof(LfnStack));  // Вершина стека с длинными записями
            top->entry = tmpLfn;    // Первая длинная запись
            int longEntryCount = 1; // Для подсчёта длинных записей
            
            // Цикл по считыванию с устройства в стек длинных записей одной короткой записи
            while (1) {
                LfnStack* stackNode = (LfnStack*)alloc(1, sizeof(LfnStack));
                fs_read(offset, sizeof(LfnEntry), &(stackNode->entry));
                
                if (stackNode->entry.id == 0xE5 || stackNode->entry.id == 0) {
                    clear_stack(top);
                    fprintf(stderr, "Неисправность в Data Region: В каталоге %s обнаружена цепочка длинных записей"
                        " по смещению %ld, не имеющая за собой короткой\n", path, offset);
                    printf("1 - Удалить эти длинные записи директории\n");
                    printf("2 - Оставить всё как есть\n");
                    if (input_int(1, 2) == 1) {
                        delete_entries_from_dir(lfnOffset, (offset - lfnOffset) / FAT_RECORD_SIZE);
                    }
                }

                // Если новая запись - длинная запись, сохранить в стеке
                if (IS_LONG_ENT(stackNode->entry.attr) && stackNode->entry.reserved == 0) {
                    stackNode->next = top;
                    top = stackNode;
                    longEntryCount += 1;    // Подсчёт длинных записей
                }
                else {                      // Если новая запись - короткая запись
                    memcpy(&(shortEntry), &(stackNode->entry), sizeof(DirEntry));  // Скопировать в поле короткой записи из дерева каталогов
                    free(stackNode);        // Освободить память
                    stackNode = NULL;
                    break;                      // Выйти из цикла
                }
                offset += sizeof(LfnEntry); // Шаг считывания записей
            }
            fileName = put_name_from_stack(&top, longEntryCount);   // Считывание из стека элементов длинного имени файла
        }
        else {  // Если короткая запись (нет перед ней длинных)
            memcpy(&shortEntry, &tmpLfn, sizeof(DirEntry));
            fileName = (char*)alloc(12, sizeof(char));
            memcpy(fileName, shortEntry.name, sizeof(shortEntry.name));
            // Для записей "." и ".."
            for (char* tmp = fileName + 10; tmp >= fileName && *tmp == ' '; tmp -= 1)
                *tmp = '\0';
        }
        
        char* fullPath = (char*)alloc(strlen(path) + strlen(fileName) + 2, sizeof(char));
        strcpy(fullPath, path);
        if (depth > 0)
            strcat(fullPath, "/");
        strcat(fullPath, fileName);

        uint32_t fileFirstClusterNum = get_first_cluster_num(&shortEntry);      // Индекс 1-ого кластера файла (в таблице FAT)
        uint32_t fileFirstClusterVal = fat[fileFirstClusterNum] & 0x0FFFFFFF;   // Считать значение
        if (fileFirstClusterVal == 0x0FFFFFF7) {
            fprintf(stderr, "Неисправность в Data Region: файл %s лежит в BAD CLUSTER\n", fileName);
            printf("1 - Удалить эту запись из директории\n");
            printf("2 - Оставить как есть\n");
            if (input_int(1,2) == 1) {
                delete_entries_from_dir(lfnOffset, ((offset - lfnOffset) / sizeof(LfnEntry)) + 1);
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
                    fs_write(lfnOffset, get_fat_offset(0) + fileFirstClusterNum * 4, (fat + fileFirstClusterNum + 2));
                    break;
                }
                case 2: {
                    delete_entries_from_dir(lfnOffset, ((offset - lfnOffset) / sizeof(LfnEntry)) + 1);
                    break;
                }
            }
        }

        if (showFileTree) {
            rewind(stdout);
            for (int i = 0; i < depth; i += 1)
                printf("  ");
            printf("%s\n", fileName);
        }

        if (shortEntry.attributes & ATTR_DIR && strcmp(fileName, ".") && strcmp(fileName, "..")) {
            // "- 2", потому что нужен индекс первого кластера отн-но региона данных, а не в FAT
            uint32_t offInDataReg = (fileFirstClusterNum - 2) * bs.sectorsPerCluster * bs.bytesPerSector;
            read_and_check_dir_tree(dataRegOff + offInDataReg, fullPath, depth + 1);
            continue;
        }
        free(fileName);
        free(fullPath);
    }
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
            init_boot_sector(bs.bkBootSec * bs.bytesPerSector);
            if (!check_boot_sector(false))         // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * bs.bytesPerSector;

            // 6 - номер сектора в резервной области диска, где хранится копия boot сектора согласно спецификации (и на практике)
            init_boot_sector(6 * bs.bytesPerSector);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * bs.bytesPerSector;
            
            // 512 - количество байт в секторе (которое чаще всего используется)
            init_boot_sector(bs.bkBootSec * 512);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * 512;

            init_boot_sector(6 * 512);
            if (!check_boot_sector(false))
                return 6 * 512;
        }
        else {
            init_boot_sector(6 * bs.bytesPerSector);
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * bs.bytesPerSector;
        
            init_boot_sector(6 * 512);
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return 6 * 512;
        }
    }
    else {
        if (bs.bkBootSec > 0) {
            // 512 - количество байт в секторе (которое чаще всего используется)
            init_boot_sector(bs.bkBootSec * 512);      
            if (!check_boot_sector(false))                     // Если ошибок нет в прочитанном boot sector
                return bs.bkBootSec * 512;

            init_boot_sector(6 * 512);
            if (!check_boot_sector(false))
                return 6 * 512;
        }
        else {
            init_boot_sector(6 * 512);
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
        read_fat(i, bs.fatSz_32 * bs.bytesPerSector / FAT_RECORD_SIZE);    // Считываем (предполагаемую) таблицу FAT в буфер fat (размер fat не меняем, т.к. boot sector правильный), не read_fat(), чтобы без лишнего выделения паямяти
        if (!check_fat_table(false)) // Если нет ошибок
            return true;
    }

    return false;
}
