#ident	"@(#)setkey.c	1.2"
/*
 *	Copyright (C) Microsoft Corporation, 1984
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Inc. and Microsoft Corporation
 *	and should be treated as Confidential.
 *
 */

/***	setkey -- set the function key definitions on a Salmon console
 *
 *	setkey  number definition
 *		- or -
 *	setkey  number -d definition
 *
 */
/*
 *	MODIFICATION HISTORY
 *	M000	21 May 84	pfb
 *	- Added check for stdout=/dev/console, cleaned up error messages
 *	M001	09 May 85	lees
 *	- Added support for new keyboard.
 *	M002	19 Aug 85	buckm
 *	- Quick kludge to allow stdout to be any MultiScreen.
 *	- SCO limit is currently 60 keys.
 *	M003	21 Aug 85	buckm
 *	- Added backslash escape processing.
 *	M004	15 Sep 86	rr
 *	- Need to include sysmacros.h since types.h doesn't define major
 *	M005	11 Dec 86	chapman
 *	- removed device checking.
 *	M006	20 Dec 86	djv
 *	- SCO limit is now 96 keys!
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysmacros.h>	/* M004 */
#include <sys/stat.h>
#include <sys/inode.h>
#define DEFLEN	30
#define PREFIX	"\033Q"

char keystr[5];
main(argc, argv)
int	argc;
char	**argv;
{
	int	building_dec, j, key;
	char	*s;

					/* M001 */
	building_dec = 0;
	if (argc == 4)
	{
		/* looking for "setkey <key #> -d <3-number definition>" */
		if ( (*argv[2] == '-') && (*(argv[2] + 1) == 'd') )
		{
			building_dec = 1;
		}
		else
		{
			fprintf(stderr, "usage: %s <key #> -d <3-number definition>\n", argv[0]);
			exit(1);
		}
	}
	else
	if ( argc != 3 ) {
		fprintf(stderr, "usage: %s <key #> [-d] <definition>\n", argv[0]);
		exit(1);
	}
#ifdef NEVER
	if ( notcons() ) {
/** fprintf(stderr, "%s: stdout must be /dev/console\n", argv[0]); **/
/* M002 */	fprintf(stderr, "%s: stdout must be a MultiScreen\n", argv[0]);
		exit(1);
	}
#endif

	key = atoi(argv[1]);
	if ( key < 1 || key > 96 ) { /* M006 */
		fprintf(stderr, "%s: key number must be between 1 and 96\n",
							argv[0]); /* M006 */
		exit(1);
	}
		/* M001 */
	if ( (!building_dec) && strlen(argv[2]) > DEFLEN) {
		fprintf(stderr, "%s: definition must be shorter than %d characters\n",
				argv[0], DEFLEN + 1);
		exit(1);
	}
	sprintf(keystr,"%d",key-1);
					/* M001 */
	if (building_dec)
		/* setkey with "-d" option */
		decdef(keystr, argv[3]);
	else
		/* setkey without "-d" option */
		def(keystr, argv[2]);

	exit(0);
}

def(key, s)
char	*key;
char	*s;
{
	char quote;
	register int c;					/* M003 */

	quote = getquote(s);
				 /* M001 */
	printf("%s%s%c", PREFIX, key, quote);
	while ( (c = *s++) != '\0' ) {			/* M003 { */
		if ( c == '\\' )
			c = getesc(&s);
		if ( c < ' ' )
			printf("^%c", c + ' ');
		else if ( c == '^' )
			printf("^^");
		else
			printf("%c", c);
	}						/* M003 } */
	printf("%c", quote);
}

	/* M001 */
decdef(key, s)
char	*key;
char	*s;
{
	int	n;
	printf("%s%s~", PREFIX, key);
	n = atoi(s);
	printf("%c~", (char) (n & 0x00ff));
}

getquote(s)
char *s;
{
	char quote;

	for ( quote = '~'; quote > 0x00; quote -= 1)
		if ( notin(quote, s) )
			return quote;
}

notin(c, s)
char c;
char *s;
{
	for ( ; *s != '\0'; s++)
		if ( *s == c )
			return 0;
	return 1;
}

	/* M003 */
getesc(sp)
char **sp;
{
	char *s;
	register int c, nc;

	s = *sp;
	c = *s++;

	if ( c >= '0' && c <= '7' ) {
		c -= '0';
		nc = *s - '0';
		if ( nc >= 0 && nc <= 7 ) {
			c = (c << 3) | nc;
			nc = *++s - '0';
			if ( nc >= 0 && nc <= 7 ) {
				c = (c << 3) | nc;
				s++;
			}
		}
	} else switch ( c ) {
		case '\0':	s--;		break;
		case 'b':	c = '\b';	break;
		case 'f':	c = '\f';	break;
		case 'n':	c = '\n';	break;
		case 'r':	c = '\r';	break;
		case 't':	c = '\t';	break;
	}

	*sp = s;
	return c;
}

#ifdef NEVER
notcons()
{
	struct stat stats, statc;

	if ( (fstat(0, &stats) < 0) || ( stat("/dev/console", &statc) < 0 ))
		return 1;
	if ( (stats.st_mode & IFMT) != IFCHR )
		return 1;
	/** return stats.st_rdev != statc.st_rdev; **/
	return major(stats.st_rdev) != major(statc.st_rdev); /* M002 */
}
#endif
