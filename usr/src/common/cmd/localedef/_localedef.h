#ident	"@(#)localedef:_localedef.h	1.1"
/* assumes prior inclusion of colldata.h and wcharm.h */

typedef wint_t CODE;
typedef enum { FALSE, TRUE } mboolean_t;

struct codent {
	CODE cd_code;				/* wide character codepoint value */
	struct codent *cd_left;	/* used for balanced tree codent storage */
	struct codent *cd_right;	/* used for balanced tree codent storage */
	unsigned short cd_flags; 
	unsigned short cd_subnbeg; /* stuff for one-to-many mappings */
	struct codent *cd_codelink; /* next character numerically (usually) */
	struct codent *cd_orderlink; /* next character in collation section */
	struct codent *cd_multbeg;  /* stuff for multtbl section */
	struct codent *cd_equiv; 	/* stuff for equivalence classes */
	unsigned long cd_classes; /* character classes */
	struct codent *cd_conv; /* character conversion */
	unsigned char cd_dispwidth;	/* display width of character */
	struct codent *cd_weights[COLL_WEIGHTS_MAX]; /* collation weights */
};

/* defns for cd_flags */
#define SYM_COLL	0x1		/* collation symbol */
#define MCCE_COLL	0x2		/* multi character collation */
#define UNDEF_COLL	0x4		/* undefined collation codent */
#define ELIP_COLL	0x8		/* elipses collation codent */
#define	COLLDEF		0x10	/* defined in collation order */
#define EQUIV_COLL	0x40	/* equivalence class collation codent */
#define REPL_COLL	0x80	/* codent for one-to-many mappings */
#define	DEFAULT_DEFN	0x100  /* used for soft definitions */
#define TOUPPER		0x200	/* indicates toupper source, else tolower source */
#define BLACK		0x400	/* used for balanced tree codent storage */
#define NOWEIGHTS	0x800	/* used to indicate no user supplied weights */

/* defns related to cd_classes */
#define CLASS_MAX	0x4fffffff
#define CLASS_INC	0x100 		

#define	DEFAULT_DISPWIDTH	1

/* symbol table structure */
struct syment {
	unsigned char *sy_name;
	struct syment *sy_next;
	struct codent *sy_codent;
};

struct codent *findcode(CODE);
struct syment *findsym(unsigned char *);
double density();

extern CODE maxcodept;

/* control structures for program parsing and execution */
struct keyword_func {
	char *kf_keyword;
	void (*kf_func)();
	void **kf_list;
};

struct kf_hdr {
	int kfh_numkf;
	struct keyword_func *kfh_arraykf;
};
	
/* error processsing */
#define	WARN	0x1
#define	LIMITS	0x2
#define ERROR	0x4
#define FATAL	(0x8|ERROR)
#define FATAL_LIMITS (0x8|LIMITS)

extern char extra_char_warn[];

/* definitions for magic places in ctype array */
#define ENCODING	ctype_out[523]
#define MULTIBYTE	ctype_out[520] > 1
#define	EUCW1	ctype_out[514]
#define	EUCW2	ctype_out[515]
#define	EUCW3	ctype_out[516]
#define SCRW1	ctype_out[517]
#define SCRW2	ctype_out[518]
#define SCRW3	ctype_out[519]
#define	_MBYTE	ctype_out[520]

void ld_exit();
void diag(int, mboolean_t, const char *, ...);

extern unsigned char escape_char;
extern unsigned char comment_char;
extern unsigned char *curfile;
extern FILE *curFILE;
extern unsigned long curline;
extern unsigned int curindex;

#define EOL(x)	(*(x) == '\0')

struct syment *addsym(unsigned char *);
unsigned char *skipspace(unsigned char *);
unsigned char *getsymchar(unsigned char **);
struct keyword_func *getkeyword(unsigned char **, struct kf_hdr *);
CODE getnum(unsigned char **);
unsigned char *getline(void);
struct codent *getcolchar(unsigned char **,unsigned char, unsigned char **symname);
struct codent *getlocchar(unsigned char **,unsigned char, unsigned char **symname);
extern void kf_copy_func(unsigned char *, void **);
extern void kf_END_func(unsigned char *, void **);
extern void kf_comesc_func(unsigned char *, void **);
extern void *getmem(void *, size_t, mboolean_t, mboolean_t);

extern void output(unsigned char *, int (*)());
extern int mymbtowc(CODE*, const unsigned char *, size_t);
extern int mywctomb(unsigned char *, CODE wc);
extern unsigned char *getstring(unsigned char **, unsigned char *, unsigned int, mboolean_t);

extern struct kf_hdr kfh_charmap, kfh_localesrc;
extern int errno;
extern struct codent strt_codent;
extern struct kf_hdr *cur_hdr;
extern mboolean_t softdefn;
extern unsigned char ctype_out[];
extern int default_dispwidth;

