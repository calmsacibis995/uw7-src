#ident	"@(#)kern-i386:fs/xxfs/xxfs.mk	1.9.1.1"
#ident "$Header$"

include $(UTSRULES)

MAKEFILE=	xxfs.mk
KBASE	= ../..
LINTDIR	= $(KBASE)/lintdir
DIR	= fs/xxfs

XXFS	= xx.cf/Driver.o
LFILE	= $(LINTDIR)/xxfs.ln

LOCALDEF=-D_FSKI=1

MODULES	= $(XXFS)

FILES = xxalloc.o \
	xxbmap.o \
	xxdata.o \
	xxdir.o \
	xxinode.o \
	xxrdwri.o \
	xxsearch.o \
	xxvfsops.o \
	xxvnops.o

LFILES = xxalloc.ln  \
	xxbmap.ln  \
	xxdir.ln  \
	xxdata.ln  \
	xxinode.ln  \
	xxrdwri.ln  \
	xxvfsops.ln  \
	xxvnops.ln

CFILES = xxalloc.c  \
	xxbmap.c  \
	xxdir.c  \
	xxdata.c  \
	xxinode.c  \
	xxrdwri.c  \
	xxvfsops.c  \
	xxvnops.c

SFILES  = \
	xxsearch.s

SRCFILES = $(CFILES) $(SFILES)

all:	$(MODULES)

install: all
	(cd xx.cf; $(IDINSTALL) -R$(CONF) -M XENIX)

$(XXFS): $(FILES)
	$(LD) -r -o $(XXFS) $(FILES)

clean:
	-rm -f $(FILES) $(LFILES) *.L $(XXFS)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e XENIX

$(LINTDIR):

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);  \
	do \
		echo $$i; \
	done

sysfsHeaders = \
	xxfblk.h \
	xxfilsys.h \
	xxparam.h \
	inode.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/fs/xxfs/xxfblk.h
	$(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/fs/xxfs/xxfilsys.h
	$(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/fs/xxfs/xxparam.h
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/fs/xxfs/inode.h

include $(UTSDEPEND)

include $(MAKEFILE).dep

