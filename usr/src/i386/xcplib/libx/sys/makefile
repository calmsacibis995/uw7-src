#	copyright	"%c%"

#ident	"@(#)xcplibx:i386/xcplib/libx/sys/makefile	1.5"
#ident  "$Header$"

include $(LIBRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.
#
# makefile for libx/i386
#
#
# The variable PROF is null by default, causing profiled object to be
# maintained.  If profiled object is not desired, the reassignment
# PROF=@# should appear in the make command line.
#

.SUFFIXES: .p
.MUTEX: prof nonprof

TMPDIR = ./tmp
LOCALDEF = -DMERGE

PROF=
NONPROF=
SDEFLIST=

M4DEFS = m4.def sys.def

POBJECTS=$(OBJECTS:.o=.p)

OBJECTS = chsize.o \
	  creatsem.o \
	  execseg.o \
	  ftime.o \
	  locking.o \
	  nap.o \
	  nbwaitsem.o \
	  opensem.o \
	  proctl.o \
	  rdchk.o \
	  sdfree.o \
	  _sdget.o \
	  sdgetv.o \
	  sdenter.o \
	  sdleave.o \
	  sdwaitv.o \
	  sigsem.o \
	  unexecseg.o \
	  waitsem.o

all: $(TMPDIR) prof nonprof

$(TMPDIR):
	[ -d $@ ] || mkdir $@

nonprof: $(OBJECTS) 

prof: $(POBJECTS) 

.c.o:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(SDEFLIST) $(INCLIST) $(CFLAGS) -c $*.c
	$(NONPROF)mv $(*F).o $*.o

.c.p:
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -p $*.c && mv $(*F).o $*.p

.s.o:
	$(NONPROF)@echo $*.s:
	$(NONPROF)$(M4) $(M4DEFS) $(SDEFLIST) -DMCOUNT=/ $*.s  > $(TMPDIR)/$(*F).s  
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) -c $(TMPDIR)/$(*F).s 
	-rm $(TMPDIR)/$(*F).s

.s.p:
	$(PROF)@echo $*.s:
	$(PROF)$(M4) $(M4DEFS) $(SDEFLIST) mcount.def $*.s  > $(TMPDIR)/$(*F).s 
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) -c -p $(TMPDIR)/$(*F).s && mv $(*F).o $*.p
	-rm $(TMPDIR)/$(*F).s

clean:
	-rm -f *.o
	-rm -f *.p
	-rm -rf $(TMPDIR)

clobber: clean

lintit:
	$(LINT) $(LINTFLAGS) $(OBJECTS:.o=.c)
