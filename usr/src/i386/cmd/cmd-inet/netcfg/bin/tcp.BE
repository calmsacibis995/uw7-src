#!/bin/sh
#ident "@(#)tcp.BE	1.13"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1995 Legent Corporation
# All rights reserved.
#
#      SCCS IDENTIFICATION
#
#    Back end script for netconfig prompter.
#

exec 3>&2
exec 2>/dev/null

INTERFACE_NAME=$1
INTERFACE_NUMBER=$2

if [ ! "$NCFGBIN" ]
then
    NCFGBIN=/usr/lib/netcfg/bin
fi

if [ -f /etc/confnet.d/inet/interface ]
then
    old_config=`grep "^$INTERFACE_NAME:$INTERFACE_NUMBER" /etc/confnet.d/inet/interface`
    if [ "$old_config" ]
    then
	IP=`echo $old_config | awk -F: '{ print $3 }'`
	NM=`expr "$old_config" : '.*netmask[ 	]*\([^ 	:]*\)'`
	BD=`expr "$old_config" : '.*broadcast[ 	]*\([^ 	:]*\)'`
	TMP=`echo $old_config | awk -F: '{ print $6 }'`
	if [ ! "$TMP" -o "$TMP" = "add_interface" ]
	then
		FRAME_FMT="Ethernet-II"
	else
		FRAME_FMT="802.n"
	fi
    fi
fi

if [ ! "$FRAME_FMT" ]
then
	for TOPOLOGY in $NETWORK_MEDIA
	do
		if [ "$TOPOLOGY" = "ETHER" -o \
		     "$TOPOLOGY" = "ether" ]
		then
			FRAME_FMT="Ethernet-II"
		fi
	done

	if [ ! "$FRAME_FMT" ]
	then
		#
		# A frame format of 802.n is only valid for LLI 4.0
		# adapters. We use MDI_VERSION to differentiate.
		#
		if [ "$MDI_VERSION" ]
		then
			FRAME_FMT="802.n"
		else
			FRAME_FMT="Ethernet-II"
		fi
	fi
fi

if [ -f /etc/inet/hosts ]
then
    if [ "$IP" ]
    then
	hosts=`grep "^$IP[ 	]" /etc/inet/hosts | head -1`
	LOCAL_NAME=`echo $hosts | awk '{ print $3 }'`
    else
	LOCAL_NAME=`uname -n`
    fi
fi

if [ -f /etc/resolv.conf ]
then
    DOMAIN=`grep "^domain" /etc/resolv.conf | awk '{print $2}'`
fi

if [ ! "$DOMAIN" ]
then
	FQDN=`grep "^$IP[ 	]" /etc/inet/hosts | awk '{ print $2 }'`
	if [ "$FQDN" ]
	then
		DOMAIN=`echo $FQDN | cut -d. -f2-`
	fi
fi

if [ -f /etc/inet/config ]
then
    DEF_ROUTER=`grep "^4c" /etc/inet/config | cut -d: -f6 | awk '{print $3}'`

    if [ "$DEF_ROUTER" = "router_placeholder" ]
    then
	DEF_ROUTER=""
    fi
fi

GATEWAY="No"
if [ -f /etc/inet/inet.dfl ]
then
    GATEWAY=`grep "^ipforwarding" /etc/inet/inet.dfl | awk '{ print $2 }'`
    if [ "$GATEWAY" = "1" ]
    then
        GATEWAY="Yes"
    else
        GATEWAY="No"
    fi
fi

if [ -f /etc/netmgt/snmpd.conf ]
then
    SYS_CONTACT=`grep "^contact=" /etc/netmgt/snmpd.conf | head -1 | sed -e 's/^contact=//'`
    SYS_LOCATION=`grep "^location" /etc/netmgt/snmpd.conf | head -1 | sed -e 's/^location=//'`
fi

N_PTTY=32
ADVANCED="Advanced options"

while read line
do
    set - $line
    REQUEST=$1
    shift
    ARGS=$*
    case $REQUEST in

        LIST)
            set - $ARGS
            case $1 in

                BASIC)
		    echo "\
			{ LOCAL_NAME \"Host name\" LABEL \"Enter host name of this machine\" } \
			{ DOMAIN \"Domain name\" LABEL \"Enter internet domain name for the network\" } \
			{ IP \"IP address\" IPBOX \"Enter IP address of this machine\" } \
			{ NM \"Netmask\" IPBOX \"Enter netmask of network, e.g. 255.255.255.0\" } \
			{ BD \"Broadcast address\" IPBOX \"Enter broadcast address of network\" } \
			{ DEF_ROUTER \"Default router\" IPBOX \"Enter IP address of default router\" } \
			{ ADVANCED \"\" NEWSCREEN \"Advanced configuration options\" } \
		    "
                    ;;
                ADVANCED)
		    echo "\
			{ GATEWAY \"Act as router?\" RADIO \"Do you want this machine to be a router on the network?\" } \
			{ N_PTTY \"Pseudo ttys\" LABEL \"Specify number of pseudo ttys to configure\" } \
			{ FRAME_FMT \"Frame format\" LIST \"Specify framing type for this network\" } \
			{ SYS_CONTACT \"Administrator for this system\" LABEL \"Specify owner of this machine\" } \
			{ SYS_LOCATION \"Location of this system\" LABEL \"Specify location of this machine\" } \
		    "
		    ;;
                *)
                    echo "BE: Unknown argument to LIST" >&3
                    exit 1
                    ;;
            esac
            ;;

        CURRENT)
            set - $ARGS
            case $1 in

                BASIC)
		    echo "\
			{ LOCAL_NAME 1 $LOCAL_NAME } \
			{ DOMAIN 1 $DOMAIN } \
			{ IP 1 $IP } \
			{ NM 1 $NM } \
			{ BD 1 $BD } \
			{ DEF_ROUTER 1 $DEF_ROUTER } \
			{ ADVANCED 1 \"$ADVANCED\" } \
		    "
		    ;;
                ADVANCED)
		    echo " \
			{ GATEWAY 1 $GATEWAY } \
			{ N_PTTY 1 $N_PTTY } \
			{ FRAME_FMT 1 $FRAME_FMT } \
			{ SYS_CONTACT 1 \"$SYS_CONTACT\" } \
			{ SYS_LOCATION 1 \"$SYS_LOCATION\" } \
		    "
		    ;;
                *)
                    echo "BE: Unknown argument to CURRENT" >&3
                    exit 1
                    ;;
            esac
            ;;

        VALUES)
            set - $ARGS
            case $1 in
                GATEWAY )
                    echo "Yes No"
                    ;;
		FRAME_FMT )
		    if [ "$MDI_VERSION" -a "$FRAME_FMT" = "802.n" ]
		    then
			echo "802.n"
		    else
			echo "Ethernet-II 802.n"
		    fi
		    ;;
                *)
                    echo "BE: Unknown argument to VALUES" >&3
                    exit 1
                    ;;
            esac
            ;;

        SET)
            set - $ARGS
            case $1 in

                IP)
		    if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
		    then
			IP="$2"
			echo "OK"
                    elif echo "$2" | \
                        grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                    then
                        echo "ERROR  IP address invalid"
                    else
                        ${NCFGBIN}/tcpcfg -i $2 -1 > /tmp/tcp.conf 2>/dev/null
                        if [ $? -eq 1 ]
                        then
                            echo "ERROR  IP address invalid"
                            rm -f /tmp/tcp.conf
                        else
                            . /tmp/tcp.conf
                            rm /tmp/tcp.conf
                            echo "OK"
                        fi
                    fi
                    ;;
                NM)
		    if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
		    then
			NM="$2"
			echo "OK"
                    elif echo "$2" | \
                        grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                    then
                        echo "ERROR  Netmask invalid"
                    elif [ "$IP" ]
		    then
                        ${NCFGBIN}/tcpcfg -i $IP -n $2 -1 > /tmp/tcp.conf
                        if [ $? -eq 1 ]
                        then
                            echo "ERROR  Netmask invalid"
                            rm -f /tmp/tcp.conf
                        else
                            . /tmp/tcp.conf
                            rm /tmp/tcp.conf
                            echo "OK"
                        fi
                    fi
                    ;;
                BD)
		    if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
		    then
			BD="$2"
			echo "OK"
                    elif echo "$2" | \
                        grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                    then
                        echo "ERROR  Broadcast address invalid"
                    else
                        BD=$2
                        echo "OK"
                    fi
                    ;;
                DEF_ROUTER)
		    if [ "$2" != "" -a "$2" != "..." ]
		    then
			if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
			then
			    DEF_ROUTER="$2"
			    echo "OK"
                	elif echo "$2" | \
                            grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                	then
                            echo "ERROR  Router address invalid"
                	else
                            DEF_ROUTER=$2
                            echo "OK"
                	fi
		    else
			#
			# User doesn't want to set a default route.
			#
			DEF_ROUTER=""
			echo "OK"
		    fi
                    ;;
                GATEWAY)
                    GATEWAY="$2"
                    echo "OK"
                    ;;
                DOMAIN)
                    DOMAIN="$2"
                    echo "OK"
                    ;;
                SYS_CONTACT | SYS_LOCATION )
                    VAR="$1"
                    shift
                    VAL="$*"
                    eval $VAR=$VAL
                    echo "OK"
                    ;;
                LOCAL_NAME)
                    if echo "$2" | egrep "^[0-9].*" > /dev/null
                    then
                        echo "ERROR  Host name must not begin with a digit"
                    elif echo "$2" | egrep "[^-0-9A-Za-z]" > /dev/null
                    then
                        echo "ERROR  Host name must consist of alphanumeric characters or '-'"
                    else
                        if [ -f /etc/inet/hosts -a "`grep $2 /etc/hosts`" ]
                        then
                            IP=`grep "$2" /etc/inet/hosts | grep -v '^#' | grep -v "^127" | head -1 | awk '{ print $1 }'`
                        fi
                        LOCAL_NAME=$2
                        echo "OK"
                    fi
                    ;;
                N_PTTY)
                    if echo "$2" | grep "[^0-9]" > /dev/null 2>&1
                    then
                        echo "ERROR Must specify a number"
                    else
                        N_PTTY=$2
                        echo "OK"
                    fi
                    ;;
		FRAME_FMT)
		    FRAME_FMT=$2
		    echo "OK"
		    ;;
                *)
                    echo "BE: Unknown argument to SET" >&3
                    exit 1
                    ;;
            esac
            ;;
	PAGEINIT)
	    case $1 in
		BASIC)
		    echo "{Internet Protocol Configuration} {} {NET_tcpip} {tcpN.config.html} {OK}"
		    ;;
		ADVANCED)
		    echo "{Advanced Configuration} {} {NET_tcpip} {tcpN.advconfig.html} {OK}"
		    ;;
	        *)
		    echo "BE: Unknown argument to PAGEINIT" >&3
		    exit 1
		    ;;
	    esac
	    ;;
        USER_DONE)
	    if [ "$IP" = "" -o "$IP" = "..." ]
	    then
		echo "ERROR You must specify an IP address"
	    elif [ "$BD" = "" -o "$BD" = "..." ]
	    then
		echo "ERROR You must specify a broadcast address"
	    else
        	echo "OK"
	    fi
            ;;
        *)
            echo "BE: Unknown request <$line>" >&3
            exit 1
            ;;
    esac
done
