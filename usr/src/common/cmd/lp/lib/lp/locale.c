
/*		copyright	"%c%" 	*/


#ident	"@(#)locale.c	1.2"
#ident	"$Header$"

#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "sys/types.h"

#include "lp.h"

/**
 ** get_charset() - READ CHARSET FROM TO DISK
 **/

char **
#if	defined(__STDC__)
get_charset (
	char *			name
)
#else
get_charset (name)
	char			*name;
#endif
{
	static long		lastdir		= -1;


	char			*file,
				buf[BUFSIZ];
	char			**charsetlist = 0;

	FILE			*fp;


	if (!name || !*name) {
		errno = EINVAL;
		return (0);
	}

	/*
	 * Getting ``all''? If so, jump into the directory
	 * wherever we left off.
	 */
	if (STREQU(NAME_ALL, name)) {
		/* CONSTCOND */
		if (!Lp_Locale) {
			return (0);
		}
		if (!(name = next_file(Lp_Locale, &lastdir)))
			return (0);
	} else
		lastdir = -1;

	/*
	 * Get the charset list.
	 */

	if (!(file = getlocalefile(name,CHARSETFILE)))
		return (0);

	if (!(fp = open_lpfile(file, "r", 0))) {
		if (errno == ENOENT)
			errno = ENODATA;
		Free (file);
		return (0);
	}
	Free (file);

	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = 0;
		addlist (&charsetlist, buf);
	}
	if (ferror(fp)) {
		int			save_errno = errno;

		freelist (charsetlist);
		close_lpfile (fp);
		errno = save_errno;
		return (0);
	}
	close_lpfile (fp);

	return (charsetlist);
}

#if	defined(__STDC__)
unsigned int		chk_locale ( char * locale );
#else
unsigned int		chk_locale();
#endif


unsigned int
#if	defined(__STDC__)
chk_locale (
	char *			locale
)
#else
chk_locale (locale)
	char			*locale;
#endif
{
	char			*parent,
				*path;

	struct stat		statbuf;

	if (!(parent = getlocaledir()))
		return (0);
	if (!(path = makepath(parent, locale, (char *)0)))
		return (0);
	if (Stat(path, &statbuf) == -1) {
		Free (path);
		return (0);
	}
	Free (path);
	return (S_ISDIR(statbuf.st_mode));
}

char *
#if	defined(__STDC__)
getlocaledir (
	void
)
#else
getlocaledir ()
#endif
{
	return (Lp_Locale);
}

char **
#if	defined(__STDC__)
get_locales (
	void
)
#else
get_locales ()
#endif
{
	static long		lastdir	= -1;


	static char		**lcl_list = 0;

	char			*name = 0;


		if (!Lp_Locale) 
			return (0);
		
		while (name = next_dir(Lp_Locale, &lastdir))
			appendlist ( &lcl_list, name );

		return (lcl_list);

}
