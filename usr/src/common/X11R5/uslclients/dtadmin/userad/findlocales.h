/*		copyright	"%c%" 	*/

#ident	"@(#)dtadmin:userad/findlocales.h	1.1"

#include "errno.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "dirent.h"


/**
 ** Misc. Macros
 **/

#define	STREQU(A,B)	( (!(A) || !(B)) ? 0: (strcmp((A), (B)) == 0) )
#define	STRNEQU(A,B,N)	( (!(A) || !(B)) ? 0: (strncmp((A), (B), (N)) == 0) )

extern int	errno;

extern char    *getenv();
char	LocalePath[BUFSIZ];

/*
 * File access:
 */

#if	defined(__STDC__)
char *		FindLocaleFile ( char * locale, char * type );
char **		FindLocales ( void );

#else

char		*FindLocaleFile(),
		**FindLocales();

#endif

/*
 * List manipulation routines:
 */

#if	defined(__STDC__)

int		appendlist ( char *** , char * );
int		lenlist ( char ** );
void		freelist ( char ** );
#else

int		appendlist(),
		lenlist();
void		freelist();

#endif	/* __STDC__ */

/*
 * File name routines:
 */

#if	defined(__STDC__)
char *		makepath ( char * , ... );
#else
char		*makepath();
#endif

/*
 * Additional string manipulation routines:
 */

#if	defined(__STDC__)

int		cs_strcmp ( char * , char * );
int		cs_strncmp ( char * , char * , int );
#else
int		cs_strcmp(),
		cs_strncmp();
#endif

#define	next_dir(base, ptr)	next_x(base, ptr, S_IFDIR)
#define	next_file(base, ptr)	next_x(base, ptr, S_IFREG)

#if	defined(__STDC__)
char *		next_x  ( char * , long * , unsigned int );
#else
char *		next_x();
#endif
