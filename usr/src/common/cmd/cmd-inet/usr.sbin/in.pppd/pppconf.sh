#!/bin/ksh
#ident	"@(#)pppconf.sh	1.2"
#ident	"$Header$"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1987-1995 Legent Corporation
# All rights reserved.
#
#      SCCS IDENTIFICATION
#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

# RESTRICTED RIGHTS
#
# These programs are supplied under a license.  They may be used,
# disclosed, and/or copied only as permitted under such license
# agreement.  Any copy must contain the above copyright notice and
# this restricted rights notice.  Use, copying, and/or disclosure
# of the programs is strictly prohibited unless otherwise provided
# in the license agreement.


#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


:
#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/pppconf.sh	1.5"
#ident	"/proj/tcp/usl/ppp-5.1/lcvs/usr/src/common/cmd/cmd-inet/usr.sbin/in.pppd/pppconf.sh,v 1.9 1995/08/25 20:47:41 neil Exp"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

CAT="pppconf"
export CAT
OK=0
FAIL=1

PATH=/bin:/usr/bin:/usr/sbin/:/etc:/etc/conf/bin

PPPHOSTS=/etc/inet/ppphosts
PPPFILTER=/etc/inet/pppfilter
POOLFILE=/etc/addrpool
PPPAUTH=/etc/inet/pppauth
HOSTS=/etc/inet/hosts
PPPSHELL=/usr/lib/ppp/ppp
PPPDIR=/usr/lib/ppp
PASSWORD=/etc/passwd
PPPDAEMON=/usr/sbin/in.pppd
PPPSTAT=/usr/sbin/pppstat
PPPATTACH=/usr/sbin/pppattach
SAMP_DIR=/etc/inet

##############################################################################
# Possible baud rates 
# 

BAUDS="50 75 134 150 200 300 600 1200 2400 4800 9600 19200 38400"

##############################################################################
# Format of parameter specifications
#
# <TAG>="<parameter name>:<default value>:<possible values>:<description string>
#

##############################################################################
# Parameters Common among all link types
#

FLOW="flow:rtscts:none rtscts xonxoff:Type of flow control to use for link:155"
IDLE="idle:forever::PPP connection inactivity timeout in minutes:156"
DEBUG="debug:0:0 1 2:PPP link debugging level:157"
PROXY="proxy:off::Install proxy arp entry in arp table:158"
REQTMOUT="reqtmout:3::Timeout value for PPP configure-request and termination-request packets:159"
CONF="conf:10::Maximum number of PPP configure request retries:160"
TEM="term:2::Maximum number of PPP termination request retries:161"
NAK="nak:10::Maximum allowable number of remote PPP configure nak retries:162"
MRU="mru:296::Maximum receive unit size in bytes:163"
ACCM="accm:ffffffff::Async control character map in hex:164"
MGC="nomgc:off::Disable Magic number negotiation (off = enabled):165"
PROTCOMP="noprotcomp:off::Disable Protocol field compression (off = enabled):166"
ACCOMP="noaccomp:off::Disable Address-control field compression (off = enabled):167"
AUTH="auth:none:none pap chap:Link Authentication:168"
AUTHTMOUT="authtmout:1::Time PPP waits for peer to authenticate itself in minutes:169"
AUTHNAME="name:none::Local host name used in PAP and CHAP negotiation:170"
IPADDR="noipaddr:off::Disable IP address negotiation (off = enabled):171"
RFC1172="rfc1172addr:off::Use RFC1172 IP addresses negotiation:172"
NOVJ="novj:off::Disable VJ TCP/IP header compression:173"
MAXSLOT="maxslot:16:3 4 5 6 7 8 9 10 11 12 13 14 15 16:Number of VJ compression slots:174"
NOSLOTCOMP="noslotcomp:off::Disable VJ TCP/IP compression slot ID compression:175"
MASK="mask:255.255.255.0::Set the subnet mask of the interface to netmask:176"
OLD="old:off::Remote side using ISC TCP Release 4:177"
CLOCAL="clocal:off::Do not wait for carrier when opening tty device:178"

##############################################################################
# Parameters Common among all dynamic link types
#
FILTER="filter:default::Filter specification to use for link:179"


##############################################################################
# Parameters Specific to Dedicated Links
#

STATICDEV="staticdev:no default::Name of tty device to use for PPP Link:180"
SPEED="speed:9600:$BAUDS:Baud rate of tty device used for PPP link:181"
DED_FILTER="filter:dedicated::Filter specification to use for link:182"

##############################################################################
# Parameters Specific to Dynamic Incoming Links
#
LOCAL="local:none::Local address for dynamic incoming link:183"
REMOTE="remote:none::Remote address for dynamic incoming link:184"


##############################################################################
# Parameters Specific to Dynamic Outgoing and Manual Bringup Links
#

UUCP="uucp:no default::Configured uucp system name for remote system:185"
ATTACH="attach:no default::Name used with pppattach command to bring up link:186"
RETRY="retry:2::Number of times to retry uucp connection establishment:187"

##############################################################################
#
# Remove temp files and exit with the status passed as argument
#
cleanup() {
	trap '' 1 2 3 15
	[ "$tmp" ] && rm -f $tmp*
	exit $1 
}

##############################################################################
#
# Restore original ppphosts file if interupted
#

restore_ppphosts() {
	trap '' CHLD
	cp -f  $PPPHOSTS.bak $PPPHOSTS
	echo "restore_ppphosts: restoring ppphosts ......"
	exit 0
}


##############################################################################
#
# Print an error message
# Usage: Error "message"
# Argument is a quoted error message
#
Error() {
	typeset num
	num=$1
	shift

	echo
	pfmt -s error -g "$CAT:$num" "$*"
	return 1
}

##############################################################################
#
# Do the "Press any key to continue" thing
#

press_any_key() {
	pfmt -s nostd -g "$CAT:90" "\nPress the 'return key' to continue ...."
	read
}       

##############################################################################
#
# Print a warning message
# Usage: warning "message"
# Argument is a quoted warning message
#
warning() {
	typeset num
	
	num=$1
	shift
	echo
	pfmt -s warn -g "$CAT:$num" "$*"
}



##############################################################################
#
# clear the screen if it is supported
#
clearscr() {
	# check if the terminal environment is set up
	[ "$TERM" ] && clear 2> /dev/null
}


##############################################################################
#
# Print a message
# Argument is a quoted message
#
message() {
	typeset num

	num=$1
	shift
	pfmt -s nostd  -g "$CAT:$num" "$*"
	echo
}


##############################################################################
#
# usage: safemv file1 file2 [ action ]
# move file1 to file2 without chance of interruption
#
safemv() {
        trap "" 1 2 3 15
        mv -f $1 $2
	if [[ $3 = "" ]]; then
	        trap "cleanup $FAIL" 1 2 3 15
	else
	        trap "$3" 1 2 3 15
	fi
}


#******************************************************
getyn() {
	typeset num
	typeset msg
	typeset y
	typeset n

	num=$1
	shift

	y=`gettxt $CAT:207 "y"`
	n=`gettxt $CAT:206 "n"`

	while true; do 
		msg=$(pfmt -s nostd -g "$CAT:$num" "$*" 2>&1)

		echo "$msg ($y/$n) \c"
		read yn rest 
	
		if [[ $yn = $y ]]; then
			return 0
		elif [[ $yn = $n ]]; then
			return 1
		else
			pfmt -g "$CAT:91" "Answer y or n" 
		fi
			
	done
}

#######################################################
# Get name of address pool to use
#

ask_pool () {
	POOL=
	typeset num
	typeset msg

	num=$1
	shift

	msg=$(pfmt -s nostd -g "$CAT:$num" "$1" 2>&1)
	POOL_PROMPT="\n$(pfmt -s nostd -g "$CAT:92" "Enter" 2>&1)"

	if [ "$2" ] ; then
		cur_pool_name=`echo $2 | sed "s/+//"`

		POOL_PROMPT="$POOL_PROMPT $msg [$cur_pool_name]: \c"
	else
		POOL_PROMPT="$POOL_PROMPT $msg: \c"
	fi

	while echo $POOL_PROMPT
	do 	read pool_name rest
		case "$pool_name" in
		"")    	if [ "$2" ] ; then
				POOL="+$cur_pool_name"
				break
			fi
			echo 
			continue;;
		 *) 	POOL=$pool_name
		       	grep "^$POOL:" $POOLFILE > /dev/null 2>&1
			if [ $? -eq 1 ] ; then
				pfmt -s nostd -g "$CAT:188" "\nNo %s entry in %s\n" $POOL $POOLFILE
				getyn 54 "\nIs this OK"
				case $? in 
				0) pfmt -s nostd -g "$CAT:189" "\nNOTE: Add %s entry to %s\n" \
					$POOL $POOLFILE
				   POOL="+$POOL"
				   return $OK
				   ;;
				1) continue;;
				esac
			fi
			POOL="+$POOL"
			return $OK			
			;;
		esac
	done
}
			

