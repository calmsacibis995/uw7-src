#!/bin/sh
#
#       @(#) libant.sh 8.1 95/02/24 SCOINC
#
#	Copyright (C) The Santa Cruz Operation, 1993-1995
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation, and should be treated as Confidential.

passfail()
{
	code=$1
	if [ "$code" != "0" ]
	then
		[ -s /tmp/tmp$$ ] && {
			errout < /tmp/tmp$$
		}

		tet_infoline FAIL
		echo FAIL
		ret=FAIL
	else
		echo PASS
		tet_infoline PASS
	fi
	rm -f /tmp/tmp$$
}

checkfail()
{
	code=$1
	if [ "$code" != "0" ]
	then
		[ -s /tmp/tmp$$ ] && {
			errout < /tmp/tmp$$
		}
		ret=FAIL
	fi
	rm -f /tmp/tmp$$
}

echoboth()
{
	echo "$*"
	tet_infoline "$*"
}

errout()
{
	while read x
	do
		echoboth "$x"
	done
}

#
# syncup test_name timeout
#
syncup()
{
	. /tmp/ant$$

	if [ "$MASTERSLAVE" = "master" ]
	then
		dlpiut syncsend fd=0 msg=$1 sap=$gsap \
			timeout=$2 omchnaddr=$OTHERADDR \
			ourdstaddr=$OTHERADDR odstaddr=$OURADDR \
			framing=$gframing loop=n match=y > /tmp/tmp$$
	else
		dlpiut syncrecv fd=0 msg=$1 sap=$gsap \
			timeout=$2 omchnaddr=$OTHERADDR \
			ourdstaddr=$OTHERADDR odstaddr=$OURADDR \
			framing=$gframing loop=n match=y > /tmp/tmp$$
	fi


	. /tmp/tmp$$
	rm -f /tmp/tmp$$

	sleep 3
	[ "$msg" = "$1" ] && return 0
	echoboth "$1: sync up failed"
	echo ABORT
	tet_result ABORT
	ret=ABORT
	return 1
}

framing()
{
	. /tmp/ant$$

	if [ "$NETTYPE" = "ethernet" ]
	then
		gsap=1235
		gframing=ethernet
	else
		gsap=c0
		gframing=802.5
	fi

	echo "gsap=$gsap" >> /tmp/ant$$
	echo "gframing=$gframing" >> /tmp/ant$$
}
