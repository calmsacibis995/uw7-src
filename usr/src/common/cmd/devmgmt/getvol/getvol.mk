#	copyright	"%c%"

#ident	"@(#)devmgmt:common/cmd/devmgmt/getvol/getvol.mk	1.11.8.1"

include $(CMDRULES)

PROC=getvol
SRC=getvol.c
OBJ=$(SRC:.c=.o)

## default paramter definitions

## libraries used by this process
LINTLIBS=$(USRLIB)/llib-lpkg.ln $(USRLIB)/llib-ladm.ln

## options used to build this command
LDLIBS=-lpkg -ladm -lw
LOCALINC=-I .
LINTFLAGS=$(DEFLIST) $(LINTLIBS)

## process build rules

all:	$(PROC) 

$(PROC): $(OBJ) $(LIBINST)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $(PROC)

OBJECTS=$(PROC)
PROTO=../Prototype
INSTALL=install
install: all
	@eval `sed -e '/^![^=]*=/!d' -e 's/^!//' $(PROTO)` ;\
	mkpath() { \
		while true ;\
		do \
			tmpdir=$$1 ;\
			[ -d $$tmpdir ] && break ;\
			while [ ! -d $$tmpdir ] ;\
			do \
				lastdir=$$tmpdir ;\
				tmpdir=`dirname $$tmpdir` ;\
			done ;\
			mkdir $$lastdir ;\
		done ;\
	} ;\
	for object in $(OBJECTS) ;\
	do \
		if entry=`grep "[ 	/]$$object[= 	]" $(PROTO)` ;\
		then \
			set -- $$entry ;\
			path=`eval echo $$3` ;\
			expr $$path : '[^/]' >/dev/null && \
				path=$(BASEDIR)/$$path ;\
			dir=$(ROOT)/$(MACH)`dirname $$path` ;\
			[ ! -d $$dir ] && mkpath $$dir ;\
			$(INS) -f $$dir -m $$4 -u $$5 -g $$6 $$object ;\
		else \
			echo "unable to install $$object" ;\
		fi ;\
	done

clobber: clean
	rm -f $(PROC)

clean:
	rm -f $(OBJ)

lintit:
	$(LINT) $(LINTFLAGS) $(SRC) $$i >lint.out 2>&1

HDRS=\
	$(INC)/stdio.h $(INC)/string.h $(INC)/errno.h \
	$(INC)/fmtmsg.h $(INC)/devmgmt.h $(INC)/signal.h $(INC)/sys/types.h
$(OBJ): $(HDRS)
