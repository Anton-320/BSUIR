/**
 * Утилита проверки целостности файловой системы FAT32
 * аргумент командной строки - название устройства, на котором нужно проверять файловую систему
*/

#include "fs_check.h"
#include <getopt.h>
#define OPTION_HELP 1000

extern uint8_t viewInfoOpt;
extern uint8_t showFileTree;

const char* progName = "fatcheck";

static struct option option[] = {
	{"help", no_argument, 0, OPTION_HELP},
};

int main(int argc, char* argv[]) {
	
	if (argc == 1 || strlen(argv[1]) < 1) {
		printf("Ошибка. Неправильное количество аргументов\n");
		usage("fatcheck");
		return 0;
	}

	int opt = 0;
	while((opt = getopt_long(argc, argv, "it", option, NULL)) != -1) {
		switch (opt)
		{
		case 'i': {
			viewInfoOpt = 1;
			break;
		}
		case 't': {
			showFileTree = 1;
			break;
		}
		case OPTION_HELP: {
			usage(progName);
			break;
		}
		default: {
			printf("Неправильные аргументы при запуске программы\n");
			usage(progName);
			return 0;
		}
		}
	}	

	fs_open(argv[argc - 1], 1);
	check_all();
	printf("Проверка файловой системы завершена\n");
	fs_close();
	return 0;
}