#!/bin/sh

#ident	"@(#)ipexec.sh	1.4"

# This script is called from pppd with the following arguments when a link
# goes up or down:
#
#    event ifname new_src new_dst old_src old_dst dns_addr dns2_addr default
#
# Where event is "up", "down", "add", or "delete", ifname is the interface
# name, and the next 4 arguments are the old and new IP addresses
# in dotted decimal notation.
#
# This is a general example shell script which will rewrite static routes
# to the interface if the address changes.  Note that pppd will wait for
# this script to complete before continuing.
#
if [ $# -lt 9 ]
then
	logger ipexec.sh "wrong number of arguments"
	exit 1;
fi

event=$1; ifname=$2; new_src=$3; new_dst=$4
old_src=$5; old_dst=$6; dns=$7; dns2=$8; defroute=$9

#logger ipexec.sh $*

case $event in
    add)
    	if [ $defroute = "default" ] ; then
	    # Set as the default route
	    # logger ipexec.sh "route add default $new_dst"
	    route add default $new_dst
	fi
	;;
    delete)
    	if [ $defroute = "default" ] ; then
	    # Delete as the default route
	    # logger ipexec.sh "route add default $old_dst"
	    route delete default $old_dst
	fi
	;;
    down)
	exit 0
	;;
    up)
	if [ "$old_src" != "$new_src" ]
	then
	    # change static ("S") routes which point to old_src and ifname
	    netstat -rn | awk '
		$6 == "'$ifname'" && $2 == "'$old_src'" && index($3, "S") \
		    { print "route change " $1 " '$new_src'" }' |
		sh 
		#| logger ipexec.sh $loglevel
	fi
	;;
    *)
	logger ipexec.sh "unknown event: $event"
	exit 1
	;;
esac

exit 0
