include $(CMDRULES)

# we must install cet_start and cet_stop in INSDIR so that packaging can pick
# it up.
MAKEFILE = cet.mk
INSDIR=$(ETC)/inst/nd/mdi/cet
DRV=cet
SCP_SRC = $(DRV)_start.c
#CETINC=../../../../uts/io/nd/mdi/cet/include
#NSSINC=../../../../uts/io/nd/mdi/cet/nss/include
#NFLXINC=../../../../uts/io/nd/mdi/cet/nss/nflx
#TLANINC=../../../../uts/io/nd/mdi/cet/nss/tlan

CETINC=$(ROOT)/usr/src/$(WORK)/uts/io/nd/mdi/cet/include
NSSINC=$(ROOT)/usr/src/$(WORK)/uts/io/nd/mdi/cet/nss/include
NFLXINC=$(ROOT)/usr/src/$(WORK)/uts/io/nd/mdi/cet/nss/nflx
TLANINC=$(ROOT)/usr/src/$(WORK)/uts/io/nd/mdi/cet/nss/tlan

LOCALINC=-I$(CETINC) -I$(NSSINC) -I$(NFLXINC) -I$(TLANINC)
LOCALDEF=-D_NF2 -D_KERNEL -D_MDI -DUW500
NSSDEPS=$(NSSINC)/NSS.H $(NSSINC)/CPQTYPES.H $(NSSINC)/NFLX.H $(NSSINC)/TLAN.H \
	$(NSSINC)/NFLXPROT.H $(NSSINC)/TLANPROT.H
DEPS=$(CETINC)/$(DRV).h $(CETINC)/$(DRV)_mac.h $(NSSDEPS)

OUTPUT_FILES=$(DRV)_start $(DRV)_stop 

CFLAGS=$(LOCALINC) $(LOCALDEF)

# Primary rules

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

##all:	$(OUTPUT_FILES)
all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV)_start ; \
		$(MAKE) -f $(MAKEFILE) $(DRV)_stop ; \
	fi

install: all 
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) $(DRV)_start
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) $(DRV)_stop

touch:
	touch -c *.c

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(OUTPUT_FILES); \
	fi

clean:
		rm -f *.o;

# Secondary rules

# "$(DRV)_start/stop" -	user programs that execute the "ioctl" calls to
#			start/stop one or more NetFlex-3/Netelligent boards.
#			They're invoked from a "rc" script at startup or
#			shutdown time.

#	$(CC) $(CFLAGS) $(DRV)_start.c -o $(DRV)_start
$(DRV)_start:	$(DRV)_start.c $(DEPS)

#	$(CC) $(CFLAGS) $(DRV)_stop.c -o $(DRV)_stop
$(DRV)_stop:	$(DRV)_stop.c $(DEPS)
