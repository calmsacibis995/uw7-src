#ident	"@(#)ihvkit:display/lfb256/FB/ati/Makefile	1.1"
OBJS = ati.o
FILES = bin/Driver.o bin/Master bin/Node bin/System

all: $(FILES)

bin/Driver.o: $(OBJS)
	ld -r -o $@ $(OBJS)
