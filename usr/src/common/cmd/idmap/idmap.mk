#	copyright	"%c%"

#ident	"@(#)idmap.mk	1.2"
#ident "$Header$"

#	Makefile for idmapping administration commands 

include $(CMDRULES)

OWN    = sys
GRP    = sys

INSADMIN = $(USRSBIN)
INSUSER  = $(USRBIN)
INSVAR   = $(VAR)/adm/log

OAMBASE    = $(USRSADM)/sysadm
OAMNAMEMAP = $(OAMBASE)/add-ons/nsu/netservices/name_map
OAMATTRMAP = $(OAMBASE)/add-ons/nsu/netservices/attr_map

OAMPKG   = $(VAR)/sadm/pkg
PKGSAV   = $(OAMPKG)/nsu/save
PKGMI    = $(PKGSAV)/intf_install

BASEDIRS = $(INSADMIN) $(INSUSER) $(INSVAR) 
ETCDIRS  = $(ETC)/idmap $(ETC)/idmap/attrmap 
OAMDIRS  = $(OAMNAMEMAP) $(OAMNAMEMAP)/mappings $(OAMATTRMAP) \
	   $(OAMATTRMAP)/mappings $(OAMPKG)/name_map $(OAMPKG)/attr_map \
	   $(PKGSAV) $(PKGMI)

LDLIBS= -lgen

#top#

MAKEFILE = idmap.mk

MAINS = idadmin uidadmin attradmin

OBJECTS =  idadmin.o uidadmin.o attradmin.o breakname.o namecmp.o check.o

SOURCES =  idadmin.c uidadmin.c attradmin.c breakname.c namecmp.c check.c


all:	$(MAINS)

idadmin:	idadmin.o breakname.o namecmp.o check.o
	$(CC) -o idadmin idadmin.o breakname.o check.o namecmp.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

uidadmin:	uidadmin.o breakname.o namecmp.o check.o
	$(CC) -o uidadmin uidadmin.o breakname.o check.o namecmp.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

attradmin:	attradmin.o breakname.o namecmp.o check.o
	$(CC) -o attradmin attradmin.o breakname.o check.o namecmp.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)


idadmin.o:	idmap.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/time.h \
		$(INC)/unistd.h \
		$(INC)/dirent.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/pfmt.h \
		$(INC)/locale.h

uidadmin.o:	idmap.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/time.h \
		$(INC)/unistd.h \
		$(INC)/dirent.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/pfmt.h \
		$(INC)/locale.h

attradmin.o:	idmap.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/time.h \
		$(INC)/unistd.h \
		$(INC)/dirent.h \
		$(INC)/pwd.h \
		$(INC)/grp.h \
		$(INC)/pfmt.h \
		$(INC)/locale.h

breakname.o:	idmap.h \
		$(INC)/string.h

namecmp.o:	idmap.h \
		$(INC)/stdio.h \
		$(INC)/string.h

check.o:	idmap.h


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:

newmakefile:
	$(MAKE) -f $(MAKEFILE) $(MAKEARGS)

#bottom#

$(BASEDIRS):
	- [ -d $@ ] || mkdir -p $@

$(ETCDIRS):
	- [ -d $@ ] || mkdir -p $@ ; \
		$(CH)chmod 0775 $@ ; \
		$(CH)chown $(OWN) $@ ; \
		$(CH)chgrp $(GRP) $@

$(OAMDIRS):
	- [ -d $@ ] || mkdir -p $@ ; \
		$(CH)chmod 0755 $@ ; \
		$(CH)chown $(OWN) $@ ; \
		$(CH)chgrp $(GRP) $@

install: all $(BASEDIRS) $(ETCDIRS) $(OAMDIRS)
	rm -f $(INSADMIN)/idadmin
	rm -f $(INSUSER)/uidadmin
	rm -f $(INSADMIN)/attradmin
	$(INS) -f $(INSADMIN) -m 00550 -u $(OWN) -g $(GRP) idadmin
	$(INS) -f $(INSUSER)  -m 02551 -u $(OWN) -g $(GRP) uidadmin
	$(INS) -f $(INSADMIN) -m 00550 -u $(OWN) -g $(GRP) attradmin
	$(INS) -f $(PKGMI)    -m 0644 -u $(OWN) -g $(GRP) oam/name_map/name_map.mi
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Help
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Menu.name_map
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Form.add
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Form.remove
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Text.add
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Text.list
	$(INS) -f $(OAMNAMEMAP) -m 0644 -u $(OWN) -g $(GRP) oam/name_map/Text.remove
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Menu.mappings
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.add
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.check
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.disable
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.enable
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.fix
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.list
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Form.remove
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.add
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.check
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.disable
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.enable
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.list
	$(INS) -f $(OAMNAMEMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/name_map/mappings/Text.remove
	$(INS) -f $(PKGMI)    -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/attr_map.mi
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Help
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Menu.attr_map
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Form.add
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Form.remove
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Text.add
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Text.list
	$(INS) -f $(OAMATTRMAP) -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/Text.remove
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Menu.mappings
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Form.add
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Form.check
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Form.fix
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Form.list
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Form.remove
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Text.add
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Text.check
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Text.list
	$(INS) -f $(OAMATTRMAP)/mappings -m 0644 -u $(OWN) -g $(GRP) oam/attr_map/mappings/Text.remove
	$(INS) -f $(INSVAR) -m 0660 -u $(OWN) -g $(GRP) idmap.log

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
