#ident	"@(#)sccs:cmd/what.c	6.12.1.1"
# include	"stdio.h"
# include	"sys/types.h"
# include	"macros.h"
# include	<pfmt.h>
# include	<locale.h>
# include	<sys/euc.h>
# include	<limits.h>

#define MINUS '-'
#define MINUS_S "-s"
#define TRUE  1
#define FALSE 0


static int found = FALSE;
static int silent = FALSE;

static char	pattern[]  =  "@(#)";

static void	dowhat();
static int	trypat();
int	strcmp(), exit(), any();


main(argc,argv)
int argc;
register char **argv;
{
	register int i;
	register FILE *iop;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxepu");
	(void)setlabel("UX:what");

	if (argc < 2)
		dowhat(stdin);
	else
		for (i = 1; i < argc; i++) {
			if(!strcmp(argv[i],MINUS_S)) {
				silent = TRUE;
				continue;
			}
			if ((iop = fopen(argv[i],"r")) == NULL)
				pfmt(stderr,MM_ERROR,
					":184:can't open %s (26)\n",argv[i]);

			else {
				printf("%s:\n",argv[i]);
				dowhat(iop);
			}
		}
	exit(!found);				/* shell return code */
}


static void
dowhat(iop)
register FILE *iop;
{
	register int c;

	while ((c = getc(iop)) != EOF) {
		if (c == pattern[0])
			if(trypat(iop, &pattern[1]) && silent) break;
	}
	fclose(iop);
}


static int
trypat(iop,pat)
register FILE *iop;
register char *pat;
{
	register int c;

	for (; *pat; pat++)
		if ((c = getc(iop)) != *pat)
			break;
	if (!*pat) {
		found = TRUE;
		putchar('\t');
		while ((c = getc(iop)) != EOF && c && !any(c,"\"\\>\n"))
			putchar(c);
		putchar('\n');
		if(silent)
			return(TRUE);
	}
	else if (c != EOF)
		ungetc(c, iop);
	return(FALSE);
}