#******************************************************
ask_ip() {
	typeset num
	typeset msg
	IP_ADDR=

	num=$1
	shift

	msg=$(pfmt -s nostd -g "$CAT:$num" "$1" 2>&1)
	IP_PROMPT="\n$(pfmt -s nostd -g "$CAT:92" "Enter" 2>&1)"

	if [ "$2" ] ; then
		IP_PROMPT="$IP_PROMPT $msg [$2]: \c"
	else
		IP_PROMPT="$IP_PROMPT $msg: \c"
	fi
	while echo $IP_PROMPT
	do	read ip_addr rest
		case "$ip_addr" in
		[0-9]*.[0-9]*.[0-9]*.[0-9]*) IP_ADDR=$ip_addr
			break
			;;
		q)	return $FAIL ;;
		"")	if [ "$2" ] ; then
				IP_ADDR=$2
				break
			else
				message 5 "Address must have n.n.n.n form"
			fi
			;;	
		-)	IP_ADDR=
			break
			;;
		*)	# assume it is a host name
			IP_ADDR=$ip_addr
			break
			;;
		esac
	done

	return $OK
}
		
#******************************************************
ask_mask() {
	NET_MASK=
	typeset num
	typeset msg

	num=$1
	shift
	msg=$(pfmt -s nostd -g "$CAT:$num" "$1" 2>&1)
	MASK_PROMPT="\n$(pfmt -s nostd -g "$CAT:92" "Enter" 2>&1)"
	MASK_PROMPT="$MASK_PROMPT $(pfmt -s nostd -g "$CAT:93" "(\"-\" for default)" 2>&1)"
	
	if [ "$2" ] ; then
		MASK_PROMPT="$MASK_PROMPT $msg"
		MASK_PROMPT=" $MASK_PROMPT $(pfmt -s nostd -g "$CAT:93" "(\"-\" for default)" 2>&1)"
		MASK_PROMPT="$MASK_PROMPT [$2]: \c"
	else
		MASK_PROMPT="$MASK_PROMPT $msg: \c"
	fi
	while echo $MASK_PROMPT
	do	read net_mask rest
		case "$net_mask" in
		[0-9]*.[0-9]*.[0-9]*.[0-9]*) NET_MASK=$net_mask
			break
			;;
		"")	if [ "$2" ] ; then
				NET_MASK=$2
				break
			else
				NET_MASK=
				break
			fi
			;;	
		-)	NET_MASK=
			break
			;;	
		*)	message 5 "Address must have n.n.n.n form"
			;;
		esac
	done
}

#******************************************************
ask_uucpname() {
	typeset num
	typeset msg
	NAME=

	num=$1
	shift

	msg=$(pfmt -s nostd -g "$CAT:$num" "$1" 2>&1)
	NAME_PROMPT="\n$(pfmt -s nostd -g "$CAT:92" "Enter" 2>&1)"
	message 6 "\nThe UUCP name is used by PPP to dial into the remote system"
 
	if [ "$2" ] ; then
		NAME_PROMPT="$NAME_PROMPT $msg [$2]: \c"
	else
		NAME_PROMPT="$NAME_PROMPT $msg: \c"
	fi
	while echo $NAME_PROMPT
	do	read name rest
		case "$name" in
		[a-zA-Z]*) NAME=$name
			break
			;;
		"")	if [ "$2" ] ; then
				NAME=$2
				break
			else
				message 7 "Name must start with letter"
			fi
			;;
		*)	message 7 "Name must start with letter"
			;;
		esac
	done
}

#******************************************************
ask_name() {
	typeset num
	typeset msg
	NAME=

	num=$1
	shift

	msg=$(pfmt -s nostd -g "$CAT:$num" "$1" 2>&1)
	NAME_PROMPT="\n$(pfmt -s nostd -g "$CAT:92" "Enter" 2>&1)"
	

	if [ "$2" ] ; then
		NAME_PROMPT="$NAME_PROMPT $msg [$2]: \c"
	else
		NAME_PROMPT="$NAME_PROMPT $msg: \c"
	fi
	while echo $NAME_PROMPT
	do	read name rest
		case "$name" in
		q) return $FAIL;;
		[a-zA-Z]*) NAME=$name
			break
			;;
		"")	if [ "$2" ] ; then
				NAME="$2"
				break
			else
				#message 7 "Name must start with letter"
				return $FAIL
			fi
			;;
		*)	message 7 "Name must start with letter"
			;;
		esac
	done
}

#######################################################
add_user() {

	getyn 56 "\nDo you want to create PPP login account?"
	case $? in
	      0) useradd -d $PPPDIR -s $PPPSHELL $1
		 if [ "$?" != "0" ] ; then
			Error 8 "Failed to add PPP login account"
			press_any_key
		else
			message 9 "Enter a password for login account"
			echo "\"$1\""
			passwd $1
			if [ "$?" != "0" ] ; then
				Error 10 "Failed to change password"
				press_any_key
			fi
		fi
		;;
	     1) ;;
	esac 
}

#######################################################
delete_user() {

	message 147 "The following login account exists for this configuration\n"
	echo "\t$1\t"
	getyn 55 "Do you wish to delete this account"
	case $? in
	      0) userdel $1
		 if [ "$?" != "0" ] ; then
			Error 11 "Failed to Delete login account"
		 fi
		;;
	     1) ;;
	esac 
}
#######################################################
add_local() {
	
	# if there is no PPPAUTH file, creat it
	if [ ! -r "$PPPAUTH" ] ; then
		if [ -r /etc/inet/pppauth.samp ] ; then
			/usr/bin/cp /etc/inet/pppauth.samp $PPPAUTH
		else
			cat "#pppauth file" > $PPPAUTH
		fi
		/usr/bin/chown root $PPPAUTH
		/usr/bin/chgrp root $PPPAUTH
		/usr/bin/chmod 600 $PPPAUTH
	fi

	message 12 "\nLocal host uses local host ID/password to identify itself." 
	ask_name 82 "local host ID"
	LID="$NAME"
	ask_name 83 "local host password"
	LPWD="$NAME"

	grep '^*' $PPPAUTH > /dev/null 2>&1
	if [ $? -eq 0 ] ; then
		# Get rid of old entry
		sed "/^*/d" $PPPAUTH > /tmp/pppauth$$
		safemv /tmp/pppauth$$ $PPPAUTH	
	fi
	echo "*$LID $LPWD" >> $PPPAUTH
}

#######################################################
add_remote() {

	# if there is no PPPAUTH file, creat it
	if [ ! -r "$PPPAUTH" ] ; then
		if [ -r /etc/inet/pppauth.samp ] ; then
			/usr/bin/cp /etc/inet/pppauth.samp $PPPAUTH
		else
			cat "#pppauth file" > $PPPAUTH
		fi
		/usr/bin/chown root $PPPAUTH
		/usr/bin/chgrp root $PPPAUTH
		/usr/bin/chmod 600 $PPPAUTH
	fi

	message 13 "\nLocal host uses remote host's ID/password to authenticate remote host." 
	ask_name 84 "remote host ID"
	RID="$NAME"
	ask_name 85 "remote host password"
	RPWD="$NAME"

	entry=`grep "^\$RID[	 ]" $PPPAUTH`
	if [ -n "$entry" ] ; then
		# Get rid of old entry
		sed "/$RID/d" $PPPAUTH > /tmp/pppauth$$
		safemv /tmp/pppauth$$ $PPPAUTH	
	fi
	echo "$RID $RPWD" >> $PPPAUTH
}

#######################################################
rm_local() {

	if [ ! -r "$PPPAUTH" ] ; then
		pfmt -s nostd -g "$CAT:190" "\n%s file does not exist\n" $PPPAUTH
		return $OK
	fi

	entry=`grep "^\*" $PPPAUTH | sed 's/*//'` 
	if [ "$entry" ] ; then
		LID=`echo $entry | awk '{ print $1 }' `
		LPWD=`echo $entry | awk '{ print $2 }' `
		message 15 "\nLocal host ID/password are:"
		echo " $LID/$LPWD"
	else
		message 16 "\nLocal host has no ID/password"
		return $OK
	fi

	getyn 57 "Are you sure you want to remove local host ID/password?"	
	case $? in
		0) sed "/^\*$LID/d" $PPPAUTH > /tmp/pppauth$$
		   safemv /tmp/pppauth$$ $PPPAUTH
		    ;;
		1) ;;
	esac 
}

