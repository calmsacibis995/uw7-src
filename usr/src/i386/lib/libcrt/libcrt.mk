#ident	"@(#)libcrt:libcrt.mk	1.4"

include $(LIBRULES)

SGSBASE=../../cmd/sgs
LIBDWARF2=$(SGSBASE)/libdwarf2/i386/libdwarf2.a

TARGET = libcrt.a

OBJECTS = abbreviation.o llasgmul.o lldivrem.o

COMINC	= $(SGSBASE)/inc/common
INCLIST = -I$(COMINC)
INS	= $(SGSBASE)/sgs.install

all: $(TARGET)

libcrt.a:	$(OBJECTS)
	rm -f $(TARGET)
	$(AR) -q $(TARGET) $(OBJECTS)

abbreviation.o: abbreviation.s
	$(CC) -c abbreviation.s

llasgmul.o: llasgmul.s
	$(CC) -c llasgmul.s

lldivrem.o: lldivrem.s
	$(CC) -c lldivrem.s

abbreviation.s:	make_abbrev
	./make_abbrev abbreviation.s

make_abbrev: make_abbrev.c $(COMINC)/abbrev.h
	$(HCC) $(CFLAGS) $(INCLIST) -o make_abbrev make_abbrev.c $(LIBDWARF2)

install:	$(TARGET)
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libcrt.a libcrt.a

clean:
	rm -f make_abbrev abbreviation.s $(OBJECTS) lint.out

clobber: clean
	rm -f $(TARGET)

lintit:	make_abbrev.c
	$(LINT) $(CFLAGS) $(INCLIST) make_abbrev.c >lint.out
