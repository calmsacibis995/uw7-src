#!/sbin/sh -

# Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
#                                                                         
#        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
#                   SANTA CRUZ OPERATION INC.                             
#                                                                         
#   The copyright notice above does not evidence any actual or intended   
#   publication of such source code.                                      

# @(#)cmd.vxvm:common/voladm/disk.ckinit.sh	1.3 3/4/98 18:01:15 - cmd.vxvm:common/voladm/disk.ckinit.sh
#ident	"@(#)cmd.vxvm:common/voladm/disk.ckinit.sh	1.3"

# Copyright(C)1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043

#
# disk_ckinit - validate that it is safe to (re)initialize a list of disks
#
# If necessary, the user will be asked to determine whether the operation
# is safe. If the operation type is encap, then only encapsulation is
# checked for.
#
# disk_ckinit has the following usage:
#
#  disk_ckinit device daname [device daname ...] op
#
#  Results are returned in the following environment variables.
#
#     ckinit_init - disks to be initialized
#     ckinit_noreinit - disks to not to be re-initialized
#     ckinit_reatt - disks to be re-attached
#     ckinit_encap - disks to be encapsulated
#
# 'op' must be set to one of 'add', 'replace', or 'encap'.
#
# Scripts that want to use these functions must source this file.
#
#. ${VOLADM_LIB:-/usr/lib/vxvm/voladm.d/lib}/vxadm_lib.sh