#######################################################
rm_remote() {

	if [ ! -r "$PPPAUTH" ] ; then
		pfmt -s nostd -g "$CAT:190" "\n%s file does not exist\n" $PPPAUTH
		return $OK
	fi

	entry=`grep -v "^#" $PPPAUTH | grep -v "^*"` 
	if [ "$entry" ] ; then
		message 17 "\nThe remote host IDs/passwords are:"
	else
		message 18 "\nThere are no ID/password for remote hosts"
		return $OK
	fi

	entry=`grep -v "^#" $PPPAUTH | grep -v "^*" | sed 's/	/:/'|sed 's/ /:/'`
	for i in $entry
	do
		RID=`echo $i | awk -F: '{ print $1 }'` 
		RPWD=`echo $i | awk -F: '{ print $2 }'` 
		echo "$RID/$RPWD"
	done

	ask_name 86 "host ID to be removed"
	RID="$NAME"

	entry=`grep "^$RID" $PPPAUTH ` 
	if [ "$entry" ] ; then
		getyn 58 "Are you sure you want to remove ID/password"
		case $? in
			0) sed "/^$RID/d" $PPPAUTH > /tmp/pppauth$$
			   safemv /tmp/pppauth$$ $PPPAUTH
			    ;;
			1) ;;
		esac 
	else
		message 19 "There is no remote host named"
		echo " $RID"
		return $OK
	fi

}

#######################################################
# if option $1 exists in option string $2, remove it.
rem_option() {
	REM_OPTIONS=`echo $2 | awk '{
			out_opts = ""
			split(opt, t, "=")
			opt_name = t[1]
			for (i=1; i<=NF; ++i) {
				split($i, t, "=")
				if (opt_name != t[1])
					out_opts = out_opts " " $i
			}
			if (length(out_opts) > 0)
				print substr(out_opts, 2)
		}' opt="$1"`
}

##############################################################################
# process ppphosts entry field which takes value
#   (variable FOREVER must be set up prior to calling)
#
getfield1 (){	
	FIELD=
	FIELD_DEFAULT=
	typeset num
	typeset msg

	fieldname=`echo $1 | awk -F: '{ print $1 }'`
	default=`echo $1 | awk -F: '{ print $2 }'`
	possible=`echo $1 | awk -F: '{ print $3 }'`
	explain=`echo $1 | awk -F: '{ print $4 }'`
	num=`echo $1 | awk -F: '{ print $5 }'`
	current=$default


	explain=$(pfmt -s nostd -g "$CAT:$num" "$explain" 2>&1)

	if [ "$2" ] ; then
		for i in $2 
		do
		name=`echo $i | sed 's/=[a-zA-Z0-9]*//'`
		if [ "$fieldname" = "$name" ] ; then
			current=`echo $i | sed 's/[a-zA-Z]*=//'`
		fi
		done
	fi

	if [ "$FOREVER" ] ; then
		prompt="\n$explain ("$FOREVER") [$current]: \c"
	else
		prompt="\n$explain [$current]: \c"
	fi
	while echo "$prompt"
	do	read field rest
		case $field in
		[//]*) FIELD="$fieldname=$field"
				break;;
		[a-zA-Z0-9]*) FIELD="$fieldname=$field"
				break;;
		"") if [ "$current" != "$default" ] ; then
			FIELD="$fieldname=$current"
		    fi
		    FIELD_DEFAULT=$default
		    break;;
		-) if [ "$FOREVER" ] ; then
			FIELD=
			break
		   else
			 message 20 "Enter a number"
		   fi
		   ;;
		*) message 20 "Enter a number"
		   ;;
		esac
	done
}
		
##############################################################################
# process ppphosts entry switch field
#
getfield2 (){	
	FIELD=
	typeset num
	typeset msg

	fieldname=`echo $1 | awk -F: '{ print $1 }'`
	default=`echo $1 | awk -F: '{ print $2 }'`
	possible=`echo $1 | awk -F: '{ print $3 }'`
	explain=`echo $1 | awk -F: '{ print $4 }'`
	num=`echo $1 | awk -F: '{ print $5 }'`
	current=$default

	explain=$(pfmt -s nostd -g "$CAT:$num" "$explain" 2>&1)

	if [ "$2" ] ; then
		for i in $2 
		do
		if [ "$fieldname" = "$i" ] ; then
			if [ "$default" = "on" ] ; then
				current="off"
			else
				current="on"
			fi
		fi
		done
	fi

	prompt="\n$explain [$current]: \c"
	while echo "$prompt"
	do	read field rest
		case $field in
		on)	field="on" 
			break;;
		off)	field="off"
			break;;
		"")	field="$current"
			break;;
		*) message 21 "Answer \"on\" or \"off\""
		   ;;
		esac
	done

	if [ "$field" = "on" ] ; then
		if [ "$default" = "off" ] ; then
			FIELD=$fieldname
		else
			FIELD=
		fi
	else
		if [ "$default" = "on" ] ; then
			FIELD=$fieldname
		else
			FIELD=
		fi
	fi
}

##############################################################################
# Get index based on attachname
#

get_index_attach() {
	INDEX=

	#
	# Its a dynamic outgoing link, and we need to 
	# find the entry based on attachname
	#
	# First get the index

	INDEX=$(cat $PPPHOSTS | awk ' BEGIN { idx = 0; } {
			if ((substr($0,length($0)) != "\\") && 
				(substr($0, 1, 1) != "#") && ($0 != ""))
					idx++;
				for (i =0; i<=NF; i++) {
					if ((split($i, j, "=") > 1) && (j[1] == "attach")) {
						if (j[2] == attach_name) {
							printf("%d", idx);
							exit(0);
						}
					}
				}
			}' attach_name=$1)
	
	if [[ $INDEX != "" ]]; then
		return $OK
	else
		return $FAIL
	fi
}

##############################################################################
#
# Get entry based on Index
#

get_entry_index () {
	ENTRY=	
	
	ENTRY=$(cat $PPPHOSTS | awk ' BEGIN { cur = 1; } {
			if (substr($0, 1, 1) != "#") {
		     		if ($0 == "")
					next;
				if (cur == ind)				
					print $0;

				if (substr($0,length($0)) != "\\") 
					cur++;
			}
	}' ind=$1 )

}
##############################################################################
# Print ppp host entry (used by delete_entry)
#

print_entry () {

	echo $1 | awk -F"|" '{ 
				printf("%-7s ", $1);
				if ($2 == "incoming")
					printf("%-37s ", substr($3,2));
				else {
					for (i=1; i<=length($3); i++) {
						if (substr($3, i,1) == ":")
							break;
					}
					str = sprintf("%s --> %s", 
							substr($3, i+1),
							substr($3,1,i-1));
					printf("%-37s ", str);
				}
				if ($2 == "manual")
					str = "Manual Outgoing";
				else if ($2 == "dynamic")
					str = "Dynamic Outgoing";
				else if ($2 == "incoming")
					str = "Dynamic Incoming";
				else if ($2 == "dedicated")
					str = "Dedicated";
				else 
					str = "Unknown Type";
				printf("%-17s ", str);
				printf("%-15s\n", $4);
			    }'

}

