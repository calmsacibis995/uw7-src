#ident	"@(#)ksh93:src/lib/libast/misc/Makefile	1.1"
#  ast library make

include $(LIBRULES)

CCFLAGS = -O $(CC.PIC)

INSTALLROOT = ../../../..

LOCALINC = -I$(INSTALLROOT)/src/lib/libast/include \
	-I$(INSTALLROOT)/src/lib/libast/obsolete \
	-I$(INSTALLROOT)/src/lib/libast/sfio \
	-I$(INSTALLROOT)/src/lib/libast/misc \
	-I$(INSTALLROOT)/src/lib/libast/dir \
	-I$(INSTALLROOT)/src/lib/libast/re \
	-I$(INSTALLROOT)/src/lib/libast/hash \
	-I$(INSTALLROOT)/src/lib/libast/string \
	-I$(INSTALLROOT)/src/lib/libast/man \
	-I$(INSTALLROOT)/src/lib/libast

HEADERSRC = univlib.h proclib.h 

SOURCES = error.c fastfind.c fs3d.c ftwalk.c ftwflags.c \
	getcwd.c \
	liberror.c magic.c \
	optget.c optjoin.c \
	procclose.c procfree.c procopen.c \
	procrun.c setenviron.c \
	sigdata.c sigcrit.c signal.c stack.c \
	state.c stk.c systrace.c univdata.c

OBJECTS = $(SOURCES:.c=.o)

all:	$(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber:	clean

malloc.o:	malloc.c
	$(CC) $(CCFLAGS) -D_PACKAGE_ast $(LOCALINC) $(GLOBALINC) -c $< && \
	cp $@ ..

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $< && ln -f $@ ..
