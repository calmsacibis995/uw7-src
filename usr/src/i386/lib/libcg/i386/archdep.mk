#ident	"@(#)libcg:i386/lib/libcg/i386/archdep.mk	1.1"

#
#  makefile for libcg/i386
#


.SUFFIXES: .O .P

VERSDEF=ansi.def
#M4DEFS=m4.def sys.def $(VERSDEF)
M4DEFS=m4.def sys.def
#M4DEFS=

OBJECTS= cgids.o cgprocs.o cginfo.o cgbind.o cgcurrent.o cgmemloc.o       

PIC_OBJECTS=$(OBJECTS:.o=.O)

PROF_OBJECTS=$(OBJECTS:.o=.P)

objects: $(OBJECTS)

pic: $(PIC_OBJECTS)

prof: $(PROF_OBJECTS)

.s.O:
#	$(AS) -o $*.O -m -- -DDSHLIB $(M4DEFS) -DMCOUNT=\# pic.def $*.s
	$(AS) -o $*.O -m -- $(M4DEFS) -DMCOUNT=\# $*.s

.s.P:
	$(AS) -o $*.P -m -- $(M4DEFS) mcount.def $*.s

clean:
	-rm -f *.o
	-rm -f *.O
	-rm -f *.P