list_entries () {


   while true; do
	temp_ppphosts='/tmp/ppphosts_temp'
	temp_ppphosts2='/tmp/ppphosts_temp2'
	seach_name=
	listtype=
	newlist=

	# Get rid of line continuations to make things easier

	awk '{
        	if (substr($0,length($0)) == "\\") {
        	        lastln = lastln substr($0,1,length($0) - 1) " ";
        	}
        	else {
        	        printf("%s%s\n",lastln,$0);
        	        lastln = "";
        	}
	}' $PPPHOSTS > $temp_ppphosts

	# get rid of comments

	cat $temp_ppphosts | grep -v "^#" > $temp_ppphosts2
	mv $temp_ppphosts2 $temp_ppphosts

	list=`cat $temp_ppphosts | awk '{ print $1 }'`

	newlist=`cat $temp_ppphosts | awk ' BEGIN { z = 1; }
				{
				if ($0 == "")
					next;
				name = $1;
				attach = ""
				uucp = ""
				if (substr(name, 1,1) == "*") 
					ltype = "incoming";
				else
					ltype = "dynamic";
				for (i=2; i <= NF; i++) {
					if (split($i, j, "=") > 1) {
						if (j[1] == "attach") {
							attach = j[2];
							ltype = "manual";
						}
						if (j[1] == "staticdev")
							ltype = "dedicated"
						if (j[1] == "uucp")
							uucp = j[2];
					}
				}
				printf("%d|%s|%s|%s|%s ", z++, ltype, name, attach, uucp);
				}'`

	ALL=$(pfmt -s nostd -g "$CAT:199" "List all entries" 2>&1)
	SRCDST=$(pfmt -s nostd -g "$CAT:95" "List entries based on "local:remote" address" 2>&1)
	LNAME=$(pfmt -s nostd -g "$CAT:96" "List entries based on PPP login name" 2>&1)
	LTYPE=$(pfmt -s nostd -g "$CAT:97" "List entries based on link type" 2>&1)
	RTN=$(pfmt -s nostd -g "$CAT:98" "Return to main menu" 2>&1)
	POSS="$ALL $SPEC $LTYPE"

	while true; do
		clearscr
		echo "1) $ALL"
		echo "2) $SRCDST"
		echo "3) $LNAME"
		echo "4) $LTYPE"
		echo "5) $RTN"
		echo "----------------------------------------------------"
		pfmt -s nostd -g "$CAT:99" "Enter Number [5]:"
		echo " \c"
		read number rest
		case $number in
			1) LIST="ALL" ;;
			2) LIST="SRCDST" ;;
			3) LIST="LNAME" ;;
			4) LIST="LTYPE" ;;
			5) return $OK ;;
			"") return $OK ;;
			*) Error 71 "Invalid number" 
			   press_any_key
			   continue ;;
		esac
		break
	done

	if [ $LIST = "SRCDST" ] ; then
		pfmt -s nostd -g "$CAT:100"  "Enter source address:"
		echo " \c"
		read src rest
		pfmt -s nostd -g "$CAT:101"  "Enter destination address:"
		echo " \c"
		read dst rest
		search_name="$dst:$src"
	elif [ $LIST = "LNAME" ] ; then
		pfmt -s nostd -g "$CAT:102"  "Enter PPP login name:"
		echo " \c"
		read search_name rest
		search_name="*$search_name"

	elif [ $LIST = "LTYPE" ] ; then
		cont=0
		while true; do
			clearscr
			pfmt -s nostd -g "$CAT:200" "1) Dedicated Link\n"
			pfmt -s nostd -g "$CAT:201" "2) Dynamic Outgoing\n"
			pfmt -s nostd -g "$CAT:202" "3) Manual Bringup\n"
			pfmt -s nostd -g "$CAT:203" "4) Dynamic Incoming\n"
			pfmt -s nostd -g "$CAT:103" "5) Return to last menu"
			echo "\n----------------------------------------------------"
			pfmt -s nostd -g "$CAT:104" "Enter link type to list [5]:"
			echo " \c"
			read number rest
			case $number in
				1) listtype="dedicated" ;;
				2) listtype="dynamic" ;;
				3) listtype="manual" ;;
				4) listtype="incoming" ;;
				5) cont=1
				   break ;;
				"") cont=1
				    break ;;
				*) Error 71 "Invalid number"
				   sleep 2 
				   continue ;;
			esac
			break
		done

		if [ $cont = "1" ] ; then
			continue
		fi
	fi

	for i in $newlist; do
		if [[ $search_name  != "" ]]; then
			name=`echo $i | awk -F"|" '{ print $3 }'`
			if [[ $name = $search_name ]]; then
				print_entry $i >> /tmp/ppplist
			fi
		elif [[ $listtype != "" ]]; then
			type=`echo $i | awk -F"|" '{ print $2 }'`
			if [[ $listtype = $type ]]; then
				print_entry $i >> /tmp/ppplist
			fi
		else
			print_entry $i >> /tmp/ppplist
		fi
			
	done

	while true; do
		clearscr
		if [[ -f /tmp/ppplist ]]; then
			pfmt -s nostd -g "$CAT:105" "Index"
			echo "   \c"
			pfmt -s nostd -g "$CAT:106" "Address/Login"
			echo "                         \c"
			pfmt -s nostd -g "$CAT:107" "Link Type"
			echo "       \c"
			pfmt -s nostd -g "$CAT:108" "Attach Name"
			echo "\n----------------------------------------------------------------------------"
			more /tmp/ppplist
			echo "----------------------------------------------------------------------------"
	
		else
			pfmt -s nostd -g "$CAT:115" "\nNothing Found ...\n\n"
		fi

		pfmt -s nostd -g "$CAT:110" "Enter index to see entry or return to continue [return]:"
		echo " \c"
		read number rest
	
		if [[ $number = "" ]]; then
			rm -f /tmp/ppplist
			break
		fi

		clearscr
		echo

		cat $PPPHOSTS | awk ' BEGIN { cur = 1; } {
				if ($0 == "")
					next;
				if (substr($0, 1, 1) != "#") {
					if (cur == ind)				
						print $0;
	
					if (substr($0,length($0)) != "\\") 
						cur++;
					if (cur > ind)
						exit(0);
				}
		}' ind=$number 
	
		press_any_key
	done
	rm -f /tmp/ppplist
    done
	

}

##############################################################################
# Remove an entry from the hosts file
#

remove_entry () {
	index=$1
	type=$2
	name=$3
	attach=$4
	uucp=$5

	tmp_ppphosts="/tmp/ppp$0"

	/bin/rm -f $tmp_ppphosts
	clearscr
	pfmt -s nostd -g "$CAT:191" "NOTE: Backing up %s in %s\n" $PPPHOSTS $PPPHOSTS.bak

	sleep 2

	cp $PPPHOSTS $PPPHOSTS.bak

	cat $PPPHOSTS | awk ' BEGIN { cur = 1; } {
			if (substr($0, 1, 1) != "#") {
				if ($0 == "")
					next;
				if (cur != ind)				
					print $0;

				if (substr($0,length($0)) != "\\") 
					cur++;
			}
			else
				print $0

	}' ind=$index > $tmp_ppphosts

	mv $tmp_ppphosts  $PPPHOSTS

	if [ $type = "incoming" ] ; then
		delete_user $(echo $name | sed "s/\*//")
	fi

	press_any_key
}

				
						
	
##############################################################################
# Delete an entry
#

