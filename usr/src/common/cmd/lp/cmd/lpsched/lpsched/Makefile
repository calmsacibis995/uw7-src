#	copyright	"%c%"


#ident	"@(#)Makefile	1.3"
#ident  "$Header$"
#
# Makefile for the lpsched command
#


include $(CMDRULES)

TOP	=	../../..

include ../../../common.mk

#
# Possible uses of TRACE_MALLOC and MALLOC_INC:
#
#	make TRACE_MALLOC=-DTRACE_MALLOC
#	make MALLOC_INC=-I/usr/foo/include
#	make TRACE_MALLOC=-DMDL MALLOC_INC=-I/usr/foo/include
#
# Define MALLOC_3X if you are using malloc(3X). Link it in
# by defining LDMALLOC=-lmalloc.
#
# Define LOST_LOCK if the mysterious problem of losing the lock
# on the SCHEDLOCK file occurs. It's been seen to happen when
# the system is under heavy load.
#
# Define CHECK_CHILDREN to cause the scheduler to look for children
# which have terminated without sending S_CHILD_DONE.  This may
# occur if the intermediate child core dumps or is killed by someone.
# Update: Starting with SVR4.0 we now use a better method of learning
# about finished child processes, so this option should no longer be
# needed.
#

#CHECK_CHILDREN	= -DCHECK_CHILDREN
#LOST_LOCK	= -DLOST_LOCK
#MALLOC_3X	= -DMALLOC_3X -DDEF_MXFAST=32 -DDEF_NLBLKS=32 -DDEF_GRAIN=4
#LDMALLOC	= -lmalloc
#TRACE_MALLOC	= -DTRACE_MALLOC
#LDDEBUG	= malloc/mymalloc.c
#NETWORKING	= -DNETWORKING

DEBUG	=

#
#  Doing -DDEBUG allows some nice log files to be generated
#  with the -d option, so don't let the top level makefile
#  override this feature.
#MYDEBUG	=	$(DEBUG) -DDEBUG $(TRACE_MALLOC) $(MALLOC_INC)

##### NEW
#  ES Note:
#  Unfortunately -DDEBUG compiles in some options that are
#  undocumented and therefore disallowed from general
#  availability.
MYDEBUG	=	$(DEBUG) $(TRACE_MALLOC) $(MALLOC_INC)

LOCALINC= -I. -I$(LPINC)
LOCALDEF= $(MYDEBUG) $(NETWORKING) $(CHECK_CHILDREN) $(LOST_LOCK) $(MALLOC_3X)

HDRS	= \
		nodes.h \
		dispatch.h \
		validate.h \
		lpsched.h

SRCS	= \
		alerts.c \
		cancel.c \
		checkchild.c \
		daisyforms.c \
		disena.c \
		disp1.c \
		disp2.c \
		disp3.c \
		disp4.c \
		disp5.c \
		disptab.c \
		dowait.c \
		exec.c \
		faults.c \
		files.c \
		flt.c \
		fncs.c \
		getkey.c \
		getpwent.c \
		getstatus.c \
		init.c \
		log.c \
		lpfsck.c \
		lpsched.c \
		msgs.c \
		notify.c \
		pickfilter.c \
		ports.c \
		putjob.c \
		rexec.c \
		requeue.c \
		rmtstatus.c \
		rstatus.c \
		security.c \
		schedule.c \
		status.c \
		terminate.c \
		validate.c 


OBJS	=	$(SRCS:.c=.o)

#LINTLB  =	../../llib-llpcmd.ln ../../../lib/llib-llplib.ln
LINTLB  =	../../../lib/llib-llplib.ln

LIBS	= \
		$(LIBMSG) \
		$(LIBFRM) \
		$(LIBREQ) \
		$(LIBPRT) \
		$(LIBCLS) \
		$(LIBACC) \
		$(LIBFLT) \
		$(LIBUSR) \
		$(LIBLP) \
		$(LIBOAM) \
		$(LIBSEC) \
		$(LIBSYS)

LDLIBS	=	$(LIBS) -lcurses -lgen

CMDS	=	lpsched lpsched.net

.MUTEX: lpsched lpsched.net

all:		$(CMDS)

install:
	$(INS) -m $(SMODES) -u $(SUPER) -g $(GROUP) -f $(USRLIBLP) lpsched
	$(INS) -m $(SMODES) -u $(SUPER) -g $(GROUP) -f $(USRLIBLP) lpsched.net
	rm -f $(USRLIB)/lpsched
	$(SYMLINK) /usr/lib/lp/lpsched $(USRLIB)/lpsched

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMDS)

strip:
	$(STRIP) $(CMDS)

lpsched:
	$(MAKE) clean
	$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" _lpsched

_lpsched:	$(OBJS) $(LIBS)
	$(CC) -o lpsched $(OBJS) $(LDFLAGS) $(LDLIBS) \
		$(LDMALLOC) $(LDDEBUG) $(SHLIBS)

lpsched.net:
	$(MAKE) clean
	$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" NETWORKING="-DNETWORKING" _lpsched.net

_lpsched.net:	$(OBJS) $(LIBS)
	$(CC) -o lpsched.net $(OBJS) $(LDFLAGS) $(LDLIBS) \
		$(LDMALLOC) $(LDDEBUG) -lnsl -liaf $(SHLIBS)

$(OBJS):	$(HDRS)

lintit:
	$(LINT) $(DEFLIST) $(SRCS) $(LINTLB)

lintlib:
	$(LINT) -abhmuvxy -o lpsch $(DEFLIST) $(SRCS)

MODULE_OBJS	=	$(MODULE_SRC:.c=.o)

module:		$(MODULE_SRC:.c=.o) $(LIBS)
	$(CC) -o $(MODULE) $(MODULE_SRC:.c=.o) $(LDFLAGS) $(LDLIBS) \
		$(LDMALLOC) $(LDDEBUG) $(SHLIBS)
