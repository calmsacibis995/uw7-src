#ident	"@(#)Makefile	11.1"

SHELL=/bin/sh

SUBDIRS		= tests/tcl libdb admin mhome c-client imap libmail mailcheck mailx metamail pine popper slocal sendmail user vacation

#
# Default targets.
#
all:		build

#
# Run make in all our subdirectories.
#
build clean clobber install local-install local-remove:
	@for dir in $(SUBDIRS); \
	do \
	    if [ ! -d $$dir ]; \
	    then \
		continue; \
	    fi; \
	    ( \
		echo "------- '$(MAKE) -$(MAKEFLAGS) $@' in $${dir}"; \
		cd $$dir; \
		$(MAKE) -$(MAKEFLAGS) $@ \
	    ); \
	done
