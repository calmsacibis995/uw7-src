#ident  "@(#)Makefile	1.3"
#ident  "$Header$"

#******************************************************************************
#	Makefile
#------------------------------------------------------------------------------
# Comments:
# secdefs utility for Gemini
#
#------------------------------------------------------------------------------
#       @(#)Makefile	7.1 97/08/30
# 
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#------------------------------------------------------------------------------
#  Revision History:
#
#	Thu Dec 19 12:38:09 PST 1996	louisi
#		Created file.
#	Tue Sep  9 11:39:17 BST 1997	andrewma
#		Moved from scoadmin tree to new location. Makefile modified.
#
#================================================================================

include $(CMDRULES)

MSGHDRS=secdefs.msgd.h secdefs.msg.h

CCPLUS=i386CC

all: secdefs


secdefs: secdefs.C $(MSGHDRS)
	rm -f secdefs
	$(CCPLUS) -I $(TOOLS)/usr/include/scoadmin $(CCPLUSINCL) -L $(USRBIN) $(CCOPTIONS) -o secdefs secdefs.C -lcmd -lsysadm

$(MSGHDRS): NLS/en/secdefs.msg
	$(MKCATDECL) -i ./moduleIds -c -m SCO_SECDEFS NLS/en/secdefs.msg

install: all 
	-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	$(INS) -m 755 -g bin -u bin -f $(USRSBIN) secdefs

clean:
	rm -f secdefs 
	rm -f *.o *.msg.h *.msgd.h
