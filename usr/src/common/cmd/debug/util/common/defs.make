#ident	"@(#)debugger:util/common/defs.make	1.14"

CPLUS	= $(C++C)
CCFLAGS	= $(CFLAGS)
LDLIBS	= 
INSDIR	= $(CCSBIN)

UIBASE = $(TOOLS)/usr/X11R6.1
SGSBASE	= $(PRODDIR)/../sgs
COMINC	= $(SGSBASE)/inc/common
CPUINC = $(SGSBASE)/inc/$(CPU)
LIBCINC = $(PRODDIR)/../../lib/libC

PRODDIR = ../..

PRODLIB	= $(PRODDIR)/lib
OSR5_LIB = $(PRODDIR)/osr5_lib
PRODINC	= $(PRODDIR)/inc
INCCOM	= $(PRODDIR)/inc/common
INCCPU	= $(PRODDIR)/inc/$(CPU)
COMMON	= ../common

# MULTIBYTE support
MBDEF	=  #-DMULTIBYTE
MBLIB	=  #-lw

INCLIST1= -I. -I$(COMMON) -I$(INCCPU) -I$(INCCOM)
INCLIST2= -I$(CPUINC) -I$(COMINC)

DFLAGS	=

CC_CMD_FLAGS = $(CFLAGS) $(INCLIST1) $(INCLIST2) $(DEFLIST) $(DFLAGS) $(MBDEF) -I$(INC)
CC_CMD	= $(CC) $(CC_CMD_FLAGS)

CPLUS_CMD_FLAGS = $(CCFLAGS) $(INCLIST1) $(INCLIST2) $(DEFLIST) $(DFLAGS)
CPLUS_CMD = $(CPLUS) $(CPLUS_CMD_FLAGS)

ARFLAGS	= qc
YFLAGS	= -ld

# The default target is used if no target is given; it must be first.

default: all
