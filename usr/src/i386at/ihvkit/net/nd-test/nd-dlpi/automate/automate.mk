include $(CMDRULES)
TARGET = ${SUITE_ROOT}/nd-dlpi
DIRS =  ${TARGET} \
	${TARGET}/bin \
	${TARGET}/nd-dlpi

COPTS= -g
INCLUDES=-I${SUITE_ROOT}/nd-dlpi/inc 
CFLAGS=${INCLUDES} ${COPTS}
SRCS= get_dlpi_conf.c
BIN= get_dlpi_conf
OBJS= get_dlpi_conf.o
LIBS=-lnsl -lsocket

all : build pkg

build : ${BIN}

get_dlpi_conf : get_dlpi_conf.c  
	$(CC) -o $@ $?

clean: 
	rm -f ${OBJS}
	rm -f ${BIN}

clobber: clean clean_pkg

clean_pkg:
	rm -rf ${TARGET}/bin/auto_dlpi
	rm -rf ${TARGET}/bin/get_dlpi_conf

pkg: 
	cp -f auto_dlpi ${TARGET}/bin
	cp -f get_dlpi_conf ${TARGET}/bin

