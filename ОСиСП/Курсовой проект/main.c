/**
 * Утилита проверки целосности файловой системы FAT32
 * аргумент командной строки - название устройства, на котором нужно проверять файловую систему
*/


#include "Header.h"

extern size_t strlcpy(char *__restrict__ __dest, const char *__restrict__ __src, size_t __n);

int main(int argc, char* argv[]) {
	
	if (argc == 1 || strlen(argv[1]) < 1) {
		perror("Error: Invalid argument");
		return 0;
	}

	char deviceName[9] = {};
	strlcpy(deviceName, argv[1], strlen(argv[1]));
	return 0;
}