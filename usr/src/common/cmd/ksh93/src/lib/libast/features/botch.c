#ident	"@(#)ksh93:src/lib/libast/features/botch.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * generate ast traps for botched standard prototypes
 */

#include <sys/types.h>

#include "FEATURE/types"
#include <ast_lib.h>

extern int		getgroups(int, gid_t*);
extern int		printf(const char*, ...);

main()
{
#if _lib_getgroups
	if (sizeof(gid_t) < sizeof(int))
	{
		register int	n;
		gid_t		groups[32 * sizeof(int) / sizeof(gid_t)];

		for (n = 0; n < sizeof(int) / sizeof(gid_t); n++)
			groups[n] = ((gid_t)0);
		if ((n = getgroups((sizeof(groups) / sizeof(groups[0])) / (sizeof(int) * sizeof(int)), groups)) > 0)
		{
			if (groups[1] != ((gid_t)0)) n = 0;
			else
			{
				for (n = 0; n < sizeof(int) / sizeof(gid_t); n++)
					groups[n] = ((gid_t)-1);
				if ((n = getgroups((sizeof(groups) / sizeof(groups[0])) / (sizeof(int) * sizeof(int)), groups)) > 0)
				{
					if (groups[1] != ((gid_t)-1)) n = 0;
				}
			}
		}
		if (n <= 0)
		{
			printf("#undef	getgroups\n");
			printf("#define getgroups	_ast_getgroups /* implementation botches gid_t* arg */\n");
		}
	}
#endif
	return(0);
}
