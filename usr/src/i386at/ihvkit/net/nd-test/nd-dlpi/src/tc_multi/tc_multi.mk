include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_multi
OBJ=tc_multi.o

all build : ${BIN}

${BIN} : ${OBJ}  
	$(CC) -o $@ $? ${TET_OBJ} ${LIBS}

clean: 
	$(RM) $(OBJ) $(BIN)

