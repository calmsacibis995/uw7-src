#ident	"@(#)r5misc:Makefile	1.8"

       SHELL = /bin/sh
          RM = rm -f
          MV = mv
        MAKE = make
         TOP = .
 CURRENT_DIR = .
   CONFIGSRC = $(TOP)/config
    IMAKESRC = $(CONFIGSRC)
    IRULESRC = $(CONFIGSRC)
       IMAKE = $(IMAKESRC)/imake
   IMAKE_CMD = $(NEWTOP)$(IMAKE) -I$(NEWTOP)$(IRULESRC) $(IMAKE_DEFINES)
BOOTSTRAPCFLAGS = -Dusl
       FLAGS = $(MFLAGS) -f Makefile.ini BOOTSTRAPCFLAGS="$(BOOTSTRAPCFLAGS)"

all:
	@echo "Please use make World, and set BOOTSTRAPCFLAGS if necessary."
	@echo "Do not name your log file make.log or it will be deleted."
	@echo "Check the configuration parameters in the config directory,
	@echo "and read the release notes carefully before proceeding."
	@echo "If the top-level Makefile gets corrupted, copy Makefile.ori
	@echo "to Makefile and try again."

World::
	cd $(IMAKESRC); $(MAKE) $(FLAGS) clean; $(MAKE) $(FLAGS)
	-$(RM) Makefile.bak; $(MV) Makefile Makefile.bak
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR) -I./config 
	$(MAKE) $(MFLAGS) all 

install::
	cd $(IMAKESRC); $(MAKE) $(FLAGS) clean; $(MAKE) $(FLAGS)
	-$(RM) Makefile.bak; $(MV) Makefile Makefile.bak
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR) -I./config 
	$(MAKE) $(MFLAGS) install

Makefile::
	cd $(IMAKESRC); $(MAKE) $(FLAGS) clean; $(MAKE) $(FLAGS)
	-$(RM) Makefile.bak; $(MV) Makefile Makefile.bak
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR) -I./config 
