#ident	"@(#)vtautofocus.c	1.2"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 *	Created	5Feb97		rodneyh@sco.com
 *	- Utility used to control the state of the kd_no_activate kernel
 *	  variable. kd_no_activate == B_TRUE prevents VT being auto activated
 *	  on open
 *
 */ 

#include <stdio.h>
#include <unistd.h>			/* For getopt */

#include <sys/types.h>
#include <sys/kd.h>			/* For KDNOAUTOFOCUS ioctl */
#include <sys/stat.h>			/* For open */
#include <fcntl.h>			/* For open */

void usage()
{

	fprintf(stderr, "UX: vtautofocus: TO FIX: Usage: "
				"vtautofocus -y | -n\n");


}	/* End function usage */

extern int errno;

main(int argc, char *argv[])
{
int option, fd;
boolean_t error = B_FALSE, yes = B_FALSE, no = B_FALSE;


	while((option = getopt(argc, argv, "yn")) != EOF){

		switch(option){

		case 'y':
			/*
			 * Enable auto focus on open
			 */
			yes = B_TRUE;

			break;

		case 'n':
			/*
			 * Disable auto focus on open
			 */
			no = B_TRUE;

			break;
		default:

			error = B_TRUE;
			break;

		}
	}	/* End while getopt */

	if((yes && no) || (!yes && !no) || error || argc == 1){

		usage();
		exit(1);

	}	/* End if option error */


	if((fd = open("/dev/tty", O_RDWR)) == -1){

		perror("vtautofocus: unable to open /dev/tty");
		exit(1);
	}

	if(ioctl(fd, KDNOAUTOFOCUS, (yes == B_TRUE) ? 0 : 1) == -1){


		perror("vtautofocus: KDNOAUTOFOCUS failed");

		close(fd);
		exit(errno);

	}

	close(fd);


}	/* End function main */

/* End file vtautofocus.c */

