/*		copyright	"%c%" 	*/

# ident	"@(#)global.c	1.3"
# ident  "$Header$"

# include <stdio.h>
# include <sac.h>
# include <sys/types.h>
# include "misc.h"
# include "structs.h"

int	Stime;			/* sanity timer interval */
struct	sactab	*Sactab;	/* linked list head of PM info */
char	Scratch[SIZE];		/* general scratch buffer */
int	Sfd;			/* _sacpipe file descriptor */
int	Cfd;			/* command pipe file descriptor */
int	Nentries;		/* # of entries in internal version of _sactab */
