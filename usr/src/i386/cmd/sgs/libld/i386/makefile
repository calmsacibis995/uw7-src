#ident	"@(#)libld:i386/makefile	1.4"
#
#	i386 LIBLD MAKEFILE
#
include $(CMDRULES)
#
#
#	Host Dependent Parameters
#	ARFORMAT = archive format
#		OLDAR	= old format
#		PORT5AR	= 5.0 random access format
#		PORTAR	= 6.0 portable ascii format
#	(defined in rulefile)
#
DEFLIST	= -D$(ARFORMAT) -D$(ARCH) $(FLEX)
INCLIST	= -I$(LIBLDCOM) -I$(COMINC)
ENV	= 
FRC	=
CC_CMD	= $(CC) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(ENV)
#
ARFLAGS	= r
AR_CMD	= $(AR) $(ARFLAGS)
#
O	= o
#
#	External Directories
#
#	Internal Directories
#
LIBLDCOM = $(SGSBASE)/libld/common
#
OBJECTS = ldaclose.$O ldaopen.$O ldclose.$O ldahread.$O ldlseek.$O ldnlseek.$O \
	ldnrseek.$O ldnsseek.$O ldnshread.$O ldopen.$O ldrseek.$O ldlread.$O \
	ldsseek.$O ldshread.$O ldfhread.$O ldtbseek.$O ldohseek.$O ldtbread.$O \
	ldtbindex.$O ldgetname.$O allocldptr.$O freeldptr.$O vldldptr.$O \
	sgetl.$O sputl.$O
#
CFILES = $(LIBLDCOM)/ldaclose.c $(LIBLDCOM)/ldaopen.c $(LIBLDCOM)/ldclose.c \
	$(LIBLDCOM)/ldahread.c $(LIBLDCOM)/ldlseek.c $(LIBLDCOM)/ldnlseek.c \
	$(LIBLDCOM)/ldnrseek.c $(LIBLDCOM)/ldnsseek.c $(LIBLDCOM)/ldnshread.c \
	$(LIBLDCOM)/ldopen.c $(LIBLDCOM)/ldrseek.c $(LIBLDCOM)/ldlread.c \
	$(LIBLDCOM)/ldsseek.c $(LIBLDCOM)/ldshread.c $(LIBLDCOM)/ldfhread.c \
	$(LIBLDCOM)/ldtbseek.c $(LIBLDCOM)/ldohseek.c $(LIBLDCOM)/ldtbread.c \
	$(LIBLDCOM)/ldtbindex.c $(LIBLDCOM)/allocldptr.c \
	$(LIBLDCOM)/freeldptr.c $(LIBLDCOM)/vldldptr.c $(LIBLDCOM)/sgetl.c \
	$(LIBLDCOM)/sputl.c $(LIBLDCOM)/ldgetname.c
#
HEADERS	= $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(COMINC)/scnhdr.h \
	  $(COMINC)/syms.h $(COMINC)/storclass.h $(COMINC)/linenum.h \
	  $(LIBLDCOM)/lddef.h $(LIBLDCOM)/synsyms.h
#
all:	libld.a
#
libld.a: $(OBJECTS) $(FRC)
	$(RM) -f libld.a
	$(AR_CMD) libld.a $(OBJECTS)