delete_entry () {

   while true; do 
	temp_ppphosts='/tmp/ppphosts_temp'
	temp_ppphosts2='/tmp/ppphosts_temp2'

	# Get rid of line continuations to make things easier

	awk '{
        	if (substr($0,length($0)) == "\\") {
        	        lastln = lastln substr($0,1,length($0) - 1) " ";
        	}
        	else {
        	        printf("%s%s\n",lastln,$0);
        	        lastln = "";
        	}
	}' $PPPHOSTS > $temp_ppphosts

	cat $temp_ppphosts | grep -v "^#" > $temp_ppphosts2
	mv $temp_ppphosts2 $temp_ppphosts

	list=`cat $temp_ppphosts | awk '{ print $1 }'`

	newlist=`cat $temp_ppphosts | awk ' BEGIN { z = 1; }
				{
				if ($0 == "")
					next;
				name = $1;
				attach = ""
				uucp = ""
				if (substr(name, 1,1) == "*")
					ltype = "incoming";
				else
					ltype = "dynamic";
				for (i=2; i <= NF; i++) {
					if (split($i, j, "=") > 1) {
						if (j[1] == "attach") {
							attach = j[2];
							ltype = "manual";
						}
						if (j[1] == "staticdev")
							ltype = "dedicated"
						if (j[1] == "uucp")
							uucp = j[2];
					}
				}
				printf("%d|%s|%s|%s|%s ", z++, ltype, name, attach, uucp);
				}'`



	ALL=$(pfmt -s nostd -g "$CAT:204" "List all possible entries to delete" 2>&1)
	SRCDST=$(pfmt -s nostd -g "$CAT:95" "List entries based on "local:remote" address" 2>&1)
	LNAME=$(pfmt -s nostd -g "$CAT:96" "List entries based on PPP login name" 2>&1)
	LTYPE=$(pfmt -s nostd -g "$CAT:97" "List entries based on link type" 2>&1)
	UNDO=$(pfmt -s nostd -g "$CAT:112" "Undo last remove" 2>&1)
	POSS="$ALL $SPEC $LTYPE"

	while true; do
		clearscr
		echo "1) $ALL"
		echo "2) $SRCDST"
		echo "3) $LNAME"
		echo "4) $LTYPE"
		echo "5) $UNDO"
		pfmt -s nostd -g "$CAT:113" "6) Return to main menu"
		echo "\n----------------------------------------------------"
		pfmt -s nostd -g "$CAT:114" "Enter Number [6]:"
		echo "\c"
		read number rest
		case $number in
			1) LIST="ALL" ;;
			2) LIST="SRCDST" ;;
			3) LIST="LNAME" ;;
			4) LIST="LTYPE" ;;
			5) undo_last $PPPHOSTS
			   LIST="UNDO"
			   ;;		
			6) return $OK ;;
			"") return $OK ;;
			*) Error 71 "Invalid number"
			   sleep 2 
			   continue ;;
		esac
		break
	done

	

	if [ $LIST = "SRCDST" ] ; then
		pfmt -s nostd -g "$CAT:100" "Enter source address:"
		echo " \c"
		read src rest
		pfmt -s nostd -g "$CAT:101" "Enter destination address:"
		echo " \c"
		read dst rest
		search_name="$dst:$src"
	elif [ $LIST = "LNAME" ] ; then
		pfmt -s nostd -g "$CAT:102" "Enter PPP login name:"
		echo " \c"
		read search_name rest
		search_name="*$search_name"

	elif [ $LIST = "LTYPE" ] ; then
		cont=0
		while true; do
			clearscr
                        pfmt -s nostd -g "$CAT:200" "1) Dedicated Link\n"
                        pfmt -s nostd -g "$CAT:201" "2) Dynamic Outgoing\n"
                        pfmt -s nostd -g "$CAT:202" "3) Manual Bringup\n"
                        pfmt -s nostd -g "$CAT:203" "4) Dynamic Incoming\n"
			pfmt -s nostd -g "$CAT:103" "5) Return to last menu"
			echo "\n----------------------------------------------------"
			pfmt -s nostd -g "$CAT:104" "Enter link type to list [5]:"
			echo " \c"
			read number rest
			case $number in
				1) ltype="dedicated" ;;
				2) ltype="dynamic" ;;
				3) ltype="manual" ;;
				4) ltype="incoming" ;;
				5) cont=1
				   break ;;
				"") cont=1
				    break ;;
				*) Error 71 "Invalid number"
				   sleep 2 
				   continue ;;
			esac
			break
		done

		if [ $cont = "1" ] ; then
			continue
		fi
	elif [ $LIST = "UNDO" ] ; then
		continue
	fi
	
	
	clearscr
	pfmt -s nostd -g "$CAT:105" "Index"
	echo "   \c"		
	pfmt -s nostd -g "$CAT:106" "Address/Login"
	echo "                         \c"
	pfmt -s nostd -g "$CAT:107" "Link Type"
	echo "       \c"
	pfmt -s nostd -g "$CAT:108" "Attach Name"
	echo "\n----------------------------------------------------------------------------"
	found=0
	foundit=0
	for i in $newlist ; do
	
		if [ $LIST = "LTYPE" ] ; then
			type=`echo $i | awk -F"|" '{ print $2 }'`
			if [ $type = $ltype ]; then
				foundit=1
				found=1
			fi
		elif [ $LIST = "ALL" ] ; then
			foundit=1
			found=1
		else
			name=`echo $i | awk -F"|" '{ print $3 }'`
			if [ $name = $search_name ] ; then
				foundit=1
				found=1
			fi
		fi
	
		if [ $foundit = "1" ] ; then
			index=`echo $i | awk -F"|" '{ print $1 }'`
			print_entry $i
			foundit=0
		fi
	done



	if [ $found = "0" ] ; then
			pfmt -s nostd -g "$CAT:115" "\nNothing Found ...\n\n"
			sleep 2
			continue;
	else
			let exit_index=index+1	
	fi

	echo "----------------------------------------------------------------------------"
	echo "$exit_index\t\c"
	pfmt -s nostd -g "$CAT:116" "Return to last menu"
	echo "\n----------------------------------------------------------------------------"


	pfmt -s nostd -g "$CAT:117" "Enter index corresponding to link to delete"
	echo "[$exit_index]: \c"
	read number rest


	if [[ $number = "" ]] || [ $number = $exit_index ] ; then
		continue;
	fi

	found="0"
	for i in $newlist ; do
		
		index=`echo $i | awk -F"|" '{ print $1 }'`

		if [ $number = $index ] ; then		
			type=`echo $i | awk -F"|" '{ print $2 }'`
			name=`echo $i | awk -F"|" '{ print $3 }'`
			attach=`echo $i | awk -F"|" '{ print $4 }'`
			uucp=`echo $i | awk -F"|" '{ print $5 }'`

			pfmt -s nostd -g "$CAT:149" "\n\nDeleting Entry:"
			echo

			print_entry $i
			echo	
			getyn 59 "Is this OK"
		
			case $? in
				0) 	remove_entry $index $type $name $attach $uucp
					;;
				1) 	break;
					;;
			esac
		fi
	done
   done
	
}

##############################################################################
# Keep advanced options for a PPP link
#
keep_adv_options () {

	OPTIONS=$1

	for i in "$STATICDEV" "$SPEED" "$LOCAL" "$REMOTE" "$UUCP" "$ATTACH"
	do
		fieldname=`echo $i | awk -F: '{ print $1 }'`
		OPTIONS=`echo $OPTIONS | sed "s/$fieldname=[/.+a-zA-Z0-9]*//g"`
	done
	
	return $OK
}

##############################################################################
# Get advanced options for a PPP link
#

get_adv_options (){	
	typeset num
	OPTIONS="$1"


	existing_opts="$1"

	while true; do
		clearscr
		let index=1
		for i in "$IDLE" "$FLOW" "$DEBUG" "$REQTMOUT" "$CONF" "$TEM" "$NAK" "$MRU" "$ACCM" "$AUTH" "$AUTHTMOUT" "$AUTHNAME" "$MAXSLOT" "$MASK" "$RETRY"
		do
			explain=`echo $i | awk -F: '{ print $4 }'`
			num=`echo $i | awk -F: '{ print $5 }'`
			explain=$(pfmt -s nostd -g "$CAT:$num" "$explain" 2>&1)
			aparam=`echo $i | awk -F: '{ print $1 }'`
			echo "$index)\t$explain <$aparam>"
			let index=index+1
		done

		echo "$index)\t\c"
		pfmt -s nostd -g "$CAT:118" "Continue"
		echo

		pfmt -s nostd -g "$CAT:119" "Enter number of parameter to change:"
		echo " \c"
		read num rest
		if [[ $num = "" ]]; then
			continue
		fi

		if [[ $num = $index ]]; then
			break;
		fi

		let index=1			
		for i in "$IDLE" "$FLOW" "$DEBUG" "$REQTMOUT" "$CONF" "$TEM" "$NAK" "$MRU" "$ACCM" "$AUTH" "$AUTHTMOUT" "$AUTHNAME" "$MAXSLOT" "$MASK" "$RETRY"
		do
			if [[ $index = $num ]]; then
				break
			fi
			let index=index+1
		done

		if [ "$i" = "$IDLE" ] ; then
			FOREVER=$(pfmt -s nostd -g "$CAT:120" "\"-\" for forever" 2>&1)
		else
			FOREVER=
		fi
		param=$i

		getfield1 "$i" "$existing_opts"

		fieldname=`echo $param | awk -F: '{ print $1 }'`
		rem_option "$fieldname" "$existing_opts"
		existing_opts="$REM_OPTIONS"
		rem_option "$fieldname" "$OPTIONS"
		OPTIONS="$REM_OPTIONS"

		if [ "$FIELD" ] ; then
			OPTIONS="$OPTIONS $FIELD"
			existing_opts="$existing_opts $FIELD"

			if [[ "$param" = "$AUTH" ]]; then
				getyn 60 "\nEdit PPP authentication parameters"
				case $? in
					0) edit_auth ;;
					1) ;;
				esac
			fi
		fi
	done

	while true; do 
		clearscr
		let index=1
		
		for i in "$PROXY" "$MGC" "$PROTCOMP" "$ACCOMP" "$IPADDR" "$RFC1172" "$NOVJ" "$NOSLOTCOMP" "$OLD" "$CLOCAL"
		do			
			explain=`echo $i | awk -F: '{ print $4 }'`
			num=`echo $i | awk -F: '{ print $5 }'`
			explain=$(pfmt -s nostd -g "$CAT:$num" "$explain" 2>&1)
			aparam=`echo $i | awk -F: '{ print $1 }'`
			echo "$index)\t$explain <$aparam>"
			let index=index+1
		done

		echo "$index)\t\c"
		pfmt -s nostd -g "$CAT:118" "Continue"
		echo

		pfmt -s nostd -g "$CAT:119" "Enter number of parameter to change:"
		echo " \c"
		read num rest
		if [[ $num = "" ]]; then
			continue
		fi

		if [[ $num = $index ]]; then
			break;
		fi

		let index=1	

		for i in "$PROXY" "$MGC" "$PROTCOMP" "$ACCOMP" "$IPADDR" "$RFC1172" "$NOVJ" "$NOSLOTCOMP" "$OLD" "$CLOCAL"
		do	
			if [[ $index = $num ]]; then
				break
			fi
			let index=index+1
		done
		param=$i

		getfield2 "$i" "$existing_opts"

		fieldname=`echo $param | awk -F: '{ print $1 }'`
		rem_option "$fieldname" "$existing_opts"
		existing_opts="$REM_OPTIONS"
		rem_option "$fieldname" "$OPTIONS"
		OPTIONS="$REM_OPTIONS"

		if [ "$FIELD" ] ; then
			if [ "$FIELD" = "pap" ] ; then
				warning 52 "Add remote hosts' ID/password to local host's authentication file"
				warning 53 "Add remote hosts' ID/password to remote hosts' authentication file"
			fi
			OPTIONS="$OPTIONS $FIELD"
			existing_opts="$existing_opts $FIELD"
		fi
	done

	clearscr

}

