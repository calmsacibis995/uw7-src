#ident	"@(#)OSRcmds:OSR.mk	1.4"
#

include $(CMDRULES)
include make.inc

# Make sure lib is first, since other commands usr it
DIRS = lib cmos compress cpio csh dd diskcp dspmsg dtox grep i486 ksh more sh sort spline tar test

all .DEFAULT:
	echo "CMDRULES = $(CMDRULES)"; \
	if [ ! -d "$(OSRDIR)" ]; then \
		mkdir -p $(OSRDIR); \
		chmod 755 $(OSRDIR); \
	fi
	if [ ! -d "$(ROOT)/$(MACH)/etc/default" ]; then \
		mkdir -p $(ROOT)/$(MACH)/etc/default; \
		chmod 755 $(ROOT)/$(MACH)/etc/default; \
	fi
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		if [ "`ls -l *.mk 2>/dev/null`" ]; \
		then \
			echo "Executing makefile in directory $$d ..."; \
			$(MAKE) -f *.mk $@ install;\
		fi; \
		cd .. ;\
	done;

install: all

clobber:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		echo "Running clobber in directory $$d ..."; \
		$(MAKE) -f *.mk clobber;\
		cd ..;\
	done;
