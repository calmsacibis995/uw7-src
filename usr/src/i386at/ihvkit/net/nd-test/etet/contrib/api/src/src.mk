# contrib/api/src/Makefile
# Ensure that these definitions are suitable for your system

CC     = cc
# GNU CC
#CC      = gcc

all:	ctest1 ctest7 ctest8 parallel-test EXTRACT

EXTRACT: ctest8.c ptest8 test5 exectool stest1 test6 build_fail_tool \
		 indirect-list stest7 test7 buildtool parallel-test stest8 test8 \
		 cleantool test2 testlist ctest1.c ptest1 test3 \
		ctest7.c ptest7 test4
	-for i in $? ; do chmod a+rx $$i; \
	done


LIBDIR = $(TET_ROOT)/lib/posix_c

#include $(CMDRULES)

ctest1: ctest1.c
	$(CC) $(CFLAGS) -I$(TET_ROOT)/inc/posix_c -o ctest1 ctest1.c \
			$(LIBDIR)/tcm.o $(LIBDIR)/libapi.a
ctest7: ctest7.c
	$(CC) $(CFLAGS) -I$(TET_ROOT)/inc/posix_c -o ctest7 ctest7.c \
			$(LIBDIR)/tcm.o $(LIBDIR)/libapi.a
ctest8: ctest8.c
	$(CC) $(CFLAGS) -I$(TET_ROOT)/inc/posix_c -o ctest8 ctest8.c \
			$(LIBDIR)/tcm.o $(LIBDIR)/libapi.a

parallel-test: remove_lock

remove_lock:
	rm -fr lockdir

CLEAN:
	rm -fr lockdir *.o ctest1 ctest7 ctest8 tty.output
