#ident	"@(#)libtelnet.mk	1.3"

#	Makefile for libtelnet

include $(LIBRULES)

DEVINC1 = -U_REENTRANT

LIBRARY = libtelnet.a

OBJECTS = genget.o setenv.o

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

install: all
	$(INS) -f $(USRLIB) -m 0644 -u bin -g bin $(LIBRARY)

clean:
	rm -f $(OBJECTS)

clobber:	clean
	rm -f $(LIBRARY)
