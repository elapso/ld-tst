#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>

void foo(char* f);
void tst_getenv(const char* n);
