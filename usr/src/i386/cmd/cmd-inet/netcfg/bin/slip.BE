#!/bin/sh
#ident	"@(#)slip.BE	1.2"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1994 Legent Corporation
# All rights reserved.
#
#      SCCS IDENTIFICATION
#
#    Back end script for netconfig prompter.
#

exec 3>&2
exec 2>/dev/null

LINK_TYPE=$1
KEY=$2
RECONF=$3

GATEWAY=No

if [ -s /etc/inet/inet.dfl ]
then
	GATEWAY=`grep "^ipforwarding" /etc/inet/inet.dfl | \
		awk '{ print $2 }'`
	if [ "$GATEWAY" = "1" ]
	then
		GATEWAY=Yes
	else
		GATEWAY=No
	fi
fi

SLIP_LOCAL_NAME="sl-`uname -n`"

if [ "$SLIP_LOCAL_NAME" ]
then
	SLIP_HOST_IP=`grep "$SLIP_LOCAL_NAME" /etc/inet/hosts | \
		grep -v '^#' | head -1 | awk '{ print $1 }'`
fi

if [ "$RECONF" ]
then
	if [ "$LINK_TYPE" = "STATIC" ]
	then
		old_sl=`grep '^[ 	]*/usr/sbin/slattach' \
			/etc/inet/slip.start | grep $KEY | grep -v '^#'`

		#
		# Is there a baud rate specified?
		#
		if echo "$old_sl" | awk '{ print $(NF) }' | \
			grep "[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}" \
			> /dev/null 2>&1
		then
			#
			# Last argument is an IP address so it must be the
			# netmask...
			#
			if [ "`echo $old_sl | grep "\-d"`" ]
			then
				SLIP_TTY=none
				SLIP_BAUD=none
				SLIP_UUCP_NAME=$KEY
				SLIP_HOST_IP=`echo $old_sl | awk '{ print $(NF-2) }'`
				SLIP_DEST_IP=`echo $old_sl | awk '{ print $(NF-1) }'`
			else
				SLIP_TTY=$KEY
				SLIP_BAUD=38400
				SLIP_UUCP_NAME=
				SLIP_DEST_IP=`echo $old_sl | awk '{ print $(NF-1) }'`
				SLIP_HOST_IP=`echo $old_sl | awk '{ print $(NF-2) }'`
			fi
		else
			#
			# Last argument is not an IP address so it must be the
			# baud rate...
			#
			if [ "`echo $old_sl | grep "\-d"`" ]
			then
				SLIP_TTY=none
				SLIP_BAUD=none
				SLIP_UUCP_NAME=$KEY
				SLIP_HOST_IP=`echo $old_sl | awk '{ print $(NF-3) }'`
				SLIP_DEST_IP=`echo $old_sl | awk '{ print $(NF-2) }'`
			else
				SLIP_TTY=$KEY
				SLIP_UUCP_NAME=
				SLIP_BAUD=`echo $old_sl | awk '{ print $(NF) }' | \
					sed -e 's/&//'`
				SLIP_DEST_IP=`echo $old_sl | awk '{ print $(NF-2) }'`
				SLIP_HOST_IP=`echo $old_sl | awk '{ print $(NF-3) }'`
			fi
		fi

		if echo "$old_sl" | grep "\+a" > /dev/null 2>&1
		then
			SLIP_ARP="On"
		else
			SLIP_ARP="Off"
		fi

		if echo "$old_sl" | grep "\+c" > /dev/null 2>&1
		then
			SLIP_COMP="On"
		else
			SLIP_COMP="Off"
		fi

		if echo "$old_sl" | grep "\+e" > /dev/null 2>&1
		then
			SLIP_AUTO_COMP="On"
		else
			SLIP_AUTO_COMP="Off"
		fi

		if echo "$old_sl" | grep "\+f" > /dev/null 2>&1
		then
			SLIP_FLOW="On"
		else
			SLIP_FLOW="Off"
		fi

		if echo "$old_sl" | grep "\+i" > /dev/null 2>&1
		then
			SLIP_ICMP="On"
		else
			SLIP_ICMP="Off"
		fi

		if echo "$old_sl" | grep "\-m" > /dev/null 2>&1
		then
			SLIP_MTU="`echo $old_sl | \
				sed -e 's/.*-m \([0-9]*\).*/\1/'`"
		fi

		if echo "$old_sl" | grep "\+v" > /dev/null 2>&1
		then
			SLIP_VERB="On"
		else
			SLIP_VERB="Off"
		fi

		hosts=`grep "^$SLIP_HOST_IP[ 	]" /etc/inet/hosts | \
			grep -v '^#' | head -1`
		if [ "$hosts" ]
		then
			SLIP_LOCAL_NAME=`echo $hosts | awk '{ print $2 }'`
		fi

		hosts=`grep "^$SLIP_DEST_IP[ 	]*" /etc/inet/hosts | \
			grep -v '^#' | head -1`
		if [ "$hosts" ]
		then
			SLIP_DEST_NAME=`echo $hosts | awk '{ print $2 }'`
		fi
	elif [ "$LINK_TYPE" = "INC" ]
	then
		SLIP_LOGIN=$KEY
		SLIP_SHELL=`grep "^$KEY" /etc/passwd | awk -F: '{ print $7}'`
	fi
