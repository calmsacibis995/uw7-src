#ident	"@(#)checkwhite.c	15.1"

#include <stdio.h>

#define TAB 9
#define NL 10
#define SP 32
#define BSL 92

int
main(int argc, char *argv[])
{

register int ch, previous, two_previous;

int single=0;
int db=0;
int back=0;
int inline=0;
int incontline=0;
int error=0;
int comment=0;
int verbose=0;
int retcode=0;

	if ( argc == 2 && strcmp(argv[1],"-x") == 0 )
		++verbose;

	previous=0;
	two_previous=0;

	while ( (ch=getchar() ) != EOF ) {
		if ( ch == NL ) {
			comment=0; /* cannot be in comment after nl. */
			if ( previous == BSL )
				switch (two_previous) {
				case TAB:
				case SP:
					incontline=0;
					break;
				default:
					incontline=1;
					break;
				}
			else
				incontline=0;
		}

		if ( ch == '<' && previous == '<' && !single && !back && !db && !comment )
		/* We've detected a here document. */
			retcode=1;

		/* if at ws and previous was NL and I'm in a string, then ERROR */

		if ( (ch == NL || ch == SP || ch == TAB) && previous == NL ) {
			if ( back || db || single || incontline ) {
				++error;
				if ( verbose )
					printf("ERROR '=%d, \"=%d, `=%d",single,db,back);
			}
		}

		/* start comment */
		if ( ch == '#' && !single && !back && !db )
			++comment;

		/* start or finish a " ' " string */
		if ( ch == '\'' && previous != BSL && !back && !db && !comment )
			if ( single )
				--single;
			else
				++single;

		/* start or finish a ' " ' string */
		if ( ch == '"' && previous != BSL && !single && !back && !comment)
			if ( db )
				--db;
			else
				++db;

		/* start or finish a " ' " string */
		if ( ch == '`' && previous != BSL && !single && !db && !comment )
			if ( back )
				--back;
			else
				++back;

		if ( verbose )
			putchar(ch);
		two_previous = previous;
		previous = ch;
	}

	if ( verbose )
		fprintf(stderr,"'=%d, \"=%d, `=%d, error=%d\n",single,db,back,error);

	if ( single || db || back || error )
		retcode=2;

	exit(retcode);
}
