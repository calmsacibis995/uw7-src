include $(CMDRULES)
include ../../dlpi.mkf
BIN=listen
OBJ=listen.o

all build : $(BIN)

$(BIN) : $(OBJ)  
	$(CC) -o $@ $? $(INCLUDE) $(TET_OBJ) $(LIBS) 
	cp $(BIN) $(DLPI_BIN)/$(BIN)

clean: 
	$(RM) $(OBJ) $(BIN)

