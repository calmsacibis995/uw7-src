#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)q-add.sh	1.2"
#ident	"$Header$"

# PURPOSE: Configure the software to a particular device type on RS232
#
#---------------------------------------------------------------------

trap 'exit 1' 1 2 3 15

/usr/bin/rm -f /usr/tmp/ttylist* /usr/tmp/ap* /usr/tmp/title*

PID=$$
NODUPES=/usr/tmp/nodupes.$PID		#strip dupes by Maj/Min		

Error=no

# lists ports configured and available for adding devices

if [ "$1" = "COLLECT" -o "$1" = "REMOVE" ]
then

	#Each vendor can setup their own naming conventions for device
	#nodes and create them "hopefully" in /dev or /dev/term.
	#Try to ensure that we get them all, but remember we can't please
	#everybody....others will need to conform.

	# The order of following "ls" lines determines which file name the
	# users will see when there are multiple file names for the same device

	DEVLIST=/usr/tmp/devlist.$PID  		#build dev entries

	ls /dev/term/[0-9]*[0-9][s] > $DEVLIST 2>/dev/null 
	ls /dev/term/[0-9]*[0-9] >> $DEVLIST 2>/dev/null  
	ls /dev/term/[0-9]*[0-9][h] >> $DEVLIST 2>/dev/null  
	ls /dev/term/[a-z]*[0-9][s] >> $DEVLIST 2>/dev/null  
	ls /dev/term/[a-z]*[0-9] >> $DEVLIST 2>/dev/null  
	ls /dev/term/[a-z]*[0-9][h] >> $DEVLIST 2>/dev/null  
	ls /dev/term/[a-z]*[a-f] >> $DEVLIST 2>/dev/null 

	ls /dev/tty[0-9]*[0-9][s] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[0-9]*[0-9] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[0-9]*[0-9][h] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[a-o]*[0-9][s] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[a-o]*[0-9] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[a-o]*[0-9][h] >> $DEVLIST 2>/dev/null 
	ls /dev/tty[a-o]*[a-f] >> $DEVLIST 2>/dev/null 

	CONSOLE=`/usr/bin/ls -l /dev/console | /usr/bin/awk '{print $5,$6}'` 2>/dev/null
	touch $NODUPES
	for dev in `cat $DEVLIST`
	do
		LL=`/usr/bin/ls -l $dev | /usr/bin/cut -c1-45` 2>/dev/null
		MAJMIN=`echo $LL | /usr/bin/awk '{print $5,$6}'` 2>/dev/null

		## check for serial printer and kick it out
		OWNER=`echo $LL | /usr/bin/awk '{print $3}' `
		[ $OWNER = "lp" ] && {
			continue;
		}
		#strip maj/min from console to ensure not hooked to tty
		[ "$CONSOLE" = "$MAJMIN" ] && {
			continue;
		}
		/usr/bin/grep "$MAJMIN" $NODUPES >/dev/null 2>&1
		[ $? -eq 1 ] && {

			echo $MAJMIN >>$NODUPES 2>/dev/null
			case $dev in

			/dev/term/00s | /dev/term/00h | /dev/term/00)
				#special case for COM ports - different device names for
				#mouse and terminal/modem
		
				##check for mouse enabled on COM1
				/usr/bin/mouseadmin -l | /sbin/grep tty00 >/dev/null 2>&1
				[ $? -eq 0 ] && {
					continue;
				}
			;;

			/dev/term/01s | /dev/term/01h | /dev/term/01)
		
				##check for mouse enabled on COM2
				/usr/bin/mouseadmin -l | /sbin/grep tty01 >/dev/null 2>&1
				[ $? -eq 0 ] && {
					continue;
				}
			;;
		
			  *)
			;;
			esac
		#everything checks out at this point
		echo $dev >> /usr/tmp/ttylist.$VPID 2>/dev/null
		}
	done
	rm -f $NODUPES $DEVLIST &
	echo 0
	exit 0
fi

