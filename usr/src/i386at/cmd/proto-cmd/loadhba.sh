#!/usr/bin/xksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)loadhba.sh	15.1	97/12/19"

function func_hbainst
{
	[ -n "$debug" ] && set -x
	typeset HBA_ROOTDIR=$1
	typeset loaded_hbas=""
	typeset hba

	/sbin/modadmin -s | cut -d: -f3 >/tmp/modloaded.$$
	for driver in `cat ${HBA_ROOTDIR}/etc/modules`
	do
		grep "/$driver$" /tmp/modloaded.$$ >/dev/null 2>&1
		if [ $? -eq 0 ]
		then
			loaded_hbas="$loaded_hbas $driver"
		fi
	done
	PKGS=""
	for hba in $loaded_hbas
	do
		if [ -f ${HBA_ROOTDIR}/$hba/pkgmap ]
		then
			PKGS="$PKGS $hba"
		fi
	done	
}

function ReadHbaFloppy
{

[ -n "$SH_VERBOSE" ] && set -x
integer t m=0
typeset MEDIA=diskette1
typeset ishba
HBA_NAME="$1"
MENU_TYPE=regular
HBA_ERROR=0
export MENU_TYPE

max ${#HBA_NAME} ${#HBA_REINSERT}
(( HBA_REINSERTCols = $? + 2 ))
place_window $HBA_REINSERTCols 5 -fg $COMBO1_FG -bg $COMBO1_BG
t=${#HBA_REINSERT}/2
wgotoxy $CURWIN $HBA_REINSERTCols/2-$t 1
wprintf $CURWIN "$HBA_REINSERT"
t=${#HBA_PROMPT}/2
wgotoxy $CURWIN $HBA_REINSERTCols/2-$t 3
wprintf $CURWIN "$HBA_PROMPT\n"
footer "$HBA_FOOTER"
call getkey
wclose $CURWIN
footer "$HBA_LOAD_FOOTER"

while :
do
	mntsts="1"
	ishba="1"
	/sbin/mount -Fs5 -r /dev/dsk/f0t /install 2>/dev/null || /sbin/mount -Fdosfs -r /dev/dsk/f0t /install 2>/dev/null || /sbin/mount -Fs5 -r /dev/dsk/f1t /install 2>/dev/null || /sbin/mount -Fdosfs -r /dev/dsk/f1t /install 2>/dev/null 
	mntsts="$?"
	hbaformat="1"

	if [ "$mntsts" = "0" ] 
	then
		loadname=""
		if [ -f /install/etc/loadmods -a -f /install/etc/load.name ]
		then
			hbaformat="0"
			grep -v '^$' /install/etc/load.name > /tmp/loadname
			read loadname < /tmp/loadname
			if [ "$HBA_NAME" = "$loadname" ]
			then
				ishba="0"
				[ "${FLPYDEV}" = /dev/dsk/f1t ] && MEDIA=diskette2
			fi

		fi

		PKGS=""
		if [ -f /install/etc/HBAINST ]
		then
			# UW1.1 compatibiliy
			PKGS="`. /install/etc/HBAINST`"
		else
			# Intersection of modules loaded and related
			# packages on the IHV HBA floppy OR "all" if
			# intersection doesn't exist.
			func_hbainst "/install"
			[ "$PKGS" = "" ] && PKGS=all
		fi

		/sbin/umount /install	 2>/dev/null
		if [ "$ishba" = "0" -a -n "$PKGS" ] 
		then
			HBA_ERROR=1

			#
			# We only want to run pdiadd from the HBA package's
			# postinstall script if the package is being added
			# as an add-on (not from this script).   Provide a
			# means for determining via a value in SETNAME
			# package environment variable.
			#
			footer "$HBA_LOAD_FOOTER"
			place_window 70 5 -title "$HBA_LOAD_TITLE"
			INSTWID=$CURWIN
			wgotoxy $CURWIN 1 1
			wputstr $CURWIN "$hbamsg $loadname"
			showPackage ""
			showAction ""
			rc=0
			(
			SETNAME=from_loadhba /usr/sbin/pkgadd -p -d ${MEDIA} $PKGS < /dev/zero 2>&1 
			if [ "$?" != "0" ]
			then
				print "##**Aiee Penguin in the HBA**##" 
			fi
			) |&
			while read -p line
			do
				if [ "$line" = "##**Aiee Penguin in the HBA**##" ]
				then
					rc=1
					break
				fi	
				set -- $line
				if [ "$1" = "Package:" ]
				then
					shift
					showPackage "$*"
					showAction ""
				elif [ "$1" = "##" ]
				then
					shift
					showAction "$*"
				fi
			done
			wclose $INSTWID
			[ "${rc}" = "0" ] && HBA_ERROR=0
		else
		   if [ -z "$PKGS" ]
		   then
			HBA_ERROR=4
		   elif [ "$hbaformat" = "0" ] 
		   then
			HBA_ERROR=2
		   else
			HBA_ERROR=3
		   fi
		fi
	else
		HBA_ERROR=1
	fi
	
	case $HBA_ERROR
	in
	0 ) 	footer ""	#clear footer on exit
		return ;;
	1 ) # cannot install hba diskette...try again.
		display -w "$HBA_EMSG1" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	2) # inserted wrong hba diskette.
		display -w "$HBA_EMSG2" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	3) # diskette is not an hba diskette.
		display -w "$HBA_EMSG3" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	4) # no drivers on hba diskette are needed on system
		display -w "$HBA_EMSG4" -bg $ERROR_BG -fg $ERROR_FG
		input_handler;;
	esac
	footer "$HBA_FOOTER"
done
}

