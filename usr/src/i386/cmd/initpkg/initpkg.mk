#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/initpkg.mk	1.19.26.1"
#ident "$Header$"

include $(CMDRULES)

INSDIR = $(SBIN)
OWN = root
GRP = sys

LDLIBS = -lcmd

SCRIPTS= bcheckrc brc vfstab rstab mountall \
	 rc0 rc1 rc2 rc3 dinit \
	 rmount shutdown umountall \
	 dumpsave configure netinfo ckroot aconf1_sinit ckmount
DIRECTORIES= init.d rc0.d rc1.d rc2.d rc3.d dinit.d
OTHERS= dumpcheck rc6
DIRS = \
	$(ETC) \
	$(ETC)/confnet.d \
	$(ETC)/init.d \
	$(ETC)/dfs \
	$(ETC)/rc0.d \
	$(ETC)/rc1.d \
	$(ETC)/rc2.d \
	$(ETC)/rc3.d \
	$(ETC)/dinit.d \
	$(ETC)/security

.MUTEX: install $(DIRECTORIES)

all:	scripts directories other

scripts: $(SCRIPTS)

directories: $(DIRECTORIES)

other: $(OTHERS)

clean:

clobber: clean
	rm -f `echo $(SCRIPTS) | sed -e "s!configure!!" -e "s!netinfo!!"`

install: $(DIRS) all

$(DIRS):
	[ -d $@ ] || mkdir $@

brc bcheckrc::
	-rm -f $(ETC)/$@
	-rm -f $(USRSBIN)/$@
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) $@
	-$(SYMLINK) /sbin/$@ $(ETC)/$@
	
ckmount::
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	
ckroot::
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	
aconf1_sinit::
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@

configure::
	-rm -f $(ETC)/confnet.d/configure
	$(INS) -f $(ETC)/confnet.d -m 0755 -u $(OWN) -g $(GRP) configure

dumpsave::
	sh $@.sh $(ROOT)
	$(INS) -f $(INSDIR) -m 0744 -u root -g sys $@

dumpcheck:	dumpcheck.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin dumpcheck
	$(INS) -f $(ETC)/default -m 0644 -u bin -g bin dump.dfl
	-mv $(ETC)/default/dump.dfl $(ETC)/default/dump

vfstab::
	sh $@.sh  $(ROOT)
	$(INS) -f $(ETC) -m 0744 -u $(OWN) -g $(GRP) $@

rstab::
	cp rstab.sh  dfstab
	$(INS) -f $(ETC)/dfs -m 0644 -u $(OWN) -g $(GRP) dfstab

mountall::
	-rm -f $(ETC)/mountall
	-rm -f $(USRSBIN)/mountall
	cp mountall.sh  ./mountall
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) mountall
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) mountall
	-$(SYMLINK) /sbin/mountall $(ETC)/mountall

netinfo::
	-rm -f $(USRSBIN)/netinfo
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) netinfo

rc0 rc1 rc2 rc3 dinit::
	-rm -f $(ETC)/$@
	-rm -f $(USRSBIN)/$@
	cp $@.sh $@
	$(INS) -f $(INSDIR) -m 0744 -u $(OWN) -g $(GRP) $@
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) $@
	-$(SYMLINK) /sbin/$@ $(ETC)/$@

rc6:	rc0
	-$(SYMLINK) /sbin/rc0 /sbin/rc6
	-$(SYMLINK) /sbin/rc0 $(ETC)/rc6

rmount::
	-rm -f $(ETC)/rmount
	cp rmount.sh $@
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rmount
	-$(SYMLINK) /usr/sbin/rmount $(ETC)/rmount

shutdown::
	-rm -f $(ETC)/shutdown
	-rm -f $(USRSBIN)/shutdown
	cp shutdown.sh shutdown
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) shutdown
	$(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) shutdown
	-$(SYMLINK) /sbin/shutdown $(ETC)/shutdown
	$(INS) -f $(ETC)/default -m 0644 -u bin -g bin shutdown.dfl
	-mv $(ETC)/default/shutdown.dfl $(ETC)/default/shutdown
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 uxshutdown.str

umountall::
	-rm -f $(ETC)/umountall
	-rm -f $(USRSBIN)/umountall
	cp umountall.sh  umountall
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) umountall
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) umountall
	-$(SYMLINK) /sbin/umountall $(ETC)/umountall

init.d rc0.d rc1.d rc2.d rc3.d dinit.d::
	cd ./$@; \
	ROOT=$(ROOT) MACH=$(MACH) CH=$(CH) INS=$(INS) sh :mk.$@.sh 
