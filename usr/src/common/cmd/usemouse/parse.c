#ident "@(#)parse.c	1.3"
/*
 *	@(#) parse.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated
 *	as Confidential.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define	WHITESPACE	" \n\t"
#define	ISQUOTE(c)	( (c)=='\"' || (c)=='`' || (c)=='\'' )

int fgetline(FILE*, char*, short);
static void xlat_octals(char*);
static int isodigit(char);
static char * getvalue(char*);
static char * strtoken(char* , char*);
static void error(char*, char*);

extern int errlev;

static int (*setparms)();

parseparms(f,s)
int	(*f)();		/* Call this function with what we parse */
char	*s;
{
	char	*parm, *val, buf[256];

	setparms = f;

#ifdef DEBUG
	printf("parseparms: enter with ``%s''\n",s);
#endif
	while (*s)	{
		while ( isspace(*s) )
			s++;
		if ( (parm = strtoken(s, "=")) == NULL)	{
			error("parseparms","bad syntax");
			return -1;
		}
#ifdef DEBUG
		printf("parseparms: got ``%s'', string==``%s''\n",
								parm,s);
#endif
		while ( isspace(*s) )
			s++;

		strcpy(buf, parm);
		if ( (val = getvalue(s)) == (char*) NULL)	{
			sprintf(buf, "can't parse value of %s", parm);
			error("parseparms",buf);
			return -1;
		}

#ifdef DEBUG
		printf("parseparms: value of ``%s'' is ``%s''\n", 
							buf, val);
#endif
		if ((*setparms)(buf, val) == -1) {
			sprintf(buf,"can't set argument to ``%s=''",parm);
			error("parseparms",buf);
			return -1;
		}
	}
#ifdef DEBUG
	printf("parseparms: normal exit\n");
#endif
	return 0;
}

/*
 * This returns a token from a string.
 * A token is either a string of nonspace characters or a quoted
 * string of any characters.
 */
static char *
getvalue(s)
char	*s;
{
	char	*p, tok[6];

	if ( s == (char*) NULL )
		return (char*) NULL;
	if ( *s == '\0' )
		return s;
	if ( ! ISQUOTE(*s) )
		strcpy(tok, WHITESPACE);
	else	{
		tok[0] = *s;
		tok[1] = 0;
	}
	if ( (p = strtoken(s, tok)) == NULL)	{
		error("getvalue","no value found");
		return (char*) NULL;
	}
	xlat_octals(p);
	return p;
}


/*
 * fgetline()
 *
 * Entries in data files used by these routines may span multiple
 * lines if a '\' character is the last character on the line. This
 * routine does that parsing by reading the entire line/entry into
 * a buffer.
 *
 * It returns the number of characters read or -1 on EOF.
 */
#define	BACKSLASH	0x5c

int
fgetline(FILE *fp, char *s, short len)
{
	int	tmp, end = -1, gotsomething = 0;
	char	buf[256];

	*s = 0;
	while(1)	{
		tmp = strlen(s);
		if ( fgets(buf, 256, fp) == (char*) NULL)
			break;
		buf[strlen(buf)-1] = '\0';	/* remove newline */
		strcpy(s+tmp, buf);
		gotsomething++;
		if ( (end = strlen(s)) == 0 )
			break;
		if ( s[end] == BACKSLASH )
			s[end] = '\0';
		else
			break;
	}
	if ( ! gotsomething )
		end = -1;
#ifdef DEBUG
	printf("fgetline: got %d characters `%s'\n",end,s);
#endif
	return end;
}

/*
 * similar to strtok(S) except that you do not pass in
 * zero for the second and subsequent calls. You pass in a
 * a string and a token string and a pointer to the first word in the
 * string is returned.
 */
static char *
strtoken(s, tokens)
char	*s;
char	*tokens;
{
	int	i=0, start;
	static char buf[256];

	if ( ! *s )
		return (char*) 0;
	while ( strchr(tokens, s[i]) != (char*) NULL )
		i++;		/* Skip leading tokens */
	start = i;
	while ( s[i] && strchr(tokens, s[i]) == (char*) 0 )
		i++;		/* Skip to next token */
	strncpy(buf, s+start, i-start);
	buf[i-start] = 0;
	if (s[i] == 0)
		*s = 0;
	else
		strcpy(s, s+i+1);
	start = strlen(s);
	strcpy(s + start + 1, buf);
	return s + start + 1;
}

/*
 * xlat_octals
 *
 * Go through the string and convert \ooo to its character (binary)
 * equivalent.
 */
static void
xlat_octals(s)
char	*s;
{
	int	num;
	char	*writep, strnum[4];

	writep = s;
	while (*s)	{
		if ( *s !=  BACKSLASH )	{	/* normal character */
			*writep++ = *s++;
			continue;
		}
		s++;				/* "read" the backslash */
		if ( *s == BACKSLASH ) {		/* '\\' */
			*writep++ = *s++;
			continue;
		}
		/* It's probably the beginning of an octal */
		/* see if there's at least one digit, if not, remove the \ */
		if ( ! isodigit(*s) )
			continue;
		sscanf(s, "%o", &num);
		*writep++ = (char) num;
		sprintf(strnum, "%o", num);
		s += strlen(strnum)+1;
	}
	*writep='\0';
}

static int
isodigit(char c)
{
	return (int) strchr("01234567", c);
}

static void
error(s,t)
char *s, *t;
{
	extern char progname[];
	extern int errno;

	if ( errlev )
		fprintf(stderr, "%s: (%s) %s (%d)\n", progname, s, t, errno);
}
