#ident	"@(#)debugger:tutorial.d/i386/makefile	1.6"

include $(CMDRULES)

include ../../util/common/std.make

FILES1 = ../common/files \
	../common/macro.c.doc \
	../common/macro.h.doc \
	../common/main.c.doc \
	../common/Macro.c.doc \
	../common/Main.c.doc \
	../common/sget.c.doc \
	../common/test1 \
	../common/test2 \
	../common/walk.c.doc \
	../common/traverse.doc

FILES2 = file1 \
	s.main.c \
	../common/testfile

TARGET = file1 main.c s.main.c

all:	$(FILES1) $(FILES2)

s.main.c:	../common/main.c.doc
	sed -e '1s/".*"/"\%\W\%"/' ../common/main.c.doc >main.c
	admin -n -imain.c s.main.c

file1:
	touch file1

install:	$(FILES1) $(FILES2)
	if [ ! -d $(CCSLIB)/tutorial ]; then \
		mkdir $(CCSLIB)/tutorial; \
	fi
	rm -rf $(CCSLIB)/tutorial/*
	cp ../common/files $(CCSLIB)/tutorial/files
	cp ../common/macro.c.doc $(CCSLIB)/tutorial/macro.c
	cp ../common/macro.h.doc $(CCSLIB)/tutorial/macro.h
	cp ../common/main.c.doc $(CCSLIB)/tutorial/main.c
	cp ../common/Macro.c.doc $(CCSLIB)/tutorial/Macro.c
	cp ../common/macro.h.doc $(CCSLIB)/tutorial/Macro.h
	cp ../common/Main.c.doc $(CCSLIB)/tutorial/Main.c
	cp ../common/sget.c.doc $(CCSLIB)/tutorial/sget.c
	cp ../common/test1 $(CCSLIB)/tutorial/test1
	cp ../common/test2 $(CCSLIB)/tutorial/test2
	cp ../common/walk.c.doc $(CCSLIB)/tutorial/walk.c
	cp ../common/traverse.doc $(CCSLIB)/tutorial/traverse.c
	$(CH)chmod 444 $(CCSLIB)/tutorial/*
	mkdir $(CCSLIB)/tutorial/src
	cp $(FILES2) $(CCSLIB)/tutorial/src
	$(CH)chmod 444 $(CCSLIB)/tutorial/src/*

install_uw:	$(FILES1) $(FILES2)
	if [ ! -d $(UW_CCSLIB)/tutorial ]; then \
		mkdir $(UW_CCSLIB)/tutorial; \
	fi
	rm -rf $(UW_CCSLIB)/tutorial/*
	cp ../common/files $(UW_CCSLIB)/tutorial/files
	cp ../common/macro.c.doc $(UW_CCSLIB)/tutorial/macro.c
	cp ../common/macro.h.doc $(UW_CCSLIB)/tutorial/macro.h
	cp ../common/main.c.doc $(UW_CCSLIB)/tutorial/main.c
	cp ../common/Macro.c.doc $(UW_CCSLIB)/tutorial/Macro.c
	cp ../common/macro.h.doc $(UW_CCSLIB)/tutorial/Macro.h
	cp ../common/Main.c.doc $(UW_CCSLIB)/tutorial/Main.c
	cp ../common/sget.c.doc $(UW_CCSLIB)/tutorial/sget.c
	cp ../common/test1 $(UW_CCSLIB)/tutorial/test1
	cp ../common/test2 $(UW_CCSLIB)/tutorial/test2
	cp ../common/walk.c.doc $(UW_CCSLIB)/tutorial/walk.c
	cp ../common/traverse.doc $(UW_CCSLIB)/tutorial/traverse.c
	$(CH)chmod 444 $(UW_CCSLIB)/tutorial/*
	mkdir $(UW_CCSLIB)/tutorial/src
	cp $(FILES2) $(UW_CCSLIB)/tutorial/src
	$(CH)chmod 444 $(UW_CCSLIB)/tutorial/src/*

install_osr5:	$(FILES1) $(FILES2)
	if [ ! -d $(OSR5_CCSLIB)/tutorial ]; then \
		mkdir $(OSR5_CCSLIB)/tutorial; \
	fi
	rm -rf $(OSR5_CCSLIB)/tutorial/*
	cp ../common/files $(OSR5_CCSLIB)/tutorial/files
	cp ../common/macro.c.doc $(OSR5_CCSLIB)/tutorial/macro.c
	cp ../common/macro.h.doc $(OSR5_CCSLIB)/tutorial/macro.h
	cp ../common/main.c.doc $(OSR5_CCSLIB)/tutorial/main.c
	cp ../common/Macro.c.doc $(OSR5_CCSLIB)/tutorial/Macro.c
	cp ../common/macro.h.doc $(OSR5_CCSLIB)/tutorial/Macro.h
	cp ../common/Main.c.doc $(OSR5_CCSLIB)/tutorial/Main.c
	cp ../common/sget.c.doc $(OSR5_CCSLIB)/tutorial/sget.c
	cp ../common/test1 $(OSR5_CCSLIB)/tutorial/test1
	cp ../common/test2 $(OSR5_CCSLIB)/tutorial/test2
	cp ../common/walk.c.doc $(OSR5_CCSLIB)/tutorial/walk.c
	cp ../common/traverse.doc $(OSR5_CCSLIB)/tutorial/traverse.c
	$(CH)chmod 444 $(OSR5_CCSLIB)/tutorial/*
	mkdir $(OSR5_CCSLIB)/tutorial/src
	cp $(FILES2) $(OSR5_CCSLIB)/tutorial/src
	$(CH)chmod 444 $(OSR5_CCSLIB)/tutorial/src/*
