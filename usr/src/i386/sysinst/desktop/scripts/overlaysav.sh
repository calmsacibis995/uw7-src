#!/sbin/sh
#ident	"@(#)overlaysav.sh	15.1"


save_volatilefiles () {
    [ "$UPDEBUG" = YES ] && set -x

#    # the volatile file list 'boot.LIST' will be save in $ETC/inst/up
#
#    [ -d $ETC/inst/up ] || mkdir -p $ETC/inst/up
#
#    # eliminate comment lines from boot.LIST
#
#    filelist=/tmp/boot.LIST
#    grep -v "^#" $HDROOT/etc/inst/up/patch/boot.LIST >$filelist
#    cp $filelist $ETC/inst/up
#
#    cd $HDROOT
#
#    [ -f "$filelist" ] && {
#
#	[ "$UPDEBUG" = YES ] && goany
#
#    	while read filename
#    	do
#    		[ -f "$filename" ] && {
#
#    			find $filename -print 2>/dev/null |
#    			  cpio -pdmu $HDROOT/$UPGRADE_STORE >/dev/null 2>&1
#    		}
#
#    	done < ${filelist}
#    }

    [ "$UPDEBUG" = YES ] && goany

    # $ETC/vfstab not included in boot.LIST.  Since it's not marked 'v' in
    # the contents file we have to save it specifically for upgrade and overlay
    # Save it in $HDROOT/$UPGRADE_STORE/etc/vfstab

    cp $ETC/vfstab $HDROOT/$UPGRADE_STORE/etc/vfstab
#
#    # Remove special files from /dev for UPGRADE_v4 only
#    [ "$PKGINSTALL_TYPE" = "UPGRADE_v4" ] && rm -rf $HDROOT/dev >/dev/null 2>&1

    # make a lock file so that 'v' files are saved once only 
    >$UPTMP/savedboot.LIST

    [ "$UPDEBUG" = YES ] && goany
}

merge_vfstab() {

    [ "$UPDEBUG" = YES ] && set -x

    IFS=$TAB

    while read special fsck_dev mount_pt fs_type fsck_pass auto_mnt mnt_flgs
    do

#only include lines from the old vfstab in the new /etc/vfstab if they 
#aren't already in the new one, because the device name might be different now

    	if  grep "^$special" $HDROOT/etc/vfstab >/dev/null || \
	  grep "${TAB}$mount_pt${TAB}" $HDROOT/etc/vfstab >/dev/null 
	then
		:
	else
		 echo  "$special\t$fsck_dev\t$mount_pt\t$fs_type\t$fsck_pass\t$auto_mnt\t$mnt_flgs" >>$HDROOT/etc/vfstab
	fi

    done <$HDROOT/$UPGRADE_STORE/etc/vfstab

    >$UPTMP/mrgvfstab

    [ "$UPDEBUG" = YES ] && goany
}

Do_vfstab() {
    [ "$UPDEBUG" = YES ] && set -x

   #Do_vfstab will create $ETC/vfstab

   VFSTAB=$ETC/vfstab
   rm -f $VFSTAB

   cp /tmp/new.vfstab $VFSTAB 2>/dev/null

   [ "$UPDEBUG" = YES ] && goany
}

#main()

[ $PKGINSTALL_TYPE = NEWINSTALL ] && exit 0  #shouldn't happen, but can't hurt...

[ "$UPDEBUG" = YES ] && set -x

UPG_GLOBALS=/tmp/upg_globals
[ -s "$UPG_GLOBALS" ] && . $UPG_GLOBALS

TAB="	"
SPACE=" "
ETC=$HDROOT/etc

[ "$UPDEBUG" = YES ] && goany

# $UPTMP/savedboot.LIST will be there if files already saved
# if resuming after interrupt or powerdown, do not save the files again
# don't do this for INTERRUPTED destructive installation.

[ -f $UPTMP/savedboot.LIST -o "$PKGINSTALL_TYPE" = "INTERRUPTED" ] || save_volatilefiles

[ "$UPDEBUG" = YES ] && goany "After save_volatilefiles"

# Now set installation type to NEWINSTALL if INTERRUPTED installation
# is being done. Our thinking here is that we don't want to deal with
# "merge"-related code in scripts, nor see any prompts from packages
# having to deal with merging. 

[ "${PKGINSTALL_TYPE}" = "INTERRUPTED" ] && {
	PKGINSTALL_TYPE=NEWINSTALL
	AUTOMERGE=NULL
	echo "PKGINSTALL_TYPE=NEWINSTALL" >> ${GLOBALS}
	echo "AUTOMERGE=NULL" >> ${GLOBALS}
	exit 0
}

#if resuming after interrupt, do not remake vfstab

[ -f $UPTMP/mrgvfstab ] || {

    Do_vfstab

    [ "$UPDEBUG" = YES ] && goany "After Do_vfstab"

    merge_vfstab
}

[ "$UPDEBUG" = YES ] && goany

