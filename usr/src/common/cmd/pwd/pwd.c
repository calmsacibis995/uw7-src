/*		copyright	"%c%" 	*/

#ident	"@(#)pwd:pwd.c	1.14.1.4"
#ident "$Header$"
/*
**	Print working (current) directory
*/

#include	<stdio.h>
#include	<unistd.h>
#include	<limits.h>
#include	<locale.h>
#include	<pfmt.h>

char	name[PATH_MAX+1];

main()
{
	int length;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:pwd");

	if (getcwd(name, PATH_MAX + 1) == (char *)0) {
		pfmt(stderr, MM_ERROR, ":129:Cannot determine current directory");
		putc('\n', stderr);
		exit(2);
	}
	length = strlen(name);
	name[length] = '\n';
	write(1, name, length + 1);
	exit(0);
}
