#	copyright	"%c%"

#ident	"@(#)dfs.cmds.mk	1.2"
#ident "$Header$"
#
# makefile for dfs.cmds
#
# These are the generic distributed file system administration commands
#

include $(CMDRULES)

OWN = bin
GRP = bin
MSGDIR = $(USRLIB)/locale/C/MSGFILES

#COMMANDS=general dfshares share shareall unshareall lidload
COMMANDS=general dfshares share shareall unshareall

GENERAL=unshare
FRC =

all:
	@for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make -f $$i.mk $(MAKEARGS);\
		cd ..;\
	done;

install:
	for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make install -f $$i.mk $(MAKEARGS);\
		cd ..;\
	done;
	for i in $(GENERAL);\
		do \
			rm -f $(USRSBIN)/$$i;\
			ln $(USRSBIN)/general $(USRSBIN)/$$i;\
		done
	rm -f $(USRSBIN)/dfmounts
	ln $(USRSBIN)/dfshares $(USRSBIN)/dfmounts

	[ -d $(MSGDIR) ] || mkdir -p $(MSGDIR)
	$(INS) -f $(MSGDIR) -m 0644 -u $(OWN) -g $(GRP) dfscmds.str

clean:
	for i in $(COMMANDS);\
	do\
		cd $$i;\
		echo $$i;\
		make -f $$i.mk clean $(MAKEARGS);\
		cd .. ;\
	done

clobber: clean
	for i in $(COMMANDS);\
		do cd $$i;\
		echo $$i;\
		make -f $$i.mk clobber $(MAKEARGS);\
		cd .. ;\
	done

FRC:
