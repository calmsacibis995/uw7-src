#ident	"@(#)in.gated.mk	1.8"
#ident	"$Header$"
#
#	in.gated.mk for GateD R3-5-7
#
# Gated Release 3.5
# Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.  All
# rights reserved.  Refer to Particulars and other Copyright notices at
# the end of this file. 
#

include	$(CMDRULES)

INSDIR	= $(USRSBIN)
CONFDIR	= $(ETC)/inet
MIBDIR	= $(ETC)/netmgt
OWN	= bin
GRP	= bin

.MUTEX:	lexer.c parser.h

SRCS=	checksum.c grand.c if.c inet.c krt.c krt_rtread_kinfo.c \
	krt_ifread_kinfo.c krt_rt_sock.c \
	krt_ipmulti_nocache.c krt_symbols.c policy.c \
	rt_aggregate.c rt_mib.c rt_radix.c rt_redirect.c rt_static.c \
	rt_table.c sockaddr.c str.c targets.c task.c trace.c \
	parse.c aspath.c asmatch.c bgp.c bgp_init.c bgp_rt.c \
	bgp_sync.c bgp_mib.c egp.c egp_init.c egp_rt.c egp_mib.c \
	icmp.c ospf_build_dbsum.c ospf_build_ls.c ospf_choose_dr.c \
	ospf_conf.c ospf_flood.c ospf_init.c ospf_log.c ospf_lsdb.c \
	ospf_mib.c ospf_newq.c ospf_rt.c ospf_rtab.c ospf_rxlinkup.c \
	ospf_rxmon.c ospf_rxpkt.c ospf_spf.c ospf_spf_leaves.c \
	ospf_states.c ospf_tqhandle.c ospf_trace.c ospf_txpkt.c \
	rdisc.c rip.c rip_mib.c snmp_isode.c parser.c
OBJS=	$(SRCS:.c=.o) lexer.o version.o

C_SRCS=	signames.c flock.c IEFBR14.c
C_OBJS=	$(C_SRCS:.c=.o)

R_SRCS=	ripquery.c
R_OBJS=	ripquery.o checksum.o str.o standalone.o

O_SRCS=	ospf_monitor.c
O_OBJS=	ospf_monitor.o checksum.o str.o standalone.o

G_SRCS=	gdc.c standalone.c
G_OBJS=	gdc.o str.o standalone.o

HDRS=	defs.h config.h if.h include.h inet.h krt.h krt_var.h \
	krt_ipmulti.h policy.h rt_table.h rt_var.h sockaddr.h \
	str.h targets.h task.h trace.h parse.h unix.h aspath.h \
	asmatch.h bgp.h bgp_proto.h bgp_var.h egp.h egp_param.h \
	icmp.h ip_icmp.h ospf.h ospf_const.h ospf_gated.h \
	ospf_log.h ospf_lsdb.h ospf_pkts.h ospf_rtab.h \
	ospf_timer_calls.h rdisc.h rip.h snmp_isode.h paths.h

DEFS=	smi.defs mib.defs rt.defs bgp.defs ospf.defs rip.defs


#  Program names
GATED		= in.gated
GDC		= gdc
RIPQUERY	= ripquery
OSPF_MONITOR	= ospf_monitor

# Optional progams we use
TAGS=etags

#  Programs we use
AWK	= awk
CAT	= cat
DATE	= date
MV	= mv
RM	= rm
SED	= sed
#MOSY	= $(TOOLS)/bin/mosy
SIGNAL_H= $(INC)/sys/signal.h

#  Stuff for gdc
GDC_MODE=4750
GDC_GROUP=gdmaint
GDC_USER=root

# Stuff for ospf_monitor
OSPFM_MODE=4755
OSPFM_USER=root

LOCALDEF	= -DSCO_GEMINI
LDLIBS		= -lsocket -lnsl -lresolv -lgen
SNMP_LIBS	= -lsmux -lsnmp -lsnmpio

LINTFLAGS	= -hx
YFLAGS		= -dv

LIBGATED=libgated.a


#
#  The default is to build gated and ripquery
#

all:	$(GATED) $(RIPQUERY) $(OSPF_MONITOR) $(GDC)


#	Rules for gated
#	
${GATED}: ${OBJS} ${LIBGATED}
	@${CC} ${CFLAGS} ${CWFLAGS} ${OBJS} -o ${GATED} ${LDLIBS} ${LIBGATED} ${SNMP_LIBS}

lint::
	${LINT} ${LINTFLAGS} ${SRCS} version.c |\
		${SED} -e '/.*math.h.*/d' -e '/.*floatingpoint.h.*/d' -e '/trace.*used inconsistently.*/d' \
		-e '/[sf]printf.*used inconsistently.*/d' -e '/.*warning: possible pointer alignment problem.*/d'


clean::
	-${RM} -f ${OBJS}

clobber::	clean
	-rm -f $(GATED)



#
#  Build sys_signame[] if not available on this system.
#

signames.c:	sigconv.awk ${SIGNAL_H}
	@${AWK} -f sigconv.awk < ${SIGNAL_H} > signames.c

clean::
	-${RM} -f signames.c


#
#	Build parser
#

parser:	parser.c lexer.c parse.c

parser.c parser.h:
	@${RM} -f parser.c parser.h
	@$(YACC) $(YFLAGS) parser.y
	@${MV} y.tab.c parser.c
	@${MV} y.tab.h parser.h


lexer.c:	lexer.l
	@${RM} -f lexer.c
	@${LEX} ${LFLAGS} lexer.l
	@${MV} lex.yy.c lexer.c

clean::
	-${RM} -f y.tab.* y.output

clobber::
	-${RM} -f lexer.c parser.c parser.h

#
#  Build version ID from RCS info in file headers
#

version:	version.c

version.c:	VERSION
	@DATE=`${DATE}` ; \
		VERSION="`${CAT} VERSION`" ; \
		echo '#include "include.h"' > version.c ; \
		echo >> version.c ; \
		echo 'const char *gated_version = "'$${VERSION}'";' >> version.c ; \
		echo 'const char *build_date = "'$${DATE}'";' >> version.c ; \
		echo >> version.c ; \
		${CAT} version.c ; \
		echo >> version.c ; \
		echo 'const char *version_id = "@(#)${GATED} '$${VERSION}' '$${DATE}'";' >> version.c ; \
		echo >> version.c ;

clean::
	-${RM} -f version.c


#
#  Build the ripquery program
#

${RIPQUERY}:	${R_OBJS} ${LIBGATED}
	@${CC} ${CFLAGS} ${CWFLAGS} ${R_OBJS} -o ${RIPQUERY} ${LDLIBS} ${LIBGATED}

lint::
	${LINT} ${LINTFLAGS} ${R_SRCS}

clean::
	-${RM} -f ${R_OBJS}

clobber::
	-rm -f $(RIPQUERY)



#
#	Build the OSPF monitor program
#

${OSPF_MONITOR}:	${O_OBJS} ${LIBGATED}
	@${CC} ${CFLAGS} ${CWFLAGS} ${O_OBJS} -o ${OSPF_MONITOR} ${LDLIBS} ${LIBGATED}

lint::
	${LINT} ${LINTFLAGS} ${O_SRCS}

clean::
	-${RM} -f ${O_OBJS}

clobber::
	-rm -f $(OSPF_MONITOR)


#
#  Build the gdc program
#

${GDC}:	${G_OBJS} ${LIBGATED}
	@${CC} ${CFLAGS} ${CWFLAGS} ${G_OBJS} -o ${GDC} ${LDLIBS} ${LIBGATED}

lint::
	${LINT} ${LINTFLAGS} ${G_SRCS}

clean::
	-${RM} -f ${G_OBJS}

clobber::
	-rm -f $(GDC)


#
#	Build the emacs TAGS file
#

tags:	TAGS

TAGS:	${SRCS} ${HDRS}
	@echo "Building:	TAGS"
	@${TAGS} ${SRCS} ${HDRS}

clean::
	-${RM} -f TAGS

#
#	Compatibility library
#

${LIBGATED}: ${C_OBJS}
	${AR} rc ${@} ${?}

clean::
	-${RM} -f ${C_OBJS}

clobber::
	-rm -f $(LIBGATED)


