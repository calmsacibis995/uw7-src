#ident	"@(#)libdshm:i386/lib/libdshm/src/src.mk	1.9.1.2"


include $(LIBRULES)

# Makefile for libdsm

OBJS		=	dshm.o dshmsys.o dshm_os_glue.o
INCLUDES	=	-I../include
CFLAGS		= 	-O2 $(INCLUDES)
LIBDSHM 	=	/usr/lib/libdshm.so
BINARIES 	= 	libdshm.so
MAKEFILE	=	src.mk
PROBEFILE       =       dshm.c

all:	
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(MAKEARGS) libdshm.so; \
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

libdshm.so: $(OBJS)
	$(CC) -G -h $(LIBDSHM) $(OBJS) -o libdshm.so

install: all
	$(INS) -f $(USRLIB) -m 755 libdshm.so
	$(INS) -f $(TOOLS)/usr/include -m 444 ../include/dshm.h
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 ../include/dshm.h

clean:	
	rm -f *.o libdshm.so

