#ident	"@(#)ksh93:src/Makefile	1.4"
include $(CMDRULES)

DIRS = lib.d cmd.d
U95DIR=$(ROOT)/$(MACH)/u95
U95BIN=$(U95DIR)/bin
SHELLNAME = sh
MKDIR = mkdir
CP = cp
RM = rm
LN = ln
OWN=bin
GRP=root

MSGS=ksh93.msg

CCFLAGS = -O $(CC.PIC)

all: $(DIRS) ksh93.str libc_shells

install:	all
	if [ ! -d $(U95DIR) ] ; \
	then \
		mkdir $(U95DIR) ; \
	fi
	if [ ! -d $(U95BIN) ] ; \
	then \
		mkdir $(U95BIN) ; \
	fi
	$(CP) cmd/ksh93/ksh93 sh 
	$(INS) -m 0555 -u $(OWN) -g $(GRP) -f $(U95BIN) sh 
	$(RM) ./sh
	$(CP) cmd/use_builtin/use_builtin use_builtin 
	$(INS) -m 0755 -u $(OWN) -g $(GRP) -f $(U95BIN) use_builtin 
	$(RM) ./use_builtin
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/alias
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/cd
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/command
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/getopts
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/read
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/umask
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/wait
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/jobs
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/unalias
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/hash
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/type
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/ulimit
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/bg
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/fg
	-$(LN) $(U95BIN)/use_builtin $(USRBIN)/fc
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 ksh93.str
	$(INS) -m 0644 -u $(OWN) -g $(GRP) -f $(ETC) libc_shells 


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
	$(RM) -f $(ROOT)/$(MACH)$(INSTDIR)/$(SHELLNAME)

$(DIRS) :
	cd $(@:.d=); make all CCFLAGS=$(CCFLAGS)

ksh93.str:

libc_shells:

