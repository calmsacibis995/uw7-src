#ident	"@(#)nfsping.mk	1.2"
#ident	"$Header$"

include $(CMDRULES)

BINS= nfsping
OBJS= nfsping.o
SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRSBIN)
OWN = bin
GRP = sys
LINTFLAGS= -hbax $(DEFLIST)
LDLIBS= $(LIBELF) -lnsl

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 02555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
