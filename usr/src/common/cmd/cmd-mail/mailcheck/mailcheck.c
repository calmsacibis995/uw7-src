#ident	"@(#)mailcheck.c	11.6"

#include <stdio.h>
#include <sys/stat.h>
#include <pfmt.h>
#include <locale.h>

/*
 * mailcheck.c, UnixWare compatible mailcheck that uses c-client to
 * find INBOX.
 */

main()
{
	struct stat statb;
	int ret;

	extern void msc1_init(int);
	extern char *conf_inbox;

	setcat("uxemail");
	setlabel("UX:mailcheck");
	setlocale(LC_ALL, "");

	msc1_init(0);
	statb.st_size = 0;
	ret = stat(conf_inbox, &statb);
	if ((ret == 0) || (statb.st_size == 0)) {
		pfmt(stderr, MM_NOSTD, ":141:No mail.\n");
		exit(1);
	}
	else {
		pfmt(stdout, MM_NOSTD, ":400:You have mail\n");
		exit(0);
	}
}
