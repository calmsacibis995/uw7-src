#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for usermgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		usermgmt.c \
		loadpri.c \
		storepri.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpusr

include ../libcommon.mk
