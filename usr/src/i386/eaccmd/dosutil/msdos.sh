#ident	"@(#)eac:i386/eaccmd/dosutil/msdos.sh	1.2"
#ident  "$Header$"
#
# This file controls the mapping of UNIX 
# devices to MS-DOS drive letters and types. 
#
#	These are fine for a 6386 WGS Running SVR3.2:
#
#		A=/dev/rdsk/f0t
#		B=/dev/rdsk/f1t
#	Remember that only MS-DOS files may have colons in them.
#
#	Only one colon per MS-DOS filename.
#
#	MS-DOS filenames are either:
#
#		[ABCD...]:filename
#
#			or
#
#		DEVICE:filename
#
#	Where DEVICE is any UNIX file capable of seeking.
#	This means you can use regular unix files for MS-DOS
#	filesystems. Just say FILE: and all is well.
#
#
# Floppy device names
A=/dev/rdsk/f0t
B=/dev/rdsk/f1t
X=/dev/rdsk/f0t
Y=/dev/rdsk/f1t
#
# MS-DOS partition of first hard disk.
# If an application expects C: to be the MS-DOS partition of the first hard
# disk then uncomment the C line and comment the D line below.
# The p1 assumes that the first partition in the fdisk table is the DOS
# partition.  If this is not the case run fdisk to determine where the DOS
# partition is, and update the p1 accordingly.  For more information, refer
# to the sd01(7) man page.
#C=/dev/rdsk/c0b0t0d0p1
D=/dev/rdsk/c0b0t0d0p1
#
# MS-DOS partition of hard disk 1.
# If an application expects D: to be the MS-DOS partition of the second hard
# disk, then uncomment the following line.
# The second disk on your system might actually be different.  Please check
# the name of your second hard disk, using "devattr disk2 cdevice", and, if
# necessary, change the line below.  For more information, refer to the
# devattr(1) man page.  The above comment for p1 applies here also.
#D=/dev/rdsk/c0b0t1d0p1

