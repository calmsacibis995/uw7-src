/*		copyright	"%c%" 	*/

#ident	"@(#)dtadmin:userad/findlocales.c	1.1"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "sys/types.h"

#include "findlocales.h"

/**
 ** FindLocaleFile() - BUILD NAME OF LOCALE FILE
 **/

#if	defined(__STDC__)
char * FindLocaleFile ( char * name, char * component )
#else
char * FindLocaleFile ( name, component )
char	*name;
char	*component;
#endif
{
    char	*path;

    if (!name)
	return (0);

    path = makepath(LocalePath, name, component, NULL);

    return (path);
}


char **
#if	defined(__STDC__)
FindLocales (
	void
)
#else
FindLocales ()
#endif
{
	static long		lastdir	= -1;
	static char		**lcl_list = 0;
	char			*name = 0;
        char                    *xhome;

                xhome = getenv("XWINHOME");
                if (!xhome)
                        xhome = "/usr/X";
                (void)sprintf( LocalePath, "%s/lib/locale", xhome);

		if (!LocalePath) 
			return (0);
		
		while (name = next_dir(LocalePath, &lastdir))
			appendlist ( &lcl_list, name );

		return (lcl_list);

}
