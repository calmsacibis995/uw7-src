#ident	"@(#)bin.mk	1.3"
#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#
include $(CMDRULES)

INSDIR	=	$(ROOT)/$(MACH)/usr/lib/netcfg/bin
FILES	=	tcp.BE addsl slconf listsl yesno slip_type slip.BE \
		addslipuser

all install:
	@if [ ! -d $(INSDIR) ];\
	then\
		mkdir -p $(INSDIR);\
	fi;\
	for i in $(FILES);\
	do\
		$(INS) -f $(INSDIR) ./$$i;\
	done;

clean lintit:

clobber: clean
