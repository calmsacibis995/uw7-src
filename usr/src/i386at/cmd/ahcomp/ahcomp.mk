#ident	"@(#)hcomp:ahcomp.mk	1.1.1.1"

include $(CMDRULES)

OWN = bin
GRP = bin

MAINS = hcomp

OBJECTS = hcomp.o wslib.o

SOURCES = hcomp.c wslib.c wslib.h

.c.o:
	$(HCC) -c $(CFLAGS) $*.c

all: $(SOURCES) $(OBJECTS) $(MAINS)

hcomp.o: $(SOURCES)

$(MAINS): $(OBJECTS)
	$(HCC) -o $(MAINS) $(OBJECTS) -lw

$(SOURCES):
	@ln -s ${ROOT}/usr/src/${WORK}/cmd/winxksh/libwin/$@ $@

install: all
	@if [ ! -d $(USRBIN) ] ;\
	then \
		mkdir -p $(USRBIN) ;\
	fi
	$(INS) -f $(USRBIN) -m 0555 hcomp

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)
