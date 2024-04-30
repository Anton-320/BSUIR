#include "fs_fun.h"


uint64_t get_fatEnt_offset(uint64_t clusterNumber, const BootSector* bs)
{
    uint32_t fat_offset = bs->resvdSectCount * bs->bytesPerSector; // Адрес начала таблицы FAT в байтах
    uint32_t offset = clusterNumber * 4;                           // Смещение от начала таблицы FAT
    return fat_offset + offset;                                    // Адрес входной точки в таблице FAT
}

uint32_t get_fatOffset(const BootSector *bs) {
    return bs->resvdSectCount * bs->bytesPerSector;
}

FAT_Entry read_fat_entry(int fd, uint32_t cluster_number, const BootSector* bs) {
    FAT_Entry entry;
    uint32_t fat_offset = bs->resvdSectCount * bs->bytesPerSector;
    uint32_t offset = (cluster_number - 2) * FAT_RECORD_SIZE;
    fs_read(fat_offset + offset, FAT_RECORD_SIZE, &(entry.cluster));
    entry.cluster &= 0x0FFFFFFF;        //4 старших бита игнорируются
    fs_read(fat_offset + entry.cluster, FAT_RECORD_SIZE, &(entry.nextCluster));
    entry.nextCluster &= 0x0FFFFFFF;    //4 старших бита игнорируются
    return entry;
}