#ident	"@(#)xenv:i386/makefile	1.16"
#
#	Intel i386 GLOBAL MAKEFILE
#
include $(CMDRULES)

#	sgsrules contains the following:
#
#	SGS indicates the prefix to be applied to the installed commands.
#
#	ARCH indicates the architecture of the host machine
#		AR16WR=11/70, AR32WR=vax, AR32W=ibm, 3b20, etc
#
#	The following macros define the various installation directories.
#	Note that if relative pathnames are used, they must allow for the
#	"cd" issued prior to each make.
#
#	SGSBASE is the directory under which all the sgs source lives
#
#	CCSBIN is the directory for installing executable ("bin") files.
#	It should correspond to the same directory as specified in AS and LD
#	in paths.h.
#
#	CCSLIB is the directory for installing libraries and executable
#	files not intended for direct user execution (e.g., assembler pass1).
#	It should correspond to the same directory as specified in COMP,
#	OPTIM, AS1, and AS2 in paths.h.
#
#	Specify the byte order for this SGS instance.
#		FBO = Forward Byte Order (3B20, IBM)
#		RBO = Reverse Byte Order (DEC)
#
#	If host machine is running a pre-5.0 release of UNIX
#	then set Archive Format to "OLDAR".
#	Starting with the 5.0 UNIX (SVR1)  release the Archive Format
#	should be set to "PORTAR".
#	If building a 5.0 release on a host that is running
#	System V Rel. 2.0 (SVR2), then set ARFORMAT to PORT5AR.
#
#
#	Starting with the SVR2 release of UNIX,
#	if flexnames feature is desired, then set FLEX
#	to -DFLEXNAMES.
#	If host machine is running a pre-SVR2 release of UNIX
#	then set FLEX to null (ie. FLEX= ).
#
#	This is the machine ID field. The valid values are
#	u3b15, u3b5, u3b2, or i386.
#
#	The following parameter specifies the default include directory
#	for cpp. If not defined the value will be ROOT/usr/include.
#
#	USRLIB is used for installing libcrypt, its value is set in
#	sgs.mk to $(ROOT)/usr/lib
#
#
# The CCS by default produces dynamically-linked executables.
# By setting LINK_MODE to -dn, dynamic linking is turned off.
#
libsgs:
	cd $(SGSBASE)/libsgs/common; $(MAKE) -f libsgs.mk
tools:
	cd $(SGSBASE)/yacc/$(CPU); $(MAKE)
	cd $(SGSBASE)/lex/$(CPU); $(MAKE)
#
libs:
	cd $(SGSBASE)/libelf/$(CPU); $(MAKE)
	cd $(SGSBASE)/libdwarf2/$(CPU); $(MAKE)
	cd $(SGSBASE)/libld/$(CPU); $(MAKE)
#
.MUTEX: libsgs toolinstall libinstall
install: libsgs toolinstall libinstall
	cd $(SGSBASE)/ar/$(CPU); $(MAKE) install
	cd $(SGSBASE)/acpp/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/acomp/$(CPU); $(MAKE) build install
	cd $(SGSBASE)/acpp/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/alint/$(CPU); $(MAKE) build install
	cd $(SGSBASE)/cpp/$(CPU); $(MAKE) install \
		PD_SYS=D_unix PD_MACH=D_newmach NEW_MACH="-DPD_MY_MACH=\\\"$(CPU)\\\""
	cd $(SGSBASE)/cmd/$(CPU); $(MAKE) install
	cd $(SGSBASE)/optim/$(CPU); $(MAKE) install
	cd $(SGSBASE)/as/$(CPU); $(MAKE) install
	cd $(SGSBASE)/ld/$(CPU); $(MAKE) install
	cd $(SGSBASE)/ccsdemos; $(MAKE) install
	cd $(SGSBASE)/cplusinc; $(MAKE) install
# cplusbe is dependent on long long support in the CCS just built
	if test  "$(NATIVE)" = "yes" ; then \
		cd $(SGSBASE)/cplusfe/$(CPU); $(MAKE) install; \
		cd $(SGSBASE)/cplusbe/$(CPU); $(MAKE) install; \
		cd $(SGSBASE)/dump/$(CPU); $(MAKE) install; \
		cd $(SGSBASE)/prelink/$(CPU); $(MAKE) install; \
	else \
		rm -f $(CCSLIB)/acomp; \
		rm -f $(CCSLIB)/optim; \
		rm -f $(CCSBIN)/as;\
		rm -f $(CCSBIN)/ld; \
		ln -s $(CCSLIB)/$(CPU)acomp $(CCSLIB)/acomp; \
		ln -s $(CCSLIB)/$(CPU)optim $(CCSLIB)/optim; \
		ln -s $(CCSBIN)/$(CPU)as $(CCSBIN)/as; \
		ln -s $(CCSBIN)/$(CPU)ld $(CCSBIN)/ld; \
		cp $(USRINC)/limits.h $(SGSBASE)/inc/i386; \
		cd $(SGSBASE)/cplusfe/$(CPU); $(MAKE) CC="cc -Y02,$(CCSLIB) -Ya,$(CCSBIN) -YI,/usr/include" install; \
		cd $(SGSBASE)/cplusbe/$(CPU); $(MAKE) CC="cc -Y02,$(CCSLIB) -Ya,$(CCSBIN) -YI,/usr/include" install; \
		cd $(SGSBASE)/dump/$(CPU); $(MAKE) CC="cc -Y02,$(CCSLIB) -Ya,$(CCSBIN) -YI,/usr/include" install; \
		cd $(SGSBASE)/prelink/$(CPU); $(MAKE) CC="cc -Y02,$(CCSLIB) -Ya,$(CCSBIN) -YI,/usr/include" install; \
		rm -f $(SGSBASE)/inc/i386/limits.h; \
	fi
	cd $(SGSBASE)/demangler/$(CPU); $(MAKE) install
	cd $(SGSBASE)/dis/$(CPU); $(MAKE) install
	cd $(SGSBASE)/elfmark/$(CPU); $(MAKE) install
	cd $(SGSBASE)/lorder/$(CPU); $(MAKE) install
	cd $(SGSBASE)/lprof/$(CPU); $(MAKE) install
	cd $(SGSBASE)/m4/$(CPU); $(MAKE) install
	cd $(SGSBASE)/mcs/$(CPU); $(MAKE) install
	cd $(SGSBASE)/nm/$(CPU); $(MAKE) install
	cd $(SGSBASE)/size/$(CPU); $(MAKE) install
	cd $(SGSBASE)/strip/$(CPU); $(MAKE) install
	cd $(SGSBASE)/cof2elf/$(CPU); $(MAKE) install
	cd $(SGSBASE)/tsort/$(CPU); $(MAKE) install
	cd $(SGSBASE)/unix_conv/$(CPU); $(MAKE) install
	echo ""
	cp $(SGSBASE)/xenv/$(CPU)/env.tmp $(CCSBIN)/$(SGS)env
	cp $(SGSBASE)/xenv/$(CPU)/make.tmp $(CCSBIN)/make
	cp $(SGSBASE)/xenv/$(CPU)/$(CPU) $(CCSBIN)
	echo "Successfully installed tools."
