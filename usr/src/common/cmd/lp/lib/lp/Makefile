#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"

#
# Makefile for LP support routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		Syscalls.c \
		addlist.c \
		addstring.c \
		appendlist.c \
		audit.c \
		alerts.c \
		charset.c \
		cs_strcmp.c \
		cs_strncmp.c \
		dellist.c \
		dashos.c \
		debug.c \
		dirs.c \
		duplist.c \
		files.c \
		freelist.c \
		getlist.c \
		getname.c \
		getpaths.c \
		getspooldir.c \
		isterminfo.c \
		joinlist.c \
		lenlist.c \
		locale.c \
		lp_errno.c\
		makepath.c \
		makestr.c \
		mergelist.c \
		next.c \
		printlist.c \
		sdn.c \
		sprintlist.c \
		searchlist.c \
		sendmail.c \
		set_charset.c \
		set_pitch.c \
		set_size.c \
		sop.c \
		strip.c \
		syntax.c \
		tidbit.c \
		tinames.c \
		wherelist.c \
		which.c	\
		GetProcLevel.c \
		SetProcLevel.c \
		SetFileLevel.c \
		TruncLevel.c \
		WrapLevel.c

LINTTXT =	$(SRCS) ../oam/llib-llpoam.ln

OBJS	=	$(SRCS:.c=.o)

LINTS	=

LIBNAME	=	lp

include ../libcommon.mk
