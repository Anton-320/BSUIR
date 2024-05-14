/**
 * fs_fun.c - Промежуточные операции в файловой системе (чтение FAT таблицы, загрузочного сектора и т.п.) 
*/

#include "fs_fun.h"
#include "lfn.h"

/**
 * Инициализировать структуру BootSector
*/
BootSector init_boot_sector(off_t offset) {
    BootSector bs;
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
    return bs;
}

void write_boot_sector(const BootSector* bs, off_t offset) {
    BootSector bsForWriting = {};
    bsForWriting.bytesPerSector = htole16(bs->bytesPerSector);
    bsForWriting.resvdSectCount = htole16(bs->resvdSectCount);
    bsForWriting.rootEntCnt_16 = htole16(bs->rootEntCnt_16);
    bsForWriting.totalSectors_16 = htole16(bs->totalSectors_16);
    bsForWriting.fatSz_16 = htole16(bs->fatSz_16);
    bsForWriting.sectorsPerTrack = htole16(bs->sectorsPerTrack);
    bsForWriting.numHeads = htole16(bs->numHeads);
    bsForWriting.hiddenSectors = htole32(bs->hiddenSectors);
    bsForWriting.totalSectors_32 = htole32(bs->totalSectors_32);
    bsForWriting.fatSz_32 = htole32(bs->fatSz_32);
    bsForWriting.extFlags = htole16(bs->extFlags);
    bsForWriting.fsVersion = htole16(bs->fsVersion);
    bsForWriting.rootClusterNum = htole32(bs->rootClusterNum);
    bsForWriting.fsInfoSectorNumber = htole16(bs->fsInfoSectorNumber);
    bsForWriting.bkBootSec = htole16(bs->bkBootSec);
    bsForWriting.volumeID = htole32(bs->volumeID);
    bsForWriting.checkSignature = htole16(bs->checkSignature);
    fs_write(offset, sizeof(BootSector), &bsForWriting);
}

/**
 * Вычислить смещение таблицы FAT32
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
*/
off_t get_fat_offset(int num, const BootSector* bs) {
    return (bs->resvdSectCount * bs->bytesPerSector) + (num * bs->fatSz_32 * bs->bytesPerSector);
}

/**
 * Прочитать FAT-таблицу в массив (не забудьте потом освободить память!)
 * Замечание: старшие 4 бита тут не игнорируются, записи переводятся из little endian в формат хоста
 * @param[in]   num   порядковый номер FAT-таблицы, начиная с 0
*/
uint32_t* read_fat_table(int num, const BootSector* bs) {
    uint32_t entCount = bs->fatSz_32 * bs->bytesPerSector / FAT_RECORD_SIZE;    // Количество записей в FAT-таблице
    uint32_t* result = (uint32_t*)alloc(entCount, sizeof(uint32_t));
    uint32_t fatOffset = get_fat_offset(num, bs);
    fs_read(fatOffset, entCount * FAT_RECORD_SIZE, result);
    for (uint32_t i = 0; i < entCount; i += 1) {
        result[i] = le32toh(result[i]);     // Перевод из little endian в формат хоста
    }
    return result;
}

/**
 * Записать FAT-таблицу
*/
void write_fat_table(int num, uint32_t* fat, uint32_t entCount, const BootSector* bs) {
    for (uint32_t i = 0; i < entCount; i += 1) {
        fat[i] = htole32(fat[i]);           // Перевод из формата хоста в little endian
    }
    uint32_t fatOffset = get_fat_offset(num, bs);
    fs_write(fatOffset, entCount * FAT_RECORD_SIZE, fat);
}



/**
 * Поиск номера сектора (начиная с 0), с которого начинается кластер, по номеру кластера
 * @param [in]  N   номер кластера
 * @param [in]  bs  указатель на структуру загрузочного сектора
 * @returns     LBA (logic block address) - номер сектора, начиная с 0
*/
inline uint64_t get_fileEnt_lba(uint32_t N, const BootSector *bs)
{
    return 0;
}