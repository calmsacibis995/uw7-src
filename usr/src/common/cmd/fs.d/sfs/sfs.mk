#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/sfs.mk	1.1.1.4"
#ident "$Header$"
#  /usr/src/cmd/lib/fs/sfs is the directory of all sfs specific commands
#  whose executable reside in $(INSDIR1) and $(INSDIR2).

include $(CMDRULES)

#
#  This is to build all the sfs commands
#
.DEFAULT:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@ ; \
		cd .. ; \
	    fi;\
	done
