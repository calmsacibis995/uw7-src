#ident	"@(#)libedit.mk	1.2"

#
# This makefile builds libedit
#

include $(CMDRULES)

LOCALDEF =
LOCALINC=-I$(ROOT)/$(MACH)/usr/include -I../include

LIBDIR =	$(USRLIB)/ppp
LIBDIRBASE =	/usr/lib/ppp

EDIT_CFILES = edit.c emacs.c history.c lib.c vi.c
EDIT_FILES =  edit.o emacs.o history.o lib.o vi.o
EDIT = ../libedit.a

SRCFILES = $(EDIT_CFILES)
FILES = $(EDIT_FILES)
TARGS = $(EDIT)

all:	$(TARGS)

clean:
	-rm -f $(FILES)

clobber: clean
	-rm -f $(TARGS)

config:

headinstall:

install:	all

fnames:
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

lintit:

$(EDIT):	$(EDIT_FILES)
		$(AR) -ru $(EDIT) $(EDIT_FILES)

#
# Header dependencies
#
