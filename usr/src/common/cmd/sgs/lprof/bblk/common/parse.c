#ident	"@(#)lprof:bblk/common/parse.c	1.3"
/*
* parse.c - common portion of assembly parsing
*/
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "bblk.h"

unsigned char	*curptr;
unsigned long	lineno;
enum e_scn	curscn;
enum e_mode	mainmode;
unsigned char	*scanfunc;
unsigned char	chtab[1 << CHAR_BIT];

struct sectinfo {
	struct sectinfo	*next;
	enum e_scn	prev;
	enum e_scn	curr;
};

static struct sectinfo	*sectavail;
static const char	*dirtab[DOT__TOTAL];

void
setchtab(const unsigned char *p, int bit) /* fill chtab[] for particular bit */
{
	do {
		chtab[*p] |= bit;
	} while (*++p != '\0');
}

void
init(void) /* startup: fill dirtab[] and chtab[] */
{
	static const struct {
		const char	*str;
		int		key;
	} dirs[DOT__TOTAL] = {
		".byte",	DOT_BYTE,
		".2byte",	DOT_2BYTE,
		".4byte",	DOT_4BYTE,
		".string",	DOT_STRING,
		".size",	DOT_SIZE,
		".type",	DOT_TYPE,
		".section",	DOT_SECTION,
		".previous",	DOT_PREVIOUS,
		".pushsection",	DOT_PUSHSECTION,
		".popsection",	DOT_POPSECTION,
		".text",	DOT_TEXT,
		".data",	DOT_DATA,
		".bss",		DOT_BSS,
		0,		DOT_OTHER,
	};
	const char *p;
	int i;

	i = 0;
	do {
		if ((p = dirs[i].str) != 0)
			p += 2; /* skip '.' and next byte */
		dirtab[dirs[i].key] = p;
	} while (++i, p != 0);
	setchtab((const unsigned char *)"\n", CH_CMT);
	setchtab((const unsigned char *)" \t", CH_WSP);
	setchtab((const unsigned char *)"abcdefghijklmnopqrstuvwxyz.", CH_LET);
	setchtab((const unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZ_", CH_LET);
	setchtab((const unsigned char *)"0123456789", CH_DIG);
	curscn = SCN_TEXT;
	mainmode = MODE_INIT;
	machinit();
}

static void
newline(void) /* read in next assembly input line */
{
	static unsigned char initbuf[256];
	static unsigned char *buffer = initbuf;
	static size_t buflen = sizeof(initbuf);
	static unsigned char inithold[256];
	static unsigned char *hold = inithold;
	static size_t holdlen = sizeof(inithold);
	unsigned char *p;
	size_t n;

	lineno++;
	buffer[buflen - 1] = 'x';
	if (fgets((char *)buffer, buflen, stdin) == 0) {
		curptr = 0;
		return;
	}
	while (buffer[buflen - 1] == '\0') { /* line possibly too long */
		if (buffer[buflen - 2] == '\n') /* it just fit */
			break;
		p = alloc(buflen + sizeof(initbuf));
		memcpy(p, buffer, buflen);
		if (buffer != initbuf)
			free(buffer);
		buffer = p;
		p += buflen - 1;
		buflen += sizeof(initbuf);
		buffer[buflen - 1] = 'x';
		if (fgets((char *)p, sizeof(initbuf) + 1, stdin) == 0) {
			p[0] = '\n';
			p[1] = '\0';
			break;
		}
	}
	curptr = buffer;
	/*
	* We also do the echoing of the input line now, before any
	* analysis.  The two twists are for MODE_SCAN and MODE_INCR:
	* For MODE_SCAN, we do not do any echoing since we are going
	* to seek back after finding out whether we really do have
	* a function.  For MODE_INCR, we want to insert an increment
	* instruction, and it needs to be done just before the next
	* regular instruction.  Moreover, the increment might need
	* to be done differently depending on the next instruction.
	* Thus, for MODE_INCR, we hold each input line until we
	* read the next line--we're echoing one line back--until we
	* get out of MODE_INCR.
	*/
	if (mainmode != MODE_SCAN) {
		if (hold[0] != '\0') {
			fputs((char *)hold, stdout);
			hold[0] = '\0';
		}
		if (mainmode != MODE_INCR) {
			fputs((char *)buffer, stdout);
			return;
		}
		if ((n = strlen((char *)buffer)) >= holdlen) {
			holdlen = n + sizeof(inithold);
			p = alloc(holdlen);
			if (hold != inithold)
				free(hold);
			hold = p;
		}
		memcpy(hold, buffer, n + 1);
	}
}

static struct sectinfo *
copystack(struct sectinfo *sip) /* return separate copy of list */
{
	struct sectinfo *new;

	if (sip == 0)
		return 0;
	if ((new = sectavail) == 0)
		new = alloc(sizeof(struct sectinfo));
	else
		sectavail = new->next;
	new->curr = sip->curr;
	new->prev = sip->prev;
	new->next = copystack(new->next);
	return new;
}

static void
chgscn(enum e_scn scn, enum e_dot item) /* keep track of sections */
{
	static struct sectinfo initsect = {0, SCN_TEXT, SCN_TEXT};
	static struct sectinfo *scanstack, *stack = &initsect;
	static unsigned long scanlineno;
	static off_t scantell;
	struct sectinfo *sip;

	switch (item) {
	case DOT_BEG_SCAN:
		if (scanstack != 0)
			error(":1662:nested scanning modes\n");
		/*
		* Save where we are for later DOT_END_SCAN.
		*/
		scanlineno = lineno;
		scantell = ftell(stdin);
		scanstack = copystack(stack);
		return;
	case DOT_END_SCAN:
		if (scanstack == 0)
			error(":1663:no saved section stack\n");
		curptr = 0; /* causes parse() to call newline() */
		/*
		* Attach bottom of stack to front of available list.
		* Then switch to the saved stack.
		*/
		for (sip = stack; sip->next != 0; sip = sip->next)
			;
		sip->next = sectavail;
		sectavail = stack;
		stack = scanstack;
		scanstack = 0;
		curscn = stack->curr;
		if (fseek(stdin, scantell, SEEK_SET) != 0)
			error(":1664:fseek failure\n");
		lineno = scanlineno;
		return;
	case DOT_POPSECTION:
		if ((sip = stack->next) == 0)
			error(":1665:too many .popsection directives\n");
		stack->next = sectavail;
		sectavail = stack;
		stack = sip;
		curscn = sip->curr;
		return;
	case DOT_PREVIOUS:
		scn = stack->prev;
		/*FALLTHROUGH*/
	case DOT_SECTION:
		stack->prev = stack->curr;
		break;
	case DOT_PUSHSECTION:
		if ((sip = sectavail) == 0)
			sip = alloc(sizeof(struct sectinfo));
		else
			sectavail = sip->next;
		sip->next = stack;
		sip->prev = scn;
		stack = sip;
		break;
	}
	stack->curr = scn;
	curscn = scn;
}

static void
funcsize(void) /* look for end of function mark (.size FUNC,.-FUNC) */
{
	unsigned char *p, *s;

	if (mainmode == MODE_INIT || mainmode == MODE_SCAN)
		return;
	/*
	* Looking for
	*	scanfunc<>,<>.<>-<>scanfunc
	* which flags the end of the current function.
	*/
	p = curptr;
	if ((s = (unsigned char *)strchr((char *)p, ',')) == 0) {
		warn(":1666:malformed .size directive\n");
		return;
	}
	*s = ' '; /* turn comma into whitespace in case not turned into \0 */
	while (chtab[*--s] & CH_WSP)
		;
	*++s = '\0';
	if (strcmp((char *)scanfunc, (char *)p) != 0)
		return;
	/*
	* At this point, we are committed to this being the end
	* of scanfunc, even if the rest of the .size directive
	* doesn't look exactly as we expect.
	*/
	while (chtab[*++s] & CH_WSP)
		;
	if (*s != '.')
		goto oops;
	while (chtab[*++s] & CH_WSP)
		;
	if (*s != '-')
		goto oops;
	while (chtab[*++s] & CH_WSP)
		;
	p = s;
	if (!(chtab[*s] & CH_LET))
		goto oops;
	while (chtab[*++s] & (CH_LET | CH_DIG))
		;
	*s = '\0';
	if (strcmp((char *)scanfunc, (char *)p) != 0) {
	oops:;
		warn(":1667:unexpected .size construct for %s\n",
			(char *)scanfunc);
	}
	/*
	* Close enough.  We've got a match.
	*/
	funcend();
	mainmode = MODE_INIT;
}

static void
functype(void) /* check for "is a function" mark (.type FUNC,"function") */
{
	unsigned char *p, *s;

	if (mainmode != MODE_SCAN)
		return;
	/*
	* Looking for either of:
	*	scanfunc<>,<>"function"
	*	scanfunc<>,<>@function
	*/
	p = curptr;
	if ((s = (unsigned char *)strchr((char *)p, ',')) == 0) {
		warn(":1668:malformed .type directive\n");
		return;
	}
	*s = ' '; /* turn comma into whitespace in case not turned into \0 */
	while (chtab[*--s] & CH_WSP)
		;
	*++s = '\0';
	if (strcmp((char *)scanfunc, (char *)p) != 0)
		return;
	while (chtab[*++s] & CH_WSP)
		;
	/*
	* Even if it isn't a function, we will be resetting.
	*/
	if (strncmp((char *)++s, "function", 8) != 0)
		mainmode = MODE_INIT;
	else if (chtab[s[8]] & (CH_LET | CH_DIG))
		mainmode = MODE_INIT;
	else {
		mainmode = MODE_BBLK;
		addline(0, 0); /* reset line number information */
	}
#ifdef DEBUG
	if (mainmode == MODE_INIT) {
		printf("#BBLK:%lu:%s isn't a function--not function type\n",
			lineno, (char *)scanfunc);
	}
#endif
	chgscn(SCN_OTHER, DOT_END_SCAN);
}

static enum e_scn
sectnum(void) /* return SCN_xxx for name at curptr */
{
	unsigned char *p = curptr;
	const char *name;
	enum e_scn item;

	/*
	* Only care about ".text", ".line", and ".debug_line".
	*/
	if (*p != '.')
		return SCN_OTHER;
	if (*++p == 't') {
		name = "ext";
		item = SCN_TEXT;
	} else if (*p == 'l') {
		name = "ine";
		item = SCN_LINE;
	} else if (strncmp((char *)p, "debug_line", 10) != 0) {
		return SCN_OTHER;
	} else if (p += 10, chtab[*p] & (CH_LET | CH_DIG)) {
		return SCN_OTHER;
	} else {
		return SCN_DEBUG_LINE;
	}
	if (strncmp((char *)++p, name, 3) != 0)
		return SCN_OTHER;
	if (chtab[p[3]] & (CH_LET | CH_DIG))
		return SCN_OTHER;
	return item;
}

static void
directive(unsigned char *ident)
{
	enum e_dot item;
	enum e_scn scn;

	/*
	* Select which directive it has to be, if we care.
	*/
	switch (ident[1]) {
	default:
	other:;
		machdirective(ident);
		return;
	case '2': /* .2byte */
		item = DOT_2BYTE;
		break;
	case '4': /* .4byte */
		item = DOT_4BYTE;
		break;
	case 'b': /* .bss, .byte */
		item = DOT_BYTE;
		if (ident[2] == 's')
			item = DOT_BSS;
		break;
	case 'd': /* .data */
		item = DOT_DATA;
		break;
	case 'p': /* .previous, .pushsection, .popsection */
		item = DOT_PREVIOUS;
		if (ident[2] == 'u')
			item = DOT_PUSHSECTION;
		else if (ident[2] == 'o')
			item = DOT_POPSECTION;
		break;
	case 's': /* .size, .string, .section */
		item = DOT_SIZE;
		if (ident[2] == 't')
			item = DOT_STRING;
		else if (ident[2] == 'e')
			item = DOT_SECTION;
		break;
	case 't': /* .text, .type */
		item = DOT_TEXT;
		if (ident[2] == 'y')
			item = DOT_TYPE;
		break;
	}
	if (strcmp((char *)&ident[2], dirtab[item]) != 0)
		goto other;
	/*
	* It's a directive we care about.  Handle the effect.
	*/
	switch (item) {
	case DOT_BYTE:
	case DOT_2BYTE:
	case DOT_4BYTE:
	case DOT_STRING:
		if (mainmode != MODE_SCAN)
			nbyte(item);
		return;
	case DOT_SIZE:
		funcsize();
		return;
	case DOT_TYPE:
		functype();
		return;
	case DOT_PREVIOUS:
	case DOT_POPSECTION:
		scn = SCN_OTHER; /* will be ignored */
		break;
	case DOT_PUSHSECTION:
	case DOT_SECTION:
		scn = sectnum();
		break;
	case DOT_TEXT:
		scn = SCN_TEXT;
		item = DOT_SECTION;
		break;
	case DOT_DATA:
		scn = SCN_OTHER;
		item = DOT_SECTION;
		break;
	case DOT_BSS:
		if (*curptr != '\0')
			return;
		scn = SCN_OTHER;
		item = DOT_SECTION;
		break;
	}
	chgscn(scn, item);
}

void
parse(void) /* top level assembly parsing */
{
	unsigned char *ident, *p;
	int ch;

	for (;;) {
		newline();
		if ((p = curptr) == 0)
			return;
		/*
		* At start of assembly statement(s).
		*/
	start:;
		while (chtab[*p] & CH_WSP)
			p++;
		if (chtab[*p] & CH_CMT)
			continue;
		if (*p == ';') {
			p++;
			goto start;
		}
		if (!(chtab[*p] & CH_LET))
			error(":1669:expecting a name: %.20s\n", p);
		ident = p;
		while (chtab[*++p] & (CH_LET | CH_DIG))
			;
		ch = *p;
		*p = '\0';
		if (chtab[ch] & CH_WSP) {
			while (chtab[*++p] & CH_WSP)
				;
			ch = *p;
		}
		if (ch == ':') {
			if (curscn == SCN_TEXT)
				label(ident);
			p++;
			goto start;
		}
		curptr = p;
		if (chtab[ch] & CH_CMT) {
			curptr = (unsigned char *)"";
			p = 0;
		} else if ((p = (unsigned char *)strchr((char *)p, ';')) != 0)
			*p++ = '\0';
		if (*ident == '.')
			directive(ident);
		else if (curscn == SCN_TEXT)
			instruction(ident);
		if (curptr != 0 && p != 0)
			goto start;
	}
}

void
scanmode(unsigned char *ident) /* start scanning potential function "ident" */
{
	static unsigned char initbuf[64];
	static unsigned char *buffer = initbuf;
	static size_t buflen = sizeof(initbuf);
	enum e_dot chgop;
	size_t len;

	if (mainmode == MODE_SCAN) {
		/*
		* Were already checking for a possible function.
		* That label must not be a function.  (What was it?!)
		* Reset to where we stopped echoing the input and
		* go back to MODE_INIT.  We'll get back here again,
		* but not in MODE_SCAN.
		*/
#ifdef DEBUG
		printf("#BBLK:%lu:%s isn't a function--found new label %s\n",
			lineno, (char *)scanfunc, (char *)ident);
#endif
		mainmode = MODE_INIT;
		chgop = DOT_END_SCAN;
	} else {
		/*
		* As a precaution, stop echoing the input and look
		* for a .type directive that asserts that ident is
		* really a function.  If we find such [functype()],
		* then we'll reset and process "for real".
		*/
		if ((len = strlen((char *)ident)) >= buflen) {
			buflen = len + sizeof(initbuf);
			if (buffer != initbuf)
				free(buffer);
			buffer = alloc(buflen);
		}
		scanfunc = memcpy(buffer, ident, len + 1);
		mainmode = MODE_SCAN;
		chgop = DOT_BEG_SCAN;
	}
	chgscn(SCN_OTHER, chgop);
}
