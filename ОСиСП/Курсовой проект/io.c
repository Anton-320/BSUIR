#include "io.h"

static int fd = 0;

void fs_open(const char *path, int rw)
{
	if ((fd = open(path, rw ? O_RDWR : O_RDONLY)) < 0) {
		perror("open");
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
void fs_read(off_t pos, int size, void *data)
{
	int got;
	if (pos > 0) {
		if (lseek(fd, pos, SEEK_SET) != pos)
			perror("Read error");
	}
	if ((got = read(fd, data, size)) < 0)
		pdie("Read %d bytes at %lld", size, (long long)pos);
	if (got != size)
		pdie("Got %d bytes instead of %d at %lld", got, size, (long long)pos);
}

/**
 * 
*/
int fs_test(off_t pos, int size)
{
	void *scratch;
	int okay;

	if (lseek(fd, pos, 0) != pos)
	pdie("Seek to %lld", (long long)pos);
	scratch = alloc(size);
	okay = read(fd, scratch, size) == size;
	free(scratch);
	return okay;
}

void fs_write(off_t pos, int size, void *data)
{
	if (lseek(fd, pos, 0) != pos)
		pdie("Seek to %lld", (long long)pos);
	
	int writtenBytesAmount = write(fd, data, size);
	if (writtenBytesAmount != size)
		pdie("Wrote %d bytes instead of %d at %lld", writtenBytesAmount, size, (long long)pos);
	if (writtenBytesAmount < 0)
		pdie("Write %d bytes at %lld", size, (long long)pos);
}

/**
 * Закрыть раздел
*/
void fs_close(int write)
{
	if (close(fd) < 0)
		pdie("closing filesystem");
	return;
}
