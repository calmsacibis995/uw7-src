#
#       @(#) Makefile 12.1 95/10/02 SCOINC
#
#	Copyright (C) The Santa Cruz Operation, 1993-1995
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
include $(CMDRULES)
#CFLAGS =	-w3 -wx -O
LDFLAGS =	-s

all:	rl

rl:	rl.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o rl rl.c -lc
	cp rl ../../bin/rl
	cp rls ../../bin/rls

clean:
	rm -f rl.o

clobber:
	rm -f rl rl.o