######################################################################
# Promp user for tty device. Check for existance of character 
# special file. If it does not exist, ask them if it is ok
#	

ask_staticdev() {
	DEVICE=

	while true ; do

		FOREVER=
		getfield1 "$1"

		if [[ $FIELD = "" ]]; then
			message 22 "\nYou must enter device name for static link"
			continue
		fi

		device=`echo $FIELD | sed 's/[a-zA-Z0-9]*=//'`

		if [[ ! -c $device ]] ; then
			pfmt -s nostd -g "$CAT:192" "%s does not exist\n" $device
			getyn 54 "\nIs this OK"
			case $? in
				0) pfmt -s nostd -g "$CAT:193" "\nNOTE: %s must exist to use link" \
						$device
				   return $OK 
				   ;;
				1) ;;
			esac
		else
			break
		fi
	done
}

##############################################################################
# Get filter specification for link
#
# $2 == ppphosts entry up to this point
#
ask_filter() {
	NEWENTRY=
	current_filter=


	if [[ $2 != "" ]]; then
		get_current "$2" "filter"
		current_filter=$CURRENT
		#
		# delete the current filter spec
		#
		for i in $2; do 
			if [[ $i != "filter=$current_filter" ]]; then
				NEWENTRY="$NEWENTRY $i"
			fi
		done
	fi

	getyn 62 "\nDo you wish to enable packet filtering on this link"
	case $? in 
		0) ;;
		1) return $FAIL
	esac

	while true ; do
		FOREVER=
		if [[ $current_filter != "" ]]; then
	       		getfield1 "$1" "filter=$current_filter"
		else
	       		getfield1 "$1" 
		fi


		if [[ $FIELD = "" ]]; then
			N_FILTER=$FIELD_DEFAULT
		else
			N_FILTER=`echo $FIELD | sed 's/filter=//'`
		fi		

		grep "^\$N_FILTER[       ]" $PPPFILTER > /dev/null 2>&1
		
		if [ $? -eq 1 ] ; then
			pfmt -s nostd -g "$CAT:194" \
				"\nThe filter specification %s does not exist in %s\n" \
				$N_FILTER $PPPFILTER
			getyn 54 "\nIs this OK"
			case $? in
				0) pfmt -s nostd -g "$CAT:195" \
					"\nMake sure you place a %s filter specification in %s\n" \
					$N_FILTER $PPPFILTER
				   press_any_key

				   NEWENTRY="$NEWENTRY $FIELD"

				   return $OK
			  	;;
				1) continue ;;
			esac
		fi

		break
	done

}

##############################################################################
# Get an entry based on PPP login name / PPP source/destination address
#

get_entry () {
	
	ENTRY=

	ENTRY=$(cat $PPPHOSTS | awk '{ 
				if (($1 == name) || cont) { 

					if (substr($0,length($0)) == "\\") {
						printf("%s ", substr($0,1,length($0) - 1));
						cont++;
					}
					else {
						print $0;
						cont = 0;
					}
				}
			}' name=$1)
	
}

##############################################################################
# Get params common to all outgoing links (including dedicated, even though
# it could be also classified as an incoming link)
# 

get_out_params() {
	NEWENTRY=
	options=

	# Get the IP addresses for the remote and local host
	ask_ip 76 "IP address or node name for local side of PPP link ('q' to quit)"
	if [ $? -eq $FAIL ] ; then
		return $FAIL
	fi
	PPP_LOCAL_ADDR="$IP_ADDR"
	ask_ip 77 "IP address or node name for remote side of PPP link ('q' to quit)"
	if [ $? -eq $FAIL ] ; then
		return $FAIL
	fi
	PPP_REMOTE_ADDR="$IP_ADDR"

	NEWENTRY="$PPP_REMOTE_ADDR:$PPP_LOCAL_ADDR"

	# See if an entry already exists

	oldentry=
	if [[ $1 = "MANUAL" ]]; then
		get_index_attach "$2"

		if [[ $INDEX != "" ]]; then
			attach_index=$INDEX
			get_entry_index $INDEX
			oldentry="$ENTRY"
		fi
	else
		get_entry $NEWENTRY
		oldentry="$ENTRY"
	fi

	if [ -n "$oldentry" ] ; then
		message 29 "\nThe following entry already exists"
		echo "$oldentry"
		getyn 63 "\nModify this entry ?"
		case $? in 
			1) return $FAIL ;;
			0) ;;
		esac


		cp -f  $PPPHOSTS $PPPHOSTS.bak
		trap restore_ppphosts HUP INT QUIT CHLD

		# Get rid of old entry
		if [[ $1 = "MANUAL" ]]; then
			remove_entry $attach_index "manual" "0.0.0.0:0.0.0.0" "$2" ""
		else
			grep "[^\$NEWENTRY[ 	]" $PPPHOSTS|grep [\\]$ >/dev/null 2>&1
			if [ $? -eq 0 ] ; then
				sed "/^$NEWENTRY/,/[^\\]$/d" $PPPHOSTS > /tmp/ppphosts$$
			else
				sed "/^$NEWENTRY/d" $PPPHOSTS > /tmp/ppphosts$$
			fi
			safemv /tmp/ppphosts$$ $PPPHOSTS restore_ppphosts
		fi

		# generate new entry
		options=`echo $oldentry | awk '{ for (i = 2 ; i <= NF; i++) printf "%s ", $i }'`
		keep_adv_options "$options"
		options=$OPTIONS

		getyn 64 "\nDo you want to specify PPP advanced options?"
	
		case $? in
			0) get_adv_options "$options"
			   NEWENTRY="$NEWENTRY $OPTIONS"
			   ;;				
			1) NEWENTRY="$NEWENTRY $options"
			   ;;
		esac
	else

		cp -f  $PPPHOSTS $PPPHOSTS.bak
		trap restore_ppphosts HUP INT QUIT CHLD

		# Create new entry

		getyn 64 "\nDo you want to specify PPP advanced options?"
	
		case $? in
			0) get_adv_options 
			   NEWENTRY="$NEWENTRY $OPTIONS"
			   ;;
			1) ;;
		esac
	fi


}
##############################################################################
# Configure Dedicated Link
#

conf_dedicated() {

	# get outgoing paramters

	get_out_params

	if [ $? -eq $FAIL ] ; then
		return $OK
	fi

	newentry=$NEWENTRY

	# Get the tty device to use for link
	ask_staticdev "$STATICDEV"
	fieldname=`echo $STATICDEV | awk -F: '{ print $1 }'`
	rem_option "$fieldname" "$newentry"
	newentry="$REM_OPTIONS $FIELD"

	FOREVER=
	getfield1 "$SPEED"
	fieldname=`echo $SPEED | awk -F: '{ print $1 }'`
	rem_option "$fieldname" "$newentry"
	newentry="$REM_OPTIONS $FIELD"

	ask_filter "$DED_FILTER" "$newentry"
	newentry=$NEWENTRY

	echo $newentry >> $PPPHOSTS

	trap - HUP INT QUIT CHLD
}


##############################################################################
# Configure a dynamic outgoing ppp interface
#

conf_dyn_out() {

	# get outgoing paramters

	get_out_params	

	if [ $? -eq $FAIL ] ; then
		return $OK
	fi

	newentry=$NEWENTRY

	ask_uucpname 80 "Enter uucp name"
        rem_option "uucp" "$newentry"
	newentry="$REM_OPTIONS uucp=$NAME"

	ask_filter "$FILTER" "$newentry"
	newentry=$NEWENTRY

	echo $newentry >> $PPPHOSTS
	trap - HUP INT QUIT CHLD
}

