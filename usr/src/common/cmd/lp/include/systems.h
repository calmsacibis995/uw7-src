/*		copyright	"%c%" 	*/


#ident	"@(#)systems.h	1.2"
#ident "$Header$"

# define	SYS_PASSWD	0
# define	SYS_PROTO	1
# define	SYS_TMO		2
# define	SYS_RETRY	3
# define	SYS_COMMENT	4
# define	SYS_MAX		5

/**
 ** The internal copy of a system as seen by the rest of the world:
 **/

typedef struct SYSTEM
{
    char	*name;		/* name of system (redundant) */
    char	*passwd;        /* the encrypted passwd of the system */
    char	*reserved1;
    int		protocol;	/* lp networking protocol s5|bsd */
    char	*reserved2;	/* system address on provider */
    int		timeout;	/* maximum permitted idle time */
    int		retry;		/* minutes before trying failed conn */
    char	*reserved3;
    char	*reserved4;
    char	*comment;
} SYSTEM;

#define	DEFAULT_TIMEOUT	10
#define	DEFAULT_RETRY	2

# define	NAME_S5PROTO	"s5"
# define	NAME_BSDPROTO	"bsd"
# define	NAME_NUCPROTO	"nuc"

# define	S5_PROTO	1
# define	BSD_PROTO	2
# define	NUC_PROTO	3

/**
 ** Various routines.
 **/

#if	defined(__STDC__)

SYSTEM		*getsystem ( const char * );

int		putsystem ( const char *, const SYSTEM * ),
		delsystem ( const char * );
void		freesystem( SYSTEM * );

#else

SYSTEM		*getsystem();

int		putsystem(),
		delsystem(),
		freesystem();

#endif
