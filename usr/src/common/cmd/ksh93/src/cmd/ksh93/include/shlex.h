#ident	"@(#)ksh93:src/cmd/ksh93/include/shlex.h	1.2"
#pragma prototyped
#ifndef NOTSYM
/*
 *	UNIX shell
 *	Written by David Korn
 *	These are the definitions for the lexical analyzer
 */

#include	"FEATURE/options"
#include	"shnodes.h"
#include	"shtable.h"
#include	"lexstates.h"

struct shlex_t
{
	struct argnod	*arg;		/* current word */
	struct ionod	*heredoc;	/* pending here document list */
	int		token;		/* current token number */
	int		lastline;	/* last line number */
	int		lasttok;	/* previous token number */
	int		digits;		/* numerical value with word token */
	char		aliasok;	/* on when alias is legal */
	char		assignok;	/* on when name=value is legal */
	int		inlineno;	/* saved value of sh.inlineno */
	int		firstline;	/* saved value of sh.st.firstline */
#ifdef SHOPT_KIA
	Sfio_t		*kiafile;	/* kia output file */
	Sfio_t		*kiatmp;	/* kia reference file */
	unsigned long	script;		/* script entity number */
	unsigned long	fscript;	/* script file entity number */
	unsigned long	current;	/* current entity number */
	unsigned long	unknown;	/* <unknown> entity number */
	off_t		kiabegin;	/* offset of first entry */
	char		*scriptname;	/* name of script file */
	Hashtab_t	*entity_tree;	/* for entity ids */
#endif /* SHOPT_KIA */
};

/* symbols for parsing */
#define NL		'\n'
#define NOTSYM		'!'
#define SYMRES		0400		/* reserved word symbols */
#define DOSYM		(SYMRES|01)
#define FISYM		(SYMRES|02)
#define ELIFSYM		(SYMRES|03)
#define ELSESYM		(SYMRES|04)
#define INSYM		(SYMRES|05)
#define THENSYM		(SYMRES|06)
#define DONESYM		(SYMRES|07)
#define ESACSYM		(SYMRES|010)
#define IFSYM		(SYMRES|011)
#define FORSYM		(SYMRES|012)
#define WHILESYM	(SYMRES|013)
#define UNTILSYM	(SYMRES|014)
#define CASESYM		(SYMRES|015)
#define FUNCTSYM	(SYMRES|016)
#define SELECTSYM	(SYMRES|017)
#define TIMESYM		(SYMRES|020)

#define SYMREP		01000		/* symbols for doubled characters */
#define BREAKCASESYM	(SYMREP|';')
#define ANDFSYM		(SYMREP|'&')
#define ORFSYM		(SYMREP|'|')
#define IOAPPSYM	(SYMREP|'>')
#define IODOCSYM	(SYMREP|'<')
#define EXPRSYM		(SYMREP|'(')
#define BTESTSYM 	(SYMREP|'[')
#define ETESTSYM	(SYMREP|']')

#define SYMMASK		0170000
#define SYMPIPE		010000	/* trailing '|' */
#define SYMLPAR		020000	/* trailing LPAREN */
#define SYMAMP		040000	/* trailing '&' */
#define SYMGT		0100000	/* trailing '>' */
#define IOMOV0SYM	(SYMAMP|'<')
#define IOMOV1SYM	(SYMAMP|'>')
#define FALLTHRUSYM	(SYMAMP|';')
#define COOPSYM		(SYMAMP|'|')
#define IORDWRSYM	(SYMGT|'<')
#define IOCLOBSYM	(SYMPIPE|'>')
#define IPROCSYM	(SYMLPAR|'<')
#define OPROCSYM	(SYMLPAR|'>')
#define EOFSYM		04000	/* end-of-file */
#define TESTUNOP	04001
#define TESTBINOP	04002
#define LABLSYM		04003

/* additional parser flag, others in <shell.h> */
#define SH_EMPTY	04
#define SH_NOIO		010
#define	SH_ASSIGN	020
#define	SH_FUNDEF	040


extern struct shlex_t		shlex;
extern const char		e_endoffile[];
extern const char		e_endoffile_id[];
extern const char		e_newline[];
extern const char		e_newline_id[];

/* odd chars */
#define LBRACE	'{'
#define RBRACE	'}'
#define LPAREN	'('
#define RPAREN	')'
#define LBRACT	'['
#define RBRACT	']'

extern int		sh_lex(void);
extern void		sh_lexinit(int);
extern void 		sh_lexskip(int,int,int);
extern void 		sh_syntax(void);

#endif /* !NOTSYM */
