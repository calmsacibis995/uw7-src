/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/utmp2wtmp.c	1.2.5.3"
#ident "$Header$"
/*
 *	create entries for users who are still logged on when accounting
 *	is being run. Look at utmp, and update the time stamp. New info
 *	goes to wtmp. Call by runacct. 
 */

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>

long time();

main(argc, argv)
int	argc;
char	**argv;
{
	struct utmp *getutent(), *utmp;
	FILE *fp;

	if ((fp = fopen(WTMP_FILE, "a+")) == NULL) {
		fprintf(stderr, "%s: cannot open %s for writing\n",
			argv[0], WTMP_FILE);
		exit(2);
	}
	while ((utmp=getutent()) != NULL) {
		if (utmp->ut_type == USER_PROCESS) {
			time( &utmp->ut_time );
			fwrite( utmp, sizeof(*utmp), 1, fp);
		}
	}
	fclose(fp);
}
