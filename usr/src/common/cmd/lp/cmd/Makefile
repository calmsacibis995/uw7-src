#	copyright	"%c%"


#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for the LP commands
#

include $(CMDRULES)


TOP	=	..

include ../common.mk


DEBUG	=

LOCALINC= -I$(LPINC)
LDLIBS = -lgen


#####
#
# $(SRCS) is just for stuff built in the current directory.
#####

SRCS	= \
		cancel.c \
		comb.c \
		lp.c \
		lpsystem.c \
		lpfilter.c \
		lpforms.c \
		lpmove.c \
		lpshut.c \
		lpusers.c \
		dumpolp.c

OBJS	=	$(SRCS:.c=.o)

LINTLB  =	../lib/llib-llplib.ln

#####
#
# $(LILCMDS) are those built in the current directory,
# while $(BIGCMDS) are those built in a subdirectory.
# Note: If a subdirectory has further subdivisions, just
# list the first subdirectory here as a ``command'', such
# as "lpsched" (which is actually lpsched, lpexec, and lpNet).
#####

LILCMDS	= \
		accept \
		cancel \
		disable \
		enable \
		lp \
		lpsystem \
		lpfilter \
		lpforms \
		lpmove \
		lpshut \
		lpusers \
		reject \
		dumpolp

BIGCMDS	= \
		lpadmin \
		lpsched \
		lpstat \
		lpdata

CMDS	=	$(BIGCMDS) $(LILCMDS)


#####
#
# $(USRCMDS1) are regular user commands, $(USERCMDS2) are LP
# commands that might be user commands (e.g. disable and enable).
# $(SYSCMDS) are strictly system commands. All of these are $(LILCMDS),
# i.e. commands built in the current directory.
#####

USRCMDS1= \
		cancel \
		lp

USRCMDS2= \
		disable \
		enable

SYSCMDS	= \
		accept \
		lpsystem \
		lpfilter \
		lpforms \
		lpmove \
		lpshut \
		lpusers \
		reject \
		dumpolp


all:		$(CMDS)


#####
#
# Doing "make install" at this level will ensure that the $(LILCMDS)
# are already built, but will simply pass on the make to the
# subdirectories for $(BIGCMDS). In other words, we don't set a
# dependency on $(BIGCMDS) at this level, to avoid redundant work--the
# lower level makefiles take care of things.
#####

install:	localinstall
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) SYMLINK="$(SYMLINK)" $(MAKEARGS) install; \
		cd ..; \
	done

localinstall:	$(LILCMDS)
	for cmd in $(USRCMDS1); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRBIN) $$cmd; \
	done
	for cmd in $(USRCMDS2); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRBIN) $$cmd; \
	done
	for cmd in $(SYSCMDS); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRSBIN) $$cmd; \
		rm -f $(USRLIB)/$$cmd; \
		$(SYMLINK) /usr/sbin/$$cmd $(USRLIB)/$$cmd; \
	done


clean:		localclean
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clean; \
		cd ..; \
	done

localclean:
	$(RM) $(OBJS)

clobber:	localclean
	$(RM) $(LILCMDS)
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) clobber; \
		cd ..; \
	done

strip:
	$(STRIP) $(LILCMDS)
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) $(MAKEARGS) strip; \
		cd ..; \
	done


#####
#
# Need the "nulldependency" because the targets in $(BIGCMDS)
# are directories. Without "nulldependency" make would always
# say the target is up to date.
#####

$(BIGCMDS):	nulldependency
	cd ./$@; $(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"

nulldependency:


accept:		comb.o \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o accept comb.o $(LDFLAGS) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

disable:	accept
	$(CP) accept disable

enable:		disable
	-$(RM) -f enable
	$(LN) disable enable

reject:		accept
	-$(RM) -f reject
	$(LN) accept reject


cancel:		cancel.o \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o cancel cancel.o $(LDFLAGS) \
			$(LIBREQ) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

lp:		lp.o \
			$(LIBPRT) \
			$(LIBREQ) \
			$(LIBBSD) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o lp lp.o $(LDFLAGS) \
			$(LIBPRT) \
			$(LIBREQ) \
			$(LIBBSD) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

lpfilter:	lpfilter.o \
			$(LIBFLT) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o lpfilter lpfilter.o $(LDFLAGS) \
			$(LIBFLT) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP) \
			-lgen $(SHLIBS)

lpforms:	lpforms.o \
			$(LIBFRM) \
			$(LIBMSG) \
			$(LIBREQ) \
			$(LIBOAM) \
			$(LIBACC) \
			$(LIBLP)
	$(CC) -o lpforms lpforms.o $(LDFLAGS) \
			$(LIBFRM) \
			$(LIBMSG) \
			$(LIBREQ) \
			$(LIBACC) \
			$(LIBLP)  \
			$(LIBOAM) $(SHLIBS) $(LDLIBS)

lpmove:		lpmove.o \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o lpmove lpmove.o $(LDFLAGS) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

lpshut:		lpshut.o \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o lpshut lpshut.o $(LDFLAGS) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

lpsystem:	lpsystem.o \
			$(LIBSYS) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP)
	$(CC) -o lpsystem lpsystem.o $(LDFLAGS) \
			$(LIBSYS) \
			$(LIBMSG) \
			$(LIBOAM) \
			$(LIBLP) \
			-lnsl $(SHLIBS) $(LDLIBS)

lpusers:	lpusers.o \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBUSR) \
			$(LIBLP)
	$(CC) -o lpusers lpusers.o $(LDFLAGS) \
			$(LIBMSG) \
			$(LIBACC) \
			$(LIBOAM) \
			$(LIBUSR) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)

dumpolp:	dumpolp.o \
			$(LIBLP)
	$(CC) -o dumpolp dumpolp.o $(LDFLAGS) \
			$(LIBLP) $(SHLIBS) $(LDLIBS)


lintit:
	$(LINT) $(DEFLIST) comb.c \
			$(LINTLB) 
	$(LINT) $(DEFLIST) cancel.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lp.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpfilter.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpforms.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpmove.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpshut.c \
			$(LINTLB)
	$(LINT) $(DEFLIST) lpusers.c \
			$(LINTLB)
	for cmd in $(BIGCMDS); \
	do \
		cd ./$$cmd; \
		$(MAKE) DEBUG="$(DEBUG)" LINT="$(LINT)" $(MAKEARGS) lint; \
		cd ..; \
	done

lintlib:
