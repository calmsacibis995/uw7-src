#ident "@(#)netbios.sh	1.8"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1995 Legent Corporation
# All rights reserved.
#
#      SCCS IDENTIFICATION
# TPI NetBIOS start/stop script
#
# Usage: /etc/netbios [ start | stop ]
#
wait_clean()
{
	netstat -a | awk '{print $4}' | egrep "nb-ss|nb-dgm|nb-ns" >/dev/null
	if [ $? -eq 1 ]
	then
		return
	else
		netstat -a | egrep "nb-ss|nb-dgm|nb-ns" | awk '{print $6}' | egrep TIME_WAIT >/dev/null
		if [ $? -eq 0 ]
		then
			#
			# When netbios is shut down the underlying tcp connections
			# may remain in TIME_WAIT state for up to four minutes.
			# During this period netbios cannot be restarted since it 
			# will be unable to bind to its port.
			# So wait until all old connections have gone.

			WARNED=0
			while [ $? -eq 0 ]
			do
				if [ "$WARNED" = 0 ] 
				then
					dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$WAITING" "TPI NetBIOS waiting for connections in TIME-WAIT state to clear. This may take several minutes."
					WARNED=1
				fi
				sleep 10
				netstat -a | egrep "nb-ss|nb-dgm|nb-ns"|grep TIME_WAIT  >/dev/null
			done
		else 
			dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$VISIONFS" "VisionFS or another NetBIOS product seems to be running. Stop this product before starting NetBIOS again."
			exit 1
		fi
	fi
}
if [ -d /etc/conf/kconfig.d ]; then
	PATH=$PATH:/usr/ucb
fi
if [ -d /usr/sbin ]; then
	PATH=$PATH:/usr/sbin
fi

PIDFILE=/etc/inet/nbd.pid
DEFAULT=/etc/inet/nb.conf
case $1 in
start)
	if [ -f $PIDFILE ]
	then
		ps -p `cat $PIDFILE` > /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$ALREADY_STARTED" "TPI NetBIOS is already running."
			exit 0
		fi
	fi

	dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$STARTING" "Starting TPI NetBIOS:"
	if [ -r $DEFAULT ]
	then
		. $DEFAULT
	fi

	export NB_ADDR 
	
	wait_clean

	/usr/sbin/in.nbd
	if [ -r /etc/lmhosts ]; then
		/usr/sbin/nbtstat -R
	fi
	dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$STARTED" "TPI NetBIOS start completed."
	;;
stop)
	dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$STOPPING" "Stopping TPI NetBIOS...\n"

	if [ -f $PIDFILE ]
	then
		kill `cat $PIDFILE` 2> /dev/null
	fi
	dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$STOPPED" "TPI NetBIOS stopped.\n"
	;;
*)
	dspmsg "$MF_NETBIOSRC" -s "$MS_NETBIOSRC" "$USAGE" "Usage: /etc/netbios [ start | stop ]\n"
	exit 1
	;;
esac
exit 0