SPEED=$2
TYPE="$1"
PREFIX=tty
shift
if [ $# -ne 0 ]
then
	shift
fi

for i in $*
do
	ckfile device $i
	CRET=$?
	if [ $CRET -ne 0 ]
	then
		case $CRET in
			1) echo "Device name is not specified." >>/usr/tmp/ap.$VPID;;
			2) echo "$i is not full pathname." >>/usr/tmp/ap.$VPID;;
			3) echo "$i does not exist." >>/usr/tmp/ap.$VPID;;
			5) echo "$i is not a chacracter device." >>/usr/tmp/ap.$VPID;;
		esac
		Error=yes
		continue
	fi
	LL=`/usr/bin/ls -l $i | /usr/bin/cut -c1-45` 2>/dev/null
	MAJMIN=`echo $LL | /usr/bin/awk '{print $5,$6}'` 2>/dev/null

	## check for serial printer and kick it out
	OWNER=`echo $LL | /usr/bin/awk '{print $3}' `
	[ $OWNER = "lp" ] && {
		echo "$i is owned by lp." >>/usr/tmp/ap.$VPID
		Error=yes
		continue;
	}
	#strip maj/min from console to ensure not hooked to tty
	[ "$CONSOLE" = "$MAJMIN" ] && {
		echo "$i is hooked to console." >>/usr/tmp/ap.$VPID
		Error=yes
		continue;
	}
	/usr/bin/grep "$MAJMIN" $NODUPES >/dev/null 2>&1
	[ $? -eq 0 ] && {
		DUPDEV=`/usr/bin/grep "$MAJMIN" $NODUPES | /usr/bin/awk '{print $3}'` 2>/dev/null 
		echo "$i is the same as $DUPDEV." >>/usr/tmp/ap.$VPID
		Error=yes
		continue;
	}

	echo $MAJMIN $i >>$NODUPES 2>/dev/null

	if [ `echo $i | /usr/bin/grep "term"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c11-14`
	elif [ `echo $i | /usr/bin/grep "tty"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c9-12`
	elif [ `echo $i | /usr/bin/grep "dev"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c6-14`
	else
		TTY=`echo $i`
	fi
	case $TTY in
	00s | 00)
		/usr/bin/mouseadmin -l | /usr/bin/grep tty00 >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			echo "$i has serial mouse assigned." >>/usr/tmp/ap.$VPID
			Error=yes
		else
			echo 00s $i >>$NODUPES 2>/dev/null
			/usr/bin/grep "^00h" $NODUPES >/dev/null 2>&1
			[ $? -eq 0 ] && {
				DUPDEV=`/usr/bin/grep "^00h" $NODUPES | /usr/bin/awk '{print $2}'` 2>/dev/null 
				echo "$i is the same as $DUPDEV, except software flow-controlled." >>/usr/tmp/ap.$VPID
				echo "\tDo not select both." >>/usr/tmp/ap.$VPID
				Error=yes
			}
		fi
		;;
	00h)
		/usr/bin/mouseadmin -l | /usr/bin/grep tty00 >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			echo "$i has serial mouse assigned." >>/usr/tmp/ap.$VPID
			Error=yes
		else
			echo 00h $i >>$NODUPES 2>/dev/null
			/usr/bin/grep "^00s" $NODUPES >/dev/null 2>&1
			[ $? -eq 0 ] && {
				DUPDEV=`/usr/bin/grep "^00s" $NODUPES | /usr/bin/awk '{print $2}'` 2>/dev/null 
				echo "$i is the same as $DUPDEV, except hardware flow-controlled." >>/usr/tmp/ap.$VPID
				echo "\tDo not select both." >>/usr/tmp/ap.$VPID
				Error=yes
			}
		fi
		;;
	01s | 01)
		/usr/bin/mouseadmin -l | /usr/bin/grep tty01 >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			echo "$i has serial mouse assigned." >>/usr/tmp/ap.$VPID
			Error=yes
		else
			echo 01s $i >>$NODUPES 2>/dev/null
			/usr/bin/grep "^01h" $NODUPES >/dev/null 2>&1
			[ $? -eq 0 ] && {
				DUPDEV=`/usr/bin/grep "^01h" $NODUPES | /usr/bin/awk '{print $2}'` 2>/dev/null 
				echo "$i is the same as $DUPDEV, except software flow-controlled." >>/usr/tmp/ap.$VPID
				echo "\tDo not select both." >>/usr/tmp/ap.$VPID
				Error=yes
			}
		fi
		;;
	01h)
		/usr/bin/mouseadmin -l | /usr/bin/grep tty01 >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			echo "$i has serial mouse assigned." >>/usr/tmp/ap.$VPID
			Error=yes
		else
			echo 01h $i >>$NODUPES 2>/dev/null
			/usr/bin/grep "^01s" $NODUPES >/dev/null 2>&1
			[ $? -eq 0 ] && {
				DUPDEV=`/usr/bin/grep "^01s" $NODUPES | /usr/bin/awk '{print $2}'` 2>/dev/null 
				echo "$i is the same as $DUPDEV, except hardware flow-controlled." >>/usr/tmp/ap.$VPID
				echo "\tDo not select both." >>/usr/tmp/ap.$VPID
				Error=yes
			}
		fi
		;;
	*)
		;;
	esac
