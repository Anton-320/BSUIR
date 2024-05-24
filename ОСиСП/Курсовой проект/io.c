#include "io.h"

int fd = 0;		// Файловый дескриптор проверяемого раздела

/**
 * Открыть раздел с файловой системой (как файл)
*/
void fs_open(const char *path, int rw)
{
	if ((fd = open(path, rw ? O_RDWR : O_RDONLY)) < 0) {
		perror("Open error");
		exit(6);
	}
}

/**
 * Чтение данных из раздела
 * 
 * @param[in]   pos     Смещение в байтах относительно начала раздела, с которого считывать              
 * @param[in]   size    Количество байтов для чтения
 * @param[out]  data    Буффер для прочитанных данных
 */
void fs_read(off_t pos, size_t size, void *data)
{
	ssize_t got;
	if (pos > 0) {
		if (lseek(fd, pos, SEEK_SET) != pos)
			perror("Read error");
	}
	if ((got = read(fd, data, size)) < 0)
		pdie("Read %d bytes at %lld error", size, (long long)pos);
	if ((long long) got != (long long) size)
		pdie("Got %d bytes instead of %d at %lld", got, size, (long long)pos);
}

/**
 * Сместиться на offset байтов (с проверкой)
*/ 
void ch_seek(off_t offset, int whence) {
	if (lseek(fd, offset, whence) != offset)
		perror("Read error");
}

/**
 * Прочитать size байтов (с проверкой)
*/
void ch_read(size_t size, void *data) {
	ssize_t got = read(fd, data, size);
	if (got < 0)
		pdie("Read %d bytes error", size);
	if ((long long) got != (long long) size)
		pdie("Got %d bytes instead of %d", got, size);
	
}

void fs_write(off_t pos, size_t size, void *data)
{
	if (lseek(fd, pos, SEEK_SET) != pos)
		pdie("Seek to %lld", (long long)pos);
	ssize_t writtenBytesAmount = write(fd, data, size);
	if ((long long)writtenBytesAmount != (long long)size)
		pdie("Wrote %d bytes instead of %d at %lld", writtenBytesAmount, size, (long long)pos);
	if (writtenBytesAmount < 0)
		pdie("Write %d bytes at %lld error", size, (long long)pos);
}

/**
 * Закрыть раздел
*/
void fs_close()
{
	if (close(fd) < 0)
		pdie("closing filesystem");
	return;
}
