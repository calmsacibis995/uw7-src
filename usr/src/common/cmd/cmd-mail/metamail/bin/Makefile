#
#	ident @(#) Makefile 11.1 97/10/30 
#
#############################################################################
#
#	Copyright (c) 1997 The Santa Cruz Operation, Inc.. 
#		All Rights Reserved. 
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
#		THE SANTA CRUZ OPERATION INC.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.
#
#############################################################################
#
#
# Modification History
#
# M000, 08-Sep-97, andrean
#	- Created.
#
#

include $(CMDRULES)

KSH =			/bin/ksh
TCL =			$(TOOLS)/usr/bin/tcl
MKCATDEFS =		$(TOOLS)/usr/bin/mkcatdefs
MKCATDECL =		$(TCL) $(TOOLS)/usr/bin/mkcatdecl
DONLS =			TCL=$(TCL) \
			INS=$(INS) \
			MKCATDECL="$(MKCATDECL)" \
			MKCATDEFS=$(MKCATDEFS) \
			USRLIB=$(USRLIB) ETC=$(ETC) \
			$(KSH) ../../bin/donls

PROGS =			extcompose \
			getfilename \
			metasend \
			showaudio \
			showexternal \
			shownonascii \
			showpartial \
			showpicture

build:			$(PROGS) nls

.sh:			metamail_msg.sh
			echo "#!/bin/sh" > $*
			cat metamail_msg.sh >> $*
			cat $*.sh >> $*

metamail_msg.sh:	NLS/en/metamail.gen
			$(MKCATDEFS) -a mail -s metamail NLS/en/metamail.gen > /dev/null

nls:
			$(DONLS) -d NLS build

clean:
			rm -f $(PROGS) metamail_msg.sh
			$(DONLS) -d NLS clean

clobber:		clean
