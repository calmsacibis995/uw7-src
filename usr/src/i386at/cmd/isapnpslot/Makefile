# @(#)Makefile	7.2	11/11/97	17:21:44
#	@(#)Makefile	

CC=$(TOOLS)/usr/ccs/bin/i386cc
INST=install
RM=rm
SRC=isapnpslot.c
BIN=isapnpslot
HW=hw/hw
IFLAGS=-I$(ROOT)/usr/src/$(WORK)/uts/io/autoconf/ca/isapnp
SBINDIR=$(ROOT)/$(MACH)/sbin


all: $(BIN) $(HW)

$(BIN):
	$(CC) $(IFLAGS) $(SRC) -o $(BIN)

$(HW):
	@(cd hw ; echo "making" $@ "in hw..."; \
        $(MAKE) -f hw.mk; \
        $(MAKE) -f test.mk);

install: $(BIN)
	@if [ ! -d $(SBINDIR) ]; then \
                mkdir -p $(SBINDIR); \
        fi
	$(INST) -f $(SBINDIR) $(BIN)

clean:
	$(RM) -f *.o core
	@(cd hw ; echo "making" $@ "in hw..."; \
        $(MAKE) -f hw.mk $@; \
        $(MAKE) -f test.mk $@);

clobber: clean
	$(RM) -f $(BIN)
	@(cd hw ; echo "making" $@ "in hw..."; \
        $(MAKE) -f hw.mk $@; \
        $(MAKE) -f test.mk $@);