#
toolinstall:
	cd $(SGSBASE)/yacc/$(CPU); $(MAKE) install
	cd $(SGSBASE)/lex/$(CPU); $(MAKE) install
	
#
libinstall:
	cd $(SGSBASE)/libelf/$(CPU); $(MAKE) install
	cd $(SGSBASE)/libdwarf2/$(CPU); $(MAKE) install
	cd $(SGSBASE)/libld/$(CPU); $(MAKE) install

#
libcopy:
	# save host version of libelf
	if [ -n "$(SGS)" ] ; \
	then \
		mv $(CCSLIB)/libelf.a $(CCSLIB)/libelf$(SGS).a;\
	fi
#
lintit:	toollintit liblintit
	cd $(SGSBASE)/ar/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/acomp/$(CPU); $(MAKE) lintit 
	cd $(SGSBASE)/cpp/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/cmd/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/optim/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/as/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/ld/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/dis/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/dump/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/elfmark/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/alint/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/lorder/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/lprof/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/m4/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/mcs/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/nm/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/size/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/strip/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/cof2elf/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/tsort/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/unix_conv/$(CPU); $(MAKE) lintit
#
toollintit:
	cd $(SGSBASE)/yacc/$(CPU); $(MAKE) lintit 
	cd $(SGSBASE)/lex/$(CPU); $(MAKE) lintit 
#
liblintit:
	cd $(SGSBASE)/libelf/$(CPU); $(MAKE) lintit
	cd $(SGSBASE)/libld/$(CPU); $(MAKE) lintit
#
clean: libsgsclean toolclean libclean
	cd $(SGSBASE)/ar/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/acomp/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/cpp/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/cmd/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/optim/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/as/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/ld/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/ccsdemos; $(MAKE) clean
	cd $(SGSBASE)/cplusinc; $(MAKE) clean
	cd $(SGSBASE)/cplusfe/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/cplusbe/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/demangler/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/dis/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/dump/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/elfmark/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/alint/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/lorder/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/lprof/$(CPU); $(MAKE) clean 
	cd $(SGSBASE)/m4/$(CPU); $(MAKE) clean 
	cd $(SGSBASE)/mcs/$(CPU); $(MAKE) clean 
	cd $(SGSBASE)/nm/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/prelink/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/size/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/strip/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/cof2elf/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/tsort/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/unix_conv/$(CPU); $(MAKE) clean
#
libsgsclean:
	cd $(SGSBASE)/libsgs/common; $(MAKE) -f libsgs.mk clean 
#
toolclean:
	cd $(SGSBASE)/yacc/$(CPU); $(MAKE) clean 
	cd $(SGSBASE)/lex/$(CPU); $(MAKE) clean 
#
libclean:
	cd $(SGSBASE)/libelf/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/libdwarf2/$(CPU); $(MAKE) clean
	cd $(SGSBASE)/libld/$(CPU); $(MAKE) clean
#
clobber: libsgsclobber toolclobber libclobber
	cd $(SGSBASE)/ar/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/acomp/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/cpp/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/cmd/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/optim/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/as/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/ld/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/ccsdemos; $(MAKE) clobber
	cd $(SGSBASE)/cplusinc; $(MAKE) clobber
	cd $(SGSBASE)/cplusfe/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/cplusbe/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/demangler/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/dis/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/dump/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/elfmark/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/alint/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/lorder/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/lprof/$(CPU); $(MAKE) clobber 
	cd $(SGSBASE)/m4/$(CPU); $(MAKE) clobber 
	cd $(SGSBASE)/mcs/$(CPU); $(MAKE) clobber 
	cd $(SGSBASE)/nm/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/prelink/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/size/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/strip/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/cof2elf/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/tsort/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/unix_conv/$(CPU); $(MAKE) clobber
#
libsgsclobber:
	cd $(SGSBASE)/libsgs/common; $(MAKE) -f libsgs.mk clobber 
#
toolclobber:
	cd $(SGSBASE)/yacc/$(CPU); $(MAKE) clobber 
	cd $(SGSBASE)/lex/$(CPU); $(MAKE) clobber 
#
libclobber:
	cd $(SGSBASE)/libelf/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/libdwarf2/$(CPU); $(MAKE) clobber
	cd $(SGSBASE)/libld/$(CPU); $(MAKE) clobber
