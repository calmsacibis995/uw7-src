#ident	"@(#)pcintf:bridge/bridge.mk	1.1"
#
# PC Interface Unix server makefile
#

#
# This makefile is a version of bridge/make.mstr to comply with the Unix System 
# V Makefile Guidelines from USL.
# The assumption is made that "CFLAGS = -O" in the CMDRULES file.  It is also
# assumed that INC, LDFLAGS, LINT, and LINTFLAGS are set in this file.
#

include		$(CMDRULES)

SRC=		../..

INCLUDES=	-I$(SRC) -I$(INC) $(IFLAGS) \
		-I$(SRC)/../pkg_rlock -I$(SRC)/../pkg_lmf \
		-I$(SRC)/../pkg_lcs
LOCALDEF=	-DNDEBUG $(DFLAGS)
LOCDEFLIST=	$(LOCALDEF) $(INCLUDES) $(WFLAGS)
LIBFLAGS=
MSG_TYPE=	En.lmf
LDLIBS=	../pkg_rlock/librlock.a ../pkg_lockset/liblset.a \
	../pkg_lmf/liblmf.a ../pkg_lcs/liblcs.a $(LIBFLAGS)

# Standard C Source files
CFILES= \
	$(SRC)/common.c		$(SRC)/dir_access.c  \
	$(SRC)/dir_search.c	$(SRC)/disksize.c	$(SRC)/dosmesg.c     \
	$(SRC)/error.c		$(SRC)/exec.c		$(SRC)/getcwd.c      \
	$(SRC)/log.c		$(SRC)/mapname.c	$(SRC)/match.c       \
	$(SRC)/mem.c		$(SRC)/net.c		$(SRC)/p_chdir.c     \
	$(SRC)/p_chmod.c	$(SRC)/p_close.c	$(SRC)/p_connect.c   \
	$(SRC)/p_exit.c		$(SRC)/p_create.c	$(SRC)/p_delete.c    \
	$(SRC)/p_consvr.c	$(SRC)/p_fstatus.c	$(SRC)/p_lock.c      \
	$(SRC)/p_lseek.c	$(SRC)/unmapname.c	$(SRC)/p_mapsvr.c    \
	$(SRC)/p_mkdir.c	$(SRC)/p_open.c		$(SRC)/p_pwd.c       \
	$(SRC)/p_rename.c	$(SRC)/rs232.c		$(SRC)/p_read.c      \
	$(SRC)/p_rmdir.c	$(SRC)/p_search.c	$(SRC)/p_server.c    \
	$(SRC)/p_conread.c	$(SRC)/p_termout.c	$(SRC)/p_write.c     \
	$(SRC)/pckframe.c	$(SRC)/remotelog.c	$(SRC)/swap.c        \
	$(SRC)/translate.c	$(SRC)/truncate.c	$(SRC)/ttymodes.c    \
	$(SRC)/valid.c		$(SRC)/vfile.c		$(SRC)/logpacket.c   \
	$(SRC)/p_mapname.c	$(SRC)/p_ipc.c		$(SRC)/p_version.c   \
	$(SRC)/nls_init.c	$(SRC)/p_nlstab.c

# Standard include files
HFILES= \
	$(SRC)/const.h		$(SRC)/flip.h		$(SRC)/pci_types.h   \
	$(SRC)/sccs.h		$(SRC)/serial.h		$(SRC)/xdir.h        \
	$(SRC)/sysconfig.h	$(SRC)/log.h


RL_HFILES=\
	$(SRC)/../pkg_rlock/rlock.h

LCS_HFILES=\
	$(SRC)/../pkg_lcs/lcs.h  $(SRC)/table.h

LMF_HFILES=\
	$(SRC)/../pkg_lmf/lmf.h

#
# Lists of object modules for each server plus modules common to all servers
#
COMMON_O= \
      common.o  mem.o  log.o  logpacket.o  swap.o  net.o \
      pckframe.o  rs232.o  ttymodes.o nls_init.o

LOADPCI_O= \
      loadpci.o

CONSVR_O= \
      p_consvr.o ifc_list.o

MAPSVR_O= \
      p_mapsvr.o ifc_list.o valid.o

DOSOUT_O= \
      p_termout.o semfunc.o

DOSSVR_O= \
      p_connect.o p_server.o p_open.o p_close.o p_read.o p_write.o \
      p_create.o p_delete.o p_rmdir.o p_mkdir.o p_rename.o p_lseek.o \
      p_search.o p_fstatus.o p_chdir.o p_pwd.o p_exit.o p_chmod.o \
      p_lock.o p_uexec.o match.o dir_search.o exec.o error.o getcwd.o \
      translate.o truncate.o mapname.o unmapname.o \
      disksize.o dir_access.o vfile.o p_conread.o remotelog.o dosmesg.o \
      p_mapname.o p_ipc.o p_version.o p_nlstab.o lock.o semfunc.o

