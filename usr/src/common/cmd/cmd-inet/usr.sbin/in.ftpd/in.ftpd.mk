#ident	"@(#)in.ftpd.mk	1.8"

include $(CMDRULES)

# Define VIRTUAL to include virtual FTP server support.
# Replace SPT_SCO with SPT_NONE to disable setproctitle functionality.
LOCALDEF=	-DINTL -DSYSV -DUXW -DVIRTUAL -DSPT_TYPE=SPT_SCO -DSENDV
LDLIBS=		-lsocket -lnsl -lgen -lcrypt -liaf
LINTFLAGS=	$(INCLIST) $(DEFLIST)
INSDIR=		$(USRSBIN)
CONFDIR=	$(ETC)/inet

SRCS=		ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c access.c\
		extensions.c realpath.c acl.c private.c authenticate.c\
		conversions.c hostacc.c strcasestr.c strsep.c getusershell.c

OBJS=		$(SRCS:.c=.o) security.o

all:		in.ftpd ftpcount ftpshut ckconfig pipeline

security.o:	../security.c
		$(CC) $(CFLAGS) -I.. $(INCLIST) $(DEFLIST) -c ../security.c

ftpcount_msg.h:	NLS/en/ftpcount.gen
		$(MKCATDEFS) ftpcount $? >/dev/null

ftpcount.o:	ftpcount_msg.h

ftpcount:	ftpcount.o vers.o
		$(CC) -o $@ $(LDFLAGS) ftpcount.o vers.o $(SHLIBS)
		rm -f ftpwho
		ln ftpcount ftpwho

ftpshut_msg.h:	NLS/en/ftpshut.gen
		$(MKCATDEFS) ftpshut $? >/dev/null

ftpshut.o:	ftpshut_msg.h

ftpshut:	ftpshut.o vers.o
		$(CC) -o $@ $(LDFLAGS) ftpshut.o vers.o $(SHLIBS)

ckconfig:	ckconfig.o
		$(CC) -o $@ $(LDFLAGS) ckconfig.o $(SHLIBS)

pipeline_msg.h:	NLS/en/pipeline.gen
		$(MKCATDEFS) pipeline $? >/dev/null

pipeline.o:	pipeline_msg.h

pipeline:	pipeline.o
		$(CC) -o $@ $(LDFLAGS) pipeline.o $(SHLIBS)

ftpd_msg.h:	NLS/en/ftpd.gen
		$(MKCATDEFS) ftpd $? >/dev/null

extensions.o ftpcmd.o ftpd.o glob.o hostacc.o private.o:	ftpd_msg.h

in.ftpd:	$(OBJS)
		$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
		[ -d $(CONFDIR) ] || mkdir -p $(CONFDIR)
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.ftpd
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ftpcount
		rm -f $(INSDIR)/ftpwho
		ln $(INSDIR)/ftpcount $(INSDIR)/ftpwho
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ftpshut
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) ftpaccess
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) ftpconversions
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) ftpgroups
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) ftphosts
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) ftpusers
		$(INS) -f $(CONFDIR) -m 0555 -u $(OWN) -g $(GRP) pipeline
		$(DOCATS) -d NLS $@

clean:
		rm -f $(OBJS) y.tab.c ftpcmd.c ftpcount.o ftpshut.o ckconfig.o pipeline.o
		rm -f pipeline_msg.h ftpcount_msg.h ftpshut_msg.h ftpd_msg.h

clobber:	clean
		rm -f in.ftpd ftpcount ftpwho ftpshut ckconfig pipeline
		$(DOCATS) -d NLS $@

lintit:
		$(LINT) $(LINTFLAGS) pipeline.c
