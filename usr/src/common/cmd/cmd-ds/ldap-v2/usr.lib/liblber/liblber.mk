#ident	"@(#)liblber.mk	1.5"
#ident	"$Header$"

LDAPTOP = ../..

include $(LIBRULES)
include $(LDAPTOP)/local.defs

INSDIR=		$(USRLIB)

HDIR=		$(LDAPTOP)/include
LOCALINC=	-I$(HDIR) 

OBJECTS=   	decode.o encode.o io.o bprint.o
SOURCES=   	decode.c encode.c io.c bprint.c

LIBRARY=	liblber.so

.c.o:
	$(CC) $(CFLAGS) $(PICFLAGS) $(INCLIST) $(DEFLIST) -c $<

all:	$(LIBRARY)  

$(LIBRARY):	$(OBJECTS)
	$(CC) -G -dy -ztext -o $@ $(OBJECTS); \
	$(RM) -f ../$@; \
	$(LN) -s liblber/$@ ../$@; \
	echo liblber/$@ ../$@

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) $(LIBRARY)

clean:
	$(RM) -f *.o 

clobber:	clean
	$(RM) -f $(LIBRARY) *.o ../$(LIBRARY)

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) $(SOURCES)

FRC:

#
# Header dependencies
#

decode.o:	decode.c \
		$(INC)/lber.h

encode.o:	encode.c \
		$(INC)/lber.h

io.o:		io.c \
		$(INC)/lber.h

bprint.o:	bprint.c \
		$(INC)/lber.h

