#ident	"@(#)ps:ps.mk	1.7.25.6"
#
# makefile for ps(1) command
#

include $(CMDRULES)

PROCISSUE = PROC Issue 2 Version 1

LDLIBS = -s -lcmd -lw -lgen

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LINTFLAGS=$(DEFLIST)

all:	ps ps.dy

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) ps
	-mkdir ./tmp
	-$(CP) ps.dy ./tmp/ps
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ./tmp/ps
	-rm -rf ./tmp

ps.o: ps.c

ps: ps.o
	$(CC) $(CFLAGS) -o ps ps.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

ps.dy: ps.o
	$(CC) -o ps.dy ps.o $(LDFLAGS) $(LDLIBS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f *.o

clobber:	clean
	rm -f ps ps.dy
