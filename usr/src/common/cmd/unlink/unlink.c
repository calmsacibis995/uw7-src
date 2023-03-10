/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unlink:unlink.c	1.4.1.2"
#ident "$Header$"

/***************************************************************************
 * Command: unlink
 * Inheritable Privileges: P_MACREAD,P_DACREAD,P_MACWRITE,P_DACWRITE,
 *			   P_FILESYS,P_COMPAT
 *       Fixed Privileges: None
 * Notes: This command calls the unlink(2) system call directly.
 * 	  The privileges passed to command are only used by unlink(2).
 *
 ***************************************************************************/

main(argc, argv)
int argc;
char *argv[];
{
	if(argc!=2) {
		write(2, "Usage: /usr/sbin/unlink name\n", 29);
		exit(1);
	}
	exit((unlink(argv[1]) == 0) ? 0 : 1);
}
