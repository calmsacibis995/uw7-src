#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for bsd routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)
#LOCALDEF= -DDEBUG

SRCS	= \
		displayq.c \
		escape.c \
		fatalmsg.c \
		findfld.c \
		gethostnm.c \
		global.c \
		log.c \
		parse.c \
		psfile.c \
		rmjob.c \
		rsendjob.c \
		sndrcv.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LIBNAME	=	lpbsd

include ../libcommon.mk
