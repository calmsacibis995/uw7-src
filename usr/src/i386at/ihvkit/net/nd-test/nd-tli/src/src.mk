include $(CMDRULES)
TARGET = ${SUITE_ROOT}/nd-tli/bin
DIRS =  ${TARGET} \
INCS = -I${TET_ROOT}/inc/posix_c
LIBS = -L${TET_ROOT}/lib/posix_c -lapi -lnsl -lelf
TET_OBJ = ${TET_ROOT}/lib/posix_c/tcm.o
CFLAGS  = ${DFLAGS} ${INCS} ${COPTS} 
LDFLAGS = ${LIBS}
SRC = datarecv.c datasend.c util.c
OBJ = datarecv.o datasend.o util.o
BIN = datarecv datasend

all: build pkg

build : ${BIN} 

${BIN} : ${OBJ}
	$(CC) $(CFLAGS) -o $@ $@.o util.o ${TET_OBJ} ${LIBS} 

clean:
	rm -f $(BIN) $(OBJ) 

clobber: clean clean_pkg

pkg: 
	cp $(BIN) ${TARGET}

clean_pkg:
	cd ${TARGET}/nd-tli; rm $(BIN) 

