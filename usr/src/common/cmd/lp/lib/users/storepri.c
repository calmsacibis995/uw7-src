/*		copyright	"%c%" 	*/

#ident	"@(#)storepri.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

# include	<stdio.h>

# include	"lp.h"
# include	"users.h"

#define WHO_AM_I        I_AM_LPUSERS
#include "oam.h"

/*
Inputs:
Outputs:
Effects:
*/
void
#if	defined(__STDC__)
print_tbl ( struct user_priority * ppri_tbl )
#else
print_tbl (ppri_tbl)
    struct user_priority	*ppri_tbl;
#endif
{
    int limit;

    LP_OUTMSG2(MM_NOSTD, E_LPU_DEFPRI, ppri_tbl->deflt, ppri_tbl->deflt_limit);
    printlist_setup ("", "", ",", "\n");
    for (limit = PRI_MIN; limit <= PRI_MAX; limit++) {
	if (ppri_tbl->users[limit - PRI_MIN])
	{
	    printf("   %2d     ", limit);
	    printlist(stdout, ppri_tbl->users[limit - PRI_MIN]);
	}
    }
}

/*
Inputs:
Outputs:
Effects:
*/
#if	defined(__STDC__)
void output_tbl ( FILE * f, struct user_priority * ppri_tbl )
#else
void output_tbl ( f, ppri_tbl )
FILE * f;
struct user_priority * ppri_tbl;
#endif
{
    int		limit;

    fprintf(f, "%d\n%d:\n", ppri_tbl->deflt, ppri_tbl->deflt_limit);
    printlist_setup ("	", "\n", "", "");
    for (limit = PRI_MIN; limit <= PRI_MAX; limit++)
	if (ppri_tbl->users[limit - PRI_MIN])
	{
	    fprintf(f, "%d:", limit);
	    printlist(f, ppri_tbl->users[limit - PRI_MIN]);
	}
}
