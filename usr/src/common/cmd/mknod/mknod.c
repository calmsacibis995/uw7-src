/*		copyright	"%c%" 	*/

#ident	"@(#)mknod:mknod.c	1.9.2.1"

/***************************************************************************
 * Command : mknod
 * Inheritable Privileges : P_DACREAD,P_MACREAD,P_DACWRITE,P_MACWRITE,
 *                          P_FSYSRANGE,P_FILESYS,P_OWNER
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************/
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


/*
 *	@(#) mknod.c 1.6 88/05/05 mknod:mknod.c
 */
/***	mknod - build special file
 *
 *	mknod  name [ b ] [ c ] major minor
 *	mknod  name m	( shared data )
 *	mknod  name p	( named pipe )
 *	mknod  name s	( semaphore )
 *
 *	MODIFICATION HISTORY
 *	M000	11 Apr 83	andyp	3.0 upgrade
 *	- (Mostly uncommented).  Picked up 3.0 source.
 *	- Added header.  Changed usage message.  Replaced hard-coded
 *	  makedev with one from <sys/types.h>. 
 *	- Added mechanism for creating name space files.
 *	- Added some error checks.
 *	- Semi-major reorganition.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/fstyp.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>
#include <errno.h>

#define	ACC	0666

extern int errno;

/*
*Procedure:     main
*
* Restrictions:
                 fprintf:none
                 makedev:none
*/
main(argc, argv)
int argc;
char **argv;
{
	register mode_t		mode;
	register dev_t		arg;
	register major_t	majno;
	register minor_t	minno;
	long			number();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmknod");
	(void)setlabel("UX:mknod");

	if(argc < 3 || argc > 5)
		usage();
	if(argv[2][1] != '\0')
		usage();
	if(argc == 3) {
		switch (argv[2][0]) {
		case 'p':
			mode = S_IFIFO;
			arg = 0;	/* (not used) */
			break;
		case 'm':
			/* if Xenix package is not installed disallow this option */
			if (sysfs(GETFSIND, "xnamfs") == -1) {
				pfmt(stderr,MM_ERROR,
					":1:invalid option - Xenix package not installed\n");
				exit(2);
			}
			mode = S_IFNAM;
			arg = S_INSHD;
			break;
		case 's':
			/* if Xenix package is not installed disallow this option */
			if (sysfs(GETFSIND, "xnamfs") == -1) {
				pfmt(stderr,MM_ERROR,
					":1:invalid option - Xenix package not installed\n");
				exit(2);	
			}
			mode = S_IFNAM;
			arg = S_INSEM;
			break;
		default:
			usage();
			/* NO RETURN */
		}
	}
	else if(argc == 5) {
		switch(argv[2][0]) {
		case 'b':
			mode = S_IFBLK;		/* M000 was 060666 */
			break;
		case 'c':
			mode = S_IFCHR;		/* M000 was 020666 */
			break;
		default:
			usage();
		}

		majno = (major_t)number(argv[3]);
		if (majno == (major_t)-1 || majno > MAXMAJ){
			pfmt(stderr,MM_ERROR,
				":2:invalid major number '%s' - valid range is 0-%d\n",
					argv[3], MAXMAJ);
			exit(2);
		}
		minno = (minor_t)number(argv[4]);
 		if (minno == (minor_t)-1) {
 			pfmt(stderr,MM_ERROR,
				":3:invalid minor number '%s' \n",
					argv[4]);
			exit(2);
		}
		arg = makedev(majno, minno);
	}
	else
		usage();

	exit(domk(argv[1], mode|ACC, arg) ? 2 : 0);
}

/*
*Procedure:     domk
*
* Restrictions:
*                mknod(2):none
                 perror:none
                 chown(2):none
*/
int
domk(path, mode, arg)
register char  *path;
mode_t	mode;
dev_t	arg;
{
	int ec;
	if ((ec = mknod(path, mode, arg)) == -1)
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"%s\n",strerror(errno));
	else {		/* chown() return deliberately ignored */
		chown(path, getuid(), getgid());
	}
	return(ec);
}

/*
*Procedure:     number
*
* Restrictions:
*/
long
number(s)
register  char  *s;
{
	register long	n, c;

	n = 0;
	if(*s == '0') {
		while(c = *s++) {
			if(c < '0' || c > '7')
				return(-1);
			n = n * 8 + c - '0';
		}
	} else {
		while(c = *s++) {
			if(c < '0' || c > '9')
				return(-1);
			n = n * 10 + c - '0';
		}
	}
	return(n);
}

/*
*Procedure:     usage
*
* Restrictions:
                 fprintf:none
*/
usage()
{
	pfmt(stderr,MM_ERROR,
		":4:Usage: mknod name b | c major minor\n");
	pfmt(stderr,MM_ERROR,
		":5:Usage: mknod name p\n");
	exit(2);
}

