#ident "@(#)smpw.mk   5.1"
#ident "$Header$"

# Makefile for generating the GEMINI driver for the SMC 8432/9332/9334 (SMPW)
#
# The assumption is that the file is in the same directory as the
# source files.
#
# Also, there should be a subdirectory there called nstallation
# Directoy), where the installation files will be copied to.


include $(UTSRULES)


LOWVPFX = smpw
UPVPFX  = SMPW
C++VEN  = SMC


MAKEFILE = $(LOWVPFX).mk
KBASE = ../../../..

STATIC= static
CDBGFLAGS = -ec -DINTERNAL_DEBUG -ec -DDEBUG_PRINTS
NAMEFLAG = -ec -D$(UPVPFX) -ec -D$(C++VEN)
MYCFLAGS   = -c -Zp4 $(CFLAGS)
CPPFLAGS   = -c -Zp1 $(CFLAGS)
CPPFFLAGS = -c -Zp1 -Xd -O -D$(C++VEN) -W2,-_s -Kno_host -Wf,--no_exceptions -Wf,--no_rtti
INTERFACE = -ec -DGEMINI_INTERFACE
UNIXWARE = -DUNIXWARE -DSCO_UNIX
LOCALDEF=$(UNIXWARE) $(DEBUG) $(INTERFACE) $(NAMEFLAG)
LOCALINC= -I. -I$(KBASE)/util

DRV = $(LOWVPFX).cf/Driver.o
SMPW_CONFIG_FILES = Node Master System space.c space.h
SCP_SRC = d21x.c

CPPOBJ = init.o \
    init_csr.o \
    init_aux.o \
    init_mem.o \
    init_pci.o \
    init_phy.o \
    miigen.o \
    miiphy.o 

COBJ = d21x.o \
      d21x_alloc.o \
      d21x_init.o \
      d21x_register.o \
      d21x_srom_hw.o \
      d21x_addr_handler.o \
      d21x_rx_tx_handler.o \
      d21x_wput_handlers.o \
      gen_pci.o \
      sco_aux.o \
      linker.o 

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
	fi
init.o:		init.C init.hpp init_mem.hpp init_phy.hpp init_csr.hpp \
                init_med.hpp common.h 
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init.C

init_csr.o:	init_csr.C common.h init_csr.hpp 
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init_csr.C

init_aux.o:	init_aux.C common.h init_mem.hpp init_aux.hpp
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init_aux.C

init_mem.o:	init_mem.C common.h init_mem.hpp
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init_mem.C

init_pci.o:	init_pci.C common.h
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init_pci.C

init_phy.o:	init_phy.C common.h mii_phy.h miigen.hpp \
		init_csr.hpp init_med.hpp init_phy.hpp
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) init_phy.C

miigen.o:	miigen.C common.h mii_phy.h miigen.hpp miiphy.hpp
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) miigen.C

miiphy.o:	miiphy.C common.h mii_phy.h miiphy.hpp 
	$(C++C) $(CPPFFLAGS) $(DEFLIST) $(INCLIST) miiphy.C

d21x_alloc.o:       d21x_alloc.c \
    	    d21x.h \
        	d21x_port_def.h\
        	d21x_include.h 
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_alloc.c

d21x_init.o:	    d21x_init.c \
    	    d21x.h \
        	d21x_port_def.h\
        	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_init.c

d21x_register.o:    d21x_register.c \
    	    d21x.h \
        	d21x_port_def.h\
        	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_register.c

d21x_srom_hw.o:    d21x_srom_hw.c \
        	common.h\
        	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_srom_hw.c

d21x_addr_handler.o:    d21x_addr_handler.c \
			d21x.h \
			d21x_port_def.h\
			d21x_include.h 
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_addr_handler.c

d21x_rx_tx_handler.o:	d21x_rx_tx_handler.c \
			d21x.h \
			d21x_port_def.h\
			d21x_include.h 
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_rx_tx_handler.c

d21x_wput_handlers.o:	d21x_wput_handlers.c \
			d21x.h \
			d21x_port_def.h\
			d21x_include.h 
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x_wput_handlers.c


d21x.o:             d21x.c \
	d21x.h \
	d21x_port_def.h\
	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) d21x.c

gen_pci.o:          gen_pci.c \
	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) gen_pci.c

space.o:          space.c \
	d21x_include.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) space.c

sco_aux.o:	sco_aux.c  common.h
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) sco_aux.c

linker.o:	linker.c 
	$(CC) $(MYCFLAGS) $(DEFLIST) $(INCLIST) linker.c

#sym_name_change: sym_name_change.c
#	$(CC) -o sym_name_change sym_name_change.c -lld
elf_sym_change: elf_sym_change.c
#	cc -o elf_sym_change elf_sym_change.c -lld -lelf
	-$(HCC) -o elf_sym_change elf_sym_change.c -lld -lelf

$(DRV): $(COBJ) $(CPPOBJ) $(LOWVPFX).mk elf_sym_change
	$(LD) -r -o $@ $(COBJ) $(CPPOBJ)
	# $(C++C) $(LDFLAGS) -o $@ $(COBJ) $(CPPOBJ)
	# strip $@
	sh ./sym_strip	 smpw

clean:
	#rm -f *.o $(CPPC) $(DRV) *.err
	rm -f *.o *.err *.smt *.out

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	$(IDINSTALL) -R$(CONF) -d -e $(LOWVPFX)

install: all
	(cd smpw.cf; $(IDINSTALL) -R$(CONF) -M $(LOWVPFX))