done
rm -f $NODUPES &

if [ $Error = "yes" ]
then
	echo "Error" > /usr/tmp/title.$VPID
	echo 0
	exit 0
fi

for i in $*
do

	/usr/sbin/sacadm -l | /usr/bin/grep ttymon3 >/dev/null 2>&1
	if [ $? = 1 ]
	then
		/usr/sbin/sacadm -a -pttymon3 -t ttymon -c "/usr/lib/saf/ttymon" -v " `/usr/sbin/ttyadm -V ` "
	fi

	if [ `echo $i | /usr/bin/grep "term"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c11-14`
	elif [ `echo $i | /usr/bin/grep "tty"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c9-12`
	elif [ `echo $i | /usr/bin/grep "dev"` ]
	then
		TTY=`echo $i | /usr/bin/cut -c6-14`
	else
		TTY=`echo $i`
	fi
	case $TTY in
	00s | 00h | 00)
		/usr/sbin/pmadm -r -p ttymon3 -s "00" >/dev/null 2>&1
		/usr/sbin/pmadm -r -p ttymon3 -s "00s" >/dev/null 2>&1
		/usr/sbin/pmadm -r -p ttymon3 -s "00h" >/dev/null 2>&1
		/usr/sbin/pmadm -a -p ttymon3 -s $TTY -S "login" -fu -v " `/usr/sbin/ttyadm -V ` " -m " `/usr/sbin/ttyadm -d $i -l $SPEED -s /usr/bin/shserv -m ldterm -p \"login: \" ` "
		;;
	01s | 01h | 01)
		/usr/sbin/pmadm -r -p ttymon3 -s "01" >/dev/null 2>&1
		/usr/sbin/pmadm -r -p ttymon3 -s "01s" >/dev/null 2>&1
		/usr/sbin/pmadm -r -p ttymon3 -s "01h" >/dev/null 2>&1
		/usr/sbin/pmadm -a -p ttymon3 -s $TTY -S "login" -fu -v " `/usr/sbin/ttyadm -V ` " -m " `/usr/sbin/ttyadm -d $i -l $SPEED -s /usr/bin/shserv -m ldterm -p \"login: \" ` "
		;;
	*)
		/usr/sbin/pmadm -r -p ttymon3 -s $TTY >/dev/null 2>&1
		/usr/sbin/pmadm -a -p ttymon3 -s $TTY -S "login" -fu -v " `/usr/sbin/ttyadm -V ` " -m " `/usr/sbin/ttyadm -d $i -l $SPEED -s /usr/bin/shserv -m ldterm -p \"login: \" ` "
		;;
	esac
	if [ $? = 0 ]
	then
		echo "Confirmation" > /usr/tmp/title.$VPID
		echo "The port $i was setup.\n" >>/usr/tmp/ap.$VPID
	else
		echo "Request Denied" > /usr/tmp/title.$VPID
		echo "The port $i setup failed.\n" >>/usr/tmp/ap.$VPID
	fi
	

done
echo 0
exit 0
