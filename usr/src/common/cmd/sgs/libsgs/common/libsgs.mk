#ident	"@(#)sgs:libsgs/common/libsgs.mk	1.26"
#
#  makefile for libsgs.a
#
#
include $(CMDRULES)

all: ../libsgs.a

empty.o:
	echo 'static int empty;' >empty.c
	$(HCC) -c $(CFLAGS) empty.c

../libsgs.a: empty.o
	$(HAR) -q $@ empty.o

clean:
	rm -f empty.[co]

clobber: clean
	rm -f ../libsgs.a