function ReadHbaCD
{
	typeset CDROOT="/cd-rom/.hba.flop"
	typeset ishba="1"
	typeset loadname=""

	[ "$SEC_MEDIUM_TYPE" != "cdrom" ] && return 1

	if [ -f ${CDROOT}/etc/loadmods -a -f ${CDROOT}/etc/load.name ]
	then
		grep -v '^$' ${CDROOT}/etc/load.name | read loadname
		[ "$1" = "$loadname" ] && ishba="0"

		HERE=`pwd`
		cd ${CDROOT}
		> /tmp/ihvname.$$
		for i in *
		do
			[ -f $i/pkgmap ] && {
				echo $i >> /tmp/ihvname.$$
			}
		done
		cd ${HERE}
	fi

	PKGS=""
	if [ -f ${CDROOT}/etc/HBAINST ]
	then
		# UW1.1 compatibiliy
		PKGS="`. ${CDROOT}/etc/HBAINST`"
	else
		func_hbainst "$CDROOT"
		[ "$PKGS" = "" ] && PKGS=all
	fi

	if [ "$ishba" = "0" -a -n "$PKGS" ] 
	then
		footer "$HBA_LOAD_FOOTER"
		place_window 70 5 -title "$HBA_LOAD_TITLE"
		INSTWID=$CURWIN
		wgotoxy $CURWIN 1 1
		wputstr $CURWIN "$hbamsg $loadname"
		showPackage ""
		showAction ""
		rc=0
		(
		SETNAME=from_loadhba /usr/sbin/pkgadd -p -d "$CDROOT" $PKGS < /dev/zero 2>&1
		if [ "$?" != "0" ]
		then
			print "##**Aiee Penguin in the HBA**##" 
		fi
		) |&
		while read -p line
		do
			if [ "$line" = "##**Aiee Penguin in the HBA**##" ]
			then
				rc=1
				break
			fi	
			set -- $line
			if [ "$1" = "Package:" ]
			then
				shift
				showPackage "$*"
				showAction ""
			elif [ "$1" = "##" ]
			then
				shift
				showAction "$*"
			fi
		done
		wclose $INSTWID
		[ "${rc}" = "0" ] && {
			for i in `cat /tmp/ihvname.$$`
			do
			   create_reverse_depend $i
			done
			rm -f /tmp/ihvname.$$ 1>/dev/null 2>&1
		}
	fi

	HBA_ERROR=0
	footer ""
	return 0
}

function SCOHBAS
{
[ -n "$debug" ] && set -x

typeset HBAROOT=/tmphba/hbaflop
loadname=""
if [ -f /${HBAROOT}/etc/loadmods -a -f /${HBAROOT}/etc/load.name ]
then
	grep -v '^$' ${HBAROOT}/etc/load.name > /tmp/loadname
	read loadname < /tmp/loadname

	PKGS=""
	func_hbainst "$HBAROOT"
	[ "$PKGS" = "" ] && PKGS=all

	for HBA_PKG in $PKGS
	do
		rc=1
		while (( rc == 1 ))
		do
			place_window 70 5 -title "$HBA_LOAD_TITLE"
			footer "$HBA_LOAD_FOOTER"
			INSTWID=$CURWIN
			wgotoxy $CURWIN 1 1
			wputstr $CURWIN "$hbamsg $loadname"
			showPackage ""
			showAction ""
			rc=0
			(
			SETNAME=from_loadhba /usr/sbin/pkgadd -p -d ${HBAROOT} \
				$HBA_PKG < /dev/zero 2>&1 
			if [ "$?" != "0" ]
			then
				print "##**Aiee Penguin in the HBA**##" 
			fi
			) |&
			while read -p line
			do
				if [ "$line" = "##**Aiee Penguin in the HBA**##" ]
				then
					rc=1
					break
				fi	
				set -- $line
				if [ "$1" = "Package:" ]
				then
					shift
					showPackage "$*"
					showAction ""
				elif [ "$1" = "##" ]
				then
					shift
					showAction "$*"
				fi
			done
			wclose $INSTWID
			if [ "$rc" != "0" ]
			then
				eval "display \"$HBA_EMSG5\" -fg $ERROR_FG -bg $ERROR_BG -above 1 -below 6"
				choose -f -e -winparms "-above 4 -below 1 -bg $ERROR_BG -fg $ERROR_FG" "$RETRY_HBA" "$RETRY_HBA" "$SHUTDOWN"      
				input_handler
				if [ "$CHOICE" = "$SHUTDOWN" ]
				then
					call uadmin 2 1
				fi
				wclose
			fi
		done
	done
	return 0
fi
return 1
}

integer n=1
typeset -x HBA_PROMPT
export HBA_ERROR HBA_PROMPT

[ ! -d /install ] && {
mkdir /install
}
[ ! -d /flpy2 ] && {
	sh_umount /flpy2	2>/dev/null
}

[ -f /etc/inst/scripts/oem.sh ] && /etc/inst/scripts/oem.sh

while [ "${IHVHBAS[n]}" != END ]
do
	let n+=1
done

SCOHBAS
(( n -= 1 ))

while (( n-=1 ))
do
	if [ -n "${IHVHBAS[n]}" ]
	then
		HBA_ERROR=1
		while [ "$HBA_ERROR" != 0 ]
		do
			HBA_PROMPT="${IHVHBAS[n]}"
			case "${IHVHBAMEDIA[n]}" in
			diskette)
				ReadHbaFloppy "${IHVHBAS[n]}"
				;;
			cdrom)
				ReadHbaCD "${IHVHBAS[n]}"
				;;
			*)
				;;
			esac
		done
	fi
done
unset -f ReadHbaFloppy
unset -f ReadHbaCD
rm -rf /tmphba
