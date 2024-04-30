/**
 * Утилита проверки целостности файловой системы FAT32
 * аргумент командной строки - название устройства, на котором нужно проверять файловую систему
*/


#include "Header.h"

extern int fd;

int main(int argc, char* argv[]) {
	
	if (argc == 1 || strlen(argv[1]) < 1) {
		perror("Error: Invalid argument");
		return 0;
	}
	
	fs_open(argv[1], 0);
	
	
	char deviceName[9] = {};
	strlcpy(deviceName, argv[1], strlen(argv[1]));
	return 0;
}