#		copyright	"%c%" 	

#ident	"@(#)sa:common/cmd/sa/sar/makefile	1.6.2.1"
#ident  "$Header$"

include $(CMDRULES)

LOCALDEF = -D_KMEMUSER -DHEADERS_INSTALLED

OBJECTS = 	sar.o\
		bufcache.o\
		cgmtr.o\
		cpu.o\
		cpucg.o\
		cpumtr.o\
		cpumtr_lwp.o\
		disk.o\
		facc.o\
		flook.o\
		free.o\
		fs.o\
		gsched.o\
		init.o\
		inode.o\
		ipc.o\
		kma.o\
		locsched.o\
		lwpres.o\
		names.o\
		output.o\
		pml.o\
		procres.o\
		psdata.o\
		sarutil.o\
		skmem.o\
		start.o\
		syscall.o\
		tty.o\
		vm.o

sar: $(OBJECTS)
	$(CC) -o sar $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

lintit:
	$(LINT) $(LINTFLAGS) \
	 	sar.c\
		bufcache.c\
		cgmtr.c\
		cpu.c\
		cpucg.c\
		cpumtr.c\
		cpumtr_lwp.c\
		disk.c\
		facc.c\
		flook.c\
		free.c\
		fs.c\
		gsched.c\
		init.c\
		inode.c\
		ipc.c\
		kma.c\
		locsched.c\
		names.c\
		output.c\
		pml.c\
		procres.c\
		psdata.c\
		sarutil.c\
		skmem.c\
		start.c\
		syscall.c\
		tty.c\
		vm.c

