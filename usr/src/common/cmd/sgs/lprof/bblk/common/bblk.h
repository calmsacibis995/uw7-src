#ident	"@(#)lprof:bblk/common/bblk.h	1.3"
/*
* bblk.h - all cross-module declarations
*/
#include <stddef.h>

enum e_scn { /* distinct sections */
	SCN_OTHER,
	SCN_TEXT,
	SCN_LINE,
	SCN_DEBUG_LINE,
	SCN__TOTAL
};

enum e_dot { /* directives */
	DOT_OTHER,
	DOT_BYTE,
	DOT_2BYTE,
	DOT_4BYTE,
	DOT_STRING,
	DOT_SIZE,
	DOT_TYPE,
	DOT_SECTION,
	DOT_PREVIOUS,
	DOT_PUSHSECTION,
	DOT_POPSECTION,
	DOT_TEXT,
	DOT_DATA,
	DOT_BSS,
	DOT_BEG_SCAN, /* fake--for chgscn() use */
	DOT_END_SCAN, /* fake--for chgscn() use */
	DOT__TOTAL
};

enum e_mode { /* main operational modes */
	MODE_INIT, /* outside of any function */
	MODE_SCAN, /* might have found a function */
	MODE_FUNC, /* in a function, looking for basic block start */
	MODE_BBLK, /* in a function, next line entry starts a basic block */
	MODE_INCR, /* in a function, want to insert increment */
	MODE__TOTAL
};

struct set { /* coverage set information */
	struct set	*next;	/* used for inline expansions */
	unsigned long	*line;	/* the line numbers */
	unsigned long	nline;	/* count of line numbers in line[] */
	unsigned long	size;	/* full length of line[] */
	unsigned long	uniq;	/* uniqueness value */
	unsigned long	file;	/* file index number */
};

extern struct set	cover;		/* scanfunc's main coverage info */
extern unsigned char	*curptr;	/* active point in current line */
extern unsigned long	lineno;		/* input assembly line number */
extern unsigned char	*scanfunc;	/* current (possible) function */
extern enum e_scn	curscn;		/* active output section */
extern enum e_mode	mainmode;	/* current processing mode */

extern unsigned char	chtab[];	/* character classification */
#define CH_WSP	0x01	/* white space */
#define CH_LET	0x02	/* start of identifier */
#define CH_DIG	0x04	/* 0-9 */
#define CH_CMT	0x08	/* start of comment */

void	addline(unsigned long, unsigned long);	/* common/util.c */
void	*alloc(size_t);				/* common/util.c */
void	error(const char *, ...);		/* common/util.c */
void	funcend(void);				/* $(CPU)/mach.c */
void	init(void);				/* common/parse.c */
void	instruction(unsigned char *);		/* $(CPU)/mach.c */
void	label(unsigned char *);			/* $(CPU)/mach.c */
void	machdirective(unsigned char *);		/* $(CPU)/mach.c */
void	machinit(void);				/* $(CPU)/mach.c */
void	nbyte(enum e_dot);			/* common/dwarf.c */
void	parse(void);				/* common/parse.c */
void	scanmode(unsigned char *);		/* common/parse.c */
void	setchtab(const unsigned char *, int);	/* common/parse.c */
void	startlabel(unsigned long);		/* $(CPU)/mach.c */
void	warn(const char *, ...);		/* common/util.c */
