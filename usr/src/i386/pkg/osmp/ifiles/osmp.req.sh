#!/usr/bin/winxksh
#ident  "@(#)pkg.osmp:ifiles/osmp.req.sh	1.20.2.9"

#exec 2>/tmp/osmp.err
#set -x
LOCALE=${LC_ALL:-${LC_MESSAGES:-${LANG:-"C"}}}

PSM_set ()
{
	PSM=$CHOICE
	echo "PSM=\"${PSM}\"" > /tmp/psm
}

choose_PSM ()
{
	typeset CHOOSE_FOOTER="$PSM_RFOOTER" CHOOSE_TITLE="$PSM_ENTRY"

	choose -f -e -exit 'PSM_set' -winparms "-fg $COMBO2_FG -bg $COMBO2_BG" "$PSM" "${PSMARRAY[@]}"
	input_handler
	. /tmp/psm
	index=0
	while [ 1 ]
	do
		[ "${PSMARRAY[${index}]}" = "${PSM}" ] && break
		let index=index+1
	done
	echo "PSMINDEX=${index}" > /tmp/psmindex
}

select_PSM ()
{
	if [ -z "${PSMARRAY}" ]
	then
		typeset OIFS="$IFS"
		IFS="$nl"
		if [ "${ON_BOOTS}" = "FALSE" ]
		then
			set -A PSMARRAY ${PSMLIST0}
		else
			set -A PSMARRAY ${PSMLIST1}
		fi
		IFS="$OIFS"
	fi

	PSMINDEX=0
	case ${DIR} in

		compaq) PSMINDEX=1;;
		mps) PSMINDEX=2;;
#		abon) PSMINDEX=3;;
#		tricord) PSMINDEX=3;;
		cbus) 
			case ${CBUS_OEM} in
				101|102) PSMINDEX=3;;
				103)	PSMINDEX=4;;
				104)	PSMINDEX=5;;
			esac
			;;
#		ast) PSMINDEX=7;;
#		olivetti) PSMINDEX=8;;
#		acer) PSMINDEX=9;;

	esac

	PSM="${PSMARRAY[${PSMINDEX}]}"
	echo "PSM=${PSM}" > /tmp/psm
	echo "PSMINDEX=${PSMINDEX}" > /tmp/psmindex
	export PSM PSMINDEX
	place_window ${PSM_WIDTH}+5 ${PSM_DEPTH} -title "$PSM_ENTRY" 
	typeset wid=$CURWIN
	set_hotkey 5 choose_PSM

	# message if we're on boot floppy must note PSM add-on
	# floppy install as an option.  If OSMP is being
	# installed as an add-on later (after boot flop install
	# complete, then the message about the PSM floppy is
	# different
	
	if [ "${ON_BOOTS}" = "FALSE" ]
	then
		NOPSM_MSG="${ADD_NOPSM_MSG}"
		PSM_MSG2="${ADD_PSM_MSG2}"
	fi

	if [ "${PSMINDEX}" = "0" ]
	then
	   wprintf $wid "${NOPSM_MSG}" 
	else
	   wprintf $wid "%s %s %s ${PSM_MSG1} ${PSM} ${PSM_MSG2}"
	fi
	footer "${PSM_FOOTER}"
	call getkey
	wclose $wid
	footer ""
}

############# Begin UPGRADE AND OVERLAY #######################

# Don't need to worry about UPGRADE case for UW2.0 since the OSMP
# is new in UW2.0

SCRIPTS=/usr/sbin/pkginst
. ${SCRIPTS}/updebug

[ "$UPDEBUG" = YES ] && set -x

export PKGINSTALL_TYPE 

PKGINSTALL_TYPE=NEWINSTALL

[ "$UPDEBUG" = YES ] && goany

#chkpkgrel, returns a code, indicating which version of this pkg is installed.
#Return code 2 indicates overlay of the same or older version. For overlay,
#existence of the file $UPGRADE_STORE/$PKGINST.ver indicates presence of older
#version. This file contains the old version.

#	${SCRIPTS}/chkpkgrel returns    0 if pkg is not installed
#					1 if pkg if unknown version
#					2 if pkg is UnixWare 2.1
#					4 if pkg is UnixWare 1.1 and ilk
#					6 if pkg is UnixWare 2.01 and ilk
#					9 if newer pkg is installed

${SCRIPTS}/chkpkgrel osmp
PKGVERSION=$?

case $PKGVERSION in
	2)	PKGINSTALL_TYPE=OVERLAY	;;
	6)	PKGINSTALL_TYPE=UPGRADE2;;
	9)	exit 3	;; #pkgrm newer pkg before older pkg can be installed.
	*)	;;
esac

[ "$UPDEBUG" = YES ] && goany

[ "${PKGINSTALL_TYPE}" = "OVERLAY" -o "${PKGINSTALL_TYPE}" = "UPGRADE2" ] && {

	# restore type to "atup" in case install fails; postinstall
	# of this package will restore it to "mp"
	/etc/conf/bin/idtype atup 1>/dev/null 2>&1

	[ "$UPDEBUG" = YES ] && {
		echo "about to check on oempsm package"
		goany
	}

	# now remove the "oempsm" package, if it exists
	pkginfo -i oempsm 1>/dev/null 2>&1
	rc=$?
	[ "${rc}" = "0" ] && {
		pkgrm -n oempsm 1>/dev/null 2>&1
		rc=$?
		[ "$UPDEBUG" = YES ] && {
		  echo "return code from pkgrm of oempsm was " ${rc}
		  goany
		}
	}

	#now check to see whether a PSM installed by OSMP is already
	#configured

#	for i in ast mps cbus compaq tricord olivetti acer abon
	for i in mps compaq cbus
	do
		/etc/conf/bin/idcheck -p $i
		rc=$?
		[ "$UPDEBUG" = YES ] && {
		  echo "idcheck of " $i " returned " ${rc}
		  goany
		}

		[ "${rc}" != "0" -a "${rc}" != "100" ] && {
		  # driver was configured;  remove it if sdevice contains
		  # $interface psm (watch out for silly name conflict)

		  if grep interface /etc/conf/mdevice.d/$i | grep psm
		  then
			/etc/conf/bin/idinstall -P osmp -d $i
		  fi 1>/dev/null 2>&1
		}
	done

	
}

