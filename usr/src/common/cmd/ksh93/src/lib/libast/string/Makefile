#ident	"@(#)ksh93:src/lib/libast/string/Makefile	1.1"
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

HEADERSRC = modelib.h

SOURCES = chresc.c chrtoi.c fmtbase.c fmtdev.c fmtelapsed.c \
	fmterror.c fmtesc.c fmttime.c fmtsignal.c \
	fmtfs.c fmtgid.c fmtls.c fmtmatch.c fmtmode.c fmtperm.c \
	fmtre.c fmtuid.c mematoe.c memdup.c memetoa.c modedata.c \
	modei.c modex.c strcopy.c strdup.c strelapsed.c strexpr.c \
	stresc.c streval.c strgid.c strmatch.c strmode.c stropt.c \
	strlook.c strperm.c strtape.c strton.c struid.c swapget.c \
	strsort.c swapmem.c swapop.c swapput.c tok.c \
	tokline.c tokscan.c


OBJECTS = $(SOURCES:.c=.o)

all:	$(OBJECTS)

clean:
	rm -f $(OBJECTS)

clobber:	clean

.c.o:
	$(CC) $(CCFLAGS) $(LOCALINC) $(GLOBALINC) -c $< && ln -f $@ ..