#
allocldptr.$O:	$(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/lddef.h \
		$(LIBLDCOM)/allocldptr.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/allocldptr.c
#
freeldptr.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/lddef.h \
	      $(LIBLDCOM)/freeldptr.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/freeldptr.c
#
vldldptr.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/lddef.h \
	     $(LIBLDCOM)/vldldptr.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/vldldptr.c
#
ldaclose.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldaclose.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldaclose.c
#
ldaopen.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldaopen.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldaopen.c
#
ldclose.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldclose.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldclose.c
#
ldahread.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldahread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldahread.c
#
ldlseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
		$(LIBLDCOM)/ldlseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldlseek.c
#
ldnlseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	     $(LIBLDCOM)/ldnlseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldnlseek.c
#
ldnrseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	     $(LIBLDCOM)/ldnrseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldnrseek.c
#
ldnsseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	     $(LIBLDCOM)/ldnsseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldnsseek.c
#
ldnshread.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	      $(LIBLDCOM)/ldnshread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldnshread.c
#
ldopen.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldopen.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldopen.c
#
ldrseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMINC)/ldfcn.h $(LIBLDCOM)/ldrseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldrseek.c
#
ldtbread.$O:  $(COMINC)/filehdr.h $(COMINC)/syms.h $(COMINC)/storclass.h \
	     $(COMINC)/ldfcn.h $(LIBLDCOM)/ldtbread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldtbread.c
#
ldsseek.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h \
		$(COMINC)/ldfcn.h $(LIBLDCOM)/ldsseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldsseek.c
#
ldshread.$O:  $(COMINC)/filehdr.h $(COMINC)/scnhdr.h $(COMINC)/ldfcn.h \
	     $(LIBLDCOM)/ldshread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldshread.c
#
ldfhread.$O:  $(COMINC)/filehdr.h $(COMINC)/ldfcn.h $(LIBLDCOM)/ldfhread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldfhread.c
#
ldtbseek.$O:  $(COMINC)/ldfcn.h $(COMINC)/filehdr.h $(LIBLDCOM)/ldtbseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldtbseek.c
#
ldohseek.$O:  $(COMINC)/ldfcn.h $(COMINC)/filehdr.h $(LIBLDCOM)/ldohseek.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldohseek.c
#
ldtbindex.$O:  $(COMINC)/ldfcn.h $(COMINC)/filehdr.h $(COMINC)/syms.h \
	      $(COMINC)/storclass.h $(LIBLDCOM)/ldtbindex.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldtbindex.c
#
ldlread.$O:  $(COMINC)/ldfcn.h $(COMINC)/filehdr.h \
	    $(COMINC)/scnhdr.h $(COMINC)/syms.h \
	    $(COMINC)/linenum.h $(COMINC)/storclass.h \
	    $(LIBLDCOM)/ldlread.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldlread.c
#
sgetl.$O:  $(LIBLDCOM)/sgetl.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/sgetl.c
#
sputl.$O:  $(LIBLDCOM)/sputl.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/sputl.c
#
ldgetname.$O:  $(COMINC)/ldfcn.h $(COMINC)/filehdr.h $(COMINC)/syms.h \
	      $(COMINC)/storclass.h $(LIBLDCOM)/ldgetname.c $(FRC)
	$(CC_CMD) $(LIBLDCOM)/ldgetname.c
#
install:	$(CCSLIB)/libld.a
#
$(CCSLIB)/libld.a:	libld.a
	sh $(SGSBASE)/sgs.install 644 $(OWN) $(GRP) $(CCSLIB)/libld.a libld.a
#
save:
	$(RM) -f $(CCSLIB)/libld.a.back
	$(CP) $(CCSLIB)/libld.a $(CCSLIB)/libld.a.back
#
uninstall:	$(CCSLIB)/libld.a.back
	$(RM) -f $(CCSLIB)/libld.a
	$(CP) $(CCSLIB)/libld.a.back $(CCSLIB)/libld.a
#
shrink:
	$(RM) -f $(OBJECTS) libld.a
#
clean:
	$(RM) -f $(OBJECTS)
#
clobber:	clean
	$(RM) -f libld.a
#
lint:	$(HEADERS) $(CFILES)
	$(LINT_CMD) $(DEFLIST) $(INCLIST) $(CFILES)
#
llib-lld.ln:	$(LIBLDCOM)/llib-lld.c
	$(LINT) $(LIBLDCOM)/llib-lld.c -c
#
listing:	$(HEADERS) $(CFILES)
	$(PRINT) $(PRFLAGS) $(HEADERS) $(SOURCE) | $(LP) $(LPFLAGS)
#
FRC:
