CFLAGS =	-O
LDFLAGS =	-s

include $(CMDRULES)
all:	ndsu

clean:
clobber: clean
	rm -f ndsu
