#ident "@(#)Makefile	1.2"
#ident "$Header$"

include $(LIBRULES)

LIBRARY= libresolv.a
LOCALDEF = -DSVR4 -DUSE_POSIX -DUSE_OPTIONS_H
LOCALINC = -I../include

SRCS=	base64.c gethnamaddr.c getnetbyaddr.c getnetbyname.c getnetent.c \
	getnetnamadr.c herror.c hostnamelen.c inet_addr.c inet_net_ntop.c \
	inet_net_pton.c inet_neta.c inet_ntop.c inet_pton.c nsap_addr.c \
	res_comp.c res_data.c res_debug.c res_init.c res_mkquery.c \
	res_query.c res_send.c sethostent.c

OBJS=	$(SRCS:.c=.o)

all install : $(LIBRARY)

libresolv.a: $(OBJS)
	$(AR) $(ARFLAGS) libresolv.a `$(LORDER) $(OBJS) | $(TSORT)`

clean:
	rm -f *.o

clobber: clean
	rm -f $(LIBRARY)

depend:
	mkdep -I$(INCL) -I$(COMPINCL) $(DEFS) $(SRCS)
