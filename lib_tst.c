#include "lib_tst.h"

void foo(char* f)
{
    struct timespec time[2];
    time[0].tv_nsec = UTIME_NOW;
    time[1].tv_nsec = UTIME_NOW;
 
    int fd;
    fd = open(f,O_RDONLY);
 
    if(fd == -1){
        printf("open file failed. %s\n",  strerror(errno));
    }
    else{
        if(-1 == futimens(fd,time)){
            printf("modify time failed.\n");
        }
        else{
            printf("modify time succeed.\n");
        }
    }
  
}


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
