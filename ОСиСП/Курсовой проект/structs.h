#pragma once
#include <stdint.h>

#define DIR_NAME_LEN 11

// Биты атрибутов файла
#define ATTR_NONE 0	        // no attribute bits
#define ATTR_RO 0x01	    // read-only
#define ATTR_HIDDEN 0x02	// hidden
#define ATTR_SYS 0x04	    // system
#define ATTR_VOLUME 0x08	// volume label
#define ATTR_DIR 0x10	    // directory
#define ATTR_ARCH 0x20	    // archived
#define ATTR_LFN (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)     // Атрибут записи длинной директории
#define IS_LONG_ENT(x) ((x & ATTR_LFN) == ATTR_LFN)       // Проверить по атрибуту, является ли запись длинной


/**
 * Структура загрузочного сектора
 * (неполная, т.к. содержит только BPB и несколько значимых полей загрузочного сектора)
*/
typedef struct {

    // До смещения 36

    uint8_t  jmpBoot[3];         // Код (инструкции) для перехода к загрузочному коду
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

typedef struct _directory_entry_structure {
    uint8_t  name[DIR_NAME_LEN];    // Имя файла (8 байт) и расширение (3 байта)
    uint8_t  attributes;            // Атрибуты файла
    uint8_t  reservedNT;            // Зарезервировано для Windows NT
    uint8_t  creationTimeTenth;     // Время создания в десятых долях секунды
    uint16_t creationTime;          // Время создания
    uint16_t creationDate;          // Дата создания
    uint16_t lastAccessDate;        // Дата последнего доступа
    uint16_t firstClusterHigh;      // Старшее слово индекса первого кластера в таблице FAT
    uint16_t lastModifiedTime;      // Время последнего изменения
    uint16_t lastModifiedDate;      // Дата последнего изменения
    uint16_t firstClusterLow;       // Младшее слово индекса первого кластера в таблице FAT
    uint32_t fileSize;              // Размер файла в байтах
} __attribute__((packed)) DirEntry;