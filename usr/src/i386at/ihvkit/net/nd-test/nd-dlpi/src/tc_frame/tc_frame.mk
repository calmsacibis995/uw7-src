include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_oframe
OBJ=tc_oframe.o

all build : ${BIN}

${BIN} : ${OBJ}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS} ${GUILIBS}
	cp ${BIN} ${DLPI_BIN}/${BIN}

clean: 
	$(RM) $(OBJ) $(BIN)

