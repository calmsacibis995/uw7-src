#ident "%W%"

#
#	L001	16Feb1998	tonylo
#	- Check for the presence of source file, if not found then
#	  assume the SCP is being built and do not attempt compile
#
include $(CMDRULES)

LDLIBS=	-lcrypt -lia -liaf

OWN=	root
GRP=	bin

all:	in.i2odialogd

#L001 vvv
SRCFILEPRESENT=`ls in.i2odialogd.c 2>/dev/null`

in.i2odialogd.o:
	-@if [ ! -z "$(SRCFILEPRESENT)" ]; then \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c in.i2odialogd.c; \
	fi
#L001 ^^^
	

in.i2odialogd:	in.i2odialogd.o
	$(CC) -o $@ in.i2odialogd.o $(LDLIBS)

install:	in.i2odialogd
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) in.i2odialogd
	@-if [ -x "$(DOCATS)" ]; \
	then \
		$(DOCATS) -d NLS $@ ; \
	fi


clean:
	-rm -f in.i2odialogd.o
	-rm -f NLS/en/temp
	-rm -f NLS/en/in.i2odialogd.cat
	-rm -f NLS/en/in.i2odialogd.cat.m

clobber:	clean
	-rm -f in.i2odialogd

lintit:
	$(LINT) $(LINTFLAGS) *.c

