#ident	"@(#)Makefile	1.2"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header$"
#
# Makefile for bsd routines
#

LOCALDEF = -DMCPOWERS -DN_PLAT_UNIX

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

DEVINC2 = -I$(CROSS)/usr/include/nw

SRCS	= 	NWcpsOps.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LIBNAME	=	lpnw

include ../libcommon.mk
