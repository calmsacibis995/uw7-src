/*		copyright	"%c%" 	*/

#ident	"@(#)logname:logname.c	1.4.3.1"

#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
main() {
	char *name;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue.abi");
	(void)setlabel("UX:logname");

	name = cuserid((char *)NULL);
	if (name == NULL) {
		pfmt(stderr, MM_ERROR, ":5:Cannot get login name\n");
		return (1);
	}
	(void) puts (name);
	return (0);
}
