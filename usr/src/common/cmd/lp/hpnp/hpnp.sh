#!/bin/ksh
#ident	"@(#)hpnp.sh	1.3"
#		copyright	"%c%"

# (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
# (c)Copyright 1983 Regents of the University of California
# (c)Copyright 1988, 1989 by Carnegie Mellon University
#
#                          RESTRICTED RIGHTS LEGEND
# Use, duplication, or disclosure by the U.S. Government is subject to
# restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
# Technical Data and Computer Software clause in DFARS 252.227-7013.
#
#                          Hewlett-Packard Company
#                          3000 Hanover Street
#                          Palo Alto, CA 94304 U.S.A.
#
#	mkdev/hpnp - HP Network Printer Configuration
#

# Define return values
OK=0 FAIL=1 STOP=10 HALT=11

# Define traps
set_trap()
{
	trap 'echo "\nInterrupted! Exiting ..."; cleanup 1 $1' 1 2 3 15
}

unset_trap()
{
	trap '' 1 2 3 15
}

# Miscellaneous functions

clear_scr()
{
	# check if the terminal environment is set up
	[ "$TERM" ] && clear 2> /dev/null
}

cleanup()
{
	case $2 in
		inetd)      if [ -f "$HPNPDIR/inetd.cf.orig" ]
		            then
				echo "Restoring $INETDCONF"
				mv -f -- "$HPNPDIR/inetd.cf.orig" "$INETDCONF"
				rm -f -- "$HPNPDIR/inetd.cf.orig"
		            fi
			    ;;
		serivces)   if [ -f "$HPNPDIR/services.orig" ]
			    then
				echo "Restoring $SERVICES"
				mv -f -- "$HPNPDIR/services.orig" "$SERVICES"
				rm -f -- "$HPNPDIR/services.orig"
			    fi
			    ;;
	esac
	unset_trap
	exit $1
}

getyn()
{
	while echo "$* (y/n) \c">&2
	do
		read -r yn rest
		case "$yn" in
			[yY]*) return $OK
			      ;;
			[nN]*) return $FAIL
			      ;;
			*)    echo "Please answer y or n" >&2
			      ;;
		esac
	done
}

prompt() {
	typeset mesg=$1
	while   echo "$mesg [$2]: \c" >&2
	do      read -r cmd
		case "$cmd" in
		+x|-x)  set $cmd
			;;
		Q|q)    return 1
			;;
		!*)     eval `expr "$cmd" : "!\(.*\)"`
			;;
		"")     # If there is an second argument use it
			# as the default else loop until 'cmd' is set
			[ -n "$2" ] && {
				cmd=$2
				return 0
			}
			: continue
			;;
		*)      return 0
			;;
		esac
	done
}

error()
{
	echo "\nError: $*" >&2
	exit $FAIL
}

install() {

clear_scr

progname=$0

if [ -f "$HPNPDIR/.installed" ]
then
	error $progname: Network Printing already installed.
fi

# Check that root is running us
typeset -i UID
UID=`id -u`
[ $UID -ne 0 ] && {
	error $progname: You must be root to execute this script.
}


STARTUTIL=1
if [ -n "$_no_prompt" ]
then

	cat <<EOF

Installing the startup configuration utilities results in the following:

    o /etc/inetd.conf and /etc/services are modified to support BOOTP if
      necessary.

EOF

	getyn Install the startup configuration utilities

	if [ $? -ne $OK ]
	then
		STARTUTIL=0
	fi

	clear_scr
fi

if [ $STARTUTIL -eq 1 ]
then
	if [ -f "$BOOTPD" ]
	then
		echo $BOOTPD >> "$HPNPDIR/install.files"
	fi

	if [ ! -f "$BOOTPTAB" ]
	then
		cp -f -- "$HPNPDIR/examples/bootptab" "$BOOTPTAB"
		chmod a=r "$BOOTPTAB"
		chown root:sys "$BOOTPTAB"
	fi
	echo $BOOTPTAB >> "$HPNPDIR/install.files"

	if [ -f "$HPNPDIR/hpnpf" ]
	then
		echo $HPNPDIR/hpnpf >> "$HPNPDIR/install.files"
	fi

	if [ -f "$HPNPDIR/hpnptyd" ]
	then
		echo $HPNPDIR/hpnptyd >> "$HPNPDIR/install.files"
	fi

	if [ -f "$HPNPDIR/hpnpcfg" ]
	then
		echo $HPNPDIR/hpnpcfg >> "$HPNPDIR/install.files"
	fi

	if [ -f "$HPNPDIR/sh/hpnp.model" ]
	then
		echo $HPNPDIR/sh/hpnp.model >> "$HPNPDIR/install.files"
	fi

	grep -- '^bootps' "$INETDCONF" > /dev/null
	if [ $? -ne 0 ]
	then
		# Remember $INETDCONF maybe a symlink
		cp -fp -- "$INETDCONF" "$HPNPDIR/inetd.cf.orig"
		set_trap inetd
		echo "Appending bootps entry to $INETDCONF"
		echo "# bootp added by hpnp" >> "$INETDCONF"
		echo "$BOOTP" >> $INETDCONF
		unset_trap
	fi

	egrep '^bootps[[:space:]]*[[:digit:]]*/udp' "$SERVICES" > /dev/null
	if [ $? -ne 0 ]
	then
		cp -fp -- "$SERVICES" "$HPNPDIR/services.orig"
		set_trap services
		echo "Appending bootps entry to $SERVICES"
		echo "$BOOTPS" >> "$SERVICES"
		unset_trap
	fi

	egrep '^bootpc[[:space:]]*[[:digit:]]*/udp' "$SERVICES" > /dev/null
	if [ $? -ne 0 ]
	then
		cp -fp -- "$SERVICES" "$HPNPDIR/services.orig"
		set_trap services
		echo "Appending bootpc entry to $SERVICES"
		echo "$BOOTPC" >> "$SERVICES"
		unset_trap
	fi

	PID=`ps -ef -o "pid comm"  | awk '($2 == "inetd" ) { print $1; exit 0}'`
	if [ -z "$PID" ]
	then
		echo "WARNING: No inetd process running!!"
	else
		kill -HUP $PID
		echo "Signalled inetd to re-read $INETDCONF"
	fi
fi

ls -- "$HPNPDIR/examples" | while read -r NAME
do
	echo $HPNPDIR/examples/$NAME >> "$HPNPDIR/install.files"
done

ls -- "$HPNPDIR/testfiles" | while read -r NAME
do
	echo $HPNPDIR/testfiles/$NAME >> "$HPNPDIR/install.files"
done

sort "$HPNPDIR/install.files" | uniq > "$HPNPDIR/install.tmp"
mv -f -- "$HPNPDIR/install.tmp" "$HPNPDIR/install.files"

if [ -n "$_no_prompt" ]
then
	clear_scr
	"$HPNPDIR/hpnpcfg"
else
	echo "You now need to run $HPNPDIR/hpnpcfg to configure your printers"
fi

if [ $STARTUTIL -eq 1 ]
then
	touch -- "$HPNPDIR/.installed"
fi

echo "Installation complete."
}

