#ident	"@(#)env:env.c	1.5.3.7"
#ident "$Header$"
/*
 *	env [ - ] [ name=value ]... [command arg...]
 *	env [ -i ] [ name=value ]... [command arg...]
 *	set environment, then execute command (or print environment)
 *	- & -i say start fresh, otherwise merge with inherited environment
 */
#include <stdio.h>
#include <ctype.h>							/*UP*/
#include <pfmt.h>							/*UP*/
#include <locale.h>							/*UP*/
#include <errno.h>

#define NENV	250
char	**newenv = (char **)NULL;
char	*nullp = NULL;

extern	char **environ;
extern	errno;
extern	char *sys_errlist[];
char	*nvmatch(), *strchr();
void	exit();

main(argc, argv, envp)
register char **argv, **envp;
{

	(void)setlocale(LC_ALL,"");					/*UP*/
	(void)setcat("uxue");						/*UP*/
	(void)setlabel("UX:env");					/*UP*/

	argc--;
	argv++;
	if (argc && ((strcmp(*argv, "-") == 0) || (strcmp(*argv, "-i") == 0))) {
		envp = &nullp;
		argc--;
		argv++;
	}

        if (argc && strcmp(*argv,"--") == 0) {
               argc--;
               argv++;
        }

	for (; *envp != NULL; envp++)
		if (strchr(*envp, '=') != NULL)
			addname(*envp);
	while (*argv != NULL && strchr(*argv, '=') != NULL)
		addname(*argv++);

	if (*argv == NULL)
		print(0); /* doesn't return */
	else {
		environ = newenv;
		(void) execvp(*argv, argv);
		(void) pfmt(stderr, MM_ERROR, ":6:%s: %s\n",
				strerror(errno), *argv);

		if (errno == ENOENT)
			exit (127);
		else
			exit (126);
	}
}

addname(arg)
register char *arg;
{
	register char **p;
	static nenv = NENV;

	if (newenv == NULL) {
		newenv = (char **)malloc(nenv * sizeof(char *));
		if (newenv == NULL) {
			(void) pfmt(stderr, MM_ERROR, ":319:malloc failed: %s\n", 
					strerror(errno));
			exit (1);
		}
		(void)memset(newenv, 0, (nenv * sizeof(char *)));
	}

	for (p = newenv; ; p++) {
		if (p >= &newenv[nenv-1]) {
			nenv *= 1.5;
			newenv = (char **)realloc(newenv, (nenv * sizeof(char *)));
			if (newenv == NULL) {
			(void) pfmt(stderr, MM_ERROR, ":320:realloc failed: %s\n", 
					strerror(errno));
				exit (1);
			}
			(void)memset(p, 0, ((&newenv[nenv] - p) * sizeof(char *)));
			break;
		}

		if (*p == NULL)
			break;

		if (nvmatch(arg, *p) != NULL)
			break;
	}
	*p = arg;
}

print(code)
{
	register char **p = newenv;

	while (*p != NULL)
		(void) puts(*p++);
	exit(code);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 */

char *
nvmatch(s1, s2)
register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++ == '=')
			return(s2);
	if (*s1 == '\0' && *(s2-1) == '=')
		return(s2);
	return(NULL);
}
