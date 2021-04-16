#include "lib_tst.h"



void tst_getenv(const char* n)
{
#ifdef _YET_ANOTHER_
    printf("Yet another libtst shared object\n");
#endif
    char* var = getenv (n);
    printf ("Call function from glibc 2.2.5\n");
	printf ("'%s' %s\n",n, var ? var: " not found\n");
    printf ("Call function from glibc 2.17\n");
    var = secure_getenv (n);
	printf ("'%s' %s\n",n, var ? var: " not found\n");
}
