#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for filtermgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		conv.c \
		delfilter.c \
		dumpfilters.c \
		freefilter.c \
		getfilter.c \
		filtertable.c \
		insfilter.c \
		loadfilters.c \
		putfilter.c \
		regex.c \
		search.c \
		trash.c

OBJS	=	$(SRCS:.c=.o)

LINTTXT =	$(SRCS)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpflt

include ../libcommon.mk
