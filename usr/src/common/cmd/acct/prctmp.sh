#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)acct:common/cmd/acct/prctmp.sh	1.7.1.3"
#ident "$Header$"
#	"print session record file (ctmp.h/ascii) with headings"
#	"prctmp file [heading]"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
(cat <<!; cat $*) | pr -h "SESSIONS, SORTED BY ENDING TIME"

MAJ/MIN			CONNECT SECONDS	START TIME	SESSION START
DEVICE	UID	LOGIN	PRIME	NPRIME	(NUMERIC)	DATE	TIME


!
