#pragma once
#include <stdint.h>

#define DIR_NAME_LEN 11

// Биты атрибутов файла
#define ATTR_NONE 0	    // no attribute bits
#define ATTR_RO 1	    // read-only
#define ATTR_HIDDEN 2	// hidden
#define ATTR_SYS 4	    // system
#define ATTR_VOLUME 8	// volume label
#define ATTR_DIR 16	    // directory
#define ATTR_ARCH 32	// archived
#define ATTR_LONG_NAME (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)     // Атрибут записи длинной директории



/**
 * Структура загрузочного сектора
 * (неполная, т.к. содержит только BPB и несколько значимых полей загрузочного сектора)
*/
typedef struct {

    // До смещения 36

    uint8_t  jmpBoot[3];         // Код для перехода к загрузочному коду
    uint8_t  OEMName[8];         // Имя OEM (производителя) системы
    uint16_t bytesPerSector;     // Количество байтов в секторе
    uint8_t  sectorsPerCluster;  // Количество секторов в кластере
    uint16_t resvdSectCount;     // Количество секторов в Reserved Region
    uint8_t  numFats;            // Количество FAT-таблиц
    uint16_t rootEntCnt_16;      // Количество записей в корневом каталоге (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint16_t totalSectors_16;    // Общее количество секторов на носителе (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint8_t  mediaType;          // Тип носителя информации (диска)
    uint16_t fatSz_16;           // Количество секторов, ВЫДЕЛЕННЫХ под одну FAT-таблицу (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint16_t sectorsPerTrack;    // Количество секторов на дорожке
    uint16_t numHeads;           // Количество головок устройства
    uint32_t hiddenSectors;      // Количество скрытых секторов перед разделом
    uint32_t totalSectors_32;    // Общее количество секторов на носителе (только для FAT32)

    // После смещения 36

    uint32_t fatSz_32;           // Количество секторов, ВЫДЕЛЕННЫХ под одну FAT-таблицу (для FAT32)
    uint16_t extFlags;           // Флаги файловой системы (по использованию FAT-таблицы)
    uint16_t fsVersion;          // Версия файловой системы
    uint32_t rootClusterNum;     // Номер кластера корневого каталога ОТНОСИТЕЛЬНО ПЕРВОГО КЛАСТЕРА DATA REGION (0-й кластер - самый первый кластер в Data Region и т.д.) (для FAT32)
    uint16_t fsInfoSectorNumber; // Номер сектора информации о файловой системе
    uint16_t bkBootSec;          // Номер сектора в резервной области диска, где хранится копия boot сектора
    uint8_t  reserved[12];       // Зарезервировано
    uint8_t  driveNumber;        // Номер дискового устройства
    uint8_t  reserved1;          // Зарезервировано
    uint8_t  bootSignature;      // Сигнатура загрузочного сектора (0x29)
    uint32_t volumeID;           // Уникальный идентификатор тома
    uint8_t  volumeLabel[11];    // Метка тома (имя)
    uint8_t  fileSystemType[8];  // Тип файловой системы ("FAT32   "), [только не по нему определять тип: это для информации] 
    uint8_t  junk[420];          // Для заполнения пространства
    uint16_t checkSignature;     // Сигнатура для проверки 0xAA55 (с учётом little endian: sector[510] == 0x55, sector[511] == 0xAA)
} __attribute__((packed)) BootSector;

typedef struct _filesystem_info_sector_structure {
    uint32_t leadSig;		    // Начальная сигнатура для точного определения сектора FSInfo, всегда равен 0x41615252
    uint8_t reserved1[480];     // Зарезервировано на будущее
    uint32_t signature;		    // Сигнатура для точного определения положения следующих за ним полей. Всегда равен 0x61417272
    uint32_t free_clusters;	    // Хранит последнее известное количество свободных кластеров диска. Если равно 0xFFFFFFFF, то количество неизвестно, и должно быть вычислено
    uint32_t nextCluster;	    // Вспомогательное значение для драйвера FAT. Содержит номер кластера, начиная с которого надо искать свободный кластер (для быстроты)
    uint8_t reserved2[12];      // Зарезервировано на будущее
    uint32_t boot_sign;         // Значение 0xAA550000. Это конечная сигнатура для точного определения сектора FSInfo
} __attribute__ ((packed)) FS_Info;

typedef struct _directory_entry_structure {
    uint8_t  name[DIR_NAME_LEN];    // Имя файла (8 байт) и расширение (3 байта)
    uint8_t  attributes;            // Атрибуты файла
    uint8_t  reservedNT;            // Зарезервировано для Windows NT
    uint8_t  creationTimeTenth;     // Время создания в десятых долях секунды
    uint16_t creationTime;          // Время создания
    uint16_t creationDate;          // Дата создания
    uint16_t lastAccessDate;        // Дата последнего доступа
    uint16_t firstClusterHigh;      // Старшее слово номера первого кластера
    uint16_t lastModifiedTime;      // Время последнего изменения
    uint16_t lastModifiedDate;      // Дата последнего изменения
    uint16_t firstClusterLow;       // Младшее слово номера первого кластера
    uint32_t fileSize;              // Размер файла в байтах
} __attribute__((packed)) DirEntry;

typedef struct _dir_tree_node {
    DirEntry this;                  // Информация о текущем файле
    char* lfn;                      // Long File Name
    struct _dir_tree_node* parent;  // Родитель
    struct _dir_tree_node* prev;    // Предыдущий в списке дочерних (от parent) директорий
    struct _dir_tree_node* next;    // Следующий в списке дочерних (от parent) директорий
} DirNode;

typedef struct _fat_table {
    uint32_t* fat;       // Массив записей таблицы
    uint32_t size;       // Размер (количество элментов)
} FAT;

typedef struct _int_pair {
    int first;
    int last;
} PAIR;