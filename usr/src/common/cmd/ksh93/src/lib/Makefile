#ident	"@(#)ksh93:src/lib/Makefile	1.2"

include $(LIBRULES)

DIRS = libast.d libcmd.d libodelta.d libvdelta.d

CCFLAGS = -O $(CC.PIC)

ARFLAGS = cr

all: $(DIRS)

clean:
	for i in $(DIRS:.d=); \
	do\
		cd $$i; make clean; cd ..;\
	done

clobber: clean
	for i in $(DIRS:.d=); \
	do\
		cd $$i; make clobber; cd ..;\
	done

$(DIRS) :
	cd $(@:.d=); make all CCFLAGS=$(CCFLAGS) ARFLAGS=$(ARFLAGS)