disk_ckinit()
{
	#
	# There should be at least three arguments.
	#
	if [ $# -lt 3 ]
	then
		ewritemsg -M vxvmshm:528 \
		"Usage: disk_ckinit device daname [device daname ...] op"
		quit 100
	fi

	if [ `expr $# % 2` -ne 1 ]
	then
		ewritemsg -M vxvmshm:631 \
		"disk_ckinit: Total number of arguments should be odd."
		quit 100
	fi

	voladm_system_hostid || quit 1

	#
	# Initialize group list variables.
	#

	# list types
	dev_num=2 	# Number of elements in device list:
			# device, daname

	dg_num=3	# Number elements in dg_list group:
			# device, daname, dgname

	hostid_num=3	# Number of elements in hostid_list group:
			# device, daname, hostid

	detach_num=4	# Number of elements in a detach_list group:
			# device, daname, dgname, dmname

	# list element indexes

	dev_index=1	# Index of the device name in all list types.
	da_index=2	# Index of the daname in all list types.
	dg_index=3	# Index of dgname in dg_list and detach_list.
	host_index=3	# Index of hostid in hostid_list.
	dm_index=4	# Index of dmname in detach_list.


	disp_file=$tmpfile4	# temporary file used to display messages

	#
	# Set 'dev_list' and 'op' from the function arguments.
	#
	dev_list=
	for i in $*
	do
		device=$1
		daname=$2

		append_list dev_list $device $daname

		shift $dev_num

		if [ $# -eq 1 ]
		then
			op=$1
			break
		fi
	done

	operations="add replace encap"

	if not list_member $op $operations
	then
		ewritemsg -M vxvmshm:630 \
		"disk_ckinit: Bad operation specified. Try one of: $operations"
		quit 100
	fi

	#
	# Initialize result variables.
	#
	ckinit_init=
	ckinit_noreinit=
	ckinit_reatt=
	ckinit_encap=

	#
	# Validate the device nodes to make sure that the
	# requested operation can proceed in a sane fashion.
	#
	validate_devfile
	[ -z "$dev_list" ] && return 0

	#
	# For the 'add' operation check for any inuse devices. The user will
	# decide whether these inuse partitions should be encapsulated.
	#
	if [ "X$op" = "Xadd" ]
	then
		inuse_check
		[ -z "$dev_list" ] && return 0
	fi

	#
	# Check for an existing DA record for the disks.
	#
	da_check

	#
	# Re-online all devices in dev_list to ensure that any status for
	# the disk is regenerated.
	#
	da_reonline

	#
	# Create temporary file used by 'get_disk_header_fields' function.
	#
	vxdisk_list_outfile=$tmpfile1

	vxdisk -s list > $vxdisk_list_outfile 2>/dev/null

	#
	# Perform basic checks on the disk header and VTOC for the disks.
	#
	# Set the variables 'attach_dev_list' and 'encap_dev_list'
	# which will be used later.
	#
	disk_check

	#
	# Handle disks that don't have the invalid and ready flags set.
	#
	# 'dev_list' is set by 'disk_check' and changed by
	# 'process_attach_disk'.
	#
	if [ -n "$attach_dev_list" ]
	then
		process_attach_disk

		if [ "X$op" != "Xencap" ]
		then
			process_already_init_disk
		fi
	fi

	#
	# Complete handling of 'encap' operation.
	#
	if [ "X$op" = "Xencap" ]
	then
		append_list ckinit_encap \
		$dev_list $encap_dev_list $attach_dev_list
		return 0
	fi

	#
	# Complete handling of 'add' and 'init' operations.
	#
	if [ -n "$attach_dev_list" ]
	then
		if [ `list_count $attach_dev_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:424 \
"The following disk you selected for use appears to already have
  been initialized for the Volume Manager.  If you are certain the
  disk has already been initialized for the Volume Manager, then you
  do not need to reinitialize the disk device.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:430 \
"The following disks you selected for use appear to already have
  been initialized for the Volume Manager.  If you are certain the
  disks already have been initialized for the Volume Manager, then
  you do not need to reinitialize these disk devices. 
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Reinitialize these devices?" Y \
		"Reinitialize this device?" y \
		attach_dev_list yes_list no_list

		append_list dev_list $yes_list

		# The user already decided to use these disks in
		# process_already_init_disk. At this point it is just a
		# matter of whether the disks will be reinitialized.
		append_list ckinit_noreinit $no_list
	fi
	
	append_list ckinit_init $dev_list

	#
	# If there is a valid VTOC for a disk but the disk has
	# never been initialized for use with the Volume Manager,
	# make sure it is okay to use the disk.
	#
	# 'encap_dev_list' will have been set by 'disk_check'.
	#
	if [ -n "$encap_dev_list" ]
	then
		if [ `list_count $encap_dev_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:419 \
"The following disk device has a valid VTOC, but does not appear to have
  been initialized for the Volume Manager.  If there is data on the disk
  that should NOT be destroyed you should encapsulate the existing disk
  partitions as volumes instead of adding the disk as a new disk.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:422 \
"The following disk devices have a valid VTOC, but do not appear to have
  been initialized for the Volume Manager.  If there is data on the disks
  that should NOT be destroyed you should encapsulate the existing disk
  partitions as volumes instead of adding the disks as new disks.
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Encapsulate these devices?" Y \
		"Encapsulate this device?" y \
		encap_dev_list yes_list no_list

		append_list ckinit_encap $yes_list

		if [ -n "$no_list" ]
		then
			voladm_yorn_batch -h \
			-g $dev_num -p $dev_index -d $dev_index \
			-M vxvmshm:0 -m vxvmshm:0 \
"Instead of encapsulating, initialize?" N \
"Instead of encapsulating, initialize?" n \
			no_list yes_list ignore_list

			append_list ckinit_init $yes_list
		fi
	fi
}

#
# Validate the device nodes to make sure that the
# requested operation can proceed in a sane fashion.
#
validate_devfile()
{
	valid_devfile_list=
	invalid_devfile_list=

	shift $#	# Reset argument count to zero.
	set -- $dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2
		STAT_SDI_NAME=
		dogi_device_rawpath $device rpath
		eval `vxparms -s $rpath 2> /dev/null`
		if [ "X$STAT_SDI_NAME" = "X$daname" ]
		then
			append_list valid_devfile_list $device $daname
		else
			append_list invalid_devfile_list $device $daname
		fi
		shift $dev_num
	done

	if [ -n "$invalid_devfile_list" ]
	then
		if [ `list_count $invalid_devfile_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:488 \
"This disk device does not appear to be valid.  The disk may not be
  formatted (format(1M)), may have a corrupted VTOC, the device file
  for the disk may be missing or invalid, or the device may be turned-off
  or detached from the system.  This disk will be ignored.
  Output format: [Device_Name,Disk_Access_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:476 \
"These disk devices do not appear to be valid.  The disks may not be
  formatted (format(1M)), may have corrupted VTOCs, the device files
  for the disks may be missing or invalid, or the devices may be turned-off
  or detached from the system.  These disks will be ignored.
  Output format: [Device_Name,Disk_Access_Name]"
		fi

		voladm_list_display -abcn -f $disp_file \
		-g $dev_num -d all $invalid_devfile_list
	fi

	dev_list="$valid_devfile_list"
}

#
# Check for an existing DA record for the disks.
#
da_check()
{
	da_error_list=
	offline_list=
	dg_list=
	da_list=

	vxdisk_outfile=$tmpfile2
	vxdisk_errfile=$tmpfile3

	vxdisk -q list > $vxdisk_outfile 2> /dev/null
	> $vxdisk_errfile

	shift $#	# Reset argument count to zero.
	set -- $dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

		save_list="$*"

		shift $#	# Reset argument count to zero.
		set -- `grep "^\$daname[ 	]" $vxdisk_outfile`
		if [ $? -ne 0 -o $# -lt 5 ]
		then
			# If the DA record does not exist, define it. The
			# define will fail if the DA name does not appear
			# to be valid.
			doit vxdisk define $daname 2>> $vxdisk_errfile
			if [ $? -ne 0 ]
			then
				append_list da_error_list $device $daname
			else
				append_list da_list $device $daname
			fi
		else
			# If the disk is already in a disk group then it
			# should be skipped. If the disk record is offline
			# then ask whether it should be onlined later.
			if [ "X$4" != "X-" ]
			then
				append_list dg_list $device $daname $4

			elif [ "X$5" = "Xoffline" ]
			then
				append_list offline_list $device $daname

			else
				append_list da_list $device $daname
			fi
		fi


		shift $#	# Reset argument count to zero.
		set -- $save_list
		shift $dev_num
	done

	# The following DA records could not be defined.
	if [ -n "$da_error_list" ]
	then
		if [ `list_count $da_error_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:409 \
"The following device could not be defined. The error encountered with
  the device will be displayed after the next prompt.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:414 \
"The following devices could not be defined. The errors encountered with
  these devices will be displayed after the next prompt.
  Output format: [Device_Name]"
		fi

		voladm_list_display -acn -f $disp_file \
		-g $dev_num -d $dev_index $da_error_list

		echo > $disp_file
		voladm_display -acn -f $disp_file $vxdisk_errfile
	fi

	# The following DA records were already in disk groups.
	if [ -n "$dg_list" ]
	then
		if [ `list_count $dg_list` -eq $dg_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:410 \
"The following device has already been added to a disk group.
  This disk will be ignored.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		else
			ewritemsg -f $disp_file -M vxvmshm:411 \
"The following devices already have been added to a disk group.
  These disks will be ignored.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		fi

		voladm_list_display -abcn -f $disp_file \
		-g $dg_num -d all $dg_list
	fi

	# The following DA records were offline. Ask whether they should
	# be onlined.
	if [ -n "$offline_list" ]
	then
		if [ `list_count $offline_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:100 \
"Access is disabled for the following disk.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:101 \
"Access is disabled for the following disks.
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Enable access for these devices?" Y \
		"Enable access for this device?" y \
		offline_list online_list ignore_list

		append_list da_list $online_list
	fi

	dev_list="$da_list"
}

#
# Re-online all devices in dev_list to ensure that any status for
# the disk is regenerated.
#
da_reonline()
{
	get_list_group $dev_num $da_index dev_list online_list
	doit vxdisk online $online_list 2> /dev/null
}

#
# Perform basic checks on the disk header and VTOC for the disks.
#
disk_check()
{
	if [ ! -f "$vxdisk_list_outfile" ]
	then
		ewritemsg -M vxvmshm:629 \
		"disk_check: file with 'vxdisk -s list' output not found"
		quit 1
	fi

	use_list=
	no_flags_list=
	offline_list=
	aliased_list=
	mpath_list=
	# These two lists are used outside of this function.
	attach_dev_list=
	encap_dev_list=

	shift $#	# Reset argument count to zero.
	set -- $dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

		# The get_disk_header_fields function sets variables for
		# diskflags, dgname, and hostid.
		if not get_disk_header_fields $daname -o [ -z "$diskflags" ]
		then
			append_list no_flags_list $device $daname
			shift $dev_num
			continue
		fi

		# mpath disabled disks are never candidates for anything
		if list_member mdisable $diskflags
		then
			append_list mpath_list $device $daname
			shift $dev_num
			continue
		fi

		# If the disk is not online, then skip it.
		if not list_member online $diskflags
		then
			append_list offline_list $device $daname
			shift $dev_num
			continue
		fi

		# Check for a valid VTOC. If invalid don't make any further
		# checks on the presumption that the device is available for
		# any use.
		dogi_device_rawpath $device rpath
		if not dogi_valid_vtoc $device
		then
			append_list use_list $device $daname
			shift $dev_num
			continue
		fi

		if list_member aliased $diskflags
		then
			append_list aliased_list $device $daname
		elif list_member ready $diskflags && \
		     not list_member invalid $diskflags
		then
			# This list will be processed outside of disk_check.
			append_list attach_dev_list $device $daname
		else
			# The disk has a valid VTOC, but has not been
			# initialized by the Volume Manager. If this is
			# not an encap operation later on ask whether the
			# disk should be encapsulated. This list will be
			# processed outside of disk_check.
			append_list encap_dev_list $device $daname
		fi

		shift $dev_num
	done

	# Flag field not found for disks.
	if [ -n "$no_flags_list" ]
	then
		if [ `list_count $no_flags_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:121 \
"An unexpected error was encountered getting information for this disk.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:513 \
"Unexpected errors were encountered getting information for these disks.
  Output format: [Device_Name]"
		fi
		voladm_list_display -acn -f $disp_file \
		-g $dev_num -d $dev_index $no_flags_list
	fi

	# mpath disks
	if [ -n "$mpath_list" ]
	then
		if [ `list_count $mpath_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:486 \
"This device is subsumed by a multi-path metadevice and cannot be used.
  Output format: [Device_name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:472 \
"These devices are subsumed by multi-path metadevices and cannot be used.
  Output format: [Device_name]"
		fi
		voladm_list_display -acn -f $disp_file \
		-g $dev_num -d $dev_index $mpath_list
	fi

	# DA records were offline.
	if [ -n "$offline_list" ]
	then
		if [ `list_count $offline_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:489 \
"This disk device is disabled (offline) and cannot be used.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:475 \
"These disk devices are disabled (offline) and cannot be used.
  Output format: [Device_Name]"
		fi
		voladm_list_display -acn -f $disp_file \
		-g $dev_num -d $dev_index $offline_list
	fi

	# Aliased DA records.
	if [ -n "$aliased_list" ]
	then
		if [ `list_count $aliased_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:487 \
"This disk device appears to have been initialized already.
  However, there is an unexpected discrepancy in the on-disk
  information.  You should check to ensure that there are no
  overlapping partitions on this device.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:473 \
"These disk devices appear to have been initialized already.
  However, there is an unexpected discrepancy in the on-disk
  information.  You should check to ensure that there are no
  overlapping partitions on these devices. 
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Use these devices?" N \
		"Use this device?" n \
		aliased_list yes_list ignore_list

		append_list use_list $yes_list
	fi

	dev_list="$use_list"
}

#
# Handle disks that have the invalid and ready flags set.
#
# 'attach_dev_list' is used as input and output for the function.
#
# This adds entries to the 'ckinit_reatt' list.
#
process_attach_disk()
{
	#
	# Use 'vxreattach -c' to determine whether the disk is a detached
	# disk that can be reattached.
	#

	detach_list=
	attach_list=

	shift $#	# Reset argument count to zero.
	set -- $attach_dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

		vxreattach_output=`doit vxreattach -c $daname 2> /dev/null`
		if [ $? -eq 0 ]
		then
			dgname=`list_item 1 $vxreattach_output`
			dmname=`list_item 2 $vxreattach_output`
			append_list detach_list \
			$device $daname $dgname $dmname
		else
			append_list attach_list $device $daname
		fi
		shift $dev_num
	done

	# 'use_list' is used to track disks that will be used.
	use_list="$attach_list"

	# For the add operation ask whether disks that were found to
	# be detached should be reattached.
	if [ "$op" = "add" -a -n "$detach_list" ]
	then
		if [ `list_count $detach_list` -eq $detach_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:484 \
"This device appears to be detached from its disk.  You can choose to
  reattach the device to the disk in its disk group, or add it as a new
  disk.  Reattaching may fail, if there is a formatting error on the
  disk or some other type of problem.  If the reattach fails, you can
  choose to add the device as a new disk.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group,Disk_Media_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:470 \
"These devices appear to be detached from their disks.  You can choose to
  reattach the devices to disks in their disk groups or add them as new
  disks. If there is a formatting error on the disk or some other type
  of problem, reattaching may fail. If the reattach fails, you can choose
  to add the devices as new disks.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group,Disk_Media_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $detach_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Reattach these devices?" Y \
		"Reattach this device?" y \
		detach_list yes_list no_list

		if [ -n "$yes_list" ]
		then
			# Convert list back to device and daname list by
			# removing the disk media and disk group names.
			remove_list_group $detach_num $dm_index yes_list
			remove_list_group 3 $dg_index yes_list
			append_list ckinit_reatt $yes_list
		fi

		if [ -n "$no_list" ]
		then
			# Convert list back to device and daname list by
			# removing the disk media and disk group names.
			remove_list_group $detach_num $dm_index no_list
			remove_list_group 3 $dg_index no_list
			append_list use_list $no_list
		fi
	elif [ -n "$detach_list" ]
	then
		# For non-add operations ask whether disks that were found to
		# be detached should be used or skipped. Attaching must be done
		# through the 'Add or initialize a disk' option.

		if [ `list_count $detach_list` -eq $detach_num ]
		then
			ewritemsg -f $disp_file -M vxmxshm:0 \
"This device appears to be detached from its disk group. You may wish
  to reattach the device.  To reattach it select \\\"Add or initialize
  one or more disks\\\" from the main disk operations menu and enter
  this device name.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group,Disk_Media_Name]"
		else
			ewritemsg -f $disp_file -M vxmxshm:0 \
"These devices appear to be detached from their disk groups. You may
  wish to reattach these devices. To reattach the devices select \\\"Add
  or initialize one or more disks\\\" from the main disk operations menu
  and enter the device names for these disks.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group,Disk_Media_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $detach_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Continue with $op operation?" N \
		"Continue with $op operation?" n \
		detach_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Convert list back to device and daname list by
			# removing the disk media and disk group names.
			remove_list_group $detach_num $dm_index yes_list
			remove_list_group 3 $dg_index yes_list
			append_list use_list $yes_list
		fi
	fi

	# Reset attach_dev_list to the disks found above.
	attach_dev_list="$use_list"

	if [ "X$op" = "Xencap" -a -n "$attach_dev_list" ]
	then
		if [ `list_count $attach_dev_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:485 \
"This device appears to have been initialized already for the Volume
  Manager.  If you encapsulate the device then any old Volume Manager
  information will be lost.  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:471 \
"These devices appear to have been initialized already for the Volume
  Manager.  If you encapsulate the devices then any old Volume Manager
  information will be lost.  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Okay to encapsulate?" N \
		"Okay to encapsulate?" n \
		attach_dev_list yes_list ignore_list

		attach_dev_list="$yes_list"
	fi
}

#
# Process an already initialized disk. Only the 'add' and 'init'
# operations use this function. Re-initialization is implied by
# 'encap'.
#
# 'attach_dev_list' is used as input and output for the function.
#
# This adds entries to the 'ckinit_reatt' list.
#
process_already_init_disk()
{

	use_list=
	share_list=
	hostid_list=
	dg_list=
	replace_list=
	vxdisk_errfile=$tmpfile2
	vxdg_outfile=$tmpfile3

	shift $#	# Reset argument count to zero.
	set -- $attach_dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

		get_disk_header_fields $daname

		if list_member shared $diskflags
		then
			append_list share_list $device $daname
			if [ "X$clusterid" != X ]
			then
				if not list_member $clusterid $clusterid_list
				then
					append_list clusterid_list $clusterid
				fi
			fi
		elif [ -n "$hostid" -a "X$system_hostid" != "X$hostid" ]
		then
			append_list hostid_list $device $daname $hostid
		elif [ -n "$dgname" ]
		then
			# Disks that have a disk group association.
			append_list dg_list $device $daname $dgname
		else
			# Disks that are replacement disks. In other words
			# the disks have no existing disk group association.
			append_list replace_list $device $daname
		fi

		shift $dev_num
	done

	#
	# Clear the share flag from disks that have the flag set.
	# Add these disks to 'use_list'.
	#
	if [ -n "$share_list" ]
	then

		if [ `list_count $share_list` -eq $dev_num ]
		then
			if [ "X$clusterid" = "X" ]
			then
				ewritemsg -f $disp_file -M vxvmshm:492 \
"This disk is listed as a shared device, but does not appear to be
  associated with a cluster. If you are certain
  that the device is not in active use by any other hosts you can
  choose to remove the status as a shared device.  You must clear
  the shared device status before using the disk on this system.
  Output format: [Device_Name]"
			else
				ewritemsg -f $disp_file -M vxvmshm:491 \
"This disk is listed as a shared device belonging to cluster $clusterid.
  If any node in cluster $clusterid is active, you should not clear the
  shared device status. If you are certain that the device is not in
  active use by other hosts you can choose to remove the statis as a
  shared device. You must clear the shared device status before using
  the disk on this system.
  Output format: [Device_Name]"
			fi
		else
			if [ `list_count $clusterid_list` -eq 0 ]
			then
				ewritemsg -f $disp_file -M vxvmshm:481\
"These disks are listed as shared devices but do not appear to be
  associated with a cluster."
			else
				if [ `list_count $clusterid_list` -eq 1 ]
				then
					ewritemsg -f $disp_file -M vxvmshm:479 \
"These disks are listed as shared devices belonging to cluster $clusterid_list."
				else
					ewritemsg -f $disp_file -M vxvmshm:480 \
"These disks are listed as shared devices belonging to the following
  clusters: $clusterid_list."
				fi
			fi
			ewritemsg -a -f $disp_file -M vxvmshm:242 \
"If any node in another cluster which can access these devices is active, you
  should not clear the shared device status. If you are certain that these
  devices are not in active use by any other hosts in any other cluster you
  can choose to remove the status as a shared device. You must clear the
  shared device status before using these disks on this system.
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Clear shared device status?" N \
		"Clear shared device status?" n \
		share_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Set-up a list for the clearimport operation.
			clearimport_list="$yes_list"
			remove_list_group $dev_num $dev_index clearimport_list
			doit vxdisk clearimport $clearimport_list \
			2> $vxdisk_errfile

			if [ $? -eq 0 ]
			then
				append_list use_list $yes_list
			else
				ewritemsg -f $disp_file -M vxvmshm:140 \
"Clear operation failed for these disks failed with the following errors:"
				echo >> $disp_file
				voladm_display -acn -f $disp_file \
				$vxdisk_errfile
			fi
		fi
	fi

	#
	# Clear a hostid association for disks that have a hostid set.
	# Add these disks to 'use_list'.
	#
	if [ -n "$hostid_list" ]
	then
		if [ `list_count $hostid_list` -eq $hostid_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:490 \
"This disk devices is currently listed as in use by another host.
  If you are certain that the other host is not using the disk, you
  can choose to clear the use status. To use the disk the use status
  must be cleared.
  Output format: [Device_Name,Disk_Access_Name,Hostid]"
		else
			ewritemsg -f $disp_file -M vxvmshm:474 \
"These disk devices are currently listed as in use by another host.
  If you are certain that the other host is not using the disks, you
  can choose to clear the use status. To use these disks the use status
  must be cleared.
  Output format: [Device_Name,Disk_Access_Name,Hostid]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $hostid_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Clear use status?" N \
		"Clear use status?" n \
		hostid_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Remove the hostid from the yes_list.
			remove_list_group $hostid_num $host_index yes_list

			# Set-up a list for the clearimport operation.
			clearimport_list="$yes_list"
			remove_list_group $dev_num $dev_index clearimport_list
			doit vxdisk clearimport $clearimport_list \
			2> $vxdisk_errfile
			if [ $? -eq 0 ]
			then
				append_list use_list $yes_list
			else
				ewritemsg -f $disp_file -M vxvmshm:141 \
"Clear operation failed with the following error:"
				echo >> $disp_file
				voladm_display -acn -f $disp_file \
				$vxdisk_errfile
			fi
		fi
	fi

	#
	# Display a message for replacement disks. These disks will be
	# added to the 'use_list'.
	#
	if [ -n "$replace_list" ]
	then
		if [ `list_count $replace_list` -eq $dev_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:417 \
"The following disk device appears to have been initialized already.
  The disk is currently available as a replacement disk.
  Output format: [Device_Name]"
		else
			ewritemsg -f $disp_file -M vxvmshm:420 \
"The following disk devices appear to have been initialized already.
  The disks are currently available as replacement disks.
  Output format: [Device_Name]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-g $dev_num -p $dev_index -d $dev_index \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Use these devices?" Y \
		"Use this device?" y \
		replace_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			append_list use_list $yes_list
		fi
	fi

	#
	# If no disks were placed on 'dg_list', then set 'attach_dev_list'
	# and return now.
	#
	if [ -z "$dg_list" ]
	then
		attach_dev_list="$use_list"
		return
	fi

	# Handle disks with an existing disk group association.
	vxdg -q list > $vxdg_outfile

	enabled_list=
	disabled_list=
	init_list=

	shift $#	# Reset argument count to zero.
	set -- $dg_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2
		dgname=$3

		state=
		eval \
		`while read name state id
		do
			if [ "$name" = "$dgname" ]
			then
				echo "state=\"$state\""
			fi
		done < $vxdg_outfile`

		case "$state" in
		enabled)
			append_list enabled_list $device $daname $dgname;;
		disabled)
			append_list disabled_list $device $daname $dgname;;
		*)
			append_list init_list $device $daname $dgname;;
		esac

		shift $dg_num
	done

	if [ -n "$enabled_list" ]
	then
		if [ `list_count $enabled_list` -eq $dg_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:418 \
"The following disk device appears to have been initialized already.
  The disk is listed as added to a disk group, but is not currently
  active.  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		else
			ewritemsg -f $disp_file -M vxvmshm:421 \
"The following disk devices appear to have been initialized already.
  The disks are listed as added to a disk group, but are not currently
  active.  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $dg_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Use these devices?" Y \
		"Use this device?" y \
		enabled_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Remove dgname from yes_list before adding the
			# devices to use_list.
			remove_list_group $dg_num $dg_index yes_list
			append_list use_list $yes_list
		fi
	fi

	if [ -n "$disabled_list" ]
	then
		if [ `list_count $disabled_list` -eq $dg_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:493 \
"This disk is listed as added to a disk group, but is not currently
  active.  The associated disk group is currently disabled due to
  errors, which means the device can not be reattached to its disk
  group.  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		else
			ewritemsg -f $disp_file -M vxvmshm:477 \
"These disks are listed as added to a disk group, but are not currently
  active.  The associated disk group is currently disabled due to errors,
  which means the devices can not be reattached to their disk group.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $dg_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Use these devices?" Y \
		"Use this device?" y \
		disabled_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Remove dgname from yes_list before adding the
			# devices to use_list.
			remove_list_group $dg_num $dg_index yes_list
			append_list use_list $yes_list
		fi
	fi

	if [ -n "$init_list" ]
	then
		if [ `list_count $init_list` -eq $dg_num ]
		then
			ewritemsg -f $disp_file -M vxvmshm:494 \
"This disk is listed as added to a disk group.  The disk group is
  not currently enabled (imported).  You may wish to import the disk
  group.  If so, quit from this operation and select \\\"Enable access
  to (import) a disk group\\\" from the main disk operations menu.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		else
			ewritemsg -f $disp_file -M vxvmshm:478 \
"These disks are listed as added to a disk group.  The disk group is
  not currently enabled (imported).  You may wish to import the disk
  group.  If so, quit from this operation and select \\\"Enable access
  to (import) a disk group\\\" from the main disk operations menu.
  Output format: [Device_Name,Disk_Access_Name,Disk_Group]"
		fi

		voladm_yorn_batch -h -f $disp_file \
		-b -g $dg_num -p $dev_index -d all \
		-M vxvmshm:0 -m vxvmshm:0 \
		"Use these devices?" Y \
		"Use this device?" y \
		init_list yes_list ignore_list

		if [ -n "$yes_list" ]
		then
			# Remove dgname from yes_list before adding the
			# devices to use_list.
			remove_list_group $dg_num $dg_index yes_list
			append_list use_list $yes_list
		fi
	fi

	attach_dev_list="$use_list"
}

#
# Find the disk header fields in $vxdisk_list_outfile for the DA name argument.
#
# Return 0 if the record was found; Return 1 if the record was
# not found.
#
# NOTE: This function assumes that $vxdisk_list_outfile contains the output
# of 'vxdisk -s list'.
#
get_disk_header_fields()
{
	daname=$1

	retval=1
	diskflags=
	dgname=
	hostid=
	clusterid=

	if [ ! -f "$vxdisk_list_outfile" ]
	then
		ewritemsg -M vxvmshm:637 \
		"get_disk_header_fields: 'vxdisk -s list' output file not found"
		quit 1
	fi

	# Search for the DA record's entry in the 'vxdisk -s' output file.
	eval \
	`while read line_label fields 
	do
		if [ "$line_label" = "Disk:" -a "$fields" = "$daname" ]
		then

			while read line
			do
				# Records are separated by blank lines. Return
				# when a blank line is encountered.
				if [ -z "$line" ]
				then
					echo \
"retval=0 diskflags=\"$diskflags\" dgname=\"$dgname\" hostid=\"$hostid\" clusterid=\"$clusterid\""
					return 0
				fi
				shift $#	# Reset argument count to zero.
				set -- $line
				case $1 in
					flags:) shift; diskflags="$*"; ;;
					dgname:) shift; dgname="$*"; ;;
					hostid:) shift; hostid="$*"; ;;
					clusterid:) shift; clusterid="$*"; ;;
				esac
			done
			echo \
"retval=0 diskflags=\"$diskflags\" dgname=\"$dgname\" hostid=\"$hostid\" clusterid=\"$clusterid\""
			return 0
		fi
	done < $vxdisk_list_outfile`
	return $retval
}


#
# Perform encapsulation operations common to disk.init.sh and
# disk.encap.sh.
#
# disk_encap has the following usage:
#
#  disk_encap device daname [device daname ...] op
#
# 'op' must be set to either 'add' or 'encap'.
#
# These variables are referenced:
# use_default_dmnames, create_group - boolean: yes,no
# disk_dgname - disk group name for encapsulated disks
#
disk_encap()
{
	#
	# There should be at least three arguments.
	#
	if [ $# -lt 3 ]
	then
		ewritemsg -M vxvmshm:529 \
		"Usage: disk_encap device daname [device daname ...] op"
		quit 100
	fi

	if [ `expr $# % 2` -ne 1 ]
	then
		ewritemsg -M vxvmshm:633 \
		"disk_encap: Total number of arguments should be odd."
		quit 100
	fi

	#
	# Initialize group list variables.
	#

	# list types
	dev_num=2 	# Number of elements in device list:
			# device, daname

	dg_num=3	# Number elements in dg_list group:
			# device, daname, dgname

	# list element indexes

	dev_index=1	# Index of the device name in all list types.
	da_index=2	# Index of the daname in all list types.
	dg_index=3	# Index of dgname in dg_list and detach_list.

	disp_file=$tmpfile4	# temporary file used to display messages

	#
	# Set 'dev_list' and 'op' from the function arguments.
	#
	dev_list=
	for i in $*
	do
		device=$1
		daname=$2

		append_list dev_list $device $daname

		shift $dev_num

		if [ $# -eq 1 ]
		then
			op=$1
			break
		fi
	done

	operations="add encap"

	if not list_member $op $operations
	then
		ewritemsg -M vxvmshm:632 \
		"disk_encap: Bad operation specified. Try one of: $operations"
		quit 100
	fi

	if [ `list_count $dev_list` -eq $dev_num ]
	then
		ewritemsg -f $disp_file -M vxvmshm:423 \
"The following disk has been selected for encapsulation.
  Output format: [Device_Name]"
	else
		ewritemsg -f $disp_file -M vxvmshm:429 \
"The following disks have been selected for encapsulation.
  Output format: [Device_Name]"
	fi

	voladm_list_display -a -f $disp_file \
	-g $dev_num -d $dev_index $dev_list

	voladm_yorn -M vxvmshm:144 "Continue with encapsulation?" y
	if [ $? -ne 0 ]
	then
		return
	fi

	if [ "$op" = "add" -a "X$disk_dgname" = "Xnone" ]
	then
		export device; \
		ewritemsg -M vxvmshm:314 \
"Previously no disk group was selected. For encapsulation a disk group
  must be specified. Please specify a disk group. If no disk group is
  specified the encapsulation operation will be skipped."

		voladm_disk_group -n "" rootdg

		[ $? -eq 10 ] && create_group=yes

		disk_dgname="$dgname"

		if [ "X$disk_dgname" = "Xnone" ]
		then
			return
		fi
	fi


	if [ "X$disk_dgname" = "" ]
	then
		ewritemsg -f $disp_file -M vxvmshm:287 \
"No disk group name specified for encapsulation."
		voladm_continue
		return
	fi

	root_devices=""
	if [ -n "$VOL_ROOTABLE_FILESYSTEMS" ]
	then
		for fs in $VOL_ROOTABLE_FILESYSTEMS
		do
			get_mountdevice $fs dsk
			append_list root_devices $dsk_disk
		done
	fi
	
	#
	# Create a list of the disks to be encapsulated along with
	# the disk group to be used. If the root disk is to be
	# encapsulated it must be added to the root disk group.
	#
	dg_list=
	shift $#	# Reset argument count to zero.
	set -- $dev_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2

		if list_member $device $root_devices
		then
			if [ "X$disk_dgname" != "Xrootdg" ]
			then
				export device; \
				ewritemsg -M vxvmshm:169 \
"Disk $device contains the /, /usr, /var, /stand, or /home' file system.  As a result
  it must be added to the rootdg disk group."
				voladm_continue -n
				dgname=rootdg
			else
				dgname="$disk_dgname"
			fi
		else
				dgname="$disk_dgname"
		fi

		append_list dg_list $device $daname $dgname

		shift $dev_num
	done

	# process each disk to be encapsulated
	failure=no
	success=no
	a_opt=-A
	shift $#	# Reset argument count to zero.
	set -- $dg_list
	while [ $# -gt 0 ]
	do
		device=$1
		daname=$2
		dgname=$3

		if [ "$use_default_dmnames" = "no" ]
		then
			voladm_new_disk_dmname "$dgname" "$device"
		else
			dmname=`vxnewdmname $dgname`
		fi

		if [ "$dgname" != "rootdg" -a "$create_group" = "yes" ]
		then
			export device dgname dmname; \
			ewritemsg -M vxvmshm:91 \
"A new disk group $dgname will be created and the disk device $device will
  be encapsulated and added to the disk group with the disk name $dmname."
			c_opt=-c
		else
			c_opt=
			export device dgname dmname; \
			ewritemsg -M vxvmshm:397 \
"The disk device $device will be encapsulated and added to the disk group
  $dgname with the disk name $dmname."
		fi

		doit vxencap $a_opt $c_opt -g "$dgname" $dmname=$daname 2> $tmpfile1
		if [ $? -ne 0 ]
		then
			export tmpfile1; \
			ewritemsg -M vxvmshm:402 \
"The encapsulation operation failed with the following error:
  `cat $tmpfile1`"
			failure=yes
			voladm_continue -n
		else
			success=yes
		fi

		shift $dg_num
	done

	if [ "$success" = "yes" ]
	then
		if [ "$failure" = "no" ]
		then
			export device; \
			ewritemsg -M vxvmshm:405 \
"The first stage of encapsulation has completed successfully.  You
  should now reboot your system at the earliest possible opportunity."

		else
			export device; \
			ewritemsg -M vxvmshm:404 \
"The first stage of encapsulation has been partially successful.  If
  you would like to continue with the encapsulation you should reboot
  the system at the earliest possible opportunity, otherwise you should
  retry the encapsulation."
		fi
  
		ewritemsg -M vxvmshm:403 \
"The encapsulation will require two or three reboots which will happen
  automatically after the next reboot.  To reboot execute the command:

shutdown -g0 -y -i6

  This will update the /etc/vfstab file so that volume devices are
  used to mount the file systems on this disk device.  You will need
  to update any other references such as backup scripts, databases,
  or manually created swap devices."

	fi
}
