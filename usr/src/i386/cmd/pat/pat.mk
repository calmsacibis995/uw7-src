#ident	"@(#)pat:pat.mk	1.1"

include $(CMDRULES)			# OSr5: ../make.inc

MAIN	= pat
INSPERM	= -m 0555 -u bin -g bin
INSDIR	= $(USRSBIN)			# OSr5: $(ROOT)/etc
LDLIBS	= -lelf -lc

HFILES	= pat.h

CFILES	= pat.c patsym.c oscompat.c

OFILES	= pat.o patsym.o oscompat.o

all: 	$(MAIN)

$(MAIN): $(OFILES)
	$(CC) -o $@ $(OFILES) $(LDFLAGS) $(LDLIBS)

pat.o patsym.o: $(HFILES)

install: all
	$(INS) -f $(INSDIR) $(INSPERM) $(MAIN)
# OSr5: cp $(MAIN) $(INSDIR)

clean:
	-rm -f $(OFILES)

clobber: 	clean
	-rm -f $(MAIN)

lint:	$(HFILES) $(CFILES)
	$(LINT) $(LINTFLAGS) $(CFILES)
