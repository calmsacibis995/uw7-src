#ident	"@(#)netcfg.mk	1.4"
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

DIRS=		bin tcp slip

all install:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@ $(MAKEARGS);\
		)\
	done;\
	wait

clean lintit:
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait

clobber: clean
	@for i in $(DIRS);\
	do\
		( cd $$i;\
		/bin/echo "\n===== $(MAKE) $(MAKEFLAGS) -f `basename $$i.mk` $@";\
		$(MAKE) -$(MAKEFLAGS) -f `basename $$i.mk` $@;\
		)\
	done;\
	wait
