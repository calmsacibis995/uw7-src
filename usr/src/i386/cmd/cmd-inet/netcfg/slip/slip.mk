#ident	"@(#)slip.mk	1.4"
#
include $(CMDRULES)

INSDIR	=	$(ROOT)/$(MACH)/usr/lib/scoadmin/slip/slip.obj
FILES	=	activate activate.scoadmin shorthelp title

all install:
	@if [ ! -d $(INSDIR) ];\
	then\
		mkdir -p $(INSDIR);\
		mkdir -p $(INSDIR)/C;\
		mkdir -p $(INSDIR)/en;\
	fi;\
	for i in $(FILES);\
	do\
		if [ $$i = "shorthelp" -o \
		     $$i = "title" ]; \
		then\
			$(INS) -f $(INSDIR)/C ./$$i;\
			$(INS) -f $(INSDIR)/en ./$$i;\
		fi;\
		$(INS) -f $(INSDIR) ./$$i;\
	done;

clean lintit:

clobber: clean
