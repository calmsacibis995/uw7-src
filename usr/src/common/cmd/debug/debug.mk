#ident	"@(#)debugger:debug.mk	1.9"

# The "all" target builds both the command line interface
# and the graphical user interface"
# The "cli" and "gui" targets build one or the other

include $(CMDRULES)
include util/common/defs.make

all:
	$(MAKE) -f debugsrc.mk cli $(MAKEARGS)
	$(MAKE) -f debugsrc.mk gui $(MAKEARGS)
	$(MAKE) -f debugsrc.mk uw_cli $(MAKEARGS)
	$(MAKE) -f debugsrc.mk uw_gui $(MAKEARGS)
	$(MAKE) -f debugsrc.mk osr5_cli $(MAKEARGS)
	$(MAKE) -f debugsrc.mk osr5_gui $(MAKEARGS)

cli:
	$(MAKE) -f debugsrc.mk cli $(MAKEARGS)

osr5_cli:
	$(MAKE) -f debugsrc.mk osr5_cli $(MAKEARGS)

uw_cli:
	$(MAKE) -f debugsrc.mk uw_cli $(MAKEARGS)

gui:
	$(MAKE) -f debugsrc.mk gui $(MAKEARGS)

osr5_gui:
	$(MAKE) -f debugsrc.mk osr5_gui $(MAKEARGS)

uw_gui:
	$(MAKE) -f debugsrc.mk osr5_gui $(MAKEARGS)

install:
	$(MAKE) -f debugsrc.mk install $(MAKEARGS)

install_cli:
	$(MAKE) -f debugsrc.mk install_cli $(MAKEARGS)
	$(MAKE) -f debugsrc.mk install_uw_cli $(MAKEARGS)
	$(MAKE) -f debugsrc.mk install_osr5_cli $(MAKEARGS)

install_osr5_cli:
	$(MAKE) -f debugsrc.mk install_osr5_cli $(MAKEARGS)

install_uw_cli:
	$(MAKE) -f debugsrc.mk install_uw_cli $(MAKEARGS)

install_gui:
	$(MAKE) -f debugsrc.mk install_gui $(MAKEARGS)

install_osr5_gui:
	$(MAKE) -f debugsrc.mk install_osr5_gui $(MAKEARGS)

install_uw_gui:
	$(MAKE) -f debugsrc.mk install_uw_gui $(MAKEARGS)

lintit:
	$(MAKE) -f debugsrc.mk lintit $(MAKEARGS)

clean:
	$(MAKE) -f debugsrc.mk clean $(MAKEARGS)

clean_cli:
	$(MAKE) -f debugsrc.mk clean_cli $(MAKEARGS)

clean_gui:
	$(MAKE) -f debugsrc.mk clean_gui $(MAKEARGS)

clobber:
	$(MAKE) -f debugsrc.mk clobber $(MAKEARGS)

depend:
	$(MAKE) -f debugsrc.mk depend $(MAKEARGS)