MESSAGES= \
	$(SRC)/messages  $(SRC)/../util/messages	\
	$(SRC)/../pkg_lockset/messages  $(SRC)/../pkg_rlock/messages


#
# Main target
#
rdpci: libs consvr dossvr dosout mapsvr pcidebug lmf

pci232: libs dossvr dosout pcidebug lmf

util:
	[ -d ../util ] || mkdir ../util
	cd ../util; \
	$(MAKE) -f $(SRC)/../util/util.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)"

libs: liblcs liblmf liblockset librlock 

liblcs:
	[ -d ../pkg_lcs ] || mkdir ../pkg_lcs
	cd ../pkg_lcs; \
	$(MAKE) -f $(SRC)/../pkg_lcs/lcs.mk \
		"DFLAGS=$(DFLAGS)"\
		"LIBFLAGS=$(LIBFLAGS)"

liblmf:
	[ -d ../pkg_lmf ] || mkdir ../pkg_lmf
	cd ../pkg_lmf; \
	$(MAKE) -f $(SRC)/../pkg_lmf/lmf.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)"

lmf:	$(MSG_TYPE)

En.lmf: liblmf $(MESSAGES)
	cat $(MESSAGES) > En.msg
	../pkg_lmf/lmfgen.bld En.msg En.lmf
	rm -f En.msg

aix.lmf: liblmf $(MESSAGES)
	cat $(MESSAGES) > En.msg
	sh $(SRC)/../install/aix/pci2aadu.sh En.msg
	../pkg_lmf/lmfgen.bld En.msg En.lmf
	cp En.lmf aix.lmf
	cp En.msg aix.msg
	rm -f En.msg

liblockset:
	[ -d ../pkg_lockset ] || mkdir ../pkg_lockset
	cd ../pkg_lockset; \
	$(MAKE) -f $(SRC)/../pkg_lockset/lockset.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)"

librlock:
	[ -d ../pkg_rlock ] || mkdir ../pkg_rlock
	cd ../pkg_rlock;  \
	$(MAKE) -f $(SRC)/../pkg_rlock/rlock.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBF=$(LIBFLAGS)"


loadpci: $(LOADPCI_O) $(COMMON_O)
	-rm -f loadpci
	$(CC) -o loadpci $(LDFLAGS) $(LOADPCI_O) $(COMMON_O) $(LDLIBS)

consvr: $(CONSVR_O) $(COMMON_O)
	-rm -f consvr
	$(CC) -o consvr $(LDFLAGS) $(CONSVR_O) $(COMMON_O) $(LDLIBS)

mapsvr: $(MAPSVR_O) $(COMMON_O)
	-rm -f mapsvr
	$(CC) -o mapsvr $(LDFLAGS) $(MAPSVR_O) $(COMMON_O) $(LDLIBS)

dossvr: $(DOSSVR_O) $(COMMON_O)
	-rm -f dossvr
	$(CC) -o dossvr $(LDFLAGS) $(DOSSVR_O) $(COMMON_O) $(LDLIBS)  

dosout: $(DOSOUT_O) $(COMMON_O)
	-rm -f dosout
	$(CC) -o dosout $(LDFLAGS) $(DOSOUT_O) $(COMMON_O) $(LDLIBS)

pcidebug: pcidebug.o 
	-rm -f pcidebug
	$(CC) -o pcidebug $(LDFLAGS) pcidebug.o $(LDLIBS)

lintit:
	$(LINT) $(LINTFLAGS) $(LOCDEFLIST) $(CFILES)
	[ -d ../util ] || mkdir ../util
	cd ../util; \
	$(MAKE) -f $(SRC)/../util/util.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)" \
		lintit
	[ -d ../pkg_lcs ] || mkdir ../pkg_lcs
	cd ../pkg_lcs; \
	$(MAKE) -f $(SRC)/../pkg_lcs/lcs.mk \
		"DFLAGS=$(DFLAGS)"\
		"LIBFLAGS=$(LIBFLAGS)" \
		lintit
	[ -d ../pkg_lmf ] || mkdir ../pkg_lmf
	cd ../pkg_lmf; \
	$(MAKE) -f $(SRC)/../pkg_lmf/lmf.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)" \
		lintit
	[ -d ../pkg_lockset ] || mkdir ../pkg_lockset
	cd ../pkg_lockset; \
	$(MAKE) -f $(SRC)/../pkg_lockset/lockset.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBFLAGS=$(LIBFLAGS)" \
		lintit
	[ -d ../pkg_rlock ] || mkdir ../pkg_rlock
	cd ../pkg_rlock;  \
	$(MAKE) -f $(SRC)/../pkg_rlock/rlock.mk \
		"DFLAGS=$(DFLAGS)" \
		"LIBF=$(LIBFLAGS)" \
		lintit

