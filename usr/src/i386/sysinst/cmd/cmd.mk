#ident  "@(#)cmd.mk	15.1	98/03/04"

include $(CMDRULES)

BINCMDS = big_file bootstrap tapeop getcylsize odm sap_nearest encrypt_str libivar.so
SCRIPTS = conframdfs cut.flop cut.netflop mini_kernel pick.set putdev rmwhite
CMDS    = $(BINCMDS) $(SCRIPTS)
NATIVE  = bzip iscompress wrt hsflop chall checkwhite 
INSDIR  = $(PROTO)/bin
LOCALDEF = -D_KMEMUSER -D_KERNEL_HEADERS
LOCALINC = -I$(ROOT)/usr/src/$(WORK)/uts
LIBCRYPT = $(ROOT)/$(MACH)/usr/lib/libcrypt.a

all: $(CMDS)

big_file: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

bootstrap: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

bootstrap.o: $(@:.o=.c)
# Move the # sign to the second line below to disable the VT0 debug shell.
#	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -DLAST_LOAD -c $<
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -D_KMEMUSER -c $<

odm: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) -lelf

sap_nearest: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) -L $(ROOT)/$(MACH)/usr/lib -lnwutil -lnsl -lthread

tapeop: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

getcylsize: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS)

native: $(NATIVE)

bzip: zip/gzip
	cp $? $@

zip/gzip:
	@(cd zip; \
	chmod +x configure; \
	touch configure; \
	./configure --srcdir=. > /dev/null 2>&1; \
	$(MAKE))

hsflop.c:
	ln -s $(PROTO)/desktop/instcmd/sflop.c $@

encrypt_str: $$@.o
	$(CC) -o $@ $? $(LDFLAGS) $(LDLIBS) $(LIBCRYPT)

libivar.so: ivar.c
	$(CC) -c -Kpic -Xc ivar.c
	$(CC) -o libivar.so -G ivar.o

install: all
	@for i in $(BINCMDS) ;\
	do \
		strip $$i ;\
		mcs -d $$i ;\
	done
	@if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	@for i in $(CMDS) ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f *.o

clobber: clean
	rm -f $(CMDS) $(NATIVE)
	(cd zip; $(MAKE) distclean)
