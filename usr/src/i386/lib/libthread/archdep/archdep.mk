#ident	"@(#)libthread:i386/lib/libthread/archdep/archdep.mk	1.1.10.1"

# 
# libthread: architecture-dependent routines
#

MAKEFILE = archdep.mk
LIBTHREADRULES = ../thread/../libthread.rules

PICDEF=pic.def
ARCH=../archinc/..
VERSDEF=
M4DEFS=symbols.def $(ARCH)/m4.def $(ARCH)/sys.def $(VERSDEF)
ASDEFS= -m -- $(M4DEFS) -DMCOUNT $(ARCH)/$(PICDEF)

CSRCS = lwppriv.c machdep.c context.c
COBJS = lwppriv.$O machdep.$O context.$O

SSRCS = machsubr.s cswtch.s
SOBJS = machsubr.$O cswtch.$O

OBJS = $(COBJS) $(SOBJS)

include $(LIBRULES)
include $(LIBTHREADRULES)

# Subcomponent-specific rules

.SUFFIXES: .c .O .P .T .ln .s .UW2 .UW2T

# P    is suffix for object compiled for shared object lib 
# O    is suffix for object compiled for archive lib
# T    is suffix for object compiled shared object lib and TRACE=-DTRACE
# UW2  is suffix for object compiled UW2.1.X for shared object lib (-KPIC)
# UW2T is suffix for object compiled UW2.1.X shared object lib and TRACE=-DTRACE

.s.P:   symbols.def
	$(MASK)\
	$(AS) -o $*.o $(ASDEFS) $*.s  && mv $*.o $*.P

.s.UW2:   symbols.def
	$(MASK)\
	$(AS) -o $*.o $(ASDEFS) $*.s -DGEMINI_ON_UW2 && mv $*.o $*.UW2

.s.O:   symbols.def
	$(MASK)\
	$(AS) -o $*.o $(ASDEFS) $*.s  && mv $*.o $*.O

.s.T:   symbols.def
	$(MASK)\
	$(AS) -o $*.o $(ASDEFS) $*.s  && mv $*.o $*.T

.s.UW2T:   symbols.def
	$(MASK)\
	$(AS) -o $*.o $(ASDEFS) $*.s -DGEMINI_ON_UW2 && mv $*.o $*.UW2T
	

symbols.def: symbols.c syms.awk
	$(MASK)\
	$(CC) $(ALLFLAGS) -KPIC -S symbols.c && \
	awk -f syms.awk <symbols.s | \
	sed -e '1,$$s;__SYMBOL__;;' >symbols.def && \
	rm -f symbols.s

all::	dotso dota dott dotsoUW2 dottUW2

dotso::	sobjs

dota::
	$(MAKE) -f $(MAKEFILE) sobjs MAKEFLAGS="$(MAKEFLAGS)" \
		INCDIR="$(INCDIR)" \
		LIBRULES="$(LIBRULES)" LIBTHREADRULES="$(LIBTHREADRULES)" O=O

dott::
	$(MAKE) -f $(MAKEFILE) sobjs MAKEFLAGS="$(MAKEFLAGS)" \
		INCDIR="$(INCDIR)" \
		LIBRULES="$(LIBRULES)" LIBTHREADRULES="$(LIBTHREADRULES)" O=T

dotsoUW2::
	$(MAKE) -f $(MAKEFILE) sobjs MAKEFLAGS="$(MAKEFLAGS)" \
		INCDIR="$(INCDIR)" \
		LIBRULES="$(LIBRULES)" LIBTHREADRULES="$(LIBTHREADRULES)" O=UW2

dottUW2::
	$(MAKE) -f $(MAKEFILE) sobjs MAKEFLAGS="$(MAKEFLAGS)" \
		INCDIR="$(INCDIR)" \
		LIBRULES="$(LIBRULES)" LIBTHREADRULES="$(LIBTHREADRULES)" O=UW2T

sobjs:	$(SOBJS)

# Dependencies

context.$O: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/synonyms.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/synch.h \
	$(COMMINC)/thread.h \
	$(INC)/errno.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/list.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/regset.h \
	$(INC)/sys/select.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysmacros_f.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/usync.h \
	$(INC)/time.h \
	$(INC)/ucontext.h

lwppriv.$O: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/synonyms.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/synch.h \
	$(COMMINC)/thread.h \
	$(INC)/errno.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/list.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/regset.h \
	$(INC)/sys/select.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysmacros_f.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/usync.h \
	$(INC)/time.h \
	$(INC)/ucontext.h

machdep.$O: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/synonyms.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/synch.h \
	$(COMMINC)/thread.h \
	$(INC)/errno.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/memory.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/list.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/regset.h \
	$(INC)/sys/select.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysmacros_f.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/usync.h \
	$(INC)/time.h \
	$(INC)/ucontext.h

symbols.$O: \
	$(ARCHINC)/archdep.h \
	$(ARCHINC)/libcsupp.h \
	$(ARCHINC)/synonyms.h \
	$(ARCHINC)/thrsig.h \
	$(ARCHINC)/tls.h \
	$(COMMINC)/debug.h \
	$(COMMINC)/libthread.h \
	$(COMMINC)/synch.h \
	$(COMMINC)/thread.h \
	$(INC)/errno.h \
	$(INC)/limits.h \
	$(INC)/lwpsynch.h \
	$(INC)/machlock.h \
	$(INC)/siginfo.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/fp.h \
	$(INC)/sys/list.h \
	$(INC)/sys/lwp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/param_p.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/reg.h \
	$(INC)/sys/regset.h \
	$(INC)/sys/select.h \
	$(INC)/sys/siginfo.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/sysmacros_f.h \
	$(INC)/sys/time.h \
	$(INC)/sys/types.h \
	$(INC)/sys/ucontext.h \
	$(INC)/sys/usync.h \
	$(INC)/time.h \
	$(INC)/ucontext.h

