#ident	"@(#)spell:spellin.c	1.4.1.4"
/*		copyright	"%c%" 	*/

#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include "hash.h"

#define S (BYTE*sizeof(long))
#define B (BYTE*sizeof(unsigned))
unsigned *table;
int index[NI];
unsigned wp;		/* word pointer*/
int bp =B;	/* bit pointer*/
int ignore;
int extra;

/*	usage: hashin N
	where N is number of words in dictionary
	and standard input contains sorted, unique
	hashed words in octal
*/
main(argc,argv)
char **argv;
{
	long h,k,d;
	register i;
	long count;
	long w;
	long x;
	int t,u;
	extern float huff();
	double atof();
	double z;
	extern char *malloc();
	k = 0;
	u = 0;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxspell");
	(void)setlabel("UX:spellin");

	if(argc!=2) {
		pfmt(stderr, MM_ERROR, ":9:Incorrect arg count\n");
		pfmt(stderr, MM_ACTION, ":10:Usage: %s N\n", argv[0]);
		exit(1);
	}
	table = (unsigned*)malloc(ND*sizeof(*table));
	if(table==0) {
		pfmt(stderr, MM_ERROR, ":11:No space for table\n");
		exit(1);
	}
	z = huff((1L<<HASHWIDTH)/atof(argv[1]));
	pfmt(stderr, MM_INFO, ":12:expected code widths = %f\n", z);
	for(count=0; scanf("%lo", &h) == 1; ++count) {
		if((t=h>>(HASHWIDTH-INDEXWIDTH)) != u) {
			if(bp!=B)
				wp++;
			bp = B;
			while(u<t)
				index[++u] = wp;
			k =  (long)t<<(HASHWIDTH-INDEXWIDTH);
		}
		d = h-k;
		k = h;
		for(;;) {
			for(x=d;;x/=2) {
				i = encode(x,&w);
				if(i>0)
					break;
			}
			if(i>B) {
				if(!(
				   append((unsigned)(w>>(i-(int)B)), B)&&
				   append((unsigned)(w<<(B+B-i)), i-B)))
					ignore++;
			} else
				if(!append((unsigned)(w<<(B-i)), i))
					ignore++;
			d -= x;
			if(d>0)
				extra++;
			else
				break;
		}
	}
	if(bp!=B)
		wp++;
	while(++u<NI)
		index[u] = wp;
	whuff();
	fwrite((char*)index, sizeof(*index), NI, stdout);
	fwrite((char*)table, sizeof(*table), wp, stdout);
	pfmt(stderr, MM_INFO,
		":13:%ld items, %d ignored, %d extra, %u words occupied\n",
		count,ignore,extra,wp);
	count -= ignore;
	pfmt(stderr, MM_INFO, ":14:%f table bits/item, ", 
		((float)BYTE*wp)*sizeof(*table)/count);
	pfmt(stderr, MM_NOSTD, ":15:%f table+index bits\n",
		BYTE*((float)wp*sizeof(*table) + sizeof(index))/count);
	return(0);
}

append(w, i)
register unsigned w;
register i;
{
	while(wp<ND-1) {
		table[wp] |= w>>(B-bp);
		i -= bp;
		if(i<0) {
			bp = -i;
			return(1);
		}
		w <<= bp;
		bp = B;
		wp++;
	}
	return(0);
}
