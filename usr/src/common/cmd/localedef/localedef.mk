#ident	"@(#)localedef:localedef.mk	1.1.1.1"

include $(CMDRULES)

#	Makefile for localedef

XENV_CCSBIN = $(TOOLS)/usr/ccs/bin
XENV_CCSLIB = $(TOOLS)/usr/ccs/lib
XENV_LIB = $(TOOLS)/usr/lib

OWN = bin
GRP = bin
LOCALINCS = _localedef.h _colldata.h _stdlock.h _wcharm.h


SOURCES = charmap.c collate.c ctype.c get.c localesrc.c main.c messages.c monetary.c numeric.c parse.c sym.c time.c
OBJECTS = $(SOURCES:.c=.o)
DFILES = defctype ascii

all: localedef

localedef: $(OBJECTS)
	$(CC) -o localedef $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

_colldata.h: \
	_wcharm.h \
	_stdlock.h \
	$(INC)/limits.h
	touch _colldata.h

charmap.o: \
	charmap.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	_colldata.h \
	_localedef.h 

collate.o: \
	collate.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/stddef.h \
	_colldata.h \
	_localedef.h 

ctype.o: \
	ctype.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	_colldata.h \
	_localedef.h 

get.o: \
	get.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	_colldata.h \
	_localedef.h 

localesrc.o: \
	localesrc.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	_colldata.h \
	_localedef.h 

main.o: \
	main.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/string.h \
	$(INC)/pfmt.h \
	$(INC)/stdarg.h \
	$(INC)/errno.h \
	_colldata.h \
	_localedef.h 

messages.o: \
	messages.c \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	_colldata.h \
	_localedef.h 

monetary.o: \
	monetary.c \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	_colldata.h \
	_localedef.h 

numeric.o: \
	numeric.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	_colldata.h \
	_localedef.h 

parse.o: \
	parse.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	_colldata.h \
	_localedef.h 

sym.o: \
	sym.c \
	$(INC)/sys/types.h \
	$(INC)/widec.h \
	$(INC)/string.h \
	_colldata.h \
	_localedef.h 

time.o: \
	time.c \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	_colldata.h \
	_localedef.h 

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) localedef 
	 [ -d $(USRLIB)/localedef/charmaps ] || mkdir -p $(USRLIB)/localedef/charmaps ; \
	 	chmod 755 $(USRLIB)/localedef $(USRLIB)/localedef/charmaps ; \
	 	$(CH) chgrp $(GRP) $(USRLIB)/localedef $(USRLIB)/localedef/charmaps ; \
	 	$(CH) chown $(OWN) $(USRLIB)/localedef $(USRLIB)/localedef/charmaps 
	 [ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES ; \
	 	chmod 755 $(USRLIB)/locale $(USRLIB)/locale/C $(USRLIB)/locale/C/MSGFILES ; \
	 	$(CH) chgrp $(GRP) $(USRLIB)/locale $(USRLIB)/locale/C $(USRLIB)/locale/C/MSGFILES ; \
	 	$(CH) chown $(OWN) $(USRLIB)/locale $(USRLIB)/locale/C $(USRLIB)/locale/C/MSGFILES 
	 $(INS) -f $(USRLIB)/localedef -m 0444 -u $(OWN) -g $(GRP) defctype
	 $(INS) -f $(USRLIB)/localedef/charmaps -m 0444 -u $(OWN) -g $(GRP) ascii
	 $(INS) -f $(USRLIB)/locale/C/MSGFILES -m 0444 -u $(OWN) -g $(GRP) localedef.str

xenv_install: all
	echo '$(XENV_CCSLIB)/localedef -p "$(XENV_LIB)/localedef/charmaps/ascii" -l "$(XENV_LIB)/localedef/defctype" $${1+"$$@"};exit $$?' > localedef.sh
	cp  localedef.sh $(CPU)localedef
	$(INS) -f $(XENV_CCSLIB) -m 0555 -u $(OWN) -g $(GRP) localedef 
	$(INS) -f $(XENV_CCSBIN) -m 0555 -u $(OWN) -g $(GRP) $(CPU)localedef 
	rm localedef.sh
	[ -d $(XENV_LIB)/localedef/charmaps ] || mkdir -p $(XENV_LIB)/localedef/charmaps ; \
		chmod 755 $(XENV_LIB)/localedef $(XENV_LIB)/localedef/charmaps ; \
	 	$(CH) chgrp $(GRP) $(XENV_LIB)/localedef $(XENV_LIB)/localedef/charmaps ; \
	 	$(CH) chown $(OWN) $(XENV_LIB)/localedef $(XENV_LIB)/localedef/charmaps
	$(INS) -f $(XENV_LIB)/localedef -m 0444 -u $(OWN) -g $(GRP) defctype 
	$(INS) -f $(XENV_LIB)/localedef/charmaps -m 0444 -u $(OWN) -g $(GRP) ascii 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f localedef

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo localedef.mk $(SOURCES) $(LOCALINCS) $(DFILES) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN)  $(USRLIB)/localedef $(USRLIB)/localedef/charmaps | tr ' ' '\012' | sort

product:
	@echo $(USRBIN)/localedef $(USRLIB)/localedef/defctype $(USRLIB)/localedef/charmaps/ascii | tr ' ' '\012' 

srcaudit:
	@fileaudit localedef.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) localedef
