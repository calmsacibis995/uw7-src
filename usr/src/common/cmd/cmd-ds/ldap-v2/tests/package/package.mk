#ident	"@(#)package.mk	1.2"

LDAPTOP=     ../..
CURRENT_DIR= ./tests/package

include $(CMDRULES)
include $(LDAPTOP)/local.defs

all clean clobber lintit:		

package: 
	pkgmk -o -r $(ROOT)/$(MACH) -d `pwd`

image: package
	pkgtrans -s `pwd` ldaptest.image ldaptests
