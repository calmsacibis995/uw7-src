#	copyright	"%c%"

#ident	"@(#)Makefile	1.2"
#ident "$Header$"
#
# Makefile for LP message management routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

TEXTSRC	= \
		e_adm__msgs.c \
		e_can__msgs.c \
		e_cmb__msgs.c \
		e_dat__msgs.c \
		e_fl__msgs.c \
		e_for__msgs.c \
		e_lp__msgs.c \
		e_lpp__msgs.c \
		e_lpu__msgs.c \
		e_mov__msgs.c \
		e_sch__msgs.c \
		e_sht__msgs.c \
		e_stat__msgs.c \
		e_sys__msgs.c

SRCS	= \
		agettxt.c \
		buffers.c \
		fmtmsg.c \
                outmsg.c \
                retmsg.c \
                vfmtmsg.c \
		skipfmts.c

LINTTXT =	$(TEXTSRC) $(SRCS)

OBJS	=	$(SRCS:.c=.o) $(TEXTSRC:.c=.o)

LIBNAME	=	lpoam

include ../libcommon.mk


clean::	
	$(RM) $(TEXTSRC) $(LPINC)/oam_def.h xx??

$(SRCS:.c=.o):	$(LPINC)/oam_def.h

$(LPINC)/oam_def.h:	msg.source \
			gen-defs \
			gen-text
	sh gen-defs >$(LPINC)/oam_def.h
	sh gen-text

lintlib:	$(LPINC)/oam_def.h

lintit:
