#ident	"@(#)kern-i386:util/kdb/kdb/dblex.c	1.13.2.1"
#ident	"$Header$"

#include <util/debug.h>
#include <util/types.h>
#include <util/param.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/kdebugger.h>

#define MAXNEST	10

static ushort_t column;
static ushort_t linx;                     /* input line index */
static char linbuf0[LINBUFSIZ];
static char *linbuf[MAXNEST] = { linbuf0 };
static uint_t lin;			/* line number (nesting) */
static ushort_t nest_inx[MAXNEST-1];
static ushort_t nest_col[MAXNEST-1];
static char strbuf[MAXSTRSIZ+1];

extern char db_xdigit[];

ushort_t	dbibase = 16;
static int	last_tab;

int	db_hard_eol;


void
db_flush_input(void)
{
    lin = 0;
    linbuf0[column = linx = 0] = '\0';
    db_hard_eol = 0;
}

int
dbgetchar(void)
{
    int c;

    while ((c = linbuf[lin][linx++]) == '\0') {
	if ((int)lin > 0) {
	    dbstrfree(linbuf[lin--]);
	    linx = nest_inx[lin];
	    column = nest_col[lin];
	    return '\0';
	}
	column = linx = 0;
	kdb_output_aborted = B_FALSE;
	(void) dbprintf("kdb>> ");
	if (dbgets(linbuf0, LINBUFSIZ) == NULL) {
	    linbuf0[0] = '\0';
	    dbprintf("\n");
	    return EOF;
	}
    }
    if (c == '\t')
	column = (((last_tab = column) + 8) & ~7);   /* update column */
    else if (c >= ' ')
	column++;
    return c;
}

void
dbunget(int c)
{
    if (c == '\0')
	return;
    --linx;
    ASSERT(linbuf[lin][linx] == c);
    if (c == '\t')
	column = (ushort_t)last_tab;
    else if (c >= ' ')
	--column;
}

char *
dbpeek(void)
{
    return &linbuf[lin][linx];
}

void
dbcmdline(char *s)
{
    if (lin >= MAXNEST - 1) {
	dberror("input nesting too deep");
	return;
    }
    nest_inx[lin] = linx;
    nest_col[lin] = column;
    linbuf[++lin] = dbstrdup(s);
    column = linx = 0;
}

void
dberror(char *s)
{
    dbprintf("\r");
    if ((int)lin > 0) {
	dbprintf("ERROR IN: %s\r", linbuf[lin]);
	column += 10;
    } else
	column += 6;
    while (--column) {
	dbputc(' ');
	if (kdb_output_aborted)
	    return;
    }
    dbprintf("^ ERROR: %s\n", s);
    lin = 0;
    linbuf0[column = linx = 0] = '\0';
}

short
dbgetitem(ip)
    struct item *ip;
{
    ullong_t n = 0;
    int c, cn, nc;
    int i;
    ushort_t d;
    char *s;
    ushort_t base = dbibase;
    char negative = 0;
    int is_sym;

    /* switch on first character */

    ip->type = NULL;
firstchar:
    c = dbgetchar();
    switch (c) {

    case EOF:    /* end of file */
	return EOF;

    case '\n':  /* newline */
    case '\0':
	if (db_hard_eol)
	    return EOL;
	/* FALLTHRU ... */
    case ' ':   /* blank */
    case '\t':  /* tab */
	goto firstchar;                 /* skip over leading white space */

    case '-':   /* minus sign may modify number */
	c = dbgetchar();
	if (c > '9' || c < '0') {   /* if not a number, it's a name */
	    dbunget(c);
	    c = '-';
	    goto doname;
	}
	negative++;
	if (c != '0')
	    goto donumber;
	/* FALLTHRU */

    case '0':    /* zero starts number, perhaps with modified base */
	c = dbgetchar();
	switch(c) {
	case 'o':               /* octal */
	    base = 8; break;
	case 'x':
	    base = 16; break;   /* hex */
	default:
	    goto donumber;
	}
	c = dbgetchar();
	/* FALLTHRU */

    /* 1-9 starts number with default base */
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
donumber:
	for (;;)
	{
	    cn = c | LCASEBIT;
	    for (d = 0; d < base && cn != db_xdigit[d]; d++) ;
	    if (d == base)
		break;
	    n *= base;
	    n += d;
	    c = dbgetchar();
	}
	dbunget(c);
	if (negative)
	    n = (ullong_t)-(llong_t)n;
rtrncon:
	ip->value.number = n;
	return (ip->type = NUMBER);

    case '\'':    /* single quote starts character number */
	for (nc = 0; (c = dbgetchar()) != '\''; nc++) {
	    if (c == EOF || c == '\0' || c == '\n') {
unterm_chr:
		dberror("unterminated character constant");
		goto baditem;
	    }
	    if (nc > 3) {
		dberror("more than 4 characters in character constant");
		goto baditem;
	    }
	    if (c != '\\')  /* if not escaped */
		cn = c;
	    else
		switch (c = dbgetchar()) {
		case EOF:
		    goto unterm_chr;
		default: cn = c; break;
		case 'n': cn = '\n'; break;
		case 't': cn = '\t'; break;
		case 'v': cn = '\v'; break;
		case 'b': cn = '\b'; break;
		case 'r': cn = '\r'; break;
		case 'f': cn = '\f'; break;
		case '\\': cn = '\\'; break;
		case '\'': cn = '\''; break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		    /* octal character number */
		    cn = c & 7;
		    for (i = 2; --i >= 0;) {  /* max three octal digits */
			c = dbgetchar();
			if (c >= '0' && c <= '7')
			    cn = (cn << 3) | (c & 7);
			else {
			    dbunget(c);
			    break;
			}
		    }
		}
	    *((char *)&n + nc) = (char)cn;
	}
	goto rtrncon;

    case '"':    /* double quote starts a string */
	for (s = strbuf; (c = dbgetchar()) != '"';) {
	    if (c == EOF || c == '\0' || (c == '\n' && lin == 0)) {
unterm_str:
		dberror("unterminated string");
		goto baditem;
	    }
	    if ((s - strbuf) >= MAXSTRSIZ) {
		dberror("string too long");
		goto baditem;
	    }
	    if (c != '\\') {
		*s++ = (char)c;
		continue;
	    }
	    /* backslash starts escape sequence */
	    switch (c = dbgetchar())
	    {
	    default: cn = c; break;
	    case EOF:
		goto unterm_str;
	    case '\n': continue;        /* ignore backslash-newline */
	    case 'n': cn = '\n'; break;
	    case 't': cn = '\t'; break;
	    case 'v': cn = '\v'; break;
	    case 'b': cn = '\b'; break;
	    case 'r': cn = '\r'; break;
	    case 'f': cn = '\f'; break;
	    case '\\': cn = '\\'; break;
	    case '\'': cn = '\''; break;
	    case '"': cn = '\"'; break;
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
		/* octal character specification */
		cn = c & 7;
		for (i = 2; --i >= 0;) {      /* max three octal digits */
		    c = dbgetchar();
		    if (c >= '0' && c <= '7')
			cn = (cn << 3) | (c & 7);
		    else {
			dbunget(c);
			break;
		    }
		}
	    }
	    *s++ = (char)cn;
	}
	*s++ = '\0';                            /* null terminated string */
	if ((ip->value.string = dbstrdup(strbuf)) == NULL)
	    goto baditem;
	return (ip->type = STRING);

    case '/':   /* slash may start comment */
	if ((c = dbgetchar()) != '*') {
	    dbunget(c);
	    c = '/';
	    goto doname;
	}
	for (;;) {
	    c = dbgetchar();
swcomment:
	    switch (c) {
	    case EOF:
		dberror("end of file in comment");
		goto baditem;
	    case '*':               /* possible end of comment */
		c = dbgetchar();
		if (c == '/')       /* end of comment */
		    goto firstchar;
		else
		    goto swcomment;
	    }
	}

    default: /* arbitrary name */
doname:
	s = strbuf;
	*s++ = (char)c;
	is_sym = (c == '/' || issym(c));
	for (;;) {
	    c = dbgetchar();
	    if (c == '\0' || isspace(c) || c == '/' || issym(c) != is_sym) {
		dbunget(c);
		break;
	    }
	    if ((s - strbuf) >= MAXSTRSIZ)
	    {
		dberror("name too long");
		goto baditem;
	    }
	    *s++ = (char)c;
	}
	*s++ = '\0';            /* null terminated string */
	ip->value.string = strbuf;
	return (ip->type = NAME);
    }

baditem:
    return NULL;
}


char *
dbstrdup(s1)
    char *s1;
{
    char *s, *s2;

    for (s = s1; *s++;) ;			/* find end of string */
    if ((s2 = dbstralloc(s - s1)) != NULL)      /* allocate string space */
	strcpy(s2, s1);				/* copy string */
    return s2;
}


char *
dbstralloc(size)
    ushort_t size;
{
    char *s, *start, *maxstart, *end;
    char skipping = 0;
    static char strspc[STRSPCSIZ];  /* space for strings */

    if (size < 2)
	return "";

    if (size > STRSPCSIZ)
	goto out;
    start = strspc;
    maxstart = strspc + (STRSPCSIZ - size);

    while (start < maxstart) {
	if (*start) {
	    start++;
	    skipping = 1;
	    continue;
	}
	else if (skipping) {    /* must skip over null */
	    start++;
	    skipping = 0;
	    continue;
	}
	/* check available space here */
	for (s = start, end = start + size; *s == '\0';) {
	    if (s++ == end) {
		if (dbverbose)
		    dbprintf("dbstralloc() got %x bytes at %lx\n",
			     size, start);
		return start;
	    }
	}
	start = s;
    }
out:
    dberror("out of string space");
    dbprintf("( %x bytes requested )\n", size);
    return NULL;
}


void
dbstrfree(s)
    char *s;
{
    while (*s)
	*s++ = 0;
}
