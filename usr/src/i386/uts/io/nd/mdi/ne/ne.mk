#ident "@(#)ne.mk	10.1"
#ident "$Header$"
#
#	Copyright (C) The Santa Cruz Operation, 1994-1996
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.
#

include $(UTSRULES)
KBASE = ../../../..
MAKEFILE = ne.mk
SCP_SRC = nemac.c

OBJS = download.o nemca.o nemac.o nemdi.o
LOBJS = download.L nemca.L nemac.L nemdi.L
DRV = ne.cf/Driver.o

DOWNLOAD_CODE = ne.dwnld
CV = cvbinCdecl
TOOLSBIN = ../../tools
CVBIN = $(TOOLSBIN)/$(CV)

.c.L:
	[ -f $(SCP_SRC) ] && $(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $< >$@

all:
	if [ -f $(SCP_SRC) ]; then \
		$(MAKE) -f $(MAKEFILE) $(DRV) $(MAKEARGS); \
		$(MAKE) -f $(MAKEFILE) download.c $(MAKEARGS); \
	fi

$(OBJS): ne3200.h

$(DRV): $(OBJS)
	$(LD) -r -o $(DRV) $(OBJS)

download.c: down_pre.c $(CVBIN)
	@echo "generating download.c"
	@(A=`sum -r $(DOWNLOAD_CODE) | awk '{print $$1}'`; [ "$$A" = "04526" ]\
                 || { echo "Download sum is not 04526."; exit 1; } )
	@cp down_pre.c download.c
	@chmod 644 download.c
	@$(CVBIN) < $(DOWNLOAD_CODE) >> download.c
	@echo '};' >> download.c

download_check: download.c
	@echo -n "Checking download image..."
	$(HCC) -O -D_KMEMUSER -o download_check -DNE_DEBUG_DOWNLOAD -I../.. download.c
	@(A=`./download_check | sum -r | awk '{print $$1}'`; [ "$$A" = "04526" ]\
                 || { echo "\nDownload sum is not 04526."; exit 1; } )
	@echo "	OK"

install: all
	(cd ne.cf; $(IDINSTALL) -R$(CONF) -M ne)

lintit: $(LOBJS)

clean:
	rm -f $(OBJS) download* test_mca *.L

clobber: clean
	if [ -f $(SCP_SRC) ]; then \
		rm -f $(DRV); \
	fi
	-$(IDINSTALL) -R$(CONF) -d -e ne

$(CVBIN):
	cd $(TOOLSBIN); make $(CV)

test_mca: nemca.c
	@echo -n "Testing multicast routines..."
	$(HCC) -O -D_KMEMUSER -DTEST_HARNESS -o test_mca nemca.c
	@./test_mca >/dev/null || { echo "\ntest_mca failed"; exit 1;}
	@echo "	PASS"
