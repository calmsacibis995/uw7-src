#ident	"@(#)lprof:cmd/lprof.h	1.2"
/*
* lprof.h - main header for lprof
*/
#include <dprof.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

#define OPT_CPPNAMES	0x00001	/* -C demangle C++ names in summary */
#define OPT_INCDIR	0x00002	/* -I include directories provided */
#define OPT_FUNCTION	0x00004	/* -f restriction by functions */
#define OPT_CNTDUMP	0x00008	/* -P direct cntfile dump */
#define OPT_SEPARATE	0x00010	/* -S separate source file report */
#define OPT_TIMESTAMP	0x00020	/* -T continue even if time stamps mismatch */
#define OPT_VERSION	0x00040	/* -V print command/sgs version */
#define OPT_CNTFILE	0x00080	/* -c cnt file provided */
#define OPT_DSTFILE	0x00100	/* -d destination cnt file for merge */
#define OPT_FORMFEED	0x00200	/* -f emit form feed between source files */
#define OPT_BLKCNTS	0x00400	/* -l include execution counts */
#define OPT_CNTMERGE	0x00800	/* -m source cnt file for merge provided */
#define OPT_PROGFILE	0x01000	/* -o program pathname provided */
#define OPT_LISTING	0x02000	/* -p default source listing report */
#define OPT_SRCFILE	0x04000	/* -r restriction by source files */
#define OPT_SUMMARY	0x08000	/* -s summary report */
#define OPT_BLKCOVER	0x10000	/* -x coverage instead of counts */

#define ADJUSTSLIN	2	/* lines to back up to try to catch start */

struct strlist /* holds a varying size list of strings */
{
	const char	**list;	/* the array of strings */
	unsigned long	nlist;	/* length of list[] */
	unsigned long	nused;	/* length of list[] in use */
};

struct arginfo /* accumulated command line information */
{
	const char	*prog;	/* instrumented program */
	const char	*cntf;	/* main cnt file (also -d destfile) */
	struct strlist	dirs;	/* include directories */
	struct strlist	fcns;	/* these functions only */
	struct strlist	srcs;	/* these source files only */
	struct strlist	mrgs;	/* merge these cnt files */
	unsigned long	option;	/* OPT_xxx flag bits */
};

struct rawfcn /* associates functions and coverage data sets */
{
	Elf32_Sym	*symp;	/* ELF symbol table entry */
	caCOVWORD	*covp;	/* primary coverage data */
	caCOVWORD	**set;	/* secondary coverage data */
	unsigned long	nset;	/* total coverage sets present */
};

struct objfile /* all the ELF information from the object file */
{
	Elf		*elfp;	/* main ELF handle */
	Elf32_Ehdr	*ehdr;	/* ELF main header */
	Elf_Data	*dbg1;	/* .debug contents--DWARF I */
	Elf_Data	*lno1;	/* .line contents--DWARF I */
	Elf_Data	*abv2;	/* .debug_abbrev contents--DWARF II */
	Elf_Data	*dbg2;	/* .debug_info contents--DWARF II */
	Elf_Data	*lno2;	/* .debug_line contents--DWARF II */
	const char	*strt;	/* string table for symbols */
	struct rawfcn	*fcns;	/* symbol table functions */
	unsigned long	slen;	/* length of strt[] */
	unsigned long	nfcn;	/* length of fcns[] */
};

struct linenumb /* single line number */
{
	unsigned long	num;	/* the line number ... */
	unsigned long	cnt;	/* ... and how often reached */
};

struct srcfile;

struct function /* single function */
{
	const char	*name;	/* from the object file symbol table */
	struct srcfile	*srcf;	/* containing source file */
	struct rawfcn	*rawp;	/* information without debugging help */
	struct function	*next;	/* in line number order in source file */
	struct function	*incl;	/* as included from other files */
	struct linenumb	*line;	/* all source lines for this function */
	unsigned long	nlnm;	/* number of line numbers (line[] length) */
	unsigned long	slin;	/* starting line number */
	unsigned long	addr;	/* start of the function */
	unsigned long	past;	/* just after the function */
};

struct srcfile /* single source file */
{
	const char	*path;	/* source pathname */
	struct srcfile	*next;	/* in address order */
	struct function	*func;	/* functions defined in this source file */
	struct function	*incl;	/* functions from other source files */
	struct function	*tail;	/* last in the incl list */
	unsigned long	lopc;	/* its lowest text address ... */
	unsigned long	hipc;	/* ... and highest */
	unsigned long	ncov;	/* number of functions with coverage */
	ino_t		inod;	/* inode and ... */
	dev_t		flsy;	/* filesystem for uniqueness */
};

extern struct arginfo	args;	/* command line arguments */
extern struct objfile	objf;	/* current ELF object file */
extern struct srcfile	*unit;	/* compilation units */
extern struct srcfile	*incl;	/* included source files */

struct function	*addfunc(struct srcfile *,
			const char *,
			struct rawfcn *,
			unsigned long,
			unsigned long,
			struct linenumb *,
			unsigned long);		/* gather.c */
struct srcfile	*addsrcf(struct srcfile **,
			const char *,
			const char *,
			unsigned long,
			unsigned long);		/* gather.c */
extern void	*alloc(size_t);			/* util.c */
extern void	applycnts(caCOVWORD *,
			struct linenumb *,
			unsigned long);		/* gather.c */
extern void	chksrcs(void);			/* reduce.c */
extern void	dwarf1(void);			/* dwarf1.c */
extern void	dwarf2(void);			/* dwarf2.c */
extern void	error(const char *, ...);	/* util.c */
extern void	fatal(const char *, ...);	/* util.c */
struct rawfcn	*findfunc(unsigned long);	/* gather.c */
extern void	gather(void);			/* gather.c */
extern void	*grow(void *, size_t);		/* util.c */
extern void	listing(void);			/* report.c */
extern void	mergecnts(struct linenumb **,
			unsigned long *,
			struct linenumb *,
			unsigned long);		/* merge.c */
extern void	mergefiles(void);		/* merge.c */
extern void	numbsort(struct linenumb *,
			unsigned long);		/* gather.c */
extern void	reduce(void);			/* reduce.c */
const char	*search(struct stat *,
			const char *,
			const char *);		/* util.c */
extern void	summary(void);			/* report.c */
extern void	warn(const char *, ...);	/* util.c */
