/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/mcce.c	1.1"

#include <stdio.h>

#include <locale.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "colldata.h"
#include "_locale.h"
#include "_regexp.h"

extern const xnd __colldata[];
static const xnd *colltbl = __colldata;
static subnd *subtbl;
static long sub_cnt;
extern char	__mcce_l_name[LC_NAMELEN];

static unsigned char _bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };


int
__mcce(ep, rp, neg, mcce_f)
register char *ep;
register unsigned char **rp;
int neg;
int *mcce_f;
{
	register int i;		/*  Loop variable  */
	int num_mcce;		/*  Number of MCCE's  */
	int n;
	
	register unsigned char *lp;	/*  Local pointer  */
	char res[2];		/*  Collation value of MCCE  */

	lp = *rp;

	if (__is_mcce(lp, res) == 1)
	{
		num_mcce = (*(ep + 32) - 1) / 2;

		for (i = 0; i < num_mcce; i++)
		{
			if (res[0] == *(ep + 33 + (i * 2)) &&
			    (res[1] == *(ep + 34 + (i * 2)) ||
			     *(ep + 34 + (i * 2)) == -1))
			{
				if (neg)
				{
					*mcce_f = 1;
					return(0);
				}
				*rp += 2;
				return(1); 
			}
		}

		if (neg)
		{
			*rp += 2;
			return(1);
		}
	}

	if (ISTHERE(*lp))	/*  Test for single-character in range  */
	{
		if (neg)
			return(0);
		(*rp)++;
		return(1);
	}

	/*  If we failed to match this character, we then have to check
	 *  whether neg is switched on.  We also have a nasty little kludge
	 *  here so that a NULL does not get matched in this situation.
	 *  Took a while to spot this - look at the code for reg_compile.c,
	 *  When they have inverted the bit map, they & the first character
	 *  with 0376 to reset NULL - very yukky, and definitely no comment,
	 *  unlike my code.
	 */
	if (neg && *lp != 0)
	{
		(*rp)++;
		return(1);
	}
	return(0);
}




int
__is_mcce(c, conv)
unsigned char *c;
char *conv;
{
	register int i;

	for (i = 0; i < (int)colltbl[*c].ch; i++)
	{ 
		if (colltbl[256 + colltbl[*c].ns + i].ch == *(c+1))
		{
			if (conv != NULL)
			{
				*conv = (char)colltbl[256 + colltbl[*c].ns + i].pwt;
				*(conv + 1) = (char)colltbl[256 + colltbl[*c].ns + i].swt;
			}
			return(1);
		}
	}
	return(0);
}

/*  I really don't like this, but it's the only way to get access to the
 *  collation table information.  This code is snarfed from strxfrm.c
 */
void
__mcce_init()
{
	register long	i;
	int		infile;
	char		*substrs;
	struct hd {
		long	coll_offst;
		long	sub_cnt;
		long	sub_offst;
		long	str_offst;
		long	flags;
	} *header;
	void *database;
	struct stat	ibuf;

	if ((infile = open(_fullocale(_cur_locale[LC_COLLATE],"LC_COLLATE"),O_RDONLY)) == -1) 
		return;

	/* get size of file, allocate space for it, and read it in */
	if (fstat(infile, &ibuf) != 0)
		goto err1;
	if ((database = (void *)malloc(ibuf.st_size)) == NULL)
		goto err1;
	if (read(infile, database, ibuf.st_size) != ibuf.st_size)
		goto err2;
	close(infile);

	header = (struct hd *)database;
	colltbl = (xnd *)(void *)((char *)database + header->coll_offst);

	/* set up subtbl */
	if ((sub_cnt = header->sub_cnt) == 0) {
		subtbl = NULL;
	} else {
		subtbl = (subnd *)(void *)((char *)database + header->sub_offst);
		substrs = (char *)database + header->str_offst;

		/* reset subtbl so that exp and repl fields are pointers
		 * instead of offsets */
		for (i = 0; i < sub_cnt; i++) {
			subtbl[i].exp = (int)subtbl[i].exp + substrs;
			subtbl[i].repl = (int)subtbl[i].repl + substrs;
		}
	}
	strcpy(__mcce_l_name, _cur_locale[LC_COLLATE]);
	return;
err2:
	free(database);
err1:
	close(infile);
}
