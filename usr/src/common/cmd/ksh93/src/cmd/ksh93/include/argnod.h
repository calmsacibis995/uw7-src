#ident	"@(#)ksh93:src/cmd/ksh93/include/argnod.h	1.1"
#pragma prototyped
#ifndef ARG_RAW
/*
 *	struct to hold a word argument
 *	Written by David Korn
 *
 */

#include	<stak.h>

struct ionod
{
	unsigned	iofile;
	char		*ioname;
	struct ionod	*ionxt;
	struct ionod	*iolst;
	char		*iodelim;
	off_t		iooffset;
	long		iosize;
};

struct comnod
{
	int		comtyp;
	struct ionod	*comio;
	struct argnod	*comarg;
	struct argnod	*comset;
	void		*comnamp;
	int		comline;
};

#define COMBITS		4
#define COMMSK		((1<<COMBITS)-1)
#define COMSCAN		(01<<COMBITS)

struct slnod 	/* struct for link list of stacks */
{
	struct slnod	*slnext;
	struct slnod	*slchild;
	Stak_t		*slptr;
};

/*
 * This struct is use to hold $* lists and arrays
 */

struct dolnod
{
	short		dolrefcnt;	/* reference count */
	short		dolmax;		/* size of dolval array */
	short		dolnum;		/* number of elements */
	short		dolbot;		/* current first element */
	struct dolnod	*dolnxt;	/* used when list are chained */
	char		*dolval[1];	/* array of value pointers */
};

/*
 * This struct is used to hold word arguments of variable size during
 * parsing and during expansion.  The flags indicate what processing
 * is required on the argument.
 */

struct argnod
{
	union
	{
		struct argnod	*ap;
		char		*cp;
	}		argnxt;
	union
	{
		struct argnod	*ap;
		int		len;
	}		argchn;
	unsigned char	argflag;
	char		argval[4];
};



/* The following should evaluate to the offset of argval in argnod */
extern int errno;	/* could be any l-value */
#define ARGVAL	((unsigned)(((struct argnod*)(&errno))->argval-(char*)(&errno)))
#define sh_argstr(ap)	((ap)->argflag&ARG_RAW?sh_fmtq((ap)->argval):(ap)->argval)
#ifdef SHOPT_VPIX
#   define ARG_SPARE 2
#else
#   define ARG_SPARE 1
#endif /* SHOPT_VPIX */


/* legal argument flags */
#define ARG_RAW		0x1	/* string needs no processing */
#define ARG_MAKE	0x2	/* bit set during argument expansion */
#define ARG_MAC		0x4	/* string needs macro expansion */
#define	ARG_EXP		0x8	/* string needs file expansion */
#define ARG_ASSIGN	0x10	/* argument is an assignment */
#define ARG_QUOTED	0x20	/* word contained quote characters */
#define ARG_MESSAGE	0x40	/* contains international string */
#define ARG_APPEND	0x80	/* for += assignment */

extern char 		**sh_argbuild(int*,const struct comnod*);
extern struct dolnod	*sh_argcreate(char*[]);
extern char 		*sh_argdolminus(void);
extern struct dolnod	*sh_argfree(struct dolnod*,int);
extern struct dolnod	*sh_argnew(char*[],struct dolnod**);
extern int		sh_argopts(int,char*[]);
extern void 		sh_argreset(struct dolnod*,struct dolnod*);
extern void 		sh_argset(char*[]);
extern struct dolnod	*sh_arguse(void);

extern const char	e_heading[];
extern const char	e_heading_id[];
extern const char	e_off[];
extern const char	e_off_id[];
extern const char	e_on[];
extern const char	e_on_id[];
extern const char	e_sptbnl[];
extern const char	e_subst[];
extern const char	e_subst_id[];
extern const char	e_option[];
extern const char	e_option_id[];
extern const char	e_argexp[];
extern const char	e_argexp_id[];
extern const char	e_exec[];
extern const char	e_exec_id[];
extern const char	e_devfdNN[];

#endif /* ARG_RAW */
