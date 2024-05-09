/**
 * Утилита проверки целостности файловой системы FAT32
 * аргумент командной строки - название устройства, на котором нужно проверять файловую систему
*/

#include "fs_check.h"


int main(int argc, char* argv[]) {
	initscr();
	
	if (argc == 1 || strlen(argv[1]) < 1) {
		perror("Error: Invalid argument");
		return 0;
	}
	fs_open(argv[1], 0);
	check_all();
	
	char deviceName[9] = {};
	strlcpy(deviceName, argv[1], strlen(argv[1]));
	
	endwin();
	return 0;
}