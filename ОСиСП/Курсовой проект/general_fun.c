#include "general_fun.h"

void pdie(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, ": %s\n", strerror(errno));
	exit(1);
}

void* alloc(int size)
{
	void *this;

	if ((this = malloc(size)))
		return this;
	pdie("malloc");
		return NULL;		/* for GCC */
}