#ident	"@(#)debugger:follow.d/i386/makefile	1.3"

include $(CMDRULES)

BASENAME = follow

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

CCSOURCES = 

LIBRARIES = 

CSOURCES = \
	follow.c


include DEPEND

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	$(CC) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSLIB)/$(BASENAME)

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)
