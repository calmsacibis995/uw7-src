#ident	"@(#)debugger:libmotif/i386/makefile	1.7"

include $(CMDRULES)

TARGET = $(PRODLIB)/libmotif.a

OSR5_TARGET = $(OSR5_LIB)/libmotif.a

include ../../util/common/std.make

include ui.defs
DFLAGS = $(XDEFS) -I../../gui.d/common -I../../gui.d/$(CPU) -I../common \
	-I$(XINC)

CCSOURCES = \
	$(COMMON)/Alert_sh.C \
	$(COMMON)/Boxes.C \
	$(COMMON)/Button_bar.C \
	$(COMMON)/Caption.C \
	$(COMMON)/Dialog_sh.C \
	$(COMMON)/DND.C \
	$(COMMON)/FSdialog.C \
	$(COMMON)/Menu.C \
	$(COMMON)/Mnemonic.C \
	$(COMMON)/Radio.C \
	$(COMMON)/Resources.C \
	$(COMMON)/Sel_list.C \
	$(COMMON)/Slider.C \
	$(COMMON)/Stext.C \
	$(COMMON)/Table.C \
	$(COMMON)/Text_area.C \
	$(COMMON)/Text_disp.C \
	$(COMMON)/Text_edit.C \
	$(COMMON)/Text_line.C \
	$(COMMON)/Timer.C \
	$(COMMON)/Toggle.C \
	$(COMMON)/Window_sh.C \
	$(COMMON)/mutil.C

CSOURCES=\
	$(COMMON)/List.c \
	$(COMMON)/xpm.c

include DEPEND

include ../../util/common/lib.make

local_depend:
	cp BASEDEPEND /tmp
	sh ../../util/common/substdir $(XINC) '$$(XINC)' </tmp/BASEDEPEND | \
	sh ../../util/common/substdir /usr/include '$$(INC)' >BASEDEPEND
	rm  -f /tmp/BASEDEPEND
