
#ident  "@(#)makefile	1.3"
#ident  "$Header$"

include $(CMDRULES)

LOCAL=../inc
LBINDIR=$(USRSBIN)
BINDIR=$(USRSADM)/sysadm/bin
HELPDIR=$(USRSADM)/bin
SKELDIR=$(ETC)/skel
HERE=.

LIBDIR = ../lib
LIBUSRGRP=$(LIBDIR)/lib.a
LDLIBS = -lcmd -lia -liaf -ladm -lgen -lnsl -lsocket
USERLSLIBS=-lia -lgen

LOCALINC=-I$(LOCAL)
LINTFLAGS=-u $(DEFLIST)

PRODUCTS=useradd userdel usermod getngroups finduid .vtlrc .profile .login expdate userls

ADD_SRC=useradd.c uidage.c  homedir.c groups.c \
	call_udel.c rmfiles.c val_lgrp.c \
	restore_ia.c ypsubr.c isbusy.c
DEL_SRC=userdel.c uidage.c rmfiles.c isbusy.c groups.c \
	restore_ia.c ypsubr.c
MOD_SRC=usermod.c  uidage.c groups.c rmfiles.c \
	isbusy.c homedir.c val_lgrp.c \
	restore_ia.c ypsubr.c
GETNGR_SRC=getngroups.c
FINDUIDSRC=finduid.c uid.c uidage.c
EXPDSRC=expdate.c
LS_SRC=userls.c

ADD_OBJ=$(ADD_SRC:.c=.o)
DEL_OBJ=$(DEL_SRC:.c=.o)
MOD_OBJ=$(MOD_SRC:.c=.o)
LS_OBJ=$(LS_SRC:.c=.o)

FINDUIDOBJ=$(FINDUIDSRC:.c=.o)

GETNGR_OBJ=$(GETNGR_SRC:.c=.o)
EXPDOBJ=$(EXPDSRC:.c=.o)

OBJECTS = $(ADD_OBJ) $(DEL_OBJ) $(MOD_OBJ) $(GETNGR_OBJ) $(FINDUIDOBJ) \
		$(EXPDOBJ) $(LS_OBJ)

all: $(PRODUCTS)

