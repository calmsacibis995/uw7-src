#ident	"@(#)debugger:util/common/lib.make	1.3"

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(AR) -qc $(TARGET) $(OBJECTS)
	chmod 664 $(TARGET)

$(OSR5_TARGET):	$(OSR5_OBJECTS)
	rm -f $(OSR5_TARGET)
	$(AR) -qc $(OSR5_TARGET) $(OSR5_OBJECTS)
	chmod 664 $(OSR5_TARGET)

all:	$(TARGET)

osr5:	$(OSR5_TARGET)

install:	all