remove() {

	clear_scr
	echo "Removing: \c"

	if [ -f "$MODELDIR/hpplotter" ]
	then
		echo "          $MODELDIR/hpplotter"
		rm -f -- "$MODELDIR/hpplotter"
	fi

	if [ -f "$MODELDIR/laserjetIIIsi" ]
	then
		echo "          $MODELDIR/laserjetIIIsi"
		rm -f -- "$MODELDIR/laserjetIIIsi"
	fi

	if [ -f "$MODELDIR/laserjethpnp" ]
	then
		echo "          $MODELDIR/laserjethpnp"
		rm -f -- "$MODELDIR/laserjethpnp"
	fi

	if [ -f "$HPNPDIR/inetd.cf.orig" ]
	then
		echo "\nRestoring the original inetd.conf file"
		mv -f -- "$HPNPDIR/inetd.cf.orig" "$INETDCONF"
		rm -f -- "$HPNPDIR/inetd.cf.orig"
	fi

	echo "\n"

	if [ -d "/usr/spool/lp/admins/lp/interfaces/model.orig" ]
	then
		rm -rf -- "/usr/spool/lp/admins/lp/interfaces/model.orig"
	fi

	if [ -f "$HPNPDIR/install.files" ]
	then
		rm -f -- "$HPNPDIR/install.files"
	fi

	rm -f -- "$HPNPDIR/.installed"
}

usage()
{
	echo "\nUsage: $0 [-i | -r] [-n]\n"
	exit 2
}

#
# Main
#

PATH="/bin:/usr/bin:/etc"		# need more???
HPNPDIR="/usr/lib/hpnp"
MODELDIR="/etc/lp/model"
BOOTPTAB="/etc/bootptab"
SERVICES="/etc/services"
INETDCONF="/etc/inetd.conf"
BOOTPD="/usr/sbin/in.bootpd"
BOOTP="bootps	dgram	udp	wait	root	$BOOTPD	in.bootpd"
BOOTPS="bootps		   67/udp		bootp	#Bootstrap protocol server"
BOOTPC="bootpc		   68/udp			#Bootstrap protocol client"

typeset -rx HPNPDIR SERVICES BOOTPTAB INETDCONF

_no_prompt=""
_install=""
_remove=""

while getopts inr opt
do
	case $opt in

		i) _install="TRUE"
		   ;;

		n) _no_prompt="TRUE"
		   ;;

		r) _remove="TRUE"
		   ;;

		\?) usage
		    ;;

	esac
done

[ "$_install" -a "$_remove" ] && {
	usage
}

[ -n "$_install" ] && {
	install
	exit $OK
}

[ -n "$_remove" ] && {
	remove
	exit $OK
}

#
# Now we must be in interactive mode
#

clear_scr

echo "\n\t\tHP Network Printer Configuration"

while prompt "\n\nDo you want to install or remove network printing (i/r/q)?" "q"
do
	[ $? -eq 1 ] && exit $FAIL

	case $cmd in

		I|i) install
		     exit $OK
		     ;;

		R|r) remove
		     exit $OK
		     ;;

		Q|q) exit $OK
		     ;;

		*) echo "Please answer 'i','r' or 'q'."
		   ;;

	esac
done
