#ident	"@(#)rebuild.sh	15.1"

function do_dump_dev
# arg 1 is the number of the disk (0 or 1)
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset DUMP_SLICE MINOR line DISK_NUM=$1

	/usr/sbin/prtvtoc ${CHAR_DISK_NODES[$DISK_NUM]} 2> /dev/null |
	while read line
	do
		set -- $line
		if [ "$3" = "DUMP" ]
		then
			DUMP_SLICE=${2%:*}
			break
		fi
	done
	[ -n "$DUMP_SLICE" ] || return 1

	# We call symlink, not link, below.  This is essential, because
	# /etc/scsi/pdimkdev would remove /dev/dump if it were a hard link.
	call symlink ${BLOCK_DISK_NODES[$DISK_NUM]%s0}s$DUMP_SLICE /dev/dump

	grep -v "PANICBOOT" < /etc/default/init > /tmp/TTT
	cat /tmp/TTT > /etc/default/init
	call unlink /tmp/TTT
	print "PANICBOOT=YES" >> /etc/default/init
	return 0
}

# See usr/src/*/sysinst/stage/*.ele -- these files control which disks can be
# used for the dump slice.
#
# For now we do not allow the user to put a dump slice on the second disk.
# The main reason is that we cannot know for certain what the minor number of
# the dump slice on the second disk will be, so we cannot know what to put in
# /etc/conf/sassign.d/kernel.

# do_dump_dev 0 || do_dump_dev 1
do_dump_dev 0

# We are now finished with the tape and disk nodes.  We remove them now so that
# when /etc/scsi/pdimkdev runs from /etc/inittab it starts with a clean slate.
sh_rm -f /dev/rdsk/c* /dev/dsk/c* /dev/rmt/*

while :
do
	if $IDCMD/idbuild -B > /tmp/kernel.build 2>&1
	then
		break
	else
		faultvt "$IDBUILD_FAILED"
	fi
done

$IDCMD/idmkinit -o /etc
call unlink /etc/.wsinitdate