#
# Dependencies
#

common.o: $(SRC)/common.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/common.c

dir_access.o: $(SRC)/dir_access.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/dir_access.c

dir_search.o: $(SRC)/dir_search.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/dir_search.c

disksize.o: $(SRC)/disksize.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/disksize.c

dosmesg.o: $(SRC)/dosmesg.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/dosmesg.c

error.o: $(SRC)/error.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/error.c

exec.o: $(SRC)/exec.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/exec.c

getcwd.o: $(SRC)/getcwd.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/getcwd.c

ifc_list.o: $(SRC)/ifc_list.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/ifc_list.c

loadpci.o: $(SRC)/loadpci.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/loadpci.c

lock.o: $(SRC)/lock.c $(RL_HFILES) $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/lock.c

log.o: $(SRC)/log.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/log.c

lset.o: $(SRC)/lset.c $(RL_HFILES) $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/lset.c

mapname.o: $(SRC)/mapname.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/mapname.c

unmapname.o: $(SRC)/unmapname.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/unmapname.c

match.o: $(SRC)/match.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/match.c

mem.o: $(SRC)/mem.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/mem.c

net.o: $(SRC)/net.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/net.c

nls_init.o: $(SRC)/nls_init.c $(HFILES) $(LMF_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/nls_init.c

pcidebug.o: $(SRC)/pcidebug.c $(SRC)/log.h
	$(CC) -c -O $(CFLAGS) $(LOCDEFLIST) $(SRC)/pcidebug.c 

p_chdir.o: $(SRC)/p_chdir.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_chdir.c

p_chmod.o: $(SRC)/p_chmod.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_chmod.c

p_close.o: $(SRC)/p_close.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_close.c

p_connect.o: $(SRC)/p_connect.c $(HFILES) $(RL_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_connect.c

p_conread.o: $(SRC)/p_conread.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_conread.c

p_consvr.o: $(SRC)/p_consvr.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_consvr.c

p_create.o: $(SRC)/p_create.c $(HFILES) 
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_create.c

p_delete.o: $(SRC)/p_delete.c $(HFILES) $(LCS_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_delete.c

p_exit.o: $(SRC)/p_exit.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_exit.c

p_fstatus.o: $(SRC)/p_fstatus.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_fstatus.c

p_lock.o: $(SRC)/p_lock.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_lock.c

p_lseek.o: $(SRC)/p_lseek.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_lseek.c

p_mapname.o: $(SRC)/p_mapname.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_mapname.c

p_mapsvr.o: $(SRC)/p_mapsvr.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_mapsvr.c

p_mkdir.o: $(SRC)/p_mkdir.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_mkdir.c

p_nlstab.o: $(SRC)/p_nlstab.c $(HFILES) $(LCS_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_nlstab.c

p_open.o: $(SRC)/p_open.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_open.c

p_pwd.o: $(SRC)/p_pwd.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_pwd.c

p_read.o: $(SRC)/p_read.c $(HFILES) $(RL_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_read.c

p_rename.o: $(SRC)/p_rename.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_rename.c

p_rmdir.o:  $(SRC)/p_rmdir.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_rmdir.c

p_search.o: $(SRC)/p_search.c $(HFILES) $(LCS_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_search.c

p_server.o: $(SRC)/p_server.c $(HFILES) $(RL_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_server.c

p_termout.o: $(SRC)/p_termout.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_termout.c

p_version.o: $(SRC)/p_version.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_version.c

p_write.o: $(SRC)/p_write.c $(HFILES) $(RL_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_write.c

p_uexec.o: $(SRC)/p_uexec.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_uexec.c

pckframe.o: $(SRC)/pckframe.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/pckframe.c

remotelog.o: $(SRC)/remotelog.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/remotelog.c

rs232.o: $(SRC)/rs232.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/rs232.c

semfunc.o: $(SRC)/semfunc.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/semfunc.c

swap.o: $(SRC)/swap.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/swap.c

translate.o: $(SRC)/translate.c $(HFILES) $(LCS_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/translate.c

truncate.o: $(SRC)/truncate.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/truncate.c

ttymodes.o: $(SRC)/ttymodes.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/ttymodes.c

valid.o: $(SRC)/valid.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/valid.c

vfile.o: $(SRC)/vfile.c $(HFILES) $(RL_HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/vfile.c

logpacket.o: $(SRC)/logpacket.c $(HFILES)
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/logpacket.c

p_ipc.o: $(SRC)/p_ipc.c $(SRC)/pci_ipc.h
	$(CC) -c $(CFLAGS) $(LOCDEFLIST) $(SRC)/p_ipc.c