##############################################################################
# Configure a manual outgoing ppp interface
#

conf_man_out () {

	# Ask the user if this is what they really want

	pfmt -s nostd -g "$CAT:30" "\nA manual bringup PPP link should only be used when one\nor both of the IP address for an outgoing link will be assigned\nby the remote host. If this is not the case, then a\nDynamic Outgoing PPP Link should be configured.\n"
	
	getyn 65 "Do you want to continue"
	case $? in
		0) ;;
		1) return $OK ;;
	esac

	pfmt -s nostd -g "$CAT:34" \
	  "\nThe attachname will be used with the pppattach command to bring up this link.\n"

	ask_name 87 "attachname"
	attach_name=$NAME

	# get outgoing paramters

	get_out_params MANUAL $attach_name

	if [ $? -eq $FAIL ] ; then
		return $OK
	fi

	rem_option "attach" "$NEWENTRY"
	NEWENTRY="$REM_OPTIONS"
	newentry="$NEWENTRY attach=$NAME"

	ask_uucpname 81 "uucp name"
        rem_option "uucp" "$newentry"
	newentry="$REM_OPTIONS"
	newentry="$newentry uucp=$NAME"

	ask_filter "$DED_FILTER" "$newentry"
	newentry=$NEWENTRY

	clearscr

	pfmt -s nostd -g "$CAT:196" \
		"\nTo bring up this link, enter 'pppattach %s' from the command line." \
		$attach_name
	message 38 "\nSee the pppattach(1M) man page for more information"

	press_any_key

	echo $newentry >> $PPPHOSTS
	trap - HUP INT QUIT CHLD
}


##############################################################################
# Get the current value of a parameter
# 

get_current () {
	CURRENT=

	for i in $1
		do
		name=`echo $i | sed 's/=[+a-zA-Z0-9]*//'`
		if [[ "$2" = "$name" ]] ; then
			CURRENT=`echo $i | sed 's/[a-zA-Z]*=//'`
		fi
	done

	return $OK
}
##############################################################################
# Configure a dynamic incoming ppp interface
#

conf_dyn_in () {
	options=

	# Get the login name

	ask_name 88 "PPP login name ('q' to quit)"
	
	if [ $? = $FAIL ] ; then
		return $FAIL
	fi

	newentry="*$NAME"

	# Check for an entry that already exists

	get_entry $newentry
	oldentry="$ENTRY"

	if [ -n "$oldentry" ] ; then
		message 29 "\nThe following entry already exists"
		echo "$oldentry"
		getyn 63 "\nModify this entry ?"
		case $? in 
			1) return $FAIL ;;
			0) ;;
		esac

		cp -f  $PPPHOSTS $PPPHOSTS.bak
		trap restore_ppphosts HUP INT QUIT CHLD

		# Get rid of old entry
		grep "[^\$newentry[ 	]" $PPPHOSTS|grep [\\]$ >/dev/null 2>&1
		if [ $? -eq 0 ] ; then
			sed "/^$newentry/,/[^\\]$/d" $PPPHOSTS > /tmp/ppphosts$$
		else
			sed "/^$newentry/d" $PPPHOSTS > /tmp/ppphosts$$
		fi
		safemv /tmp/ppphosts$$ $PPPHOSTS restore_ppphosts

		# generate new entry
		options=`echo $oldentry | awk '{ for (i = 2 ; i <= NF; i++) printf "%s ", $i }'`

		get_current "$options" "remote"		
		cur_remote="$CURRENT"

		get_current "$options" "local"		
		cur_local="$CURRENT"

		keep_adv_options "$options"
		options=$OPTIONS
	fi

	getyn 66 "\nAre you going to use address pooling for the local address"
	if [ $? -eq 0 ] ; then
		ask_pool 74 "name of address pool for local address" "$cur_local"
		newentry="$newentry local=$POOL"
	else
		ask_ip 78 "IP address or node name for local side of PPP link (- for none)" "$cur_local"
		if [ "$IP_ADDR" ] ; then
			newentry="$newentry local=$IP_ADDR"
		fi
	fi

	getyn 67 "\nAre you going to use address pooling for the remote address"
	if [ $? -eq 0 ] ; then
		ask_pool 75 "name of address pool for remote address" "$cur_remote"
		newentry="$newentry remote=$POOL"
	else
		ask_ip 79 "IP address or node name for remote side of PPP link (- for none)" "$cur_remote"
		if [ "$IP_ADDR" ] ; then
			newentry="$newentry remote=$IP_ADDR"
		fi
        fi


	getyn 64 "\nDo you want to specify PPP advanced options?"
	
	case $? in
		0) get_adv_options "$options"
		   newentry="$newentry $OPTIONS"
		   ;;				
		1) if [ "$options" ] ; then
			keep_adv_options "$options"
			newentry="$newentry $OPTIONS"
		   fi
		   ;;
	esac

	#
	# only save a backup if there is no old entry.  If there
	# was an old entry then we have alreay saved a backup earlier
	#

	if [ ! -n "$oldentry" ] ; then
		cp -f  $PPPHOSTS $PPPHOSTS.bak
		trap restore_ppphosts HUP INT QUIT CHLD
	fi

	ask_filter "$DED_FILTER" "$newentry"
	newentry=$NEWENTRY

	echo $newentry >> $PPPHOSTS

	add_user $NAME	

	trap - HUP INT QUIT CHLD

	return $OK
}

##############################################################################
# Create needed PPP files
#

create_files () {
	if [[ ! -f $PPPHOSTS ]]; then
	        # see if there is a sample
		if [[ -f $SAMP_DIR/ppphosts.samp ]] ; then
			cp -f $SAMP_DIR/ppphosts.samp $PPPHOSTS
		else
			touch $PPPHOSTS
		fi
	fi

	if [[ ! -f $PPPFILTER ]]; then
	        # see if there is a sample
		if [[ -f $SAMP_DIR/pppfilter.samp ]] ; then
			cp -f $SAMP_DIR/pppfilter.samp $PPPFILTER
		else
			touch $PPPFILTER
		fi
	fi

	if [[ ! -f $POOLFILE ]]; then
	        # see if there is a sample
		if [[ -f /etc/addrpool.samp ]] ; then
			cp -f /etc/addrpool.samp $POOLFILE
		else
			touch $POOLFILE
		fi
	fi

	if [[ ! -f $PPPAUTH ]]; then
	        # see if there is a sample
		if [[ -f $SAMP_DIR/pppauth.samp ]] ; then
			cp -f $SAMP_DIR/pppauth.samp $PPPAUTH
		else
			touch $PPPAUTH
		fi
	fi
}

##############################################################################
# Check for the existance of PPP releated commands & see of the PPP daemon 
# is running.  If it is not, warn the user.	
#

check_daemons_cmds () {

	for i in $PPPSHELL $PPPDAEMON $PPPSTAT $PPPATTACH; do
		if [[ ! -f $i ]]; then
			pfmt -s nostd -g "$CAT:197" \
				"Missing the %s command" $i
			message 39 "Make sure the PPP is installed"				
			getyn 68 "Continue anyway"
			case $? in 
				0) ;;
				1) exit 1 ;;
			esac
		fi		
	done
}

#######################################################
add_auth() {
	typeset id_msg

	id_msg=$(pfmt -s nostd -g "$CAT:205" "Enter ID" 2>&1)

	if [ $1 ]; then
		echo "$id_msg [$1]: \c"
	else
		echo "$id_msg: \c"
	fi
	read id rest

	if [[ $id = "" && $1 != "" ]]; then
		id=$1
	fi

	stty -echo
	pfmt -s nostd -g "$CAT:121" "\nNOTE: Password will not be echoed"
	echo
	while true; do
		pfmt -s nostd -g "$CAT:122" "Enter password:"
		echo " \c"
		read passwd rest
		pfmt -s nostd -g "$CAT:123" "\nRe-enter password:"
		echo " \c"
		read

		if [[ $REPLY = $passwd ]]; then
			stty echo
			break;
		else
			Error 73 "Did not re-enter password correctly"
			echo
		fi
	done

	echo "$id\t$passwd" >> $PPPAUTH

	pfmt -s nostd -g "$CAT:150" "\nEntry Added ...."

	press_any_key		

}

#######################################################
# Delete entry in pppauth file 
#
# Place deleted entry in AUTH_REMOVED
#

