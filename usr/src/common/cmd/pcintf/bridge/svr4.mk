#ident	"@(#)pcintf:bridge/svr4.mk	1.1"
# 
#	 	PC Interface Unix Makefile.
#
#  WHAT : 	This makefile is a special version of bridge/makefile to comply 
#		with the UNIX System V Makefile Guidelines from USL.
#		It is assumed that INC is set in the file CMDRULES.
#
#  HOW  :       By changing the current directory to the system_label which the
#		make was invoked w/. All the source files (*.c) & header files
#		(*.h) may be referenced from common directory (..).
#
#
#				+-------------------------+
#				|    Common Directory :   |
#				| bridge.mk, svr4.mk      |
#				|       *.c & *.h         |
#				+-------------------------+
#				  |   |                 |
#			         /    |                  \
#		               /      |       . . .        \
#			     /        |                      \
#		+---------------+ +---------------+       +---------------+ 
#		| *.o & targets | | *.o & Targets | . . . | *.o & Targets |
#		+---------------+ +---------------+       +---------------+ 
#		svr4/eth           svr4/rs232              svr4/*
#
#
#		The function of this primary makefile is to set proper flags
#		for CC, LD, & LIB, and invoke the make again w/ the secondary
#		makefile called bridge.mk.


include		$(CMDRULES)

# Generic System 5 version 4.1 with TLI on a 386 machine, ethernet version
svr4eth:
	[ -d svr4     ] || mkdir svr4
	[ -d svr4/eth ] || mkdir svr4/eth
	cd svr4/eth; \
	$(MAKE) -f ../../bridge.mk \
		"WFLAGS = -v" \
		"DFLAGS = -DSVR4 -DETHNETPCI" \
		"IFLAGS  = -I$(INC)/sys/net" \
		"LIBFLAGS = -lsocket -lnsl" \
		rdpci loadpci util

# Generic System 5 version 4.1 with TLI on a 386 machine, RS232 version
svr4232:
	[ -d svr4       ] || mkdir svr4
	[ -d svr4/rs232 ] || mkdir svr4/rs232
	cd svr4/rs232; \
	$(MAKE) -f ../../bridge.mk \
		"WFLAGS = -v" \
		"DFLAGS = -DSVR4 -DRS232PCI" \
		pci232

lintit:
	[ -d svr4     ] || mkdir svr4
	[ -d svr4/eth ] || mkdir svr4/eth
	cd svr4/eth; \
	$(MAKE) -f ../../bridge.mk \
		"WFLAGS = -v" \
		"DFLAGS = -DSVR4 -DETHNETPCI" \
		"IFLAGS  = -I$(INC)/sys/net" \
		"LIBFLAGS = -lsocket -lnsl" \
		lintit

	[ -d svr4       ] || mkdir svr4
	[ -d svr4/rs232 ] || mkdir svr4/rs232
	cd svr4/rs232; \
	$(MAKE) -f ../../bridge.mk \
		"WFLAGS = -v" \
		"DFLAGS = -DSVR4 -DRS232PCI" \
		lintit
