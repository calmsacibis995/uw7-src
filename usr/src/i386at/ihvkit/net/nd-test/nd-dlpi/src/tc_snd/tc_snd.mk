include $(CMDRULES)
include ../../dlpi.mkf
BIN=tc_snd
OBJ=tc_snd.o

all build : ${BIN}

${BIN} : ${OBJ}  
	$(CC) -O -o $@ $? ${TET_OBJ} ${LIBS} ${GUILIBS} 
	cp ${BIN} ${DLPI_BIN}/${BIN}

clean: 
	$(RM) $(OBJ) $(BIN)

