#ident	"@(#)nullptr:nullptr.c	1.1"
#ident	"$Header$"

/*
 * nullptr [ enable | disable ]
 *
 * For the current effective uid, overide the default setting of the
 * kernel's user null-pointer dereference workaround.  With no args,
 * display the current setting.
 */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysi86.h>

#define DISABLE	0
#define ENABLE	1
#define QUERY	2

#define USAGE	"Usage:  nullptr  [ enable | disable ]\n"

void
main(int argc, char *argv[])
{
	int	cmd, rv;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:nullptr");

	if (argc == 1)
		cmd = QUERY;
	else if (argc == 2 && strcmp(argv[1], "enable") == 0)
		cmd = ENABLE;
	else if (argc == 2 && strcmp(argv[1], "disable") == 0)
		cmd = DISABLE;
	else {
		pfmt(stderr, MM_ERROR, ":0:Syntax error\n");
		pfmt(stderr, MM_ACTION, ":0:" USAGE);
		exit(1);
	}

	rv = sysi86(SI86NULLPTR, cmd);
	if (rv == -1) {
		if (errno == EINVAL)
			pfmt(stderr, MM_ERROR, ":0:System does not support "
			     "enabling null pointer dereferences\n");
		else
			pfmt(stderr, MM_ERROR,
			     ":0:SI86NULLPTR system call error: %s\n",
			     strerror(errno));
		exit(1);
	}

	if (cmd == QUERY) {
		pfmt(stdout, MM_NOSTD,
		     (rv ? ":0:Null pointer dereferences are currently "
			   "allowed\n"
			 : ":0:Null pointer dereferences are currently "
			   "disallowed\n"));
	}
	exit(0);
}
