#!/sbin/sh
#ident	"@(#)initpkg:i386/cmd/initpkg/bcheckrc.sh	1.5.37.1"

if [ -z "$LC_ALL" -a -z "$LC_MESSAGES" ]
then
	if [ -z "$LANG" ]
	then
		LNG=`defadm locale LANG 2>/dev/null`
		if [ "$?" != 0 ]
		then LANG=C
		else eval $LNG
		fi
	fi
	LC_MESSAGES=/etc/inst/locale/$LANG
	export LANG LC_MESSAGES
fi
LABEL="UX:$0"

CAT=uxrc; export CAT

# This file has those commands necessary to check the file
# system, date, and anything else that should be done before mounting
# the file systems.

[ -x /sbin/dumpcheck ] && /sbin/dumpcheck

# Initialize node name from name stored in /etc/nodename
NODE=
[ -s /etc/nodename ] && read node </etc/nodename && [ -n "$node" ] &&
      NODE="-n $node"
if [ -n "$NODE" ]
then
	/sbin/setuname -t $NODE
fi
if [ -n "$node" ]
then
	pfmt -s nostd -g $CAT:47 "Node: %s\n" $node
else
	pfmt -s nostd -g $CAT:47 "Node: %s\n" "`/sbin/uname -n`"
fi

exec >/dev/null 2>&1

mkdir /proc
mkdir -p /dev/fd
mkdir -p /dev/_tcp
/sbin/mount /proc&
/sbin/mount /dev/fd&

wait
