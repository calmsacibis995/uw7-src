#	copyright	"%c%"

#ident	"@(#)devmgmt:common/cmd/devmgmt/putdev/putdev.mk	1.5.9.6"
#ident "$Header$"

include $(CMDRULES)

INSDIR=$(SBIN)
OWN=root
GRP=sys
HDRS=$(INC)/stdio.h $(INC)/string.h $(INC)/ctype.h $(INC)/stdlib.h \
	$(INC)/errno.h $(INC)/unistd.h $(INC)/mac.h \
	$(INC)/devmgmt.h
PROGRAM=putdev
SRC=main.c
OBJS=$(SRC:.c=.o)
LOCALINC=-I.
LDLIBS=-ladm
LINTFLAGS=$(DEFLIST)

all:	$(PROGRAM)

install:	all
		-rm -f $(INSDIR)/putdev
		$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) $(PROGRAM) 

clobber: clean
		rm -f $(PROGRAM)

clean:
		rm -f $(OBJS)

lintit:
		for i in $(SRC); \
		do \
		    $(LINT) $(LINTFLAGS) $$i; \
		done

$(PROGRAM): $(OBJS)
		$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

$(OBJS): $(HDRS)
