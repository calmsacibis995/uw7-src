#ident	"@(#)dstime:dstime/dstime.mk	1.2"

include $(CMDRULES)

INSDIR = $(USRLIB)/dstime
OWN = root
GRP = sys

CFLAGS = -O -DSTATIC=static
LDFLAGS =
LIBS = 
INCDIR = /usr/include

OBJS = dst_next.o dst_pgen.o dst_adtodc.o norm_tm.o
SRCS = $(OBJS:.o=.c)

MAIN = dst_adtodc dst_next dst_pgen dst_sched
# MAIN = dst_adtodc dst_next dst_pgen

all: $(MAIN)

dst_adtodc: dst_adtodc.o norm_tm.o
	$(CC) -o $@ $? $(LDFLAGS) $(LIBS)

dst_next: dst_next.o
	$(CC) -o $@ $? $(LDFLAGS) $(LIBS)

dst_pgen: dst_pgen.o
	$(CC) -o $@ $? $(LDFLAGS) $(LIBS)

dst_sched: dst_sched.sh
	cp $? $@
	chmod +x $@

norm_tm.o: norm_tm.c \
	timem.h

lintit:
	lint $(CFLAGS) $(SRCS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(MAIN)

install: all dirs
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) dst_adtodc
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) dst_next
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) dst_pgen
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) dst_sched

dirs:
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)

size: all
	$(SIZE) $(MAIN)

strip: all
	$(STRIP) $(MAIN)

