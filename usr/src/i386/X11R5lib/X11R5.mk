#ident	"@(#)X11R5libs:X11R5.mk	1.5"

# This makefile just copies the provided binaries into the $ROOT/$MACH tree,
#	so the acp package can pick them up.  These are for compatibility
#	with UnixWare 2.X.  If any of these are updated in 2.X, the
#	binary must be delta'd again.

include $(LIBRULES)

DESTDIR=$(ROOT)/$(MACH)/usr/lib/X11R5lib

# Note that the following files have already been 'stripped/mcs -d'.
# All libs in MAIN are from the 2.1 release, except for the
#	libOlit.so.1 (2.1.1) and dtruntime.?? (2.1.2) files

MAINS = dtruntime.aa \
	dtruntime.ab \
	dtruntime.ac \
	dtruntime.ad \
	libDt.so.1 \
	libDtI.so.1 \
	libGizmo.so.1 \
	libMrm.so.1.2 \
	libOlit.so.1 \
	libOlitM.so.1 \
	libOlitO.so.1 \
	libX11.so.1 \
	libXaw.so \
	libXimp.so.5.0 \
	libXm.so.5.0 \
	libXol.so.1 \
	libXsi.so.5.0

install: $(MAINS)
	[ -d $(DESTDIR) ] || mkdir -p $(DESTDIR)
	for x in $(MAINS); do \
		$(INS) -f $(DESTDIR) $$x; \
		chmod 555 $(DESTDIR)/$$x; \
	done
	cat dtruntime.a? > dtruntime.so.1
	$(INS) -f $(DESTDIR) dtruntime.so.1
	chmod 555 $(DESTDIR)/dtruntime.so.1
