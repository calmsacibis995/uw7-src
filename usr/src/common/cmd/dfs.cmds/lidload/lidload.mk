#	copyright	"%c%"

#ident	"@(#)lidload.mk	1.2"
#ident "$Header$"

include $(CMDRULES)

BINS= lidload
OBJS= lidload.o
SRCS= $(OBJS:.o=.c)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
LOCALDEF=-DSYSV
LINTFLAGS= -bcnsu $(DEFLIST)
IS_COFF=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "_s -lsocket" ; fi`
LDLIBS= -lnsl$(IS_COFF) -lcmd

all: $(BINS)


$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	if [ ! -d $(INSDIR) ] ; \
	then mkdir -p $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)

lidload.o: lidload.c \
	lidload.h
