#ident	"@(#)tcp.mk	1.4"
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

INSDIR	=	$(ROOT)/$(MACH)/usr/lib/netcfg
FILES	=	control info init list reconf remove

all install:
	@for i in $(FILES);\
	do\
		if [ ! -d $(INSDIR)/$$i ];\
		then\
			mkdir -p $(INSDIR)/$$i;\
		fi;\
		cp $$i tcp;\
		chmod 755 tcp;\
		$(INS) -f $(INSDIR)/$$i tcp;\
	done;

clean lintit:
	rm -f tcp

clobber: clean
