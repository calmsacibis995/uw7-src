#ident	"@(#)ntp.mk	1.3"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
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

DIRS = lib authstuff in.xntpd ntpdate ntpq ntptrace xntpdc

all install clean clobber:
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk $@";\
			$(MAKE) -f $$i.mk $@ $(MAKEARGS);\
			cd ..;\
		done;\
		wait
