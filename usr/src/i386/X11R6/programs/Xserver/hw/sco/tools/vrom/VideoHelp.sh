#!/bin/sh
#
#	@(#)VideoHelp.sh	11.1	10/22/97	12:41:14
#
#	VideoHelp - create a dump of video BIOS.
#		scan for strings likely to identify the card.
#
#	This must be executed with root privledges to allow access
#		to /dev/mem
#
#	This should work on OpenServer or UnixWare, just about
#		any SCO OS version.
#
#	The use of the vrom dump program on Gemini is the best method
#	to find video BIOS.  vrom will use interrupt jump vectors to
#	find the video BIOS.  The dd method may not actually find the
#	video BIOS correctly.  This should be rare though.
#
AllThere=""

VROMDUMP="/usr/X/lib/vidconf/AOF/bin/vrom"
FOLD="/usr/bin/fold -s -w 72"

if [ ! -x ${FOLD} ]; then {
	FOLD="/usr/bin/xargs /bin/echo"
} fi

if [ ! -x /usr/X/lib/vidconf/AOF/bin/vrom ]; then {
	echo "VideoHelp: /usr/X/lib/vidconf/AOF/bin/vrom: not found, using dd instead."
	VROMDUMP="dd if=/dev/mem skip=768 count=32 bs=1024"
} fi

for i in /bin/strings /usr/bin/egrep /bin/sort /usr/bin/xargs /bin/echo
do
	if [ ! -x $i ]; then {
		echo "VideoHelp: $i: not found."
		AllThere="No"
	} fi
done

if [ -z "$AllThere" ]; then {
	TmpFile=/tmp/vrom.$$
	${VROMDUMP} > ${TmpFile} 2> /dev/null
	/bin/strings ${TmpFile} | /bin/sort -u | \
	    /usr/bin/egrep -i "ver|bios|date|rev|\(c\)|copyright|[12][09][06789][0-9]" \
		| ${FOLD}
	rm -f ${TmpFile}
} fi

