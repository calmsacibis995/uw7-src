#ident	"@(#)lprof:lprofinc.mk	1.8"

PROF_SAVE	=
SRCBASE		= common


INS		= $(SGSBASE)/sgs.install
INSDIR		= $(CCSBIN)
HFILES		= 
SOURCES		=
OBJECTS		=
PRODUCTS	=

PLBBASE		= $(SGSBASE)/lprof/libprof
LIBPROF		= $(SGSBASE)/lprof/libprof/$(CPU)/libprof.a
LIBSYMINT	= $(SGSBASE)/lprof/libprof/$(CPU)/libsymint.a
LDFLAGS		= -s
