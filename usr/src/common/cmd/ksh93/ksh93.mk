#ident	"@(#)ksh93:ksh93.mk	1.1"
all:
	cd src; $(MAKE)

clean:
	cd src; $(MAKE) clean

clobber: 
	cd src; $(MAKE) clobber

install: all
	cd src; $(MAKE) install
