#		copyright	"%c%"

#ident	"@(#)include.mk	15.1"

include $(LIBRULES)

.c.a:;

LIBSTUBS = libstubs.a
OBJECTS = $(SOURCES:.c=.o)

SOURCES = stubs.c


all: $(LIBSTUBS)

.PRECIOUS: $(LIBSTUBS)

$(LIBSTUBS): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBSTUBS) $(OBJECTS)
