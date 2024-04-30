#pragma once
#include <stdint.h>

#define DIR_NAME_LEN 11

typedef struct {

    //до смещения 36

    uint8_t  jmpBoot[3];         // Код для перехода к загрузочному коду
    uint8_t  OEMName[8];         // Имя OEM (производителя) системы
    uint16_t bytesPerSector;     // Количество байтов в секторе
    uint8_t  sectorsPerCluster;  // Количество секторов в кластере
    uint16_t resvdSectCount;     // Количество секторов в Reserved Region
    uint8_t  fatCount;           // Количество FAT-таблиц
    uint16_t rootEntCnt_16;      // Количество записей в корневом каталоге (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint16_t totalSectors_16;    // Общее количество секторов на носителе (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint8_t  mediaType;          // Тип носителя информации (диска)
    uint16_t fatSz_16;           // Количество секторов в одной FAT-таблице (только для FAT12/FAT16, для FAT32 должно быть равно 0)
    uint16_t sectorsPerTrack;    // Количество секторов на дорожке
    uint16_t heads;              // Количество головок устройства
    uint32_t hiddenSectors;      // Количество скрытых секторов перед разделом
    uint32_t totalSectors_32;    // Общее количество секторов на носителе (только для FAT32)

    //после смещения 36

    uint32_t fatSz_32;           // Количество секторов в одной FAT-таблице (для FAT32)
    uint16_t extFlags;           // Флаги расширений файловой системы
    uint16_t fsVersion;          // Версия файловой системы
    uint32_t rootClusterNum;     // Номер кластера корневого каталога (для FAT32)
    uint16_t fsInfoSectorNumber; // Номер сектора информации о файловой системе
    uint16_t bkBootSec;          // Номер резервной копии загрузочного сектора
    uint8_t  reserved[12];       // Зарезервировано
    uint8_t  driveNumber;        // Номер дискового устройства
    uint8_t  reserved1;          // Зарезервировано
    uint8_t  bootSignature;      // Сигнатура загрузочного сектора (0x29)
    uint32_t volumeID;           // Уникальный идентификатор тома
    uint8_t  volumeLabel[11];    // Метка тома (имя)
    uint8_t  fileSystemType[8];  // Тип файловой системы ("FAT32   ")
} __attribute__((packed)) BootSector;

typedef struct _filesystem_info_sector_structure {
    uint32_t leadSig;		    // Начальная сигнатура для точного определения сектора FSInfo, всегда равен 0x41615252
    uint8_t reserved1[480];     // Зарезервировано на будущее
    uint32_t signature;		    // Сигнатура для точного определения положения следующих за ним полей. Всегда равен 0x61417272
    uint32_t free_clusters;	    // Хранит последнее известное количество свободных кластеров диска. Если равно 0xFFFFFFFF, то количество неизвестно, и должно быть вычислено
    uint32_t next_cluster;	    // Вспомогательное значение для драйвера FAT. Содержит номер кластера, начиная с которого надо искать свободный кластер (для быстроты)
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

typedef struct _fat_list_node {
    uint32_t cluster;       // Номер текущего кластера
    uint32_t next_cluster;  // Номер следующего кластера в цепочке
} FAT_Entry;

typedef struct _int_pair {
    int first;
    int last;
} PAIR;