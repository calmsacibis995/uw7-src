include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_nbind
OBJ=tc_nbind.o
BIN1=tc_pbind
OBJ1=tc_pbind.o

all build : ${BIN} ${BIN1}

${BIN} : ${OBJ}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS} 
	cp ${BIN} ${DLPI_BIN}/${BIN}

${BIN1} : ${OBJ1}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS}
	cp ${BIN1} ${DLPI_BIN}

clean: 
	$(RM) $(OBJ) $(OBJ1) $(BIN) $(BIN1)

