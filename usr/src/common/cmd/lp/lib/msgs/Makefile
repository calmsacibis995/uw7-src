#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for messagemgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		_getmessage.c \
		_putmessage.c \
		getmessage.c \
		hslconv.c \
		putmessage.c \
		msgfmts.c \
		mclose.c \
		mconnect.c \
		mdisconnect.c \
		mgetputm.c \
		mlisten.c \
		mcreate.c \
		mdestroy.c \
		mneeds.c \
		mopen.c \
		mread.c \
		mrecv.c \
		msend.c \
		mwrite.c \
		streamio.c \
		fifo_buffs.c \
		read_fifo.c \
		write_fifo.c

LINTTXT =	$(SRCS) ../lp/llib-llp.ln

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpmsg

include ../libcommon.mk

$(OBJS):	$(LPINC)/msgs.h
