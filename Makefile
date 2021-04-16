all: lib pure rpath runpath rpath-interp

lib:
	gcc  -o libtst.o  -c lib_tst.c
	gcc --shared libtst.o -o libtst.so

pure:
	gcc -o foo tst.c -L. -l:libtst.so 

rpath:
	gcc -o foo-rpath tst.c -L. -l:libtst.so -Wl,-rpath,./

runpath:
	gcc -o foo-runpath tst.c -L. -l:libtst.so -Wl,--enable-new-dtags -Wl,-rpath,./

rpath-interp:	
	gcc -o foo-rpath-interp tst.c -L. -l:./libtst.so -Wl,-I -Wl,./ld-2.28.so -Wl,-rpath,./

clean:
	rm foo* libtst.o libtst.so