#
#	SNMP
#

.SUFFIXES:	.my .defs

.my.defs:
	$(MOSY) -s -o - $< > $@ || rm $@

#gated-mib.defs:	${DEFS}
#	@${CAT} ${DEFS} > gated-mib.defs

#clean::
#	-rm -f gated-mib.defs

clobber::
	-rm -f $(DEFS)


#
#	INSTALL
#
install:	all gated-mib.defs
	@-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@-[ -d $(MIBDIR) ] || mkdir -p $(MIBDIR)
	@-[ -d $(CONFDIR) ] || mkdir -p $(CONFDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.gated
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ripquery
	$(INS) -f $(INSDIR) -m $(GDC_MODE) -u $(OWN) -g $(GRP) gdc
	$(INS) -f $(INSDIR) -m $(OSPFM_MODE) -u $(OSPFM_USER) -g $(GRP) ospf_monitor
	$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.bgp
	$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.egp
	$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.rip
	$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.ospf
	$(INS) -f $(MIBDIR) -m 0444 -u $(OWN) -g $(GRP) gated-mib.defs

lintit:	lint



#
# ------------------------------------------------------------------------
# 
# 	GateD, Release 3.5
# 
# 	Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
# 	    All rights reserved.
# 
# 	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
# 	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
# 	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# 	AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# 	Royalty-free licenses to redistribute GateD Release
# 	3 in whole or in part may be obtained by writing to:
# 
# 	    GateDaemon Project
# 	    Information Technologies/Network Resources
# 	    200 CCC
# 	    Cornell University
# 	    Ithaca, NY  14853-2601  USA
# 
# 	GateD is based on Kirton's EGP, UC Berkeley's routing
# 	daemon	 (routed), and DCN's HELLO routing Protocol.
# 	Development of GateD has been supported in part by the
# 	National Science Foundation.
# 
# 	Please forward bug fixes, enhancements and questions to the
# 	gated mailing list: gated-people@gated.cornell.edu.
# 
# ------------------------------------------------------------------------
# 
#       Portions of this software may fall under the following
#       copyrights:
# 
# 	Copyright (c) 1988 Regents of the University of California.
# 	All rights reserved.
# 
# 	Redistribution and use in source and binary forms are
# 	permitted provided that the above copyright notice and
# 	this paragraph are duplicated in all such forms and that
# 	any documentation, advertising materials, and other
# 	materials related to such distribution and use
# 	acknowledge that the software was developed by the
# 	University of California, Berkeley.  The name of the
# 	University may not be used to endorse or promote
# 	products derived from this software without specific
# 	prior written permission.  THIS SOFTWARE IS PROVIDED
# 	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
# 	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# 	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

#
# mkdep.build,v 1.14.2.5 1995/05/01 16:05:37 jch Exp
#
# This file automatically generated
#
IEFBR14.o:	IEFBR14.c
IEFBR14.o:	include.h
IEFBR14.o:	defines.h
IEFBR14.o:	paths.h
IEFBR14.o:	config.h
IEFBR14.o:	defs.h
IEFBR14.o:	sockaddr.h
IEFBR14.o:	str.h
IEFBR14.o:	policy.h
IEFBR14.o:	rt_table.h
IEFBR14.o:	if.h
IEFBR14.o:	task.h
IEFBR14.o:	trace.h
IEFBR14.o:	asmatch.h
IEFBR14.o:	aspath.h
asmatch.o:	asmatch.c
asmatch.o:	include.h
asmatch.o:	defines.h
asmatch.o:	paths.h
asmatch.o:	config.h
asmatch.o:	defs.h
asmatch.o:	sockaddr.h
asmatch.o:	str.h
asmatch.o:	aspath.h
asmatch.o:	asmatch.h
asmatch.o:	policy.h
asmatch.o:	rt_table.h
asmatch.o:	if.h
asmatch.o:	task.h
asmatch.o:	trace.h
asmatch.o:	parse.h
aspath.o:	aspath.c
aspath.o:	include.h
aspath.o:	defines.h
aspath.o:	paths.h
aspath.o:	config.h
aspath.o:	defs.h
aspath.o:	sockaddr.h
aspath.o:	str.h
aspath.o:	aspath.h
aspath.o:	asmatch.h
aspath.o:	policy.h
aspath.o:	rt_table.h
aspath.o:	if.h
aspath.o:	task.h
aspath.o:	trace.h
aspath.o:	bgp.h
aspath.o:	ospf.h
aspath.o:	ospf_log.h
aspath.o:	ospf_rtab.h
aspath.o:	ospf_timer_calls.h
aspath.o:	ospf_pkts.h
aspath.o:	ospf_lsdb.h
aspath.o:	ospf_const.h
aspath.o:	ospf_gated.h
bgp.o:	bgp.c
bgp.o:	include.h
bgp.o:	defines.h
bgp.o:	paths.h
bgp.o:	config.h
bgp.o:	defs.h
bgp.o:	sockaddr.h
bgp.o:	str.h
bgp.o:	policy.h
bgp.o:	rt_table.h
bgp.o:	if.h
bgp.o:	task.h
bgp.o:	trace.h
bgp.o:	bgp_proto.h
bgp.o:	bgp.h
bgp.o:	bgp_var.h
bgp.o:	asmatch.h
bgp.o:	aspath.h
bgp_init.o:	bgp_init.c
bgp_init.o:	include.h
bgp_init.o:	defines.h
bgp_init.o:	paths.h
bgp_init.o:	config.h
bgp_init.o:	defs.h
bgp_init.o:	sockaddr.h
bgp_init.o:	str.h
bgp_init.o:	policy.h
bgp_init.o:	rt_table.h
bgp_init.o:	if.h
bgp_init.o:	task.h
bgp_init.o:	trace.h
bgp_init.o:	bgp_proto.h
bgp_init.o:	bgp.h
bgp_init.o:	bgp_var.h
bgp_init.o:	krt.h
bgp_init.o:	asmatch.h
bgp_init.o:	aspath.h
bgp_rt.o:	bgp_rt.c
bgp_rt.o:	include.h
bgp_rt.o:	defines.h
bgp_rt.o:	paths.h
bgp_rt.o:	config.h
bgp_rt.o:	defs.h
bgp_rt.o:	sockaddr.h
bgp_rt.o:	str.h
bgp_rt.o:	policy.h
bgp_rt.o:	rt_table.h
bgp_rt.o:	if.h
bgp_rt.o:	task.h
bgp_rt.o:	trace.h
bgp_rt.o:	bgp_proto.h
bgp_rt.o:	bgp.h
bgp_rt.o:	bgp_var.h
bgp_rt.o:	asmatch.h
bgp_rt.o:	aspath.h
bgp_sync.o:	bgp_sync.c
bgp_sync.o:	include.h
bgp_sync.o:	defines.h
bgp_sync.o:	paths.h
bgp_sync.o:	config.h
bgp_sync.o:	defs.h
bgp_sync.o:	sockaddr.h
bgp_sync.o:	str.h
bgp_sync.o:	policy.h
bgp_sync.o:	rt_table.h
bgp_sync.o:	if.h
bgp_sync.o:	task.h
bgp_sync.o:	trace.h
bgp_sync.o:	bgp_proto.h
bgp_sync.o:	bgp.h
bgp_sync.o:	bgp_var.h
bgp_sync.o:	asmatch.h
bgp_sync.o:	aspath.h
checksum.o:	checksum.c
checksum.o:	include.h
checksum.o:	defines.h
checksum.o:	paths.h
checksum.o:	config.h
checksum.o:	defs.h
checksum.o:	sockaddr.h
checksum.o:	str.h
checksum.o:	policy.h
checksum.o:	rt_table.h
checksum.o:	if.h
checksum.o:	task.h
checksum.o:	trace.h
checksum.o:	asmatch.h
checksum.o:	aspath.h
dvmrp.o:	dvmrp.c
dvmrp.o:	include.h
dvmrp.o:	defines.h
dvmrp.o:	paths.h
dvmrp.o:	config.h
dvmrp.o:	defs.h
dvmrp.o:	sockaddr.h
dvmrp.o:	str.h
dvmrp.o:	policy.h
dvmrp.o:	rt_table.h
dvmrp.o:	if.h
dvmrp.o:	task.h
dvmrp.o:	trace.h
dvmrp.o:	krt.h
dvmrp.o:	krt_ipmulti.h
dvmrp.o:	dvmrp.h
dvmrp.o:	dvmrp_targets.h
dvmrp.o:	asmatch.h
dvmrp.o:	aspath.h
dvmrp_targets.o:	dvmrp_targets.c
dvmrp_targets.o:	include.h
dvmrp_targets.o:	defines.h
dvmrp_targets.o:	paths.h
dvmrp_targets.o:	config.h
dvmrp_targets.o:	defs.h
dvmrp_targets.o:	sockaddr.h
dvmrp_targets.o:	str.h
dvmrp_targets.o:	policy.h
dvmrp_targets.o:	rt_table.h
dvmrp_targets.o:	if.h
dvmrp_targets.o:	task.h
dvmrp_targets.o:	trace.h
dvmrp_targets.o:	dvmrp.h
dvmrp_targets.o:	dvmrp_targets.h
dvmrp_targets.o:	asmatch.h
dvmrp_targets.o:	aspath.h
egp.o:	egp.c
egp.o:	include.h
egp.o:	defines.h
egp.o:	paths.h
egp.o:	config.h
egp.o:	defs.h
egp.o:	sockaddr.h
egp.o:	str.h
egp.o:	policy.h
egp.o:	rt_table.h
egp.o:	if.h
egp.o:	task.h
egp.o:	trace.h
egp.o:	egp.h
egp.o:	egp_param.h
egp.o:	asmatch.h
egp.o:	aspath.h
egp_init.o:	egp_init.c
egp_init.o:	include.h
egp_init.o:	defines.h
egp_init.o:	paths.h
egp_init.o:	config.h
egp_init.o:	defs.h
egp_init.o:	sockaddr.h
egp_init.o:	str.h
egp_init.o:	policy.h
egp_init.o:	rt_table.h
egp_init.o:	if.h
egp_init.o:	task.h
egp_init.o:	trace.h
egp_init.o:	egp.h
egp_init.o:	egp_param.h
egp_init.o:	asmatch.h
egp_init.o:	aspath.h
egp_rt.o:	egp_rt.c
egp_rt.o:	include.h
egp_rt.o:	defines.h
egp_rt.o:	paths.h
egp_rt.o:	config.h
egp_rt.o:	defs.h
egp_rt.o:	sockaddr.h
egp_rt.o:	str.h
egp_rt.o:	policy.h
egp_rt.o:	rt_table.h
egp_rt.o:	if.h
egp_rt.o:	task.h
egp_rt.o:	trace.h
egp_rt.o:	egp.h
egp_rt.o:	egp_param.h
egp_rt.o:	asmatch.h
egp_rt.o:	aspath.h
gdc.o:	gdc.c
gdc.o:	include.h
gdc.o:	defines.h
gdc.o:	paths.h
gdc.o:	config.h
gdc.o:	defs.h
gdc.o:	sockaddr.h
gdc.o:	str.h
gdc.o:	policy.h
gdc.o:	rt_table.h
gdc.o:	if.h
gdc.o:	task.h
gdc.o:	trace.h
gdc.o:	asmatch.h
gdc.o:	aspath.h
grand.o:	grand.c
grand.o:	include.h
grand.o:	defines.h
grand.o:	paths.h
grand.o:	config.h
grand.o:	defs.h
grand.o:	sockaddr.h
grand.o:	str.h
grand.o:	policy.h
grand.o:	rt_table.h
grand.o:	if.h
grand.o:	task.h
grand.o:	trace.h
grand.o:	asmatch.h
grand.o:	aspath.h
icmp.o:	icmp.c
icmp.o:	include.h
icmp.o:	defines.h
icmp.o:	paths.h
icmp.o:	config.h
icmp.o:	defs.h
icmp.o:	sockaddr.h
icmp.o:	str.h
icmp.o:	policy.h
icmp.o:	rt_table.h
icmp.o:	if.h
icmp.o:	task.h
icmp.o:	trace.h
icmp.o:	ip_icmp.h
icmp.o:	icmp.h
icmp.o:	asmatch.h
icmp.o:	aspath.h
if.o:	if.c
if.o:	include.h
if.o:	defines.h
if.o:	paths.h
if.o:	config.h
if.o:	defs.h
if.o:	sockaddr.h
if.o:	str.h
if.o:	policy.h
if.o:	rt_table.h
if.o:	if.h
if.o:	task.h
if.o:	trace.h
if.o:	krt.h
if.o:	asmatch.h
if.o:	aspath.h
krt.o:	krt.c
krt.o:	include.h
krt.o:	defines.h
krt.o:	paths.h
krt.o:	config.h
krt.o:	defs.h
krt.o:	sockaddr.h
krt.o:	str.h
krt.o:	policy.h
krt.o:	rt_table.h
krt.o:	if.h
krt.o:	task.h
krt.o:	trace.h
krt.o:	krt.h
krt.o:	krt_var.h
krt.o:	asmatch.h
krt.o:	aspath.h
krt_ifread_kinfo.o:	krt_ifread_kinfo.c
krt_ifread_kinfo.o:	include.h
krt_ifread_kinfo.o:	defines.h
krt_ifread_kinfo.o:	paths.h
krt_ifread_kinfo.o:	config.h
krt_ifread_kinfo.o:	defs.h
krt_ifread_kinfo.o:	sockaddr.h
krt_ifread_kinfo.o:	str.h
krt_ifread_kinfo.o:	policy.h
krt_ifread_kinfo.o:	rt_table.h
krt_ifread_kinfo.o:	if.h
krt_ifread_kinfo.o:	task.h
krt_ifread_kinfo.o:	trace.h
krt_ifread_kinfo.o:	krt.h
krt_ifread_kinfo.o:	krt_var.h
krt_ifread_kinfo.o:	asmatch.h
krt_ifread_kinfo.o:	aspath.h
krt_ipmulti.o:	krt_ipmulti.c
krt_ipmulti.o:	include.h
krt_ipmulti.o:	defines.h
krt_ipmulti.o:	paths.h
krt_ipmulti.o:	config.h
krt_ipmulti.o:	defs.h
krt_ipmulti.o:	sockaddr.h
krt_ipmulti.o:	str.h
krt_ipmulti.o:	policy.h
krt_ipmulti.o:	rt_table.h
krt_ipmulti.o:	if.h
krt_ipmulti.o:	task.h
krt_ipmulti.o:	trace.h
krt_ipmulti.o:	krt.h
krt_ipmulti.o:	krt_var.h
krt_ipmulti.o:	krt_ipmulti.h
krt_ipmulti.o:	asmatch.h
krt_ipmulti.o:	aspath.h
krt_ipmulti_nocache.o:	krt_ipmulti_nocache.c
krt_ipmulti_nocache.o:	include.h
krt_ipmulti_nocache.o:	defines.h
krt_ipmulti_nocache.o:	paths.h
krt_ipmulti_nocache.o:	config.h
krt_ipmulti_nocache.o:	defs.h
krt_ipmulti_nocache.o:	sockaddr.h
krt_ipmulti_nocache.o:	str.h
krt_ipmulti_nocache.o:	policy.h
krt_ipmulti_nocache.o:	rt_table.h
krt_ipmulti_nocache.o:	if.h
krt_ipmulti_nocache.o:	task.h
krt_ipmulti_nocache.o:	trace.h
krt_ipmulti_nocache.o:	krt.h
krt_ipmulti_nocache.o:	krt_var.h
krt_ipmulti_nocache.o:	asmatch.h
krt_ipmulti_nocache.o:	aspath.h
krt_ipmulti_rtsock.o:	krt_ipmulti_rtsock.c
krt_ipmulti_rtsock.o:	include.h
krt_ipmulti_rtsock.o:	defines.h
krt_ipmulti_rtsock.o:	paths.h
krt_ipmulti_rtsock.o:	config.h
krt_ipmulti_rtsock.o:	defs.h
krt_ipmulti_rtsock.o:	sockaddr.h
krt_ipmulti_rtsock.o:	str.h
krt_ipmulti_rtsock.o:	policy.h
krt_ipmulti_rtsock.o:	rt_table.h
krt_ipmulti_rtsock.o:	if.h
krt_ipmulti_rtsock.o:	task.h
krt_ipmulti_rtsock.o:	trace.h
krt_ipmulti_rtsock.o:	krt.h
krt_ipmulti_rtsock.o:	krt_var.h
krt_ipmulti_rtsock.o:	krt_ipmulti.h
krt_ipmulti_rtsock.o:	asmatch.h
krt_ipmulti_rtsock.o:	aspath.h
krt_ipmulti_ttl0.o:	krt_ipmulti_ttl0.c
krt_ipmulti_ttl0.o:	include.h
krt_ipmulti_ttl0.o:	defines.h
krt_ipmulti_ttl0.o:	paths.h
krt_ipmulti_ttl0.o:	config.h
krt_ipmulti_ttl0.o:	defs.h
krt_ipmulti_ttl0.o:	sockaddr.h
krt_ipmulti_ttl0.o:	str.h
krt_ipmulti_ttl0.o:	policy.h
krt_ipmulti_ttl0.o:	rt_table.h
krt_ipmulti_ttl0.o:	if.h
krt_ipmulti_ttl0.o:	task.h
krt_ipmulti_ttl0.o:	trace.h
krt_ipmulti_ttl0.o:	krt.h
krt_ipmulti_ttl0.o:	krt_var.h
krt_ipmulti_ttl0.o:	krt_ipmulti.h
krt_ipmulti_ttl0.o:	asmatch.h
krt_ipmulti_ttl0.o:	aspath.h
krt_ipmulticast.o:	krt_ipmulticast.c
krt_ipmulticast.o:	include.h
krt_ipmulticast.o:	defines.h
krt_ipmulticast.o:	paths.h
krt_ipmulticast.o:	config.h
krt_ipmulticast.o:	defs.h
krt_ipmulticast.o:	sockaddr.h
krt_ipmulticast.o:	str.h
krt_ipmulticast.o:	policy.h
krt_ipmulticast.o:	rt_table.h
krt_ipmulticast.o:	if.h
krt_ipmulticast.o:	task.h
krt_ipmulticast.o:	trace.h
krt_ipmulticast.o:	krt.h
krt_ipmulticast.o:	krt_var.h
krt_ipmulticast.o:	asmatch.h
krt_ipmulticast.o:	aspath.h
krt_lladdr_linux.o:	krt_lladdr_linux.c
krt_lladdr_linux.o:	include.h
krt_lladdr_linux.o:	defines.h
krt_lladdr_linux.o:	paths.h
krt_lladdr_linux.o:	config.h
krt_lladdr_linux.o:	defs.h
krt_lladdr_linux.o:	sockaddr.h
krt_lladdr_linux.o:	str.h
krt_lladdr_linux.o:	policy.h
krt_lladdr_linux.o:	rt_table.h
krt_lladdr_linux.o:	if.h
krt_lladdr_linux.o:	task.h
krt_lladdr_linux.o:	trace.h
krt_lladdr_linux.o:	krt.h
krt_lladdr_linux.o:	krt_var.h
krt_lladdr_linux.o:	asmatch.h
krt_lladdr_linux.o:	aspath.h
krt_lladdr_sunos4.o:	krt_lladdr_sunos4.c
krt_lladdr_sunos4.o:	include.h
krt_lladdr_sunos4.o:	defines.h
krt_lladdr_sunos4.o:	paths.h
krt_lladdr_sunos4.o:	config.h
krt_lladdr_sunos4.o:	defs.h
krt_lladdr_sunos4.o:	sockaddr.h
krt_lladdr_sunos4.o:	str.h
krt_lladdr_sunos4.o:	policy.h
krt_lladdr_sunos4.o:	rt_table.h
krt_lladdr_sunos4.o:	if.h
krt_lladdr_sunos4.o:	task.h
krt_lladdr_sunos4.o:	trace.h
krt_lladdr_sunos4.o:	krt.h
krt_lladdr_sunos4.o:	krt_var.h
krt_lladdr_sunos4.o:	asmatch.h
krt_lladdr_sunos4.o:	aspath.h
krt_rt_sock.o:	krt_rt_sock.c
krt_rt_sock.o:	include.h
krt_rt_sock.o:	defines.h
krt_rt_sock.o:	paths.h
krt_rt_sock.o:	config.h
krt_rt_sock.o:	defs.h
krt_rt_sock.o:	sockaddr.h
krt_rt_sock.o:	str.h
krt_rt_sock.o:	policy.h
krt_rt_sock.o:	rt_table.h
krt_rt_sock.o:	if.h
krt_rt_sock.o:	task.h
krt_rt_sock.o:	trace.h
krt_rt_sock.o:	krt.h
krt_rt_sock.o:	krt_var.h
krt_rt_sock.o:	asmatch.h
krt_rt_sock.o:	aspath.h
krt_rtread_kinfo.o:	krt_rtread_kinfo.c
krt_rtread_kinfo.o:	include.h
krt_rtread_kinfo.o:	defines.h
krt_rtread_kinfo.o:	paths.h
krt_rtread_kinfo.o:	config.h
krt_rtread_kinfo.o:	defs.h
krt_rtread_kinfo.o:	sockaddr.h
krt_rtread_kinfo.o:	str.h
krt_rtread_kinfo.o:	policy.h
krt_rtread_kinfo.o:	rt_table.h
krt_rtread_kinfo.o:	if.h
krt_rtread_kinfo.o:	task.h
krt_rtread_kinfo.o:	trace.h
krt_rtread_kinfo.o:	krt.h
krt_rtread_kinfo.o:	krt_var.h
krt_rtread_kinfo.o:	asmatch.h
krt_rtread_kinfo.o:	aspath.h
lexer.o:	lexer.c
lexer.o:	include.h
lexer.o:	defines.h
lexer.o:	paths.h
lexer.o:	config.h
lexer.o:	defs.h
lexer.o:	sockaddr.h
lexer.o:	str.h
lexer.o:	policy.h
lexer.o:	rt_table.h
lexer.o:	if.h
lexer.o:	task.h
lexer.o:	trace.h
lexer.o:	parse.h
lexer.o:	parser.h
lexer.o:	asmatch.h
lexer.o:	aspath.h
ospf_build_dbsum.o:	ospf_build_dbsum.c
ospf_build_dbsum.o:	include.h
ospf_build_dbsum.o:	defines.h
ospf_build_dbsum.o:	paths.h
ospf_build_dbsum.o:	config.h
ospf_build_dbsum.o:	defs.h
ospf_build_dbsum.o:	sockaddr.h
ospf_build_dbsum.o:	str.h
ospf_build_dbsum.o:	policy.h
ospf_build_dbsum.o:	rt_table.h
ospf_build_dbsum.o:	if.h
ospf_build_dbsum.o:	task.h
ospf_build_dbsum.o:	trace.h
ospf_build_dbsum.o:	ospf.h
ospf_build_dbsum.o:	ospf_log.h
ospf_build_dbsum.o:	ospf_rtab.h
ospf_build_dbsum.o:	ospf_timer_calls.h
ospf_build_dbsum.o:	ospf_pkts.h
ospf_build_dbsum.o:	ospf_lsdb.h
ospf_build_dbsum.o:	ospf_const.h
ospf_build_dbsum.o:	ospf_gated.h
ospf_build_dbsum.o:	asmatch.h
ospf_build_dbsum.o:	aspath.h
ospf_build_ls.o:	ospf_build_ls.c
ospf_build_ls.o:	include.h
ospf_build_ls.o:	defines.h
ospf_build_ls.o:	paths.h
ospf_build_ls.o:	config.h
ospf_build_ls.o:	defs.h
ospf_build_ls.o:	sockaddr.h
ospf_build_ls.o:	str.h
ospf_build_ls.o:	policy.h
ospf_build_ls.o:	rt_table.h
ospf_build_ls.o:	if.h
ospf_build_ls.o:	task.h
ospf_build_ls.o:	trace.h
ospf_build_ls.o:	ospf.h
ospf_build_ls.o:	ospf_log.h
ospf_build_ls.o:	ospf_rtab.h
ospf_build_ls.o:	ospf_timer_calls.h
ospf_build_ls.o:	ospf_pkts.h
ospf_build_ls.o:	ospf_lsdb.h
ospf_build_ls.o:	ospf_const.h
ospf_build_ls.o:	ospf_gated.h
ospf_build_ls.o:	asmatch.h
ospf_build_ls.o:	aspath.h
ospf_chksum.o:	ospf_chksum.c
ospf_chksum.o:	include.h
ospf_chksum.o:	defines.h
ospf_chksum.o:	paths.h
ospf_chksum.o:	config.h
ospf_chksum.o:	defs.h
ospf_chksum.o:	sockaddr.h
ospf_chksum.o:	str.h
ospf_chksum.o:	policy.h
ospf_chksum.o:	rt_table.h
ospf_chksum.o:	if.h
ospf_chksum.o:	task.h
ospf_chksum.o:	trace.h
ospf_chksum.o:	ospf.h
ospf_chksum.o:	ospf_log.h
ospf_chksum.o:	ospf_rtab.h
ospf_chksum.o:	ospf_timer_calls.h
ospf_chksum.o:	ospf_pkts.h
ospf_chksum.o:	ospf_lsdb.h
ospf_chksum.o:	ospf_const.h
ospf_chksum.o:	ospf_gated.h
ospf_chksum.o:	asmatch.h
ospf_chksum.o:	aspath.h
ospf_choose_dr.o:	ospf_choose_dr.c
ospf_choose_dr.o:	include.h
ospf_choose_dr.o:	defines.h
ospf_choose_dr.o:	paths.h
ospf_choose_dr.o:	config.h
ospf_choose_dr.o:	defs.h
ospf_choose_dr.o:	sockaddr.h
ospf_choose_dr.o:	str.h
ospf_choose_dr.o:	policy.h
ospf_choose_dr.o:	rt_table.h
ospf_choose_dr.o:	if.h
ospf_choose_dr.o:	task.h
ospf_choose_dr.o:	trace.h
ospf_choose_dr.o:	ospf.h
ospf_choose_dr.o:	ospf_log.h
ospf_choose_dr.o:	ospf_rtab.h
ospf_choose_dr.o:	ospf_timer_calls.h
ospf_choose_dr.o:	ospf_pkts.h
ospf_choose_dr.o:	ospf_lsdb.h
ospf_choose_dr.o:	ospf_const.h
ospf_choose_dr.o:	ospf_gated.h
ospf_choose_dr.o:	asmatch.h
ospf_choose_dr.o:	aspath.h
ospf_conf.o:	ospf_conf.c
ospf_conf.o:	include.h
ospf_conf.o:	defines.h
ospf_conf.o:	paths.h
ospf_conf.o:	config.h
ospf_conf.o:	defs.h
ospf_conf.o:	sockaddr.h
ospf_conf.o:	str.h
ospf_conf.o:	policy.h
ospf_conf.o:	rt_table.h
ospf_conf.o:	if.h
ospf_conf.o:	task.h
ospf_conf.o:	trace.h
ospf_conf.o:	ospf.h
ospf_conf.o:	ospf_log.h
ospf_conf.o:	ospf_rtab.h
ospf_conf.o:	ospf_timer_calls.h
ospf_conf.o:	ospf_pkts.h
ospf_conf.o:	ospf_lsdb.h
ospf_conf.o:	ospf_const.h
ospf_conf.o:	ospf_gated.h
ospf_conf.o:	asmatch.h
ospf_conf.o:	aspath.h
ospf_flood.o:	ospf_flood.c
ospf_flood.o:	include.h
ospf_flood.o:	defines.h
ospf_flood.o:	paths.h
ospf_flood.o:	config.h
ospf_flood.o:	defs.h
ospf_flood.o:	sockaddr.h
ospf_flood.o:	str.h
ospf_flood.o:	policy.h
ospf_flood.o:	rt_table.h
ospf_flood.o:	if.h
ospf_flood.o:	task.h
ospf_flood.o:	trace.h
ospf_flood.o:	ospf.h
ospf_flood.o:	ospf_log.h
ospf_flood.o:	ospf_rtab.h
ospf_flood.o:	ospf_timer_calls.h
ospf_flood.o:	ospf_pkts.h
ospf_flood.o:	ospf_lsdb.h
ospf_flood.o:	ospf_const.h
ospf_flood.o:	ospf_gated.h
ospf_flood.o:	asmatch.h
ospf_flood.o:	aspath.h
ospf_init.o:	ospf_init.c
ospf_init.o:	include.h
ospf_init.o:	defines.h
ospf_init.o:	paths.h
ospf_init.o:	config.h
ospf_init.o:	defs.h
ospf_init.o:	sockaddr.h
ospf_init.o:	str.h
ospf_init.o:	policy.h
ospf_init.o:	rt_table.h
ospf_init.o:	if.h
ospf_init.o:	task.h
ospf_init.o:	trace.h
ospf_init.o:	ospf.h
ospf_init.o:	ospf_log.h
ospf_init.o:	ospf_rtab.h
ospf_init.o:	ospf_timer_calls.h
ospf_init.o:	ospf_pkts.h
ospf_init.o:	ospf_lsdb.h
ospf_init.o:	ospf_const.h
ospf_init.o:	ospf_gated.h
ospf_init.o:	krt.h
ospf_init.o:	asmatch.h
ospf_init.o:	aspath.h
ospf_log.o:	ospf_log.c
ospf_log.o:	include.h
ospf_log.o:	defines.h
ospf_log.o:	paths.h
ospf_log.o:	config.h
ospf_log.o:	defs.h
ospf_log.o:	sockaddr.h
ospf_log.o:	str.h
ospf_log.o:	policy.h
ospf_log.o:	rt_table.h
ospf_log.o:	if.h
ospf_log.o:	task.h
ospf_log.o:	trace.h
ospf_log.o:	ospf.h
ospf_log.o:	ospf_log.h
ospf_log.o:	ospf_rtab.h
ospf_log.o:	ospf_timer_calls.h
ospf_log.o:	ospf_pkts.h
ospf_log.o:	ospf_lsdb.h
ospf_log.o:	ospf_const.h
ospf_log.o:	ospf_gated.h
ospf_log.o:	asmatch.h
ospf_log.o:	aspath.h
ospf_lsdb.o:	ospf_lsdb.c
ospf_lsdb.o:	include.h
ospf_lsdb.o:	defines.h
ospf_lsdb.o:	paths.h
ospf_lsdb.o:	config.h
ospf_lsdb.o:	defs.h
ospf_lsdb.o:	sockaddr.h
ospf_lsdb.o:	str.h
ospf_lsdb.o:	policy.h
ospf_lsdb.o:	rt_table.h
ospf_lsdb.o:	if.h
ospf_lsdb.o:	task.h
ospf_lsdb.o:	trace.h
ospf_lsdb.o:	ospf.h
ospf_lsdb.o:	ospf_log.h
ospf_lsdb.o:	ospf_rtab.h
ospf_lsdb.o:	ospf_timer_calls.h
ospf_lsdb.o:	ospf_pkts.h
ospf_lsdb.o:	ospf_lsdb.h
ospf_lsdb.o:	ospf_const.h
ospf_lsdb.o:	ospf_gated.h
ospf_lsdb.o:	asmatch.h
ospf_lsdb.o:	aspath.h
ospf_monitor.o:	ospf_monitor.c
ospf_monitor.o:	include.h
ospf_monitor.o:	defines.h
ospf_monitor.o:	paths.h
ospf_monitor.o:	config.h
ospf_monitor.o:	defs.h
ospf_monitor.o:	sockaddr.h
ospf_monitor.o:	str.h
ospf_monitor.o:	policy.h
ospf_monitor.o:	rt_table.h
ospf_monitor.o:	if.h
ospf_monitor.o:	task.h
ospf_monitor.o:	trace.h
ospf_monitor.o:	ospf.h
ospf_monitor.o:	ospf_log.h
ospf_monitor.o:	ospf_rtab.h
ospf_monitor.o:	ospf_timer_calls.h
ospf_monitor.o:	ospf_pkts.h
ospf_monitor.o:	ospf_lsdb.h
ospf_monitor.o:	ospf_const.h
ospf_monitor.o:	ospf_gated.h
ospf_monitor.o:	asmatch.h
ospf_monitor.o:	aspath.h
ospf_newq.o:	ospf_newq.c
ospf_newq.o:	include.h
ospf_newq.o:	defines.h
ospf_newq.o:	paths.h
ospf_newq.o:	config.h
ospf_newq.o:	defs.h
ospf_newq.o:	sockaddr.h
ospf_newq.o:	str.h
ospf_newq.o:	policy.h
ospf_newq.o:	rt_table.h
ospf_newq.o:	if.h
ospf_newq.o:	task.h
ospf_newq.o:	trace.h
ospf_newq.o:	ospf.h
ospf_newq.o:	ospf_log.h
ospf_newq.o:	ospf_rtab.h
ospf_newq.o:	ospf_timer_calls.h
ospf_newq.o:	ospf_pkts.h
ospf_newq.o:	ospf_lsdb.h
ospf_newq.o:	ospf_const.h
ospf_newq.o:	ospf_gated.h
ospf_newq.o:	asmatch.h
ospf_newq.o:	aspath.h
ospf_rt.o:	ospf_rt.c
ospf_rt.o:	include.h
ospf_rt.o:	defines.h
ospf_rt.o:	paths.h
ospf_rt.o:	config.h
ospf_rt.o:	defs.h
ospf_rt.o:	sockaddr.h
ospf_rt.o:	str.h
ospf_rt.o:	policy.h
ospf_rt.o:	rt_table.h
ospf_rt.o:	if.h
ospf_rt.o:	task.h
ospf_rt.o:	trace.h
ospf_rt.o:	ospf.h
ospf_rt.o:	ospf_log.h
ospf_rt.o:	ospf_rtab.h
ospf_rt.o:	ospf_timer_calls.h
ospf_rt.o:	ospf_pkts.h
ospf_rt.o:	ospf_lsdb.h
ospf_rt.o:	ospf_const.h
ospf_rt.o:	ospf_gated.h
ospf_rt.o:	asmatch.h
ospf_rt.o:	aspath.h
ospf_rtab.o:	ospf_rtab.c
ospf_rtab.o:	include.h
ospf_rtab.o:	defines.h
ospf_rtab.o:	paths.h
ospf_rtab.o:	config.h
ospf_rtab.o:	defs.h
ospf_rtab.o:	sockaddr.h
ospf_rtab.o:	str.h
ospf_rtab.o:	policy.h
ospf_rtab.o:	rt_table.h
ospf_rtab.o:	if.h
ospf_rtab.o:	task.h
ospf_rtab.o:	trace.h
ospf_rtab.o:	ospf.h
ospf_rtab.o:	ospf_log.h
ospf_rtab.o:	ospf_rtab.h
ospf_rtab.o:	ospf_timer_calls.h
ospf_rtab.o:	ospf_pkts.h
ospf_rtab.o:	ospf_lsdb.h
ospf_rtab.o:	ospf_const.h
ospf_rtab.o:	ospf_gated.h
ospf_rtab.o:	asmatch.h
ospf_rtab.o:	aspath.h
ospf_rxlinkup.o:	ospf_rxlinkup.c
ospf_rxlinkup.o:	include.h
ospf_rxlinkup.o:	defines.h
ospf_rxlinkup.o:	paths.h
ospf_rxlinkup.o:	config.h
ospf_rxlinkup.o:	defs.h
ospf_rxlinkup.o:	sockaddr.h
ospf_rxlinkup.o:	str.h
ospf_rxlinkup.o:	policy.h
ospf_rxlinkup.o:	rt_table.h
ospf_rxlinkup.o:	if.h
ospf_rxlinkup.o:	task.h
ospf_rxlinkup.o:	trace.h
ospf_rxlinkup.o:	ospf.h
ospf_rxlinkup.o:	ospf_log.h
ospf_rxlinkup.o:	ospf_rtab.h
ospf_rxlinkup.o:	ospf_timer_calls.h
ospf_rxlinkup.o:	ospf_pkts.h
ospf_rxlinkup.o:	ospf_lsdb.h
ospf_rxlinkup.o:	ospf_const.h
ospf_rxlinkup.o:	ospf_gated.h
ospf_rxlinkup.o:	asmatch.h
ospf_rxlinkup.o:	aspath.h
ospf_rxmon.o:	ospf_rxmon.c
ospf_rxmon.o:	include.h
ospf_rxmon.o:	defines.h
ospf_rxmon.o:	paths.h
ospf_rxmon.o:	config.h
ospf_rxmon.o:	defs.h
ospf_rxmon.o:	sockaddr.h
ospf_rxmon.o:	str.h
ospf_rxmon.o:	policy.h
ospf_rxmon.o:	rt_table.h
ospf_rxmon.o:	if.h
ospf_rxmon.o:	task.h
ospf_rxmon.o:	trace.h
ospf_rxmon.o:	krt.h
ospf_rxmon.o:	ospf.h
ospf_rxmon.o:	ospf_log.h
ospf_rxmon.o:	ospf_rtab.h
ospf_rxmon.o:	ospf_timer_calls.h
ospf_rxmon.o:	ospf_pkts.h
ospf_rxmon.o:	ospf_lsdb.h
ospf_rxmon.o:	ospf_const.h
ospf_rxmon.o:	ospf_gated.h
ospf_rxmon.o:	asmatch.h
ospf_rxmon.o:	aspath.h
ospf_rxpkt.o:	ospf_rxpkt.c
ospf_rxpkt.o:	include.h
ospf_rxpkt.o:	defines.h
ospf_rxpkt.o:	paths.h
ospf_rxpkt.o:	config.h
ospf_rxpkt.o:	defs.h
ospf_rxpkt.o:	sockaddr.h
ospf_rxpkt.o:	str.h
ospf_rxpkt.o:	policy.h
ospf_rxpkt.o:	rt_table.h
ospf_rxpkt.o:	if.h
ospf_rxpkt.o:	task.h
ospf_rxpkt.o:	trace.h
ospf_rxpkt.o:	ospf.h
ospf_rxpkt.o:	ospf_log.h
ospf_rxpkt.o:	ospf_rtab.h
ospf_rxpkt.o:	ospf_timer_calls.h
ospf_rxpkt.o:	ospf_pkts.h
ospf_rxpkt.o:	ospf_lsdb.h
ospf_rxpkt.o:	ospf_const.h
ospf_rxpkt.o:	ospf_gated.h
ospf_rxpkt.o:	asmatch.h
ospf_rxpkt.o:	aspath.h
ospf_spf.o:	ospf_spf.c
ospf_spf.o:	include.h
ospf_spf.o:	defines.h
ospf_spf.o:	paths.h
ospf_spf.o:	config.h
ospf_spf.o:	defs.h
ospf_spf.o:	sockaddr.h
ospf_spf.o:	str.h
ospf_spf.o:	policy.h
ospf_spf.o:	rt_table.h
ospf_spf.o:	if.h
ospf_spf.o:	task.h
ospf_spf.o:	trace.h
ospf_spf.o:	ospf.h
ospf_spf.o:	ospf_log.h
ospf_spf.o:	ospf_rtab.h
ospf_spf.o:	ospf_timer_calls.h
ospf_spf.o:	ospf_pkts.h
ospf_spf.o:	ospf_lsdb.h
ospf_spf.o:	ospf_const.h
ospf_spf.o:	ospf_gated.h
ospf_spf.o:	asmatch.h
ospf_spf.o:	aspath.h
ospf_spf_leaves.o:	ospf_spf_leaves.c
ospf_spf_leaves.o:	include.h
ospf_spf_leaves.o:	defines.h
ospf_spf_leaves.o:	paths.h
ospf_spf_leaves.o:	config.h
ospf_spf_leaves.o:	defs.h
ospf_spf_leaves.o:	sockaddr.h
ospf_spf_leaves.o:	str.h
ospf_spf_leaves.o:	policy.h
ospf_spf_leaves.o:	rt_table.h
ospf_spf_leaves.o:	if.h
ospf_spf_leaves.o:	task.h
ospf_spf_leaves.o:	trace.h
ospf_spf_leaves.o:	ospf.h
ospf_spf_leaves.o:	ospf_log.h
ospf_spf_leaves.o:	ospf_rtab.h
ospf_spf_leaves.o:	ospf_timer_calls.h
ospf_spf_leaves.o:	ospf_pkts.h
ospf_spf_leaves.o:	ospf_lsdb.h
ospf_spf_leaves.o:	ospf_const.h
ospf_spf_leaves.o:	ospf_gated.h
ospf_spf_leaves.o:	asmatch.h
ospf_spf_leaves.o:	aspath.h
ospf_states.o:	ospf_states.c
ospf_states.o:	include.h
ospf_states.o:	defines.h
ospf_states.o:	paths.h
ospf_states.o:	config.h
ospf_states.o:	defs.h
ospf_states.o:	sockaddr.h
ospf_states.o:	str.h
ospf_states.o:	policy.h
ospf_states.o:	rt_table.h
ospf_states.o:	if.h
ospf_states.o:	task.h
ospf_states.o:	trace.h
ospf_states.o:	ospf.h
ospf_states.o:	ospf_log.h
ospf_states.o:	ospf_rtab.h
ospf_states.o:	ospf_timer_calls.h
ospf_states.o:	ospf_pkts.h
ospf_states.o:	ospf_lsdb.h
ospf_states.o:	ospf_const.h
ospf_states.o:	ospf_gated.h
ospf_states.o:	asmatch.h
ospf_states.o:	aspath.h
ospf_tqhandle.o:	ospf_tqhandle.c
ospf_tqhandle.o:	include.h
ospf_tqhandle.o:	defines.h
ospf_tqhandle.o:	paths.h
ospf_tqhandle.o:	config.h
ospf_tqhandle.o:	defs.h
ospf_tqhandle.o:	sockaddr.h
ospf_tqhandle.o:	str.h
ospf_tqhandle.o:	policy.h
ospf_tqhandle.o:	rt_table.h
ospf_tqhandle.o:	if.h
ospf_tqhandle.o:	task.h
ospf_tqhandle.o:	trace.h
ospf_tqhandle.o:	ospf.h
ospf_tqhandle.o:	ospf_log.h
ospf_tqhandle.o:	ospf_rtab.h
ospf_tqhandle.o:	ospf_timer_calls.h
ospf_tqhandle.o:	ospf_pkts.h
ospf_tqhandle.o:	ospf_lsdb.h
ospf_tqhandle.o:	ospf_const.h
ospf_tqhandle.o:	ospf_gated.h
ospf_tqhandle.o:	asmatch.h
ospf_tqhandle.o:	aspath.h
ospf_trace.o:	ospf_trace.c
ospf_trace.o:	include.h
ospf_trace.o:	defines.h
ospf_trace.o:	paths.h
ospf_trace.o:	config.h
ospf_trace.o:	defs.h
ospf_trace.o:	sockaddr.h
ospf_trace.o:	str.h
ospf_trace.o:	policy.h
ospf_trace.o:	rt_table.h
ospf_trace.o:	if.h
ospf_trace.o:	task.h
ospf_trace.o:	trace.h
ospf_trace.o:	ospf.h
ospf_trace.o:	ospf_log.h
ospf_trace.o:	ospf_rtab.h
ospf_trace.o:	ospf_timer_calls.h
ospf_trace.o:	ospf_pkts.h
ospf_trace.o:	ospf_lsdb.h
ospf_trace.o:	ospf_const.h
ospf_trace.o:	ospf_gated.h
ospf_trace.o:	asmatch.h
ospf_trace.o:	aspath.h
ospf_txpkt.o:	ospf_txpkt.c
ospf_txpkt.o:	include.h
ospf_txpkt.o:	defines.h
ospf_txpkt.o:	paths.h
ospf_txpkt.o:	config.h
ospf_txpkt.o:	defs.h
ospf_txpkt.o:	sockaddr.h
ospf_txpkt.o:	str.h
ospf_txpkt.o:	policy.h
ospf_txpkt.o:	rt_table.h
ospf_txpkt.o:	if.h
ospf_txpkt.o:	task.h
ospf_txpkt.o:	trace.h
ospf_txpkt.o:	ospf.h
ospf_txpkt.o:	ospf_log.h
ospf_txpkt.o:	ospf_rtab.h
ospf_txpkt.o:	ospf_timer_calls.h
ospf_txpkt.o:	ospf_pkts.h
ospf_txpkt.o:	ospf_lsdb.h
ospf_txpkt.o:	ospf_const.h
ospf_txpkt.o:	ospf_gated.h
ospf_txpkt.o:	asmatch.h
ospf_txpkt.o:	aspath.h
parse.o:	parse.c
parse.o:	include.h
parse.o:	defines.h
parse.o:	paths.h
parse.o:	config.h
parse.o:	defs.h
parse.o:	sockaddr.h
parse.o:	str.h
parse.o:	policy.h
parse.o:	rt_table.h
parse.o:	if.h
parse.o:	task.h
parse.o:	trace.h
parse.o:	parse.h
parse.o:	krt.h
parse.o:	parser.h
parse.o:	asmatch.h
parse.o:	bgp.h
parse.o:	egp.h
parse.o:	egp_param.h
parse.o:	rip.h
parse.o:	aspath.h
parse.o:	ospf.h
parse.o:	ospf_log.h
parse.o:	ospf_rtab.h
parse.o:	ospf_timer_calls.h
parse.o:	ospf_pkts.h
parse.o:	ospf_lsdb.h
parse.o:	ospf_const.h
parse.o:	ospf_gated.h
parser.o:	parser.c
parser.o:	include.h
parser.o:	defines.h
parser.o:	paths.h
parser.o:	config.h
parser.o:	defs.h
parser.o:	sockaddr.h
parser.o:	str.h
parser.o:	policy.h
parser.o:	rt_table.h
parser.o:	if.h
parser.o:	task.h
parser.o:	trace.h
parser.o:	parse.h
parser.o:	krt.h
parser.o:	asmatch.h
parser.o:	icmp.h
parser.o:	bgp.h
parser.o:	egp.h
parser.o:	egp_param.h
parser.o:	rip.h
parser.o:	aspath.h
parser.o:	ospf.h
parser.o:	ospf_log.h
parser.o:	ospf_rtab.h
parser.o:	ospf_timer_calls.h
parser.o:	ospf_pkts.h
parser.o:	ospf_lsdb.h
parser.o:	ospf_const.h
parser.o:	ospf_gated.h
pim.o:	pim.c
pim.o:	include.h
pim.o:	defines.h
pim.o:	paths.h
pim.o:	config.h
pim.o:	defs.h
pim.o:	sockaddr.h
pim.o:	str.h
pim.o:	policy.h
pim.o:	rt_table.h
pim.o:	if.h
pim.o:	task.h
pim.o:	trace.h
pim.o:	krt_ipmulti.h
pim.o:	targets.h
pim.o:	pim.h
pim.o:	asmatch.h
pim.o:	aspath.h
policy.o:	policy.c
policy.o:	include.h
policy.o:	defines.h
policy.o:	paths.h
policy.o:	config.h
policy.o:	defs.h
policy.o:	sockaddr.h
policy.o:	str.h
policy.o:	policy.h
policy.o:	rt_table.h
policy.o:	if.h
policy.o:	task.h
policy.o:	trace.h
policy.o:	parse.h
policy.o:	asmatch.h
policy.o:	aspath.h
rdisc.o:	rdisc.c
rdisc.o:	include.h
rdisc.o:	defines.h
rdisc.o:	paths.h
rdisc.o:	config.h
rdisc.o:	defs.h
rdisc.o:	sockaddr.h
rdisc.o:	str.h
rdisc.o:	policy.h
rdisc.o:	rt_table.h
rdisc.o:	if.h
rdisc.o:	task.h
rdisc.o:	trace.h
rdisc.o:	ip_icmp.h
rdisc.o:	rdisc.h
rdisc.o:	asmatch.h
rdisc.o:	icmp.h
rdisc.o:	aspath.h
rip.o:	rip.c
rip.o:	include.h
rip.o:	defines.h
rip.o:	paths.h
rip.o:	config.h
rip.o:	defs.h
rip.o:	sockaddr.h
rip.o:	str.h
rip.o:	policy.h
rip.o:	rt_table.h
rip.o:	if.h
rip.o:	task.h
rip.o:	trace.h
rip.o:	targets.h
rip.o:	rip.h
rip.o:	krt.h
rip.o:	asmatch.h
rip.o:	aspath.h
ripquery.o:	ripquery.c
ripquery.o:	include.h
ripquery.o:	defines.h
ripquery.o:	paths.h
ripquery.o:	config.h
ripquery.o:	defs.h
ripquery.o:	sockaddr.h
ripquery.o:	str.h
ripquery.o:	policy.h
ripquery.o:	rt_table.h
ripquery.o:	if.h
ripquery.o:	task.h
ripquery.o:	trace.h
ripquery.o:	rip.h
ripquery.o:	asmatch.h
ripquery.o:	aspath.h
rt_aggregate.o:	rt_aggregate.c
rt_aggregate.o:	include.h
rt_aggregate.o:	defines.h
rt_aggregate.o:	paths.h
rt_aggregate.o:	config.h
rt_aggregate.o:	defs.h
rt_aggregate.o:	sockaddr.h
rt_aggregate.o:	str.h
rt_aggregate.o:	policy.h
rt_aggregate.o:	rt_var.h
rt_aggregate.o:	rt_table.h
rt_aggregate.o:	if.h
rt_aggregate.o:	task.h
rt_aggregate.o:	trace.h
rt_aggregate.o:	parse.h
rt_aggregate.o:	asmatch.h
rt_aggregate.o:	aspath.h
rt_radix.o:	rt_radix.c
rt_radix.o:	include.h
rt_radix.o:	defines.h
rt_radix.o:	paths.h
rt_radix.o:	config.h
rt_radix.o:	defs.h
rt_radix.o:	sockaddr.h
rt_radix.o:	str.h
rt_radix.o:	policy.h
rt_radix.o:	rt_var.h
rt_radix.o:	rt_table.h
rt_radix.o:	if.h
rt_radix.o:	task.h
rt_radix.o:	trace.h
rt_radix.o:	asmatch.h
rt_radix.o:	aspath.h
rt_redirect.o:	rt_redirect.c
rt_redirect.o:	include.h
rt_redirect.o:	defines.h
rt_redirect.o:	paths.h
rt_redirect.o:	config.h
rt_redirect.o:	defs.h
rt_redirect.o:	sockaddr.h
rt_redirect.o:	str.h
rt_redirect.o:	policy.h
rt_redirect.o:	rt_var.h
rt_redirect.o:	rt_table.h
rt_redirect.o:	if.h
rt_redirect.o:	task.h
rt_redirect.o:	trace.h
rt_redirect.o:	krt.h
rt_redirect.o:	asmatch.h
rt_redirect.o:	aspath.h
rt_static.o:	rt_static.c
rt_static.o:	include.h
rt_static.o:	defines.h
rt_static.o:	paths.h
rt_static.o:	config.h
rt_static.o:	defs.h
rt_static.o:	sockaddr.h
rt_static.o:	str.h
rt_static.o:	policy.h
rt_static.o:	rt_var.h
rt_static.o:	rt_table.h
rt_static.o:	if.h
rt_static.o:	task.h
rt_static.o:	trace.h
rt_static.o:	asmatch.h
rt_static.o:	aspath.h
rt_table.o:	rt_table.c
rt_table.o:	include.h
rt_table.o:	defines.h
rt_table.o:	paths.h
rt_table.o:	config.h
rt_table.o:	defs.h
rt_table.o:	sockaddr.h
rt_table.o:	str.h
rt_table.o:	policy.h
rt_table.o:	rt_var.h
rt_table.o:	rt_table.h
rt_table.o:	if.h
rt_table.o:	task.h
rt_table.o:	trace.h
rt_table.o:	krt.h
rt_table.o:	asmatch.h
rt_table.o:	aspath.h
signames.o:	signames.c
signames.o:	include.h
signames.o:	defines.h
signames.o:	paths.h
signames.o:	config.h
signames.o:	defs.h
signames.o:	sockaddr.h
signames.o:	str.h
signames.o:	policy.h
signames.o:	rt_table.h
signames.o:	if.h
signames.o:	task.h
signames.o:	trace.h
signames.o:	asmatch.h
signames.o:	aspath.h
sockaddr.o:	sockaddr.c
sockaddr.o:	include.h
sockaddr.o:	defines.h
sockaddr.o:	paths.h
sockaddr.o:	config.h
sockaddr.o:	defs.h
sockaddr.o:	sockaddr.h
sockaddr.o:	str.h
sockaddr.o:	policy.h
sockaddr.o:	rt_table.h
sockaddr.o:	if.h
sockaddr.o:	task.h
sockaddr.o:	trace.h
sockaddr.o:	asmatch.h
sockaddr.o:	aspath.h
standalone.o:	standalone.c
standalone.o:	include.h
standalone.o:	defines.h
standalone.o:	paths.h
standalone.o:	config.h
standalone.o:	defs.h
standalone.o:	sockaddr.h
standalone.o:	str.h
standalone.o:	policy.h
standalone.o:	rt_table.h
standalone.o:	if.h
standalone.o:	task.h
standalone.o:	trace.h
standalone.o:	asmatch.h
standalone.o:	aspath.h
str.o:	str.c
str.o:	include.h
str.o:	defines.h
str.o:	paths.h
str.o:	config.h
str.o:	defs.h
str.o:	sockaddr.h
str.o:	str.h
str.o:	policy.h
str.o:	rt_table.h
str.o:	if.h
str.o:	task.h
str.o:	trace.h
str.o:	asmatch.h
str.o:	aspath.h
strerror.o:	strerror.c
strerror.o:	include.h
strerror.o:	defines.h
strerror.o:	paths.h
strerror.o:	config.h
strerror.o:	defs.h
strerror.o:	sockaddr.h
strerror.o:	str.h
strerror.o:	policy.h
strerror.o:	rt_table.h
strerror.o:	if.h
strerror.o:	task.h
strerror.o:	trace.h
strerror.o:	asmatch.h
strerror.o:	aspath.h
targets.o:	targets.c
targets.o:	include.h
targets.o:	defines.h
targets.o:	paths.h
targets.o:	config.h
targets.o:	defs.h
targets.o:	sockaddr.h
targets.o:	str.h
targets.o:	policy.h
targets.o:	rt_table.h
targets.o:	if.h
targets.o:	task.h
targets.o:	trace.h
targets.o:	targets.h
targets.o:	asmatch.h
targets.o:	aspath.h
task.o:	task.c
task.o:	include.h
task.o:	defines.h
task.o:	paths.h
task.o:	config.h
task.o:	defs.h
task.o:	sockaddr.h
task.o:	str.h
task.o:	policy.h
task.o:	rt_table.h
task.o:	if.h
task.o:	task.h
task.o:	trace.h
task.o:	krt.h
task.o:	parse.h
task.o:	asmatch.h
task.o:	icmp.h
task.o:	bgp.h
task.o:	egp.h
task.o:	egp_param.h
task.o:	rip.h
task.o:	rdisc.h
task.o:	aspath.h
task.o:	ospf.h
task.o:	ospf_log.h
task.o:	ospf_rtab.h
task.o:	ospf_timer_calls.h
task.o:	ospf_pkts.h
task.o:	ospf_lsdb.h
task.o:	ospf_const.h
task.o:	ospf_gated.h
trace.o:	trace.c
trace.o:	include.h
trace.o:	defines.h
trace.o:	paths.h
trace.o:	config.h
trace.o:	defs.h
trace.o:	sockaddr.h
trace.o:	str.h
trace.o:	policy.h
trace.o:	rt_table.h
trace.o:	if.h
trace.o:	task.h
trace.o:	trace.h
trace.o:	krt.h
trace.o:	parse.h
trace.o:	asmatch.h
trace.o:	aspath.h
version.o:	version.c
version.o:	include.h
version.o:	defines.h
version.o:	paths.h
version.o:	config.h
version.o:	defs.h
version.o:	sockaddr.h
version.o:	str.h
version.o:	policy.h
version.o:	rt_table.h
version.o:	if.h
version.o:	task.h
version.o:	trace.h
version.o:	asmatch.h
version.o:	aspath.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
