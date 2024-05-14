#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

extern int alphasort (const struct dirent **__e1,
		      const struct dirent **__e2)
     __THROW __attribute_pure__ __nonnull ((1, 2));

extern int scandir (const char *__restrict __dir,
    struct dirent ***__restrict __namelist,
    int (*__selector) (const struct dirent *),
    int (*__cmp) (const struct dirent **,
		  const struct dirent **))
   __nonnull ((1, 2));

extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __THROW __nonnull ((2, 3));

extern int optind;

struct Params {
	unsigned link : 1;
	unsigned dir : 1;
	unsigned file : 1;
	unsigned sort : 1;
	unsigned recursive : 1;
};

void dirWalk(const char* path, struct Params opt) {	
	
	struct dirent** dirInfo;	//the information about directory
	errno = 0;		//for chasing the errors
	int n;			//Number of files in the directory
	if (opt.sort)
		n = scandir(path, &dirInfo, NULL, alphasort);
	else
		n = scandir(path, &dirInfo, NULL, NULL);
	if (n == 0)
		return;
	for (int i = 0; i < n; i += 1) {

		char* fullpath = calloc(strlen(path) + strlen(dirInfo[i]->d_name) + 3, sizeof(char));
        strcpy(fullpath, path);
		if (fullpath[strlen(fullpath) - 1] != '/' && fullpath[strlen(fullpath) - 1] != '\\')
        	strcat(fullpath, "/");
		strcat(fullpath, dirInfo[i]->d_name);
		
		struct stat statbuf;
		if (stat(fullpath, &statbuf) == -1)
				perror("stat function error");

		if (!opt.file && !opt.dir && !opt.link ||
			opt.dir && S_ISDIR(statbuf.st_mode) ||
			opt.link && S_ISLNK(statbuf.st_mode) ||
			opt.file && S_ISREG(statbuf.st_mode))
			printf("%s\n", fullpath);

		if (opt.recursive && S_ISDIR(statbuf.st_mode) && strcmp(dirInfo[i]->d_name, "..") && strcmp(dirInfo[i]->d_name, "."))
			dirWalk(fullpath, opt);

		free(fullpath);
		free(dirInfo[i]);

	}
	free(dirInfo);

	if (errno != 0)
		perror(NULL);
}

int main(int argc, char** argv) {

	if (argc <= 1) {
		perror("There isn't filepath\n");
		exit(1);
	}
	char opt = 0;
	struct Params params = { 0 };
	while ((opt = getopt(argc, argv, "fdlsr")) != -1)	//set flags
	{
		switch (opt)
		{
		case 'f':
			params.file = 1;
			break;
		case 'd':
			params.dir = 1;
			break;
		case 'l':
			params.link = 1;
			break;
		case 's':
			params.sort = 1;
			break;
		case 'r':
			params.recursive = 1;
			break;
		default:
			perror("Wrong param");
			exit(1);
		}
	}

	for (int i = 1; i < argc; i+=1) {
		if (argv[i][0] != '-')
		{
			dirWalk(argv[i], params);
			break;
		}
	}
	return 0;
}
