include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_subsunbind
OBJ=tc_subsunbind.o

all build : ${BIN} ${BIN1}

${BIN} : ${OBJ}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS}
	cp ${BIN} ${DLPI_BIN}/${BIN}
clean: 
	$(RM) $(OBJ) $(BIN)

