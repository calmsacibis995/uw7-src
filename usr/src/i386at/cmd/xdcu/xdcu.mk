#ident	"@(#)dcu:xdcu.mk	1.2.1.18"

include $(CMDRULES)

INSDIR = $(ETC)/dcu.d
DCUINSDIR = $(SBIN)
OWN = root
GRP = sys

COMPILE = $(USRLIB)/winxksh/compile

all: dcusilent dculib.so terminfo menus winxksh help
	@echo "DCU build complete."

dcusilent:	dcusilent.o
	$(CC) -o dcusilent dcusilent.o $(LDFLAGS) $(LDLIBS) -dn

dculib.so:
	[ -d dculib ] || mkdir -p dculib
	cp $(TOOLS)/usr/lib/libresmgr.so dculib/dculib.so

menus: $(COMPILE) config

config: FRC
	cd locale/$(LOCALE); \
	$(COMPILE) ./config.sh; \
	cd ../.. 

help: FRC
	cd locale/$(LOCALE)/help; \
	cp $(ROOT)/usr/src/$(WORK)/sysinst/desktop/menus/helpwin helpwin; \
	$(MAKE) -f help.mk ;\
	cd ../../..

terminfo: FRC
	[ -d terminfo/a ] || mkdir -p terminfo/a
	[ -d terminfo/A ] || mkdir -p terminfo/A
	[ -d terminfo/x ] || mkdir -p terminfo/x
	cp $(ROOT)/$(MACH)/usr/share/lib/terminfo/A/AT386* terminfo/A
	cp $(ROOT)/$(MACH)/usr/share/lib/terminfo/a/ansi terminfo/a
	cp $(ROOT)/$(MACH)/usr/share/lib/terminfo/x/xterm terminfo/x

winxksh: FRC
	cp $(ROOT)/usr/src/$(WORK)/cmd/winxksh/xksh/winxksh.static winxksh
	cp $(ROOT)/usr/src/$(WORK)/cmd/winxksh/libwin/scr_init scripts/scr_init
	cp $(ROOT)/usr/src/$(WORK)/cmd/winxksh/libwin/winrc scripts/winrc
	cp $(ROOT)/usr/src/$(WORK)/sysinst/desktop/scripts/funcrc scripts/funcrc
	cp $(ROOT)/usr/src/$(WORK)/sysinst/desktop/menus/choose menus/choose 

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	cpio -puvdL $(INSDIR) <dcu.list
	$(CH)chmod +x $(INSDIR)/scripts/dcu
	$(INS) -f $(DCUINSDIR) -m 0555 -u $(OWN) -g $(GRP) dcu 
	$(INS) -f $(DCUINSDIR) -m 0555 -u $(OWN) -g $(GRP) dcusilent

clean:
	rm -f winxksh
	rm -f dculib/*.o dculib/dculib.so
	rm -rf terminfo
	rm -rf dcusilent.o

clobber:  clean

FRC:
