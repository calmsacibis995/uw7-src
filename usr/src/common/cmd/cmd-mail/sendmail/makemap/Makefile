#ident	"@(#)Makefile	11.1

include		../Make.defs

LOCALINC =	-I../src -I../../libdb/include $(DEFS) -DNOT_SENDMAIL

include		$(CMDRULES)

LDLIBS =	-L../../libdb -L$(USRLIB) -ldb

OBJS =		makemap.o safefile.o

makemap:	$(OBJS)
		$(CC) -o makemap $(CFLAGS) $(DEFLIST) $(OBJS) $(LDFLAGS) $(LDLIBS)

safefile.o:	../src/safefile.c
		$(CC) -c $(CFLAGS) $(DEFLIST) ../src/safefile.c

build:		makemap

clean clobber:
		rm -f makemap $(OBJS)
