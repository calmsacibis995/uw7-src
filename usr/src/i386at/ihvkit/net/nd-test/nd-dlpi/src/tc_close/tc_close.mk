include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_nclose 
OBJ=tc_nclose.o
BIN1=tc_pclose
OBJ1=tc_pclose.o

all build : ${BIN} ${BIN1}

${BIN} : ${OBJ}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS}
	cp ${BIN} ${DLPI_BIN}/${BIN}

${BIN1} : ${OBJ1}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS}
	cp ${BIN1} ${DLPI_BIN}/${BIN1}

clean: 
	$(RM) $(OBJ) $(OBJ1) $(BIN) $(BIN1)

