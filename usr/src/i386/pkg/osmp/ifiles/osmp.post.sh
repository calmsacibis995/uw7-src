#!/usr/bin/winxksh
#ident  "@(#)pkg.osmp:ifiles/osmp.post.sh	1.25.2.8"

#exec 2>/tmp/osmp.err
#set -x

PKGMSG=${PKGINST}.pkg
LOCALE=${LC_ALL:-${LC_MESSAGES:-${LANG:-"C"}}}

if [ ! -f /usr/lib/locale/${LOCALE}/LC_MESSAGES/${PKGMSG} ]
then
   if [ -f ${REQDIR}/inst/locale/${LOCALE}/${PKGMSG} -a \
	-d /usr/lib/locale/${LOCALE}/LC_MESSAGES ]
   then
	cp ${REQDIR}/inst/locale/${LOCALE}/${PKGMSG} \
	   /usr/lib/locale/${LOCALE}/LC_MESSAGES
   fi
fi

#
#	User decide continue to try install from floppy
#	or exit now.
#

Psm_Retry ()
{
	case $CHOICE in
	${PsmRetryArray[0]})
		RETRY="TRUE"
		;;
	${PsmRetryArray[1]})
		RETRY="FALSE"
		;;
	esac

	echo "RETRY=$RETRY" >/tmp/retry
}

#
#	check if a true PSM floppy has been inserted.
#

ReadPsmFloppy ()
{

	LOCALE=${LOCALE:-C}
	OSMPLOCALEDIR=/etc/inst/locale/${LOCALE}/menus/osmp
	defaultOSMPLOCALEDIR=/etc/inst/locale/C/menus/osmp
	
	[ -r ${OSMPLOCALEDIR}/txtstrings ] &&
	        . ${OSMPLOCALEDIR}/txtstrings || {
	        #have to use C locale, we have no choice. Used in help
	        OSMPLOCALEDIR=${defaultOSMPLOCALEDIR}
	        . ${defaultOSMPLOCALEDIR}/txtstrings
	}

	integer t
	MENU_TYPE=regular
	PSM_ERROR=0
	export MENU_TYPE PSM_ERROR 

#	eval display -w \"$PSM_REINSERT\"
	place_window $PSM_REINSERTCols $PSM_REINSERTLines+5 -fg $WHITE -bg $BLUE
	wprintf $CURWIN "$PSM_REINSERT"
	footer $PSM_FLOPFOOTER
	call getkey
	wclose $CURWIN

	RETRY="TRUE"
	>/tmp/retry
	while [ "${RETRY}" = "TRUE" ]
	do
		PSM_ERROR=1
		if dd if=/dev/rdsk/f0t bs=512 count=1 >/dev/null 2>&1
		then
			MEDIA=diskette1
			PSM_ERROR=0
		else 
			if dd if=/dev/rdsk/f1t bs=512 count=1 >/dev/null 2>&1
			then
				MEDIA=diskette2
				PSM_ERROR=0
			fi
		fi
		
		[ "${PSM_ERROR}" = "0" ] && {

				pkgadd -p -n -d ${MEDIA} oempsm < /dev/vt02 > /dev/vt02 2>&1
				rc=$?
				call ioctl 0 30213 1 # VT_ACTIVATE to reset to VT 01

				case ${rc} in
				0) PSM_ERROR=0
				   ;;
				10) PSM_ERROR=0
				   ;;
				*) PSM_ERROR=3
				   ;;
				esac
	 	}

	 	case $PSM_ERROR
		 in
			0 ) 
				footer ""
				return ;;
			1 ) # cannot install psm diskette...try again.
				display -w "$PSM_EMSG1" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler
				;;
			2) # inserted wrong psm diskette.
				display -w "$PSM_EMSG2" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler;;
			3) # diskette does not contain a psm package
				display -w "$PSM_EMSG3" -bg $RED -fg $WHITE 
				footer "$PSM_FLOPFOOTER"
				input_handler;;
		esac
		typeset OIFS="$IFS"
		IFS="$nl"
		set -A PsmRetryArray ${PsmRetryList}
		IFS="$OIFS"
		PsmRetry="${PsmRetryArray[0]}"
		typeset CHOOSE_FOOTER="$PsmRetryFooter" 
		typeset CHOOSE_TITLE="$PsmRetryENTRY"
		choose -f -e -exit 'Psm_Retry' -winparms "-fg $COMBO2_FG -bg $COMBO2_BG" "$PsmRetry" "${PsmRetryArray[@]}"
		input_handler
		PSM_ERROR=0
		. /tmp/retry
		[ "${RETRY}" = "FALSE" ] && {
			display -w "$PSM_EMSG4" -bg $RED -fg $WHITE 
			footer "$PSM_FLOPFOOTER"
			input_handler
		}
	done

}

# osmp.post.sh only gets called if the user wants a PSM floppy
ReadPsmFloppy
