#ident	"@(#)install_more.sh	15.1"

# The error messages in this file are not internationalized because
# they all indicate programmer error, not real problems.

function install_more_err
{
	[ -n "$SH_VERBOSE" ] && set -x

	faultvt "install_more: ERROR: $* $CONTROL_D"
	halt
}

function get_pkg_list
# arg 1 is the full name of a setinfo file.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset line

	while read line
	do
		set -- $line
		case "$1" in
		*#* | '')
			continue
			;;
		*)
			#Do nothing
			;;
		esac
		[ "$3" = "n" ] || print $1
	done < $1
	return
}

function get_pkg_name
# arg 1 is the full name of a pkginfo file.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset line OIFS="$IFS"

	IFS='='
	while read line
	do
		set -- $line
		case "$1" in
		NAME)
			print $2
			break # break out of the while loop
			;;
		*)
			#Do nothing
			;;
		esac
	done < $1
	IFS="$OIFS"
	return
}

function do_pkgadd
# arg 1 is the mount point of the CD.
# arg 2 is the name of the set or package.
# arg 3 is the serial id (product id plus serial number)
# arg 4 is the activation key
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset pkg_list pkg product_id version
	typeset respdir=/var/tmp/silent.resp$$

	if [ "$2" = "odm_upgrade" ]
	then
		pkg_list=""
		for pkg in vxfs vxvm vxvm-va ODMdocs
		do
			/usr/bin/pkginfo -i $pkg >/dev/null 2>&1
			[ $? = 0 ] && pkg_list="$pkg_list $pkg"
		done
	elif [ -s $1/$2/setinfo ]
	then
		pkg_list=$(get_pkg_list $1/$2/setinfo)
		# Since vxvm-va is set to "n" in odm setinfo file, we need
		# to add it now to the list of odm packages to install.
		[ $2 = "odm" ] && {
			echo $pkg_list | grep "\<vxvm-va\>" >/dev/null 2>&1
			[ $? -ne 0 ] && pkg_list="$pkg_list vxvm-va"
		}
	elif
		[ -s $1/$2/pkginfo ]
	then
		pkg_list=$2
	else
		install_more_err Cannot find set or package \"$2\".
	fi

	if [ "$2" = "odm_upgrade" ]
	then
		msg "$INSTALL_MORE_MIDDLE $(get_pkg_name $1/odm/pkginfo)."
	else
		msg "$INSTALL_MORE_MIDDLE $(get_pkg_name $1/$2/pkginfo)."
	fi
	sh_mkdir $respdir
	for pkg in $pkg_list
	do
		if [ -s $1/$pkg/install/response ]
		then
			sh_cp $1/$pkg/install/response $respdir/$pkg
		else
			> $respdir/$pkg
		fi
	done

	export SERIALID=$3
	export SERIALKEY=$4

	# pkgadd seems to close stdin when called with these options.  Since
	# we can't allow it to close *our* stdin (the keyboard), we give it
	# something harmless to close (/dev/zero).

	# All of pkgadd's true error messages to go the log file
	# /var/sadm/install/logs/$pkg.  The stuff that pkgadd
	# writes to stderr is just chaff.

	if [ "$2" = "nws" -o "$2" = "nwsJ" ]
	then 
	# The nws set must be installed as a set not as individual packages.
	/usr/sbin/pkgadd -d $1 -lpq $2 \
		< /dev/zero > /dev/null 2>> /tmp/more_pkgadd.err

	elif [ "$2" = "dosmerge" ]
	then 
	## The SerialID determines which dosmerge license package to install.
	## If the UnixWare SerialID is used, the evaluation version is 
	## installed.
	#product_id=${SERIALID%??????????}
	#[ "${product_id%?}" = "MRG" ] && {
	#	if [ "${product_id#???}" = "U" ]
	#		then version="u"; else version="2"; fi 
	#	pkg_list="$pkg_list merge${version}lic"
	
	# The above code is commented since now only the unlimited-user
	# merge is offered via silent installation.
		pkg_list="$pkg_list"
		# We must put the key pair into the licensekeys file and
		# not into the environment. If pkgadd finds a key pair 
		# in the environment it will not look in the file. 
		# We need to force pkgadd to look in the file because 
		# not all dosmerge packages use the same key.
		echo "$SERIALID\t$SERIALKEY"|keyadm -a >/dev/null 2>&1
		unset SERIALID SERIALKEY
	#}
	# Determine which language version of merge to install.
	[ "$LANG" = "C" ] || [ -z "$LANG" ] || {
		print $pkg_list|sed "s/^merge /${LANG}merge /"|read pkg_list
		if [ -s $1/${LANG}merge/install/response ]
		then 
			sh_cp $1/${LANG}merge/install/response $respdir/${LANG}merge
		else 
			> $respdir/${LANG}merge
		fi
	} 
	/usr/sbin/pkgadd -d $1 -lpqn -r $respdir $pkg_list \
		< /dev/zero > /dev/null 2>> /tmp/more_pkgadd.err

	elif [ "$2" = "compaq" ]
	then
	# The compaq request script must be run.
	/usr/sbin/pkgadd -d $1 -lpq compaq \
                < /dev/zero > /dev/null 2>> /tmp/more_pkgadd.err

	elif [ "$2" = "odm_upgrade" ]
	then
	# The odm set is upgraded using a special purpose script.
	. $SCRIPTS/odm

	else
	# For all the rest.
	/usr/sbin/pkgadd -d $1 -lpqn -r $respdir $pkg_list \
		< /dev/zero > /dev/null 2>> /tmp/more_pkgadd.err
	fi
	sh_rm -rf $respdir
	msg
	return 
}

