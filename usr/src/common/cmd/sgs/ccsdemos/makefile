#ident	"@(#)ccsdemos:makefile	1.4"

# This is an install-only makefile.  

include $(CMDRULES)

# This should contain the names of all subdirectories
DEMOS = thr_demos sc_demos

INSDIR = $(CCSBIN)/../demos

all:

install: $(INSDIR) 
	cp -r $(DEMOS) $(INSDIR)

clean:

clobber:
	$(RM) -rf $(INSDIR)

$(INSDIR) :
	mkdir -p $(@)
