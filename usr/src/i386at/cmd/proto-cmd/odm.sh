#ident	"@(#)odm.sh	15.1"

typeset pwin pkg_list pkg respdir

touch /etc/conf/pack.d/vxfs/Driver_vj.o
[ -d /etc/vx ] || mkdir /etc/vx
touch /etc/vx/upgrade

if [ "$PKGINSTALL_TYPE" = "UPGRADE" ]
then
	/usr/sbin/pkgrm -n onlinemgr >/dev/null 2>&1
elif [ "$PKGINSTALL_TYPE" = "UPGRADE2" ]
then
	/usr/sbin/pkgrm -n odm >/dev/null 2>&1
	[ $? != 0 ] && {
		/usr/sbin/pkgrm -n vxfs >/dev/null 2>&1
		/usr/sbin/pkgrm -n vxvm-va >/dev/null 2>&1
		/usr/sbin/pkgrm -n vxvm >/dev/null 2>&1
		/usr/sbin/pkgrm -n ODMdocs >/dev/null 2>&1
	}
fi

while :
do
	if $BACK_END_MANUAL
	then
		/usr/sbin/pkgadd -d /cd-rom -lpqn -r $respdir $pkg_list \
		< /dev/zero > /dev/null 2>> /tmp/more_pkgadd.err
		retval=$?
	else
		sh_umount /cd-rom
		display -w "$ODM_PROMPT"
		input_handler
		# This only works if odm_prepare was run; otherwise,
		# /tmp/cdrom1 will not exist. odm_prepare will be run
		# if vxinstall was run prior to upgrading.
		/sbin/mount -Fcdfs -r /tmp/cdrom1 /cd-rom

		call ioctl 0 30213 2
		/usr/sbin/pkgadd -q -d /cd-rom odm </dev/vt02 >/dev/vt02 2>&1
		retval=$?
		call ioctl 0 30213 1
	fi
	stty min 1
	case $retval in
		0 | 10 | 20 )
			break	#out of while loop
			;;
	esac
done
/sbin/putdev -d cdrom1 >/dev/null 2>&1
/sbin/rm -f /etc/scsi/pdi_edt
