#ident	"@(#)asa:asa.mk	1.3"

include $(CMDRULES)

INS = ../install/install.sh
INSDIR = $(USRBIN)
OWN = bin
GRP = bin

FILE = asa

all: $(FILE)

$(FILE): $(FILE).o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS) $(LDLIBS) 

install: all
	cp asa asa.bak
	$(STRIP) asa
	/bin/sh $(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) $(FILE)
	mv asa.bak asa

clean:
	rm -rf *.o

clobber: clean
	rm -f $(FILE)
