#ident "@(#)in.mrouted.mk	1.3"

include ${CMDRULES}

LOCALDEF=	-DSYSV
INSDIR=		$(USRSBIN)
CONFDIR=	$(ETC)/inet
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

IGMP_SRCS=	igmp.c inet.c kern.c
IGMP_OBJS=	$(IGMP_SRCS:.c=.o)

ROUTER_SRCS=	config.c cfparse.y main.c route.c vif.c prune.c callout.c
ROUTER_OBJS=	config.o cfparse.o main.o route.o vif.o prune.o callout.o

MAPPER_SRCS=	mapper.c
MAPPER_OBJS=	$(MAPPER_SRCS:.c=.o)

MRINFO_SRCS=	mrinfo.c
MRINFO_OBJS=	$(MRINFO_SRCS:.c=.o)

MTRACE_SRCS=	mtrace.c
MTRACE_OBJS=	$(MTRACE_SRCS:.c=.o)

PROGRAMS=	in.mrouted map-mbone mrinfo mtrace
HDRS=		defs.h dvmrp.h route.h vif.h prune.h

SRCS= $(IGMP_SRCS) $(ROUTER_SRCS) $(MAPPER_SRCS) $(MRINFO_SRCS) \
      $(MTRACE_SRCS)

OBJS= $(IGMP_OBJS) $(ROUTER_OBJS) $(MAPPER_OBJS) $(MRINFO_OBJS) \
      $(MTRACE_OBJS)

all : $(PROGRAMS)

install : all
	@for i in $(PROGRAMS);\
	do\
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$i;\
	done
	$(INS) -f $(CONFDIR) -m 0555 -u $(OWN) -g $(GRP) mrouted.conf

$(OBJS): $(HDRS)

cfparse.o : cfparse.c

cfparse.c : cfparse.y

in.mrouted: $(IGMP_OBJS) $(ROUTER_OBJS)
	$(CC) -o $@ $(IGMP_OBJS) $(ROUTER_OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

map-mbone: $(IGMP_OBJS) $(MAPPER_OBJS)
	$(CC) -o $@ $(IGMP_OBJS) $(MAPPER_OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

mrinfo: $(IGMP_OBJS) $(MRINFO_OBJS)
	$(CC) -o $@ $(IGMP_OBJS) $(MRINFO_OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

mtrace: $(IGMP_OBJS) $(MTRACE_OBJS)
	$(CC) -o $@ $(IGMP_OBJS) $(MTRACE_OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

clean:
	rm -f $(OBJS) cfparse.c

clobber: clean
	rm -f in.mrouted map-mbone mrinfo mstat mtrace