function install_more
#arg 1 is the mount point of the CD.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset -i mpu_cnt uu_cnt 
	typeset product

	if [ "$PKGINSTALL_TYPE" != "NEWINSTALL" ]
	then
	# The postreboot script must umount /tmp even if the filesystem
	# is not memfs. Otherwise, packaging scripts run by postreboot
	# cannot find flags set in tmp directory in root. 
	# Edit postreboot script so that any /tmp filesystem is umounted.
	# This is a temporary fix for back-end-manual UnixWare 2.1 upnover.
	S02POSTINST=/etc/init.d/S02POSTINST
	[ -f $S02POSTINST ] && {
	/bin/ed $S02POSTINST <<EOEDIT 1>/dev/null 2>&1
?/tmp on /tmp type memfs
s?/tmp on /tmp type memfs?on /tmp type?
w 
q
EOEDIT
}
	fi

	(( $# == 1 )) || install_more_err Must supply the mount point.
	[ -d "$1" ]   || install_more_err $1 is not a directory.

	display "$INSTALL_MORE_BEGIN"
	DEFCONF=/var/sadm/install/admin/default
	mv $DEFCONF $DEFCONF.$$
	cat $DEFCONF.$$ | sed 's/^mail=.*$/mail=/' >$DEFCONF
	for product in $INSTALL_LIST 
	do
		case "$product" in
		UW)
			#We've already installed it.
			continue
			;;
		jale)
			do_pkgadd $1 jale $UW_SerialID $UW_KEY
			;;
		compaq)
			do_pkgadd $1 compaq $UW_SerialID $UW_KEY
			;;
		NWS)
			if [ "$LANG" = "ja" ]
			then
				do_pkgadd $1 nwsJ $UW_SerialID $UW_KEY
			else
				do_pkgadd $1 nws $UW_SerialID $UW_KEY
			fi	
			;;
		SDK)
			do_pkgadd $1 sdk $SDK_SerialID $SDK_KEY
			;;
		ODM)
			do_pkgadd $1 odm $ODM_SerialID $ODM_KEY
			S02POSTINST=/etc/init.d/S02POSTINST
			[ -f $S02POSTINST -a -f /tmp/unixware.dat \
					  -a -d /var/sadm/pkg/vxvm-va ] && {
			# ed cmd preserves permissions/owner/group
			/bin/ed $S02POSTINST <<EOEDIT 1>/dev/null 2>&1
/^bye_bye$
.i
sh /opt/vxvm-va/install/FindSysOwner >/dev/null 2>&1
.
w
q
EOEDIT
			}
			;;
		ODM_upgrade)
			do_pkgadd $1 odm_upgrade $ODM_SerialID $ODM_KEY 
			;;	
		MRG)
			if [ -n "$MRG_SerialID" ] 	
			then
			# Install 2-user or unlimited-user version.
				do_pkgadd $1 dosmerge $MRG_SerialID $MRG_KEY
			else
			# Install evaluation version.
				do_pkgadd $1 dosmerge $UW_SerialID $UW_KEY
			fi
			;;
		MPU)
			let mpu_cnt="$MPU_COUNT"
			while [ $mpu_cnt -gt 0 ]
			do
				echo "${MPU_SerialID[$mpu_cnt]}\t${MPU_KEY[$mpu_cnt]}" |keyadm -a >/dev/null 2>&1
				let mpu_cnt-=1
			done
			;;	
		UU)
			let uu_cnt="$UW_USER_BUMPS"
			while [ $uu_cnt -gt 0 ]
			do
				echo "${UU_SerialID[$uu_cnt]}\t${UU_KEY[$uu_cnt]}" |keyadm -a >/dev/null 2>&1
				let uu_cnt-=1
			done
			;;
		*)
			install_more_err Unknown product ID \"$product\".
			;;
		esac
	done
	mv $DEFCONF.$$ $DEFCONF

