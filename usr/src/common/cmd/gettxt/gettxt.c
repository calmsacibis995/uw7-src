/*		copyright	"%c%" 	*/

#ident	"@(#)gettxt:gettxt.c	1.4.8.1"
#ident "$Header$"

#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	char *dfltp, *str;

	if ((str = setlocale(LC_ALL, "")) == 0) {
		(void)setlocale(LC_CTYPE, "");
		(void)setlocale(LC_MESSAGES, "");
	}

	if (argc != 2 && argc != 3) {
		(void)setlabel("UX:gettxt");
		(void)setcat("uxcore.abi");
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION,
			":316:Usage: gettxt msgid [ dflt_msg ]\n");
		return 1;
	}

	dfltp = "";
	if (argc == 3)
		dfltp = argv[2];
	
	(void)setcat("");
	str = gettxt(argv[1], dfltp);
	if (strcmp(str, "Message not found!!\n") == 0 && dfltp[0] != '\0')
		str = dfltp;
	
	fputs(str, stdout);
	return 0;
}
