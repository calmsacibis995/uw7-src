#ident	"@(#)proto-cmd.mk	15.1	98/03/04"
# Makefile for Packaging and Installation Commands

include $(CMDRULES)

LOCALDEF = -DAT386

# MSGS = bootcntl.str

ALL = \
	adminobj \
	get_tz_offset \
	install_more \
	applysid \
	loadhba \
	odm \
	pdiconfig \
	rebuild \
	instlist \
	links \
	setmods \
	setmods.elf \
	mkflist \
	contents \
	x286 \
	check_uart \
	choose_lang

DIRS = \
	$(USRBIN) \
	$(SBIN) \
	$(VAR)/sadm/pkg/dfm \
	$(VAR)/tmp \
	$(ETC)/inst/scripts \
	$(USRLIB)/locale/C/MSGFILES

all: $(ALL) pkginfo default

# install: all $(DIRS) $(MSGS)
install: all $(DIRS)
	@$(INS) -f $(ETC)/inst/scripts adminobj
	@$(INS) -f $(ETC)/inst/scripts get_tz_offset
	@$(INS) -f $(ETC)/inst/scripts install_more
	@$(INS) -f $(ETC)/inst/scripts loadhba
	@$(INS) -f $(ETC)/inst/scripts applysid
	@$(INS) -f $(ETC)/inst/scripts odm
	@$(INS) -f $(ETC)/inst/scripts pdiconfig
	@$(INS) -f $(ETC)/inst/scripts rebuild
	@$(INS) -f $(SBIN) instlist
#	@$(INS) -f $(USRBIN) bootcntl
	@$(INS) -f $(USRBIN) x286
	@$(INS) -f $(USRBIN) choose_lang
#	@$(INS) -f $(USRLIB)/locale/C/MSGFILES bootcntl.str
	@$(INS) -f $(ETC) links
	@$(INS) -f $(ETC) setmods
	@$(INS) -f $(ETC) setmods.elf
	@$(INS) -f $(ETC) mkflist
	@$(INS) -f $(ETC) contents
	@$(INS) -f $(ETC) check_uart
	@$(INS) -f $(VAR)/sadm/pkg/dfm pkginfo
	@$(INS) -f $(VAR) default

$(DIRS):
	-mkdir -p $@

setmods contents mkflist: $$@.c
	$(HCC) $(CFLAGS) -I/usr/include -o $@ $@.c $(LDFLAGS)

check_uart: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(NOSHLIBS) $(LDFLAGS)

setmods.elf: $(@:.elf=.c)
	$(CC) $(CFLAGS) -I$(INC) -o $@ $(@:.elf=.c) $(LDFLAGS)

# links x286 bootcntl instlist: $$@.c
links x286 instlist: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

choose_lang: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS) -lcurses


adminobj get_tz_offset install_more loadhba applysid pdiconfig rebuild: $$@.sh

clean:
	$(RM) -f *.o

clobber: clean
	$(RM) -f $(ALL)

