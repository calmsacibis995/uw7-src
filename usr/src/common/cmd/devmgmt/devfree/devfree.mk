#	copyright	"%c%"

#ident	"@(#)devmgmt:common/cmd/devmgmt/devfree/devfree.mk	1.7.5.3"
#ident "$Header$"

include $(CMDRULES)

HDRS=$(INC)/stdio.h $(INC)/string.h $(INC)/errno.h $(INC)/fmtmsg.h $(INC)/devmgmt.h
FILE=devfree
INSTALLS=devfree
PROTO=../Prototype
SRC=main.c
OBJ=$(SRC:.c=.o)
LOCALINC=-I.
LDLIBS=-ladm
LINTFLAGS=$(DEFLIST)

all		: $(FILE) 

install		: all
		@eval `sed -e '/^![^=]*=/!d' -e 's/^!//' $(PROTO)` ;\
		for object in $(INSTALLS) ;\
		do \
		    if entry=`grep "[ 	/]$$object[= 	]" $(PROTO)` ;\
		    then \
			set -- $$entry ;\
			path=`eval echo $$3` ;\
			if expr $$path : '[^/]' >/dev/null ;\
			then \
			    path=$(BASEDIR)/$$path ;\
			fi ;\
			dir=$(ROOT)/$(MACH)`dirname $$path` ;\
			if [ ! -d $$dir ] ;\
			then \
			    mkdir -p $$dir ;\
			fi ;\
			$(INS) -f $$dir -m $$4 -u $$5 -g $$6 $$object ;\
		    else \
			echo "Unable to install $$object" ;\
		    fi ;\
		done

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

lintit		:
		for i in $(SRC); \
		do \
		    $(LINT) $(LINTFLAGS) $$i; \
		done

$(FILE)		: $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(OBJ)		: $(HDRS)
