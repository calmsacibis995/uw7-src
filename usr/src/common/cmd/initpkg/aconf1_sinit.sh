#!/etc/dcu.d/winxksh
#ident	"@(#)initpkg:common/cmd/initpkg/aconf1_sinit.sh	1.11.1.1"

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

. /etc/TIMEZONE

if [ "`/sbin/resmgr -k-1 -pRM_INITFILE,s 2>/dev/null`" = "resmgr" ]
then
	if [ /stand/resmgr -nt /stand/resmgr.sav ]
	then
		cp /stand/resmgr /stand/resmgr.sav
	fi
fi

/etc/conf/bin/idconfupdate -f

DCU_ACTION="`/sbin/resmgr -k-1 -pDCU_ACTION,s 2>/dev/null`"

case "$DCU_ACTION" in

"REBUILD")
	/etc/conf/bin/idbuild 2>&1 > /dev/null
;;
"REBOOT")
	pfmt -l $LABEL -s info -g $CAT:164 \
		"New hardware instance mapped to static driver,\n\
		 rebooting to incorporate addition.\n"

	sleep 5
	/sbin/uadmin 2 1
;;
esac
