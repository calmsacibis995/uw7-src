#ident	"@(#)kern-i386:svc/gdb.d/gdb.mk	1.1"
#ident	"$Header$"

include $(UTSRULES)

MAKEFILE = gdb.mk
KBASE = ../..
DIR = svc/gdb
GDB = gdb.o

FILES = \
	db_lock.o \
	db_port.o \
	db_router.o \
	db_seqnum.o \
	db_stub.o

CFILES = \
	db_lock.c \
	db_port.c \
	db_router.c \
	db_seqnum.c \
	db_stub.c

SFILES =

SRCFILES = $(CFILES) $(SFILES)

all:	local FRC

local:	gdb.o

install: localinstall FRC

localinstall:	local FRC

.c.o:
	$(CC) $(MINUSG) $(CFLAGS:-O=) $(INCLIST) $(DEFLIST) -c $<

$(GDB): $(FILES)
	$(LD) -r -o $(GDB) $(FILES)

clean:	localclean

localclean: FRC
	-rm -f *.o $(LFILES) name.ln *.L $(GDB)

clobber: localclobber

localclobber: localclean FRC

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

FRC:

sysHeaders = 

headinstall: $(sysHeaders)

include $(UTSDEPEND)

include $(MAKEFILE).dep

db_lock.c     db_port.c     db_router.c   db_seqnum.c   db_stub.c: db_lock.h     db_port.h     db_router.h   db_seqnum.h
