#ident	"@(#)x286emul:x286emul.mk	1.1.2.3"

#
#	x286emul:  Xenix 286 API environment emulator
#

# Setting DFLAG to -DDEBUG turns on a lot of debugging output.
# Setting -DTRACE turns on system call tracing when the environment
#	variable SYSTRACE is set to Y.
#
# DFLAG = -DDEBUG -DTRACE

include	$(CMDRULES)

CFLAGS = -I. -D_STYPES $(DFLAG)
LDFLAGS	= -s -lx

# Warning, the kernel expects x286emul to be in /usr/bin!

TARGET = x286emul

SFILES = sendsig.s \
syscalla.s \
run286.s \
float.s \
main.c \
utils.c \
sysent.c \
setdscr.c \
syscall.c \
miscsys.c \
exec.c \
debug.c \
ioctl.c \
cxenix.c \
signal.c \
moresys.c \
msgsys.c \
shm.c

HFILES = vars.h \
sysent.h \
h/syscall.h

OFILES = sendsig.o \
syscalla.o \
run286.o \
float.o \
main.o \
utils.o \
sysent.o \
setdscr.o \
syscall.o \
miscsys.o \
exec.o \
debug.o \
ioctl.o \
cxenix.o \
signal.o \
moresys.o \
msgsys.o \
shm.o

all:	x286emul

x286emul:	$(OFILES)
	$(CC) $(OFILES) -o $(TARGET) $(LDFLAGS) $(SHLIBS)

main.o:	vars.h main.c

install:        x286emul
	$(INS) -f $(USRBIN) -m 755 -u bin -g bin x286emul
	-$(SYMLINK) $(INSDIR)/x286emul $(ROOT)/bin/x286emul

clobber:	clean
	rm -f $(TARGET)

clean:
	rm -f $(OFILES)
