#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufs.mk	1.3.5.4"
#ident "$Header$"
#  /usr/src/cmd/lib/fs/ufs is the directory of all ufs specific commands
#  whose executable reside in $(INSDIR1) and $(INSDIR2).

include $(CMDRULES)

#
#  This is to build all the ufs commands
#
.DEFAULT:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
		cd .. ; \
	    fi;\
	done
