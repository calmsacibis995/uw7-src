#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for printermgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		chkprinter.c \
		default.c \
		delprinter.c \
		freeprinter.c \
		getprinter.c \
		p_head.c \
		okprinter.c \
		printwheels.c \
		putprinter.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpprt

include ../libcommon.mk
