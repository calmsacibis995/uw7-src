#ident	"@(#)Makefile	1.3"
#ident  "$Header$"
#
# Makefile for LP private libraries
#

include $(CMDRULES)

TOP	=	..

include ../common.mk


##########
#
# If you add directories other than one level deep
# you'll have to change the code below.
##########
LIBDIRS	= \
		oam \
		access \
		bsd \
		class \
		filters \
		forms \
		lp \
		lpNet \
		msgs \
		nuc \
		requests \
		secure \
		printers \
		systems \
		users


########
#  LINTDIRS includes all the directories in lib/ excluding bsd/ and lpNet.
#  When bsd/ and lpNet are made lint free LINTDIRS can be replaced by LIBDIRS.
########
LINTDIRS	= \
		./oam \
		./access \
		./class \
		./filters \
		./forms \
		./lp \
		./msgs \
		./requests \
		./secure \
		./printers \
		./systems \
		./users

all: $(LPINC)/oam_def.h
	@for dir in $(LIBDIRS); do\
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) DEBUG="$(DEBUG)" $(MAKEARGS) $@; \
			cd ..; \
		fi; \
	done

$(LPINC)/oam_def.h:
	cd ./oam ; $(MAKE) DEBUG="$(DEBUG)" $(MAKEARGS) ../$(LPINC)/oam_def.h

install: $(LPINC)/oam_def.h
	for dir in $(LIBDIRS);do \
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) DEBUG="$(DEBUG)" $(MAKEARGS) $@; \
			cd ..; \
		fi; \
	done ;\
	[ -d $(USRLIB)/locale/C/MSGFILES ] || mkdir -p $(USRLIB)/locale/C/MSGFILES
	cp ./oam/lp.messages $(USRLIB)/locale/C/MSGFILES/lp.str

clean clobber:
	for dir in $(LIBDIRS);do \
		if [ -d $$dir ]; then \
			cd $$dir; \
			$(MAKE) $(MAKEARGS) $@; \
			cd ..; \
		fi; \
	done

strip:

lintit lintsrc:
	for dir in $(LINTDIRS); \
	do \
		cd $$dir; \
		$(MAKE) DEBUG="$(DEBUG)" FUNCDLC="$(FUNCDCL)" LINT="$(LINT)" $(MAKEARGS) $@; \
		cd ..; \
	done

lintlib:
	for dir in $(LINTDIRS); \
	do \
		cd $$dir; \
		$(MAKE) DEBUG="$(DEBUG)" FUNCDLC="$(FUNCDCL)" LINT="$(LINT)" $(MAKEARGS) $@; \
		cd ..; \
	done 
	$(LINT) -abhmuvxy -o lplib $(LINTLN)

libs:		all
