#ident	"@(#)ksh93:src/lib/libast/features/int.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * generate integral type size features
 */

#include "FEATURE/types"

#define elementsof(x)	(sizeof(x)/sizeof(x[0]))

extern int		printf(const char*, ...);

static char		i_char = 1;
static short		i_short = 1;
static int		i_int = 1;
static long		i_long = 1;
#if _typ_long_long
static long long	i_long_long = 1;
#endif

static struct
{
	char*	name;
	int	size;
	char*	swap;
} type[] = 
{
	"char",		sizeof(char),		(char*)&i_char,
	"short",	sizeof(short),		(char*)&i_short,
	"int",		sizeof(int),		(char*)&i_int,
	"long",		sizeof(long),		(char*)&i_long,
#if _typ_long_long
	"long long",	sizeof(long long),	(char*)&i_long_long,
#endif
};

static int	size[] = { 1, 2, 4, 8 };

main()
{
	register int	t;
	register int	s;
	register int	m = 1;

	for (s = 0; s < elementsof(size); s++)
	{
		for (t = 0; t < elementsof(type) && type[t].size < size[s]; t++);
		if (t < elementsof(type))
		{
			m = size[s];
			printf("#ifndef int_%d\n", m);
			printf("#define int_%d		%s\n", m, type[t].name);
			printf("#endif\n");
		}
	}
	printf("#ifndef	int_max\n");
	printf("#define int_max		int_%d\n", m);
	printf("#endif\n");
	for (t = 0; t < elementsof(type) - 1 && *type[t + 1].swap; t++);
	printf("#ifndef	int_swap\n");
	printf("#define int_swap	%d\n", t ? type[t].size : 0);
	printf("#endif\n");
	return(0);
}
