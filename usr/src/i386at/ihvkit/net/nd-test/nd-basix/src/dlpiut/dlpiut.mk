#
# File execute.c
#
#      Copyright (C) The Santa Cruz Operation, 1994-1995.
#      This Module contains Proprietary Information of
#      The Santa Cruz Operation and should be treated
#      as Confidential.
#
# @(#) Makefile 11.1 95/05/01 SCOINC
include $(CMDRULES)
INCDIR =		$(ROOT)/usr/src/i386/uts/io/nd
CFLAGS =		-O -I${INCDIR} -s -D_KMEMUSER

LIB =			libut.a

.PRECIOUS:		$(LIB)

TARGET =		dlpiut

#$(TARGET):		$(LIB) Makefile
$(TARGET):		$(LIB) 
			$(CC) $(CFLAGS) -o dlpiut $(LIB) -lx

clean:

clobber:
			rm -f dlpiut $(LIB)


$(LIB):		$(LIB)(api_dlpi.o) \
			$(LIB)(api_mdi.o) \
			$(LIB)(client.o) \
			$(LIB)(dump.o) \
			$(LIB)(execute.o) \
			$(LIB)(framing.o) \
			$(LIB)(ipc.o) \
			$(LIB)(load.o) \
			$(LIB)(main.o) \
			$(LIB)(parse.o) \
			$(LIB)(server.o)

$(LIB)(api_dlpi.o):	api_dlpi.c \
					${INCDIR}/sys/scodlpi.h \
					dlpiut.h \

$(LIB)(api_mdi.o):	api_mdi.c \
					${INCDIR}/sys/mdi.h \
					dlpiut.h \

$(LIB)(client.o):	client.c \
					dlpiut.h \

$(LIB)(dump.o):		dump.c \
					dlpiut.h \

$(LIB)(execute.o):	execute.c \
					dlpiut.h \

$(LIB)(framing.o):	framing.c \
					${INCDIR}/sys/scodlpi.h \
					dlpiut.h \

$(LIB)(ipc.o):		ipc.c \

$(LIB)(load.o):		load.c \
					dlpiut.h \

$(LIB)(main.o):		main.c \
					dlpiut.h \

$(LIB)(parse.o):	parse.c \
					${INCDIR}/sys/scodlpi.h \
					dlpiut.h \

$(LIB)(server.o):	server.c \
					dlpiut.h \

