#ident  "@(#)Makefile	1.3"
#ident  "$Header$"

#******************************************************************************
#	Makefile
#------------------------------------------------------------------------------
# Comments:
# Relax utility for Gemini
#
#------------------------------------------------------------------------------
#       @(#)Makefile	7.4 97/08/30
# 
#       Copyright (C) The Santa Cruz Operation, 1996.
#       This Module contains Proprietary Information of
#       The Santa Cruz Operation, and should be treated as Confidential.
#------------------------------------------------------------------------------
#  Revision History:
#
#	Thu Dec 19 12:38:09 PST 1996	louisi
#		Created file.
#   Tue Sep  9 11:39:17 BST 1997    andrewma
#       Moved from scoadmin tree to new location. Makefile modified.
#
#================================================================================

include $(CMDRULES)

MSGHDRS=relax.msgd.h relax.msg.h
CCPLUS=i386CC

all: relax

relax: relax.C $(MSGHDRS)
	rm -f relax
	$(CCPLUS) -I $(TOOLS)/usr/include/scoadmin $(CCPLUSINCL) -L $(USRBIN) $(CCOPTIONS) -o relax relax.C -lsysadm

$(MSGHDRS): NLS/en/relax.msg
	$(MKCATDECL) -i ./moduleIds -c -m SCO_RELAX NLS/en/relax.msg

install: all 
	-[ -d $(USRSBIN) ] || mkdir -p $(USRSBIN)
	-[ -d $(ETC)/security/seclevel/high ] || mkdir -p $(ETC)/security/seclevel/high
	-[ -d $(ETC)/security/seclevel/improved ] || mkdir -p $(ETC)/security/seclevel/improved 
	-[ -d $(ETC)/security/seclevel/traditional ] || mkdir -p $(ETC)/security/seclevel/traditional 
	-[ -d $(ETC)/security/seclevel/low ] || mkdir -p $(ETC)/security/seclevel/low 
	$(INS) -m 755 -g bin -u bin -f $(USRSBIN) relax
	cp -f high.etc_def etc_def
	cp -f high.script script
	$(INS) -m 660 -g bin -u bin -f $(ETC)/security/seclevel/high etc_def
	$(INS) -m 750 -g bin -u bin -f $(ETC)/security/seclevel/high script
	cp -f improved.etc_def etc_def
	cp -f improved.script script
	$(INS) -m 660 -g bin -u bin -f $(ETC)/security/seclevel/improved etc_def
	$(INS) -m 750 -g bin -u bin -f $(ETC)/security/seclevel/improved script
	cp -f traditional.etc_def etc_def
	cp -f traditional.script script
	$(INS) -m 660 -g bin -u bin -f $(ETC)/security/seclevel/traditional etc_def
	$(INS) -m 750 -g bin -u bin -f $(ETC)/security/seclevel/traditional script
	cp -f low.etc_def etc_def
	cp -f low.script script
	$(INS) -m 660 -g bin -u bin -f $(ETC)/security/seclevel/low etc_def
	$(INS) -m 750 -g bin -u bin -f $(ETC)/security/seclevel/low script
	rm -f etc_def script

clean:
	rm -f relax 
	rm -f *.o *.msg.h *.msgd.h
