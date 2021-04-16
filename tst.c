#include "lib_tst.h"

int main(int argc, char *argv[ ]) {
    if (argc != 2)
    {
	printf("usage: %s enviroment-variable-name\n", argv[0]);    
	return 0;
    }	    
    
    tst_getenv(argv[1]);
}

