#		copyright	"%c%" 	

#ident	"@(#)sa:i386/cmd/sa/sa.mk	1.1.2.1"
#ident  "$Header$"

include $(CMDRULES)

OWN = bin
GRP = bin

# how to use this makefile
# to make sure all files  are up to date: $(MAKE) -f sa.mk all
#
# to force recompilation of all files: $(MAKE) -f sa.mk all FRC=FRC
#
# to test new executables before installing in 
# /usr/lib/sa: $(MAKE) -f sa.mk testbi
#
# The sadc module must be able to read /dev/kmem,
# which standardly has restricted read permission.
# They must have set-group-ID mode
# and have the same group as /dev/kmem.
# The chmod and chgrp commmands below ensure this.
#
LOCALDEF = -D_KMEMUSER -DHEADERS_INSTALLED
LDLIBS = -lgen -lmas
INSDIR = $(USRLIB)/sa
CRON   = $(VAR)/spool/cron
CRONDIR= $(CRON)/crontabs
CRONTAB= $(CRON)/crontabs/sys

ENTRY1= '0 * * * 0-6 $$TFADMIN /usr/lib/sa/sa1'
ENTRY2= '20,40 8-17 * * 1-5 $$TFADMIN /usr/lib/sa/sa1'
ENTRY3= '5 18 * * 1-5 $$TFADMIN /usr/lib/sa/sa2 -s 8:00 -e 18:01 -i 1200 -A'

MAINS = sadc sar/sar sa1 sa2 perf timex 

all: $(MAINS)

sadc:: sadc.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(LIBELF) $(SHLIBS)

sar/sar:: 
	@cd sar; $(MAKE) $(MAKEARGS)

sa2:: sa2.sh
	cp sa2.sh sa2

sa1:: sa1.sh
	cp sa1.sh sa1

perf:: perf.sh
	cp perf.sh perf

timex:: timex.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

sadc.o: sadc.c \
	$(INC)/sys/time.h \
	$(INC)/sys/ioctl.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/nlist.h \
	$(INC)/sys/metdisk.h \
	sa.h
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -c $<

timex.o: timex.c \
	$(INC)/sys/types.h \
	$(INC)/sys/times.h \
	$(INC)/sys/param.h \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/pwd.h

test: testai

testbi: #test for before installing
	sh runtest new $(ROOT)/usr/src/cmd/sa

testai: #test for after install
	sh runtest new

dirs:
	[ -d $(ETC)/init.d ] || mkdir -p $(ETC)/init.d
	[ -d $(ETC)/rc2.d ] || mkdir -p $(ETC)/rc2.d
	[ -d $(USRLIB) ] || mkdir -p $(USRLIB)
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(CRONDIR) ] || mkdir -p $(CRONDIR)

$(CRONTAB):
	[ -f $@ ] || > $@

install: all dirs $(CRONTAB)
	if [ -f $(CRONTAB) ];\
	then \
		chmod u+w $(CRONTAB) ;\
		if grep "sa1" $(CRONTAB) >/dev/null 2>&1 ; then :;\
		else \
			echo $(ENTRY1) >> $(CRONTAB);\
			echo $(ENTRY2) >> $(CRONTAB);\
		fi;\
		if grep "sa2" $(CRONTAB) >/dev/null 2>&1 ; then :;\
		else\
			echo $(ENTRY3) >> $(CRONTAB);\
		fi;\
	fi;
	-rm -f $(USRBIN)/sar
	-rm -f $(USRBIN)/sar
	-rm -f $(ETC)/rc2.d/S21perf
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) sa2
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) sa1
	$(INS) -f $(ETC)/init.d -m 0444 -u root -g sys perf
	$(INS) -f $(USRBIN)  -m 0555 -u $(OWN) -g sys timex
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) sar/sar
	$(INS) -o -f $(INSDIR) -m 02555 -u $(OWN) -g sys sadc
	ln $(ETC)/init.d/perf $(ETC)/rc2.d/S21perf
	$(SYMLINK) /usr/sbin/sar $(USRBIN)/sar

clean:
	-rm -f *.o
	-rm -f sar/*.o

clobber: clean
	-rm -f $(MAINS)

lintit:
	@cd sar; $(MAKE) $(MAKEARGS)
	$(LINT) $(LINTFLAGS) sadc.c
	$(LINT) $(LINTFLAGS) timex.c
