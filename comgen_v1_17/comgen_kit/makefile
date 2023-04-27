#
#
# Makefile for comgen
# This makefile should automatically determine the platform type.
#
#
OS = $(shell uname -s | dd conv=lcase 2> /dev/null)

all:
	@echo "Building comgen for $(OS)"
	${MAKE} $(OS)

clean:
	rm -f *.o comgen

#
# ---- Linux build 
#
linux:	comgen_linux

comgen_linux.o:	comgen.c
	gcc -c -Wall -D_linux_ ${CFLAGS} -D_LARGEFILE_ comgen.c -o comgen_linux.o

comgen_linux:	comgen_linux.o
	gcc -Wall -O3 -D_linux_ ${CFLAGS} -D_LARGEFILE_ comgen_linux.o -o comgen

#
# ---- Freebsd build 
#
freebsd:	comgen_bsd

comgen_bsd.o:	comgen.c
	gcc -c -Wall -D_freebsd_ ${CFLAGS} comgen.c -o comgen_bsd.o

comgen_bsd:	comgen_bsd.o
	gcc -Wall -O3 -D_freebsd_ ${CFLAGS} comgen_bsd.o -o comgen

#
# ---- Solaris build 
#
sunos:	comgen_sunos

comgen_sunos.o:	comgen.c
	gcc -c -Wall -D_solaris_ ${CFLAGS} comgen.c -o comgen_sunos.o

comgen_sunos:	comgen_sunos.o
	gcc -Wall -O3 -D_solaris_ ${CFLAGS} comgen_sunos.o -o comgen
#
# ---- MacOS build 
#
darwin:	comgen_darwin

comgen_darwin.o:	comgen.c
	gcc -c -Wall -D_macos_ ${CFLAGS} comgen.c -o comgen_darwin.o

comgen_darwin:	comgen_darwin.o
	gcc -Wall -O3 -D_macos_ ${CFLAGS} comgen_darwin.o -o comgen

#
# ---- AIX build 
#
aix:	comgen_aix

comgen_aix.o:	comgen.c
	xlc -c -Wall -D_aix_ ${CFLAGS} comgen.c -o comgen_aix.o

comgen_aix:	comgen_aix.o
	xlc -Wall -O3 -D_aix_ ${CFLAGS} comgen_aix.o -o comgen

#
# ---- hpux build 
#
hpux:	comgen_hpux

comgen_hpux.o:	comgen.c
	gcc -c -Wall -D_hpux_ ${CFLAGS} comgen.c -o comgen_hpux.o

comgen_hpux:	comgen_hpux.o
	gcc -Wall -O3 -D_hpux_ ${CFLAGS} comgen_hpux.o -o comgen

