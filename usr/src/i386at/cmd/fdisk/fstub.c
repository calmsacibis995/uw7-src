#ident	"@(#)fdisk:i386at/cmd/fdisk/fstub.c	1.1"

/*
 *	FILE:	fstub.c 
 *	Description:
 *		This file contains a stub for the devattr function that
 *		fdisk uses to determine the special for the default
 *		disk drive.  Including devattr makes fdisk 3 times larger
 *		and this is bad on the install floppy.  fdisk does not
 *		need this capability on the install floppy so we will
 *		stub it out.
 */
char *
devattr(arg1,arg2)
{
	char *result="/dev/rdsk/c0b0t0d0s0";
	return(result);
}
