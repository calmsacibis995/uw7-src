#ident	"@(#)nfs.mk	1.2"
#ident	"$Header$"

#
# makefile for nfs.cmds
#
# These are the nfs specific subcommands for the generic distributed file
# system administration commands, along with many other nfs-specific
# administrative commands
#

include $(CMDRULES)

OWN = bin
GRP = bin
MSGDIR = $(USRLIB)/locale/C/MSGFILES

COMMANDS=automount biod bootpd dfmounts dfshares exportfs mount mountd nfsd share showmount umount unshare statd lockd nfsstat pcnfsd nfsping

install:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo "====> $(MAKE) -f $$i.mk $@" ;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;
	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 0644 -u $(OWN) -g $(GRP) nfscmds.str

.DEFAULT:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo "====> $(MAKE) -f $$i.mk $@" ;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@;\
		cd ..;\
	done;