# ul96-12203 child a1. This is a work around for the dynatext 
# problem for ja user. Also, the ODMdocs collection is cleared, if
# the ODMdocs pkg is not installed (upgrade /overlay scenario).

MYDIR=`pwd`
EBTRCDIR=/etc/opt/dynatext
EBTRCDIRJA=/etc/opt/dynatext/ja
# remove any ja DynaText Browser Help collections from .ebtrc
[ -f $EBTRCDIR/.ebtrc ] && {
	[ -f $EBTRCDIR/.ebtrc.old ] || \
		cp $EBTRCDIR/.ebtrc $EBTRCDIR/.ebtrc.old
	grep -v "/usr/doc.*=DynaText " $EBTRCDIR/.ebtrc >/tmp/$$.ebtrc.1
	grep "/usr/doc.*=DynaText " $EBTRCDIR/.ebtrc >/tmp/$$.ebtrc.2
	grep "=DynaText Browser" /tmp/$$.ebtrc.2 >/tmp/$$.ebtrc.3
	cat /tmp/$$.ebtrc.1 /tmp/$$.ebtrc.3 >$EBTRCDIR/.ebtrc
}
# In $EBTRCDIRJA/.ebtrc, 1) correct the path in SYSCONFIG line,
# 2) add the ja Dynatetx browser line, if not present.
[ -f $EBTRCDIRJA/.ebtrc ] && {
	cd $EBTRCDIRJA
	[ -f .ebtrc.old ] || cp .ebtrc .ebtrc.old
ed .ebtrc <<EOEDIT 1>/dev/null 2>&1
/^SYSCONFIG	
s/text\/sysdocs.cfg/text\/ja\/sysdocs.cfg
w
w
q
EOEDIT
grep "^COLLECTION	/usr/doc/data/help/ja=DynaText ブラウザのヘルプ" .ebtrc >/dev/null 2>&1 || {
ed .ebtrc <<EOEDIT 1>/dev/null 2>&1
a
COLLECTION	/usr/doc/data/help/ja=DynaText ブラウザのヘルプ
.
w
w
q
EOEDIT
}

# remove ja DynaText Browser Help collections accessed from the C directory
grep -v "/usr/doc.*/ja/.*/C=DynaText " .ebtrc >/tmp/$$.ebtrc.1
grep "/usr/doc.*/ja/.*/C=DynaText " .ebtrc | \
 grep "=DynaText B" | uniq >/tmp/$$.ebtrc.2
cat /tmp/$$.ebtrc.1 /tmp/$$.ebtrc.2 >.ebtrc

}

# remove ODMdocs COLLECTION, if the ODMdocs pkg is not installed.
[ -f /var/sadm/pkg/ODMdocs/pkginfo ] || {
	[ -f $EBTRCDIRJA/.ebtrc ] && {
	grep -v "COLLECTION.*/odm=Online Data Manager" $EBTRCDIRJA/.ebtrc >/tmp/$$.ebtrc.1
	mv /tmp/$$.ebtrc.1 $EBTRCDIRJA/.ebtrc
	}
	[ -f $EBTRCDIR/sysdocs.cfg ] && {
	grep -v "COLLECTION.*/odm=Online Data Manager" $EBTRCDIR/sysdocs.cfg >/tmp/$$.sysdocs.cfg
	mv /tmp/$$.sysdocs.cfg $EBTRCDIR/sysdocs.cfg
	}
}

# fix the ja readme file access problem for ja DynaText Broswer Help
[ -d  /usr/doc/data/help/ja/styles/aspects ] && {
	[ -d /usr/doc/data/help/ja/styles/Oaspects ] || {
		mv /usr/doc/data/help/ja/styles/aspects \
		/usr/doc/data/help/ja/styles/Oaspects  1>/dev/null 2>&1

		[ -d  /usr/doc/Libraries/ja/STYLE/styles/aspects ] && {
			cd /usr/doc/Libraries/ja/STYLE/styles 1>/dev/null 2>&1
	        	find aspects -print | \
				cpio -pcduv /usr/doc/data/help/ja/styles 1>/dev/null 2>&1
		}
		# if cpio -pcduv failed restore the old aspects directory
		[ -d  /usr/doc/data/help/ja/styles/aspects ] || {
			mv /usr/doc/data/help/ja/styles/Oaspects \
			/usr/doc/data/help/ja/styles/aspects 1>/dev/null 2>&1
		}
	}
}
cd $MYDIR
# end of work around for ul96-12203

	wclose
	display "$INSTALL_MORE_END"
	call sleep 3 # Let the user see the screen before it disappears.
	wclose
	return
}
