#ident	"@(#)ksh93:src/cmd/Makefile	1.3"

include $(CMDRULES)

DIRS = ksh93.d use_builtin.d

CCFLAGS = -O $(CC.PIC)

all: $(DIRS)

install:
	for i in $(DIRS:.d=); \
	do\
		cd $$i; make install; cd ..;\
	done

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
	cd $(@:.d=); make all CCFLAGS=$(CCFLAGS)

