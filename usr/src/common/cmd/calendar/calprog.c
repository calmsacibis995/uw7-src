/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)calendar:calprog.c	1.8.4.2"

/* /usr/lib/calprog produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/


#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <locale.h>
#include <pfmt.h>


#define DAY	(3600*24L)

extern  char	*getenv(), *malloc();
extern	int	errno;

static char	*file;
static int	old_behavior;
static int	linenum = 1;
static long	t;

static
char *month[] = {
	"[Jj]an",
	"[Ff]eb",
	"[Mm]ar",
	"[Aa]pr",
	"[Mm]ay",
	"[Jj]un",
	"[Jj]ul",
	"[Aa]ug",
	"[Ss]ep",
	"[Oo]ct",
	"[Nn]ov",
	"[Dd]ec"
};

static
tprint(t)
long t;
{
	struct tm *tm;
	tm = localtime(&t);
	(void)printf("(^|[ \t(,;])((%s[^ ]* *|0*%d/|\\*/)0*%d)([^0123456789]|$)\n",
		month[tm->tm_mon], tm->tm_mon + 1, tm->tm_mday);
}

main()
{

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue");
	(void)setlabel("UX:calendar");
	(void)time(&t);
	if (((file = getenv("DATEMSK")) == 0) || file[0] == '\0')
		old_behavior = 1;
	if (old_behavior)
		tprint(t);
	else
		read_tmpl();
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		if (old_behavior)
			tprint(t);
		else
			read_tmpl();
	case 6:
		t += DAY;
		if (old_behavior)
			tprint(t);
		else
			read_tmpl();
	default:
		t += DAY;
		if (old_behavior)
			tprint(t);
		else
			read_tmpl();
	}
	exit(0);
}


static
read_tmpl()
{
	char	*clean_line();
	FILE  *fp;
	char *bp, *start;
	struct stat sb;
	int	no_empty = 0;

	if ((start = (char *)malloc(512)) == NULL)
		{
		pfmt(stderr, MM_ERROR,":60:could not malloc enough memory\n" );
		exit(1);
		}
	if ((fp = fopen(file, "r")) == NULL)
		{
		pfmt(stderr, MM_ERROR,":61:error on open of \"%s\", errno = %d\n" , file, errno);
		exit(1);
		}
	if (stat(file, &sb) < 0)
		{
		pfmt(stderr, MM_ERROR,":62:error on stat of \"%s\", errno = %d\n" , file, errno);
		exit(1);
		}
	if ((sb.st_mode & S_IFMT) != S_IFREG)
		{
		pfmt(stderr, MM_ERROR, ":63:file \"%s\" is not a regular file\n", file);
		exit(1);
		}
	for(;;) {
	 	bp = start;
		if (!fgets(bp, 512, fp)) {
			if (!feof(fp)) 
				{
				free(start);
				fclose(fp);
				pfmt(stderr, MM_ERROR, ":64:error in reading the file \"%s\"\n", file);
				exit(1);
				}
			break;
		}
		if (*(bp+strlen(bp)-1) != '\n')   /* terminating newline? */
			{
			free(start);
			fclose(fp);
			pfmt(stderr, MM_ERROR, ":65:\"%s\" file: error on line %d\n", file, linenum);
			exit(1);
			}
		bp = clean_line(bp);
		if (strlen(bp))  /*  anything left?  */
			{
			no_empty++;
			generate(bp);
			}
	linenum++;
	}
	free(start);
	fclose(fp);
	if (!no_empty)
		{
		pfmt(stderr, MM_ERROR, ":66:\"%s\" file: format descriptions are missing\n", file);
		exit(1);
		}
}


char  *
clean_line(s)
char *s;
{
	char  *ns;

	*(s + strlen(s) -1) = (char) 0; /* delete newline */
	if (!strlen(s))
		return(s);
	ns = s + strlen(s) - 1; /* s->start; ns->end */
	while ((ns != s) && (isspace(*ns))) {
		*ns = (char)0;	/* delete terminating spaces */
		--ns;
		}
	while (*s)             /* delete beginning white spaces */
		if (isspace(*s))
			++s;
		else
			break;
	return(s);
}

static
generate(fmt)
char *fmt;
{
	struct tm *tm;
	char      *tb, *ob;
	char      timebuf[1024];
	char      outbuf[2 * 1024];
	int	space = 0;

	tm = localtime(&t);
	(void)strftime(timebuf,sizeof(timebuf),fmt,tm);
	tb = timebuf;
	ob = outbuf;
	while(*tb)
		if (isspace(*tb))
			{
			++tb;
			space++;
			}
		else
			{
			if (space)
				{
				*ob++ = '[';
				*ob++ = ' ';
				*ob++ = '\t';
				*ob++ = ']';
				*ob++ = '*';
				space = 0;
				continue;
				}
			if(isalpha(*tb))
				{
				*ob++ = '[';
				*ob++ = toupper(*tb);
				*ob++ = tolower(*tb++);
				*ob++ = ']';
				continue;
				}
			else
				*ob++ = *tb++;
				if (*(tb - 1) == '0')
					*ob++ = '*';
			}
	*ob++ = '(';
	*ob++ = '[';
	*ob++ = '^';
	*ob++ = '0';
	*ob++ = '-';
	*ob++ = '9';
	*ob++ = ']';
	*ob++ = '|';
	*ob++ = '$';
	*ob++ = ')';
	*ob = '\0';
	printf("%s\n",outbuf);
}
