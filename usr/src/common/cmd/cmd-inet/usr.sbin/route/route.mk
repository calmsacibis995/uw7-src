#ident "@(#)route.mk	1.4"
#ident "$Header$"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

include $(CMDRULES)

LDLIBS=	-lsocket -lnsl -lresolv
OWN=	root
GRP=	bin

all:	route

route:	route_msg.h keywords.h route.o linkaddr.o err.o
	$(CC) -o $@ route.o linkaddr.o err.o $(LDLIBS)

route_msg.h:	NLS/en/route.gen
		mkcatdefs route $? >/dev/null

keywords.h:	keywords
	@-awk 'begin { seq = 0 } { \
		if ($$0 ~ /^$$/ || $$0 ~ /^#/) { next } \
		if (NF == 1) { seq++ ; \
			printf "#define\tK_%s\t%d\n\t{\"%s\", K_%s},\n", \
			    toupper($$1), seq, tolower($$1), toupper($$1) } \
		}' keywords > keywords.h

install:	route
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) route
	@-if [ -x "$(DOCATS)" ]; \
	then \
		$(DOCATS) -d NLS $@ ; \
	fi

clean:
	-rm -f route.o linkaddr.o err.o route_msg.h keywords.h
	-rm -f NLS/*/*cat* NLS/*/temp

clobber:	clean
	-rm -f route

lintit:
	$(LINT) $(LINTFLAGS) *.c