delete_auth_ent () {
	AUTH_REMOVED=

	cp $PPPAUTH $PPPAUTH.bak

	cat $PPPAUTH.bak | awk ' BEGIN { ind = 1; } {
				if ((substr($0, 1, 1) != "#") && ($0 != "")) {
					if (ind != rem_index)
						print $0;
					ind++; 
				}
				else
					print $0;
			     }' rem_index=$1 > $PPPAUTH

	AUTH_REMOVED=$(diff $PPPAUTH $PPPAUTH.bak | awk '{ printf("%s %s", $2, $3) }')
}

#######################################################
# Modify/Remove entry in pppauth file
# 

mod_rem_auth () {
	typeset -i max_auth_index=0

	if [[ $1 = "cleanup" ]]; then
		rm -f $TMP_AUTH_LIST
		exit 0
	fi

	TMP_AUTH_LIST=/tmp/auth_list

	rm -f $TMP_AUTH_LIST

	trap "mod_rem_auth cleanup" 1 2 3 15

	touch $TMP_AUTH_LIST
	chmod 600 $TMP_AUTH_LIST

	clearscr
	getyn 69 "\nShow passwords when printing entries"
	case $? in 
		0) show_passwd="TRUE" ;;
		1) show_passwd="FALSE" ;;
	esac
	
	
	#
	# Create list of auth file entries
	#

	cat $PPPAUTH | awk ' BEGIN { ind = 1; } {
		if ((substr($0, 1, 1) != "#") &&  ($0 != "")) {
			if (pswd == "TRUE")
				printf("%d) %-20s\t\t%-20s\n", ind, $1, $2);
			else
				printf("%d) %-20s\t\t%-20s\n", ind, $1, "*******");
			ind++;
		}
		}' pswd=$show_passwd >> $TMP_AUTH_LIST

	#
	# Prompt user for entry to Modify/Delete
	#

	if [[ -s $TMP_AUTH_LIST ]]; then
		clear
		pfmt -s nostd -g "$CAT:105" "Index"
		echo " \c"
		pfmt -s nostd -g "$CAT:125" "Host ID            \t"
		pfmt -s nostd -g "$CAT:126" "Password"
		echo "\n-------------------------------------------------------"
		more $TMP_AUTH_LIST
		echo "-------------------------------------------------------"

		max_auth_index=$(wc -l /tmp/auth_list | awk '{ print $1 }')		


		while true; do
			pfmt -s nostd -g "$CAT:127" "Enter entry to Modify/Remove ('q' to quit):"
			echo " \c"
			read auth_index rest

			case $auth_index in
				"") 	continue;;
				'q')	trap - 1 2 3 15
                                	return $OK;;

				*([0-9]))	;;
				*) continue;;
			esac

			if (( auth_index > max_auth_index || auth_index <= 0 )); then
				continue
			else
				break
			fi
		done

		#
		# See if they want to delete or modify this entry
		#

		while true; do
			pfmt -s nostd -g "$CAT:128" "Delete or Modify ('d' or 'm'):"
			echo " \c"
			read auth_action rest
			
			if [[ $auth_action  = "" ]]; then
				pfmt -s nostd -g "$CAT:129" "Enter 'd' or 'm'"
				continue;
			else
				break
			fi
		done
		
		if [[ $auth_action = 'd' ]]; then
			delete_auth_ent $auth_index
		else
			delete_auth_ent $auth_index
			#
			# delete_auth_ent returns removed entry in AUTH_REMOVED
			#

			id=$( echo $AUTH_REMOVED | awk '{ print $1 }')
			passwd=$( echo $AUTH_REMOVED | awk '{ print $2 }')

			add_auth $id
			
		fi

	else
		pfmt -s nostd -g "$CAT:130" "\nNo Entries in %s\n" $PPPAUTH 
		rm -f $TMP_AUTH_LIST
		press_any_key
		trap - 1 2 3 15
		return $FAIL
	fi

	rm -f $TMP_AUTH_LIST
	trap - 1 2 3 15		
	return $OK
}	
#######################################################
edit_auth() {

	# if there is no PPPAUTH file, creat it
	if [ ! -r "$PPPAUTH" ] ; then
		if [ -r $SAMP_DIR/pppauth.samp ] ; then
			/usr/bin/cp $SAMP_DIR/pppauth.samp $PPPAUTH
		else
			cat "#pppauth file" > $PPPAUTH
		fi
		/usr/bin/chown root $PPPAUTH
		/usr/bin/chgrp root $PPPAUTH
		/usr/bin/chmod 600 $PPPAUTH
	fi

	while true
	do
		clearscr
		pfmt -s nostd -g "$CAT:151" "Configure PPP authentication parameters\n"
		message 46 "1) Modify/Remove ID/password"
		message 47 "2) Add New ID/password"
		message 48 "3) Undo Last Modify,Remove or Add"
		message 49 "4) Previous Menu"
		echo "------------------------------------"
		pfmt -s nostd -g "$CAT:131" "Enter choice [4]:"
		echo " \c"
		read a_choice rest
		case $a_choice in
			1) mod_rem_auth ;;
			2) add_auth ;;
			3) undo_last $PPPAUTH ;;
			4) return $FAIL ;;
			"") return $FAIL ;;
			*) continue;;
		esac
	done
}

##############################################################################
#
# Undo last action
#

undo_last () {
	
	clearscr

	touch /tmp/undo_last_tmp
	chmod 600 /tmp/undo_last_tmp

	diff -c -b $1 $1.bak >> /tmp/undo_last_tmp

	if [ $? -eq 1 ] ; then
		pfmt -s nostd -g "$CAT:198" \
			"\nThe following changes to %s will be undone: \n" $1
		echo "------------------------------------------------------------"
		cat /tmp/undo_last_tmp
		echo "------------------------------------------------------------"
		pfmt -s nostd -g "$CAT:132" \
			"\nNOTE:\tThe output above was generated by executing the\n\tfollowing command:\n"
		echo "\n\t   diff $1 $1.bak\n"
		pfmt -s nostd -g "$CAT:134" \
			"\tSee the 'diff(1)' man page for an explaination\n\tof the above output\n\n"
		getyn 70 "Continue with undo ?"
		case $? in 
			0) cp $1.bak $1
			   pfmt -s nostd -g "$CAT:136" "\nLast action undone ...\n"
			   rm -f /tmp/undo_last_tmp
			   press_any_key
			   return $OK
			   ;;
			1) rm -f /tmp/undo_last_tmp
			   return $OK
		esac
	else
		pfmt -s nostd -g "$CAT:137" "\nNothing to Undo ....\n"
		press_any_key
	fi

}

##############################################################################
#main

uid=`/bin/id|awk '{ print $1}`
uid=`echo $uid| sed 's/uid=//'| sed 's/(.*//`
if [ "$uid" -ne 0 ] ; then
	message 148 "Must be privileged user to use"
	echo " $0"
	exit $FAIL	
fi

#
# First create configuration files we may need 
#
create_files

check_daemons_cmds

clearscr

2>&1

while true; do
	pfmt -s nostd -g "$CAT:146" "\n\nPPP Configuration Menu\n"
	pfmt -s nostd -g "$CAT:138" "1) Configure/Modify Dedicated PPP Link\n"
	pfmt -s nostd -g "$CAT:139" "2) Configure/Modify Dynamic Outgoing PPP Link\n"
	pfmt -s nostd -g "$CAT:140" "3) Configure/modify Manual Bringup PPP Link\n"
	pfmt -s nostd -g "$CAT:141" "4) Configure/Modify Dynamic Incoming PPP Link\n"
	pfmt -s nostd -g "$CAT:142" "5) Delete an entry\n"
	pfmt -s nostd -g "$CAT:143" "6) Undo Last Session (Selections 1-5)\n"
	pfmt -s nostd -g "$CAT:144" "7) List Configured Links\n"
	pfmt -s nostd -g "$CAT:145" "8) Configure PPP authentication parameters\n"
	pfmt -s nostd -g "$CAT:153" "q) Quit"
	pfmt -s nostd -g "$CAT:154" "\n\nEnter choice:"
	echo " \c"

 	read cmd rest
	case $cmd in
	1) conf_dedicated ;;
	2) conf_dyn_out ;;
	3) conf_man_out ;;
	4) conf_dyn_in ;;
	5) delete_entry ;;
	6) undo_last $PPPHOSTS;;
	7) list_entries ;;
	8) edit_auth ;;
	q) exit $OK ;;
	*) pfmt -s error -g "$CAT:152" "illegal choice\n" ;;
	esac

	clearscr
done
