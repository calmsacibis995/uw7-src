#ident	"@(#)OSRcmds:ksh/shlib/gsort.c	1.1"
#pragma comment(exestr, "@(#) gsort.c 25.2 92/12/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 *  gsort - sort an array of pointers to data
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 *  Derived from Bourne Shell
 */

/*
 * sort the array of strings argv with n elements
 */

void	gsort(argv,n, fn)
char *argv[];
int (*fn)();
{
	register int 	i, j, m;
	int  k;
	for(j=1; j<=n; j*=2);
	for(m=2*j-1; m/=2;)
	{
		k=n-m;
		for(j=0; j<k; j++)
		{
			for(i=j; i>=0; i-=m)
			{
				register char **ap;
				ap = &argv[i];
				if((*fn)(ap[m],ap[0])>0)
					break;
				else
				{
					char *s;
					s=ap[m];
					ap[m]=ap[0];
					ap[0]=s;
				}
			}
		}
	}
}

