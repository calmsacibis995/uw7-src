#ident	"@(#)libdwarf2:i386/makefile	1.7"

include $(CMDRULES)

TARGET = libdwarf2.a

HFILES= $(COMINC)/dwarf2.h $(COMINC)/libdwarf2.h

SOURCES = $(COMDIR)/attr_name.c $(COMDIR)/btype_name.c $(COMDIR)/form_name.c \
		$(COMDIR)/lang_name.c $(COMDIR)/loc_name.c $(COMDIR)/tag_name.c \
		$(COMDIR)/get_name.c $(COMDIR)/sencode.c $(COMDIR)/sdecode.c \
		$(COMDIR)/uencode.c $(COMDIR)/udecode.c \
		$(COMDIR)/abbrevtab.c $(COMDIR)/abbrev.c \
		$(COMDIR)/line_table.c

OBJECTS = attr_name.o btype_name.o form_name.o lang_name.o loc_name.o tag_name.o \
		line_table.o \
		get_name.o sencode.o sdecode.o uencode.o udecode.o \
		abbrevtab.o abbrev.o

COMDIR = ../common
COMINC = ../../inc/common
DEFLIST = -D_REENTRANT
INCLIST = -I$(COMDIR) -I$(COMINC)
CC_CMD = $(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)
INS = $(SGSBASE)/sgs.install

all:	$(TARGET)

install:	$(CCSLIB)/libdwarf2.a

$(CCSLIB)/libdwarf2.a:  $(TARGET)
	/bin/sh $(INS) 644 $(OWN) $(GRP) $(CCSLIB)/libdwarf2.a libdwarf2.a

libdwarf2.a:	$(OBJECTS)
	rm -f libdwarf2.a
	$(AR) q libdwarf2.a $(OBJECTS)

attr_name.o:	$(COMDIR)/attr_name.c
	$(CC_CMD) -c $?

btype_name.o:	$(COMDIR)/btype_name.c
	$(CC_CMD) -c $?

form_name.o:	$(COMDIR)/form_name.c
	$(CC_CMD) -c $?

lang_name.o:	$(COMDIR)/lang_name.c
	$(CC_CMD) -c $?

loc_name.o:	$(COMDIR)/loc_name.c
	$(CC_CMD) -c $?

tag_name.o:	$(COMDIR)/tag_name.c
	$(CC_CMD) -c $?

get_name.o:	$(COMDIR)/get_name.c
	$(CC_CMD) -c $?

sdecode.o:	$(COMDIR)/sdecode.c
	$(CC_CMD) -c $?

udecode.o:	$(COMDIR)/udecode.c
	$(CC_CMD) -c $?

sencode.o:	$(COMDIR)/sencode.c
	$(CC_CMD) -c $?

uencode.o:	$(COMDIR)/uencode.c
	$(CC_CMD) -c $?

abbrevtab.o:	$(COMDIR)/abbrevtab.c
	$(CC_CMD) -c $?

abbrev.o:	$(COMDIR)/abbrev.c
	$(CC_CMD) -c $?

line_table.o:	$(COMDIR)/line_table.c
	$(CC_CMD) -c $?

clean:
	rm -f $(OBJECTS)

clobber:	clean
	rm -f $(TARGET)

lintit:	$(HFILES) $(SOURCES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)
