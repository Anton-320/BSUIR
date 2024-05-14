
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

extern char **environ;

#define VAR_NUM 9			//количество переменных окружения для считывания

char** getShortVariables(const char* fname);                    // получение список переменных окружения из файла
int includeString(const char* str1, const char* str2);          // входит ли строка в другую строку
char* findEnvpVariable(char* envp[], const char* variable);     // поиск параметра

int main(int argc, char* argv[], char* envp[]) {

	printf("Process name: %s\nProcess pid: %d\nProcess ppdid: %d\n", argv[0], (int) getpid(), (int) getppid());

	char **variables = getShortVariables(argv[1]);                             // получение списка необходимых переменных среды из файла
	if (variables != NULL) {
		switch (argv[2][0]) {
			case '+':
				for (int i = 0; i < VAR_NUM; i++)                              // получение переменной среды системной функцией
					printf("%s=%s\n", variables[i], getenv(variables[i]));
				break;
			case '*':
				for (int i = 0; i < VAR_NUM; i++)                              // получение переменной среды из аргументов main()
					printf("%s\n", findEnvpVariable(envp, variables[i]));
				break;
			case '&':
				for (int i = 0; i < VAR_NUM; i++)                              // получение переменной среды из внешней переменной environ
					printf("%s\n", findEnvpVariable(environ, variables[i]));
				break;
		}
		for (int i = 0; i < VAR_NUM; i++)
			free(variables[i]);
	}
	free(variables);
	return 0;
}

char** getShortVariables(const char* fname) {
	FILE *f = NULL;
	if ((f = fopen(fname, "r")) != NULL) {                             
		fseek(f, 0, SEEK_SET);
		char **result = (char **)calloc(VAR_NUM, sizeof(char *));
		for (int i = 0; i < VAR_NUM; i++) {
			result[i] = (char*)calloc(80, sizeof(char));
			fgets(result[i], 80, f);
			if (result[i][strlen(result[i])-1] == '\n')		// чтение всех строк
				result[i][strlen(result[i])-1] = '\0';
		}
		fclose(f);
		return result;
	}
	printf("Error while open file.\n");
	return NULL;
}

int includeString(const char* str1, const char* str2) {
	for(size_t i = 0; i<strlen(str2); i++)
		if(str1[i]!=str2[i])
			return 0;
	return 1;
}

char* findEnvpVariable(char* envp[], const char* variable) {
	int i = 0;
	while(envp[i]) {
		if(includeString(envp[i], variable))
			break;
		i++;
	}
	return envp[i];
}