[ "$UPDEBUG" = YES ] && goany

############# End  UPGRADE AND OVERLAY #######################

# detect if being invoked via boot floppy!

if [ -f /etc/inst/scripts/postreboot.sh ]
then
	ON_BOOTS=TRUE
else
	ON_BOOTS=FALSE
fi
ON_BOOTS=FALSE

# For DEBUGGING!
# echo ON_BOOTS is $ON_BOOTS
#

# Call autodetect to install appropriate modules.
unset DIR OTHERS CBUS_OEM AUTODETECT_RETCODE
/usr/bin/autodetect 1>/dev/null 2>&1
AUTODETECT_RETCODE=$?
case ${AUTODETECT_RETCODE} in
	1 )
		DIR=compaq
		OTHERS="mps cbus"
		;;
	6 | 7 )
		DIR=mps
		OTHERS="compaq cbus"
		;;
#	10 ) 
#		DIR=abon
#		OTHERS="compaq mps cbus"
	101|102 )
		DIR=cbus
		CBUS_OEM=${AUTODETECT_RETCODE}
		OTHERS="mps compaq"
		;;
	103 )
		DIR=cbus
		CBUS_OEM=103
		OTHERS="mps compaq"
		;;
	104 )
		DIR=cbus
		CBUS_OEM=104
		OTHERS="mps compaq"
		;;
#	2 )
#		DIR=ast
#		OTHERS="mps cbus compaq tricord olivetti acer"
#		;;

#	3 )
#		DIR=acer
#		OTHERS="mps cbus compaq tricord olivetti"
#		;;

#	4 )
#		DIR=tricord
#		OTHERS="ast mps cbus compaq olivetti acer" 
#		;;
#	8 )
#		DIR=olivetti
#		OTHERS="ast mps cbus compaq tricord acer" 
#		;;
	*)
		OTHERS="mps compaq cbus"
		;;
esac

#
#	Not called from boot floppy, 
#	so set up winxksh environment.
#
if [ "${ON_BOOTS}" = "FALSE" ] 
then
	. /tmp/osmp_wininit
else

	LOCALE=${LOCALE:-C}
	OSMPLOCALEDIR=/etc/inst/locale/${LOCALE}/menus/osmp
	defaultOSMPLOCALEDIR=/etc/inst/locale/C/menus/osmp
	
	[ -r ${OSMPLOCALEDIR}/txtstrings ] &&
	        . ${OSMPLOCALEDIR}/txtstrings || {
	        #have to use C locale, we have no choice. Used in help
	        OSMPLOCALEDIR=${defaultOSMPLOCALEDIR}
	        . ${defaultOSMPLOCALEDIR}/txtstrings
	}
fi

# silent mode boot floppy installation support
# if the variable SILENT_PSM is set, its value will be an index used to
# select the PSM.  Valid index values do not have an asterisk:
# Note that autodetect has not been updated to find audobon HW.
#
#	Index		PSM
#	  1		Compaq TriFlex PSM (ProLiant, SystemPro)
#	  2		MP Specification 1.1&1.4
#	  3		Corollary Cbus-I XM Symmetric, Cbus-II
#	  4		IBM 720 PC SERVER Corollary Cbus-II
#	  5		OLIVETTI SNX200/400 Corollary Cbus-II
#	  3*		Tricord PowerFrame 
#	  7*		AST Manhattan SMP (5x<CPU>)
#	  8*		Olivetti LSX 5050
#	  9*		AcerAltos 17000, AcerFrame
#	  10		Data General NUMALiiNE
#
# check if ${SILENT_INSTALL} = true.  If the variable "SILENT_PSM"
# is set, Set PSMINDEX to its value.
# otherwise, check to see if autodetect determined the box was MP
# Specification compliant, in which case the PSMINDEX is 2, otherwise,
# set PSMINDEX to 1 (Compaq ProLiant)

if [ "${ON_BOOTS}" = "TRUE" ] && ${SILENT_INSTALL}
then
	if [ "${SILENT_PSM}" != "" ]
	then
		PSMINDEX=${SILENT_PSM}
	else
	   if [ "${PLATFORM}" = "compaq" ]
	   then
		if [ "$DIR" = "mps" ]
		then
			PSMINDEX=2	# MP Specification v1.1 and 1.4
		else
			PSMINDEX=1	# Compaq ProLiant
		fi
	   fi
	fi
	echo "PSMINDEX=$PSMINDEX" > /tmp/psmindex
else

	#
	# Just
	#	1. prompt the user for PSM selection,
	#	2. store her/his choice under #	/tmp/psm and /tmp/psmindex, and
	#       3. Boot floppies will (1) execute /tmp/osmp.req.sh
	#                             (2) rm /tmp/osmp.req.sh # IMPORTANT!!!!
	#                             (3) execute /tmp/osmp.post.sh 
	
	select_PSM
fi

#
#	Reset tty and clean up the screen 
#
[ "${ON_BOOTS}" = "FALSE" ]  && . /tmp/osmp_winexit

# do the following so that the return code from this script, when
# executed from the request script, returns 0
echo > /dev/null