useradd: $(ADD_OBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(ADD_OBJ) $(LDFLAGS) $(LIBUSRGRP) \
		$(LDLIBS)

userdel: $(DEL_OBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(DEL_OBJ) $(LDFLAGS) $(LIBUSRGRP) \
		$(LDLIBS)

usermod: $(MOD_OBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(MOD_OBJ) $(LDFLAGS) $(LIBUSRGRP) \
		$(LDLIBS)

getngroups: $(GETNGR_OBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(GETNGR_OBJ) $(LDFLAGS) \
		$(LIBUSRGRP) $(LDLIBS)

finduid: $(FINDUIDOBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(FINDUIDOBJ) $(LDFLAGS) \
		$(LIBUSRGRP) $(LDLIBS)

.profile: stdprofile
	grep -v "^#ident" stdprofile > .profile

.vtlrc: stdvtlrc
	grep -v "^#ident" stdvtlrc > .vtlrc

.login: stdlogin
	grep -v "^#ident" stdlogin > .login

expdate: $(EXPDOBJ) $(LIBUSRGRP)
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(EXPDOBJ) $(LDFLAGS) $(LIBUSRGRP) \
		$(LDLIBS)

userls: $(LS_OBJ) $(LIBUSRGRP)
	$(CC) -DSTATS $(CFLAGS) $(DEFLIST) -o $(@) $(LS_OBJ) $(LDFLAGS) $(LIBUSRGRP) \
		$(USERLSLIBS)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

install: all 
	-[ -d $(ETC)/default ] || mkdir -p $(ETC)/default
	cp userdel.dfl $(ETC)/default/userdel
	cp useradd.dfl $(ETC)/default/useradd
	cp usermod.dfl $(ETC)/default/usermod
	$(INS) -m 555 -g sys -u root -f $(LBINDIR) useradd
	$(INS) -m 555 -g sys -u root -f $(LBINDIR) userdel
	$(INS) -m 555 -g sys -u root -f $(LBINDIR) usermod
	$(INS) -m 555 -g sys -u root -f $(LBINDIR) userls
	$(INS) -m 500 -g sys -u root -f $(BINDIR) getngroups
	$(INS) -m 500 -g sys -u root -f $(BINDIR) expdate
	$(INS) -m 500 -g sys -u root -f $(BINDIR) finduid
	$(INS) -m 644 -g other -u root -f $(SKELDIR) .profile
	$(INS) -m 644 -g other -u root -f $(SKELDIR) .vtlrc
	$(INS) -m 644 -g other -u root -f $(SKELDIR) .login
	$(INS) -m 555 -g sys -u root -f $(HELPDIR) usermodU

size: all
	$(SIZE) $(PRODUCTS)

strip: all
	$(STRIP) $(PRODUCTS)

lintit:
	$(LINT) $(LINTFLAGS) $(ADD_SRC)
	$(LINT) $(LINTFLAGS) $(DEL_SRC)
	$(LINT) $(LINTFLAGS) $(MOD_SRC)
	$(LINT) $(LINTFLAGS) $(GETNGR_SRC)
	$(LINT) $(LINTFLAGS) $(FINDUIDSRC)
	$(LINT) $(LINTFLAGS) $(EXPDSRC)
	$(LINT) $(LINTFLAGS) $(LS_SRC)

groups.o: $(INC)/sys/types.h
groups.o: $(INC)/stdio.h
groups.o: $(INC)/ctype.h
groups.o: $(INC)/grp.h
groups.o: $(INC)/unistd.h
groups.o: $(INC)/userdefs.h
groups.o: $(LOCAL)/users.h
rmfiles.o: $(INC)/sys/types.h
rmfiles.o: $(INC)/stdio.h
rmfiles.o: $(INC)/userdefs.h
rmfiles.o: $(INC)/errno.h
rmfiles.o: $(LOCAL)/messages.h
userdel.o: $(INC)/sys/types.h
userdel.o: $(INC)/stdio.h
userdel.o: $(INC)/ctype.h
userdel.o: $(INC)/limits.h
userdel.o: $(INC)/pwd.h
userdel.o: $(INC)/string.h
userdel.o: $(INC)/sys/types.h
userdel.o: $(INC)/sys/stat.h
userdel.o: $(INC)/userdefs.h
userdel.o: $(INC)/errno.h
userdel.o: $(LOCAL)/users.h
userdel.o: $(LOCAL)/messages.h
finduid.o: $(INC)/sys/types.h
finduid.o: $(INC)/stdio.h
finduid.o: $(INC)/userdefs.h
homedir.o: $(INC)/errno.h
homedir.o: $(INC)/sys/types.h
homedir.o: $(INC)/stdio.h
homedir.o: $(INC)/userdefs.h
homedir.o: $(LOCAL)/messages.h
movedir.o: $(INC)/stdio.h
movedir.o: $(INC)/sys/types.h
movedir.o: $(INC)/userdefs.h
movedir.o: $(LOCAL)/messages.h
uid.o: $(INC)/sys/types.h
uid.o: $(INC)/stdio.h
uid.o: $(INC)/userdefs.h
uid.o: $(INC)/sys/param.h
uid.o: $(INC)/limits.h
usermod.o: $(INC)/sys/types.h
usermod.o: $(INC)/sys/stat.h
usermod.o: $(INC)/sys/param.h
usermod.o: $(INC)/stdio.h
usermod.o: $(INC)/ctype.h
usermod.o: $(INC)/limits.h
usermod.o: $(INC)/pwd.h
usermod.o: $(INC)/grp.h
usermod.o: $(INC)/string.h
usermod.o: $(INC)/userdefs.h
usermod.o: $(LOCAL)/users.h
usermod.o: $(LOCAL)/messages.h
usermod.o: $(INC)/audit.h
isbusy.o: $(INC)/stdio.h
isbusy.o: $(INC)/string.h
isbusy.o: $(INC)/sys/types.h
isbusy.o: $(INC)/utmp.h
useradd.o: $(INC)/sys/types.h
useradd.o: $(INC)/sys/stat.h
useradd.o: $(INC)/sys/param.h
useradd.o: $(INC)/stdio.h
useradd.o: $(INC)/ctype.h
useradd.o: $(INC)/limits.h
useradd.o: $(INC)/grp.h
useradd.o: $(INC)/string.h
useradd.o: $(INC)/userdefs.h
useradd.o: $(LOCAL)/users.h
useradd.o: $(LOCAL)/messages.h
useradd.o: $(INC)/audit.h
val_lgrp.o: $(INC)/sys/types.h
val_lgrp.o: $(INC)/stdio.h
val_lgrp.o: $(INC)/sys/param.h
val_lgrp.o: $(INC)/grp.h
val_lgrp.o: $(LOCAL)/users.h
val_lgrp.o: $(INC)/userdefs.h
val_lgrp.o: $(LOCAL)/messages.h
getngroups.o: $(INC)/stdio.h
expdate.o: $(INC)/sys/types.h
expdate.o: $(INC)/stdio.h
expdate.o: $(INC)/userdefs.h
expdate.o: $(LOCAL)/users.h
userls.o: $(INC)/sys/types.h
userls.o: $(INC)/stdio.h
userls.o: $(INC)/errno.h
userls.o: $(INC)/userdefs.h
userls.o: $(INC)/sys/param.h
userls.o: $(INC)/limits.h
userls.o: $(INC)/grp.h
userls.o: $(LOCAL)/messages.h
userls.o: $(LOCAL)/getxopt.h
