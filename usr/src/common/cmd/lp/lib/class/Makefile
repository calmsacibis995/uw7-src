#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for classmgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		delclass.c \
		freeclass.c \
		getclass.c \
		putclass.c

OBJS	=	$(SRCS:.c=.o)

LINTTXT =	$(SRCS)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpcls

include ../libcommon.mk
