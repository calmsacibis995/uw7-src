# @(#)in.tcpd.mk	1.3
include $(CMDRULES)

INSDIR=		$(USRSBIN)
CONFDIR=	$(ETC)/inet
OWN=		bin
GRP=		bin

LOCALDEF=	-DGETPEERNAME_BUG -DBROKEN_FGETS -DLIBC_CALLS_STRTOK \
		-DPROCESS_OPTIONS -DDAEMON_UMASK=022 \
		-DHOSTS_ACCESS -DPARANOID -DTLI \
		-DKILL_IP_OPTIONS -DRFC931_TIMEOUT=10 \
		-DALWAYS_HOSTNAME \
		-DFACILITY=LOG_DAEMON -DSEVERITY=LOG_INFO \
		-DREAL_DAEMON_DIR=\"/usr/sbin\" \
		-DHOSTS_DENY=\"/etc/inet/hosts.deny\" \
		-DHOSTS_ALLOW=\"/etc/inet/hosts.allow\"

LIBS=	-lsocket -lnsl

LIB_OBJ= hosts_access.o options.o shell_cmd.o rfc931.o eval.o \
	hosts_ctl.o refuse.o percent_x.o clean_exit.o \
	fix_options.o socket.o tli.o workarounds.o \
	update.o misc.o diag.o percent_m.o \
	setenv.o strcasecmp.o fromhost.o

LIB	= libwrap.a
TARGETS	= in.tcpd tcpdmatch try-from safe_finger tcpdchk

all: $(TARGETS)

install: all
	@for f in $(TARGETS) ; do \
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$f ; \
	done
	[ -d $(CONFDIR) ] || mkdir -p $(CONFDIR)
	$(INS) -f $(CONFDIR) -m 0660 -u $(OWN) -g $(GRP) hosts.allow 
	$(INS) -f $(CONFDIR) -m 0660 -u $(OWN) -g $(GRP) hosts.deny

$(LIB):	$(LIB_OBJ)
	rm -f $(LIB)
	$(AR) rcv $(LIB) $(LIB_OBJ)

in.tcpd: tcpd.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ tcpd.o $(LIB) $(LIBS)

safe_finger: safe_finger.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ safe_finger.o $(LIB) $(LIBS)

TCPDMATCH_OBJ = tcpdmatch.o fakelog.o inetcf.o scaffold.o

tcpdmatch: $(TCPDMATCH_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $(TCPDMATCH_OBJ) $(LIB) $(LIBS)

try-from: try-from.o fakelog.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ try-from.o fakelog.o $(LIB) $(LIBS)

TCPDCHK_OBJ = tcpdchk.o fakelog.o inetcf.o scaffold.o

tcpdchk: $(TCPDCHK_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $(TCPDCHK_OBJ) $(LIB) $(LIBS)

clean:
	rm -f *.[oa] core

clobber: clean
	rm -f in.tcpd safe_finger tcpdmatch tcpdchk try-from

# Enable all bells and whistles for linting.

lintit: 
	$(LINT) $(LINTFLAGS) $(LOCALDEF) *.c

# Internal compilation dependencies.

clean_exit.o: tcpd.h
diag.o: mystdarg.h tcpd.h
eval.o: tcpd.h
fakelog.o: mystdarg.h
fix_options.o: tcpd.h
fromhost.o: tcpd.h
hosts_access.o: tcpd.h
hosts_ctl.o: tcpd.h
inetcf.o: inetcf.h tcpd.h
misc.o: tcpd.h
options.o: tcpd.h
percent_m.o: mystdarg.h
percent_x.o: tcpd.h
refuse.o: tcpd.h
rfc931.o: tcpd.h
scaffold.o: scaffold.h tcpd.h
shell_cmd.o: tcpd.h
socket.o: tcpd.h
tcpd.o: patchlevel.h tcpd.h
tcpdchk.o: inetcf.h scaffold.h tcpd.h
tcpdmatch.o: scaffold.h tcpd.h
tli.o: tcpd.h
try-from.o: tcpd.h
update.o: mystdarg.h tcpd.h
workarounds.o: tcpd.h
