include $(CMDRULES)
INCLUDES=-I${SUITE_ROOT}/nd-dlpi/inc -I${TET_ROOT}/inc/posix_c 
CFLAGS=${DFLAGS} ${INCLUDES} ${COPTS}
SRCS= dl.c tc_net.c tc_xtra.c
OBJS= dl.o tc_net.o tc_xtra.o

all : build

build : ${OBJS}
	ar cr libdl.a ${OBJS}
clean: 
	rm -f ${OBJS} 

CLOBBER: clean
	rm -f libdl.a
