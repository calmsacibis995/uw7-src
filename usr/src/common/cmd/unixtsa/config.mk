#ident	"@(#)config.mk	1.2"
###
#
#  name		config.mk - configuration makefile for unixtsa
#		@(#)config.mk	1.2	10/9/96
#
###

include $(CMDRULES)

OPTBIN		=	$(ROOT)/$(MACH)/opt/bin
#INC=.
LOCALINC=-I$(TOP)/include
#GLOBALDEF=-DUNIX -DSYSV -DSVR4 -DTLI -DDEBUG
GLOBALDEF=-DUNIX -DSYSV -DSVR4 -DTLI
LIBDIR=$(TOP)/lib
BINDIR=$(USRSBIN)

LIBLIST=-L$(LIBDIR) -lsms -lsocket -lnsl $(LDLIBS) $(PERFLIBS)

lintit:

do_subdirs ::
	@for Dir in $(SUBDIRS); do \
		if [ -d $$Dir ]; then\
			(cd $$Dir; echo ">>> [`pwd`] $(MAKE) $(MAKEARGS) -f `basename $$Dir.mk` $(MTARG)"; $(MAKE) $(MAKEARGS) -f `basename $$Dir.mk` $(MTARG)) \
		fi;\
	done

# a frequently usable clean rule
do_clean ::
	-$(RM) -f *.o core nohup.out lint.out *.ln *.lint

# a frequently usable clobber rule
do_clobber :: 
	-$(RM) -f $(TARGETS) $(TARGET)

.C.o:
	$(C++C) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<