else
	SLIP_LOGIN=nslip
	SLIP_TTY=/dev/term/01
	SLIP_BAUD=38400
	SLIP_ARP="Off"
	SLIP_COMP="Off"
	SLIP_AUTO_COMP="Off"
	SLIP_FLOW="Off"
	SLIP_ICMP="Off"
	SLIP_MTU="296"
	SLIP_VERB="Off"
	SLIP_SHELL=
fi

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
		    if [ "$LINK_TYPE" = "STATIC" ]
		    then
			echo "\
			    { SLIP_UUCP_NAME \"UUCP system name\" LABEL \"\" } \
			    { SLIP_TTY \"Tty line for SLIP\" LIST \"\" } \
			    { SLIP_BAUD \"Baud rate for $SLIP_TTY\" LIST \"\" } \
			    { SLIP_LOCAL_NAME \"Local host name\" LABEL \"\" } \
			    { SLIP_HOST_IP \"Host IP address\" IPBOX \"\" } \
			    { SLIP_DEST_NAME \"Destination name\" LABEL \"\" } \
			    { SLIP_DEST_IP \"Destination IP address\" IPBOX \"\" } \
			    { GATEWAY \"Gateway\" RADIO \"\" } \
			    { ADVANCED \"Advanced options\" NEWSCREEN \"\" } \
			"
		    else
		        echo "\
			    { SLIP_LOGIN \"Login name for SLIP\" LABEL \"\" } \
			    { SLIP_SHELL \"Login shell script\" LABEL \"\" } \
			"
		    fi
		    ;;
                ADVANCED)
		    if [ "$LINK_TYPE" = "STATIC" ]
		    then
			echo "\
			    { SLIP_VERB \"Debugging\" RADIO \"\" } \
			    { SLIP_COMP \"TCP/IP header compression\" RADIO \"\" } \
			    { SLIP_AUTO_COMP \"Auto detect header compression\" RADIO \"\" } \
			    { SLIP_FLOW \"Hardware flow control\" RADIO \"\" } \
			    { SLIP_ICMP \"Suppress ICMP packets\" RADIO \"\" } \
			    { SLIP_MTU \"Maximum transmission unit (MTU)\" LABEL \"\" } \
			    { SLIP_ARP \"Proxy ARP\" RADIO \"\" } \
		        "
		    else
			echo ""
		    fi
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
		    if [ "$LINK_TYPE" = "STATIC" ]
		    then
			    if [ "$SLIP_UUCP_NAME" = "" ]
			    then
			        echo "{ SLIP_TTY 1 \"$SLIP_TTY\" } \
			              { SLIP_BAUD 1 \"$SLIP_BAUD\" } \c"
			    else
			        echo "{ SLIP_TTY 0 \"$SLIP_TTY\" } \
			              { SLIP_BAUD 0 \"$SLIP_BAUD\" } \c"
			    fi
			    echo "\
			    { SLIP_HOST_IP 1 \"$SLIP_HOST_IP\" } \
			    { SLIP_DEST_IP 1 \"$SLIP_DEST_IP\" } \
			    { SLIP_DEST_NAME 1 \"$SLIP_DEST_NAME\" } \
			    { SLIP_LOCAL_NAME 1 \"$SLIP_LOCAL_NAME\" } \c"
			    if [ "$SLIP_TTY" = "none" ]
			    then
			         echo "{ SLIP_UUCP_NAME 1 \"$SLIP_UUCP_NAME\" } \c"
			    else
			         echo "{ SLIP_UUCP_NAME 0 \"$SLIP_UUCP_NAME\" } \c"
			    fi
			    echo "{ GATEWAY 1 \"$GATEWAY\" } \c"
			    echo "{ ADVANCED 1 \"$ADVANCED\" }"
		    else
			echo "\
			    { SLIP_LOGIN 1 \"$SLIP_LOGIN\" } \
			    { SLIP_SHELL 1 \"$SLIP_SHELL\" } \
			"
		    fi
		    ;;
                ADVANCED)
		    if [ "$LINK_TYPE" = "STATIC" ]
		    then
			echo "\
			    { SLIP_ARP 1 \"$SLIP_ARP\" } \
			    { SLIP_COMP 1 \"$SLIP_COMP\" } \
			    { SLIP_AUTO_COMP 1 \"$SLIP_AUTO_COMP\" } \
			    { SLIP_FLOW 1 \"$SLIP_FLOW\" } \
			    { SLIP_MTU 1 \"$SLIP_MTU\" } \
			    { SLIP_ICMP 1 \"$SLIP_ICMP\" } \
			    { SLIP_VERB 1 \"$SLIP_VERB\" } \
		        "
		    else
			echo ""
		    fi
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

                GATEWAY)
                    echo "Yes No"
                    ;;
		SLIP_ARP | SLIP_ICMP | SLIP_VERB | SLIP_AUTO_COMP | SLIP_COMP |\
		SLIP_FLOW)
		    echo "On Off"
		    ;;
                SLIP_TTY)
                    echo "`ls -m /dev/term/* | tr ',\n' '  '` none"
                    ;;
                SLIP_BAUD)
                    echo "50 75 110 134 150 200 300 600 1200 1800 2400 4800 9600 19200 38400 none"
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

                SLIP_HOST_IP)
		    if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
		    then
			IP="$2"
			echo "OK"
                    elif echo "$2" | \
                        grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                    then
                        echo "ERROR  IP address invalid"
                    else
                        SLIP_HOST_IP=$2
                        echo "OK"
                    fi
                    ;;
                SLIP_DEST_IP)
		    if echo "$2" | egrep '^\.|\.\.|\.$' > /dev/null
		    then
			IP="$2"
			echo "OK"
                    elif echo "$2" | \
                        grep -v "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
                    then
                        echo "ERROR  IP address invalid"
                    else
                        SLIP_DEST_IP=$2
                        echo "OK"
                    fi
                    ;;
		SLIP_ARP | SLIP_VERB | SLIP_COMP | SLIP_FLOW | SLIP_ICMP | \
		SLIP_AUTO_COMP | GATEWAY)
		    VAR=$1
		    VAL=$2
		    eval $VAR=$VAL
		    echo "OK"
		    ;;
                SLIP_TTY)
		    if [ "$2" = "none" ]
		    then
			SLIP_TTY=$2
			SLIP_BAUD=$2
			echo "OK"
                    elif ls $2 > /dev/null 2>&1
                    then
                        SLIP_TTY=$2
                        echo "OK"
                    else
                        echo "ERROR  $2 not a valid tty"
                    fi
                    ;;
                SLIP_BAUD)
                    ok=
                    for BAUD in 50 75 110 134 200 300 600 \
                        1200 2400 4800 9600 19200 38400 none
                    do
                        if [ "$2" = "$BAUD" ]
                        then
                            SLIP_BAUD=$2
			    if [ "$SLIP_BAUD" = "none" ]
			    then
				SLIP_TTY="none"
			    fi
                            echo "OK"
                            ok=yes
                        fi
                    done
                    if [ ! "$ok" ]
                    then
                        echo "ERROR  illegal baud rate - $2"
                    fi
                    ;;
                SLIP_LOCAL_NAME)
		    if [ ! "$2" -o "$2" = "\"\"" ]
		    then
			SLIP_LOCAL_NAME=
			echo "OK"
		    else
                    if echo "$2" | egrep "^[0-9].*" > /dev/null 2>&1
                    then
                        echo "ERROR Host name must not begin with a digit"
                    elif echo "$2" | egrep "[^-0-9A-Za-z_]" > /dev/null 2>&1
                    then
                        echo "ERROR Host name must consist of alphanumeric characters, '-', or '_'"
                    else
                        SLIP_LOCAL_NAME="$2"
                        if [ -f ${SSO_CLIENT_ROOT}/etc/hosts -a \
			     "`grep $2 ${SSO_CLIENT_ROOT}/etc/hosts`" ]
                        then
                            SLIP_HOST_IP=`grep "$2" ${SSO_CLIENT_ROOT}/etc/hosts | grep -v '^#' | head -1 | awk '{ print $1 }'`
                        fi
                        echo "OK"
                    fi
		    fi
                    ;;
                SLIP_DEST_NAME)
		    if [ ! "$2" -o "$2" = "\"\"" ]
		    then
			SLIP_DEST_NAME=
			echo "OK"
		    else
                    if echo "$2" | egrep "^[0-9].*" > /dev/null 2>&1
                    then
                        echo "ERROR Host name must not begin with a digit"
                    elif echo "$2" | egrep "[^-0-9A-Za-z_]" > /dev/null 2>&1
                    then
                        echo "ERROR Host name must consist of alphanumeric characters, '-', or '_'"
                    else
                        SLIP_DEST_NAME="$2"
                        if [ -f ${SSO_CLIENT_ROOT}/etc/hosts -a \
			     "`grep $2 ${SSO_CLIENT_ROOT}/etc/hosts`" ]
                        then
                            SLIP_DEST_IP=`grep "$2" ${SSO_CLIENT_ROOT}/etc/hosts | grep -v '^#' | head -1 | awk '{ print $1 }'`
                        fi
                        echo "OK"
                    fi
		    fi
                    ;;
		SLIP_UUCP_NAME)
		    if [ "$2" = "\"\"" -o "$2" = "" ]
		    then
			SLIP_UUCP_NAME=""
			echo "OK"
		    elif echo "$2" | egrep "^[0-9].*" > /dev/null 2>&1
		    then
			echo "ERROR  UUCP System name must not begin with a digit"
		    elif echo "$2" | egrep "[^-0-9A-Za-z_]" > /dev/null 2>&1
		    then
			echo "ERROR  UUCP System name must consist of alpanumeric characters, '-', or '_'"
		    else
			SLIP_UUCP_NAME="$2"
			echo "OK"
		    fi
		    ;;
                SLIP_LOGIN)
                    if [ "$2" = "\"\"" -o "$2" = "" ]
                    then
                        echo "ERROR  Must specify a login name for SLIP"
                    else
                        if [ `echo $2 | wc -c` -gt 9 ]
                        then
                            echo "ERROR login name must be 8 characters or less"
                        elif echo $2 | grep "^[^a-zA-Z]" > /dev/null 2>&1
                        then
                            echo "ERROR login name must begin with an alphabetic character"
                        elif echo $2 | grep "[*/:?]" > /dev/null 2>&1
                        then
                            echo "ERROR login name must only contain alpanumeric characters"
                        else
                            SLIP_LOGIN=$2
                            echo "OK"
                        fi
                    fi
                    ;;
		SLIP_SHELL)
                    if [ "$2" = "\"\"" -o "$2" = "" ]
                    then
                        echo "ERROR  Must specify a login script for SLIP"
                    else
		        SLIP_SHELL=$2
		        echo "OK"
		    fi
		    ;;
		SLIP_MTU)
                    if [ "$2" = "\"\"" -o "$2" = "" ]
                    then
                        SLIP_MTU=
                        echo "OK"
                    else
		        if echo "$2" | egrep "^[0-9]*$" > /dev/null 2>&1
		        then
			    SLIP_MTU=$2
			    echo "OK"
		        else
			    echo "ERROR  Invalid maximum transmission unit.\nValue must be a digit."
		        fi
		    fi
		    ;;
                *)
                    echo "BE: Unknown argument to SET" >&3
                    exit 1
                    ;;
            esac
            ;;
        USER_DONE)
	    if [ "$SLIP_TTY" = "none" ]
	    then
		SLIP_TTY=""
		SLIP_BAUD=""
	    fi
            echo "OK"
            ;;
	PAGEINIT)
	    case $1 in
		BASIC)
		    if [ "$LINK_TYPE" = "STATIC" ]
		    then
		    	echo "{SLIP Configuration} {} {NET_tcpip} {slipT.outgoing.html} {OK}"
		    else
			echo "{SLIP Configuration} {} {NET_tcpip} {slipT.incoming.html} {OK}"
		    fi
		    ;;
		ADVANCED)
		    echo "{Advanced Configuration} {} {NET_tcpip} {slipT.advconfig.html} {OK}"
		    ;;
		*)
		    echo "BE: Unknown argument to PAGEINIT" >&3
		    exit 1
		    ;;
	    esac
	    ;;
        *)
            echo "BE: Unknown request <$line>" >&3
            exit 1
            ;;
    esac
done
