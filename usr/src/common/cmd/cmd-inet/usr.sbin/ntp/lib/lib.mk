#ident "@(#)lib.mk	1.4"

include $(LIBRULES)

LIB= libntp.a

LOCALINC=-I../include
LOCALDEF= -DSYS_UNIXWARE2 -DHAVE_TERMIOS \
	  -DCONFIG_FILE=\"/etc/inet/ntp.conf\" -DMD5 \
	  -DDEBUG -DREFCLOCK -DDEBUG

SRCS=	atoint.c atolfp.c atouint.c auth12crypt.c authdecrypt.c \
	authencrypt.c authkeys.c authparity.c authreadkeys.c authusekey.c \
	buftvtots.c caljulian.c calleapwhen.c caltontp.c calyearstart.c \
	clocktime.c dofptoa.c dolfptoa.c emalloc.c fptoa.c fptoms.c getopt.c \
	gettstamp.c hextoint.c hextolfp.c humandate.c inttoa.c \
	lib_strbuf.c mfptoa.c mfptoms.c modetoa.c mstolfp.c \
	msutotsf.c numtoa.c refnumtoa.c numtohost.c octtoint.c \
	prettydate.c ranny.c tsftomsu.c tstotv.c tvtoa.c tvtots.c \
	uglydate.c uinttoa.c utvtoa.c machines.c clocktypes.c \
	md5.c a_md5encrypt.c a_md5decrypt.c \
	a_md512crypt.c decodenetnum.c systime.c msyslog.c syssignal.c \
	findconfig.c netof.c statestr.c

OBJS = $(SRCS:.c=.o)

all install : $(LIB)

$(LIB):	$(OBJS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIB)
