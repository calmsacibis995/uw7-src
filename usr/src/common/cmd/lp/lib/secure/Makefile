#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for securemgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		secure.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpsec

include ../libcommon.mk

$(OBJS):	$(LPINC)/secure.h
