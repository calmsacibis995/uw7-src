#ident	"@(#)debugger:libedit/common/history.h	1.2"
/*
 *	UNIX shell
 *	Header File for history mechanism
 *	written by David Korn
 *
 */



#ifndef IOBSIZE
#   define IOBSIZE	1024
#endif
#define FC_CHAR		'!'
#define HIS_DFLT	128		/* default size of history list */
#define HISMAX		(sizeof(int)*IOBSIZE)
#define HISBIG		(0100000-1024)	/* 1K less than maximum short */
#define HISLINE		16		/* estimate of average sized history line */
#ifndef GEMINI_ON_OSR5
#define MAXLINE		IOBSIZE		/* longest history line permitted */
#else
#define MAXLINE		258
#endif

#define H_UNDO		0201		/* invalidate previous command */
#define H_CMDNO		0202		/* next 3 bytes give command number */
#define H_VERSION	1		/* history file format version no. */

struct history
{
	struct fileblk	*fixfp;		/* file descriptor for history file */
	int 		fixfd;		/* file number for history file */
	char		*fixname;	/* name of history file */
	off_t		fixcnt;		/* offset into history file */
	int		fixind;		/* current command number index */
	int		fixmax;		/* number of accessible history lines */
	int		fixflush;	/* set if flushed outside of hflush() */
	off_t		fixcmds[1];	/* byte offset for recent commands */
};

typedef struct
{
	short his_command;
	short his_line;
} histloc;

extern struct history	*hist_ptr;

#ifndef KSHELL
#ifdef __cplusplus
extern "C" {
#endif
    extern void	p_flush();
    extern void	p_setout(int);
    extern void	p_char(int);
    extern void	p_num(int, int);
    extern void	p_str(char *, int);
#ifdef __cplusplus
}
#endif
#   define NIL		((char*)0)
#   define sh_fail	ed_failed
#   define sh_copy	ed_movstr
#endif	/* KSHELL */

/* the following are readonly */
extern char	hist_fname[];
extern char	e_history[];

#ifdef __cplusplus
extern "C" {
#endif
/* these are the history interface routines */
    extern int		hist_open(void);
    extern void 	hist_cancel(void);
    extern void 	hist_close(void);
    extern int		hist_copy(char*,int,int);
    extern void 	hist_eof(void);
    extern histloc	hist_find(char*,int,int,int);
    extern void 	hist_flush(void);
    extern void 	hist_list(off_t,int,char*);
    extern int		hist_match(off_t,char*,int);
    extern off_t	hist_position(int);
    extern char 	*hist_word(char*,int);
    extern int		io_mktmp(char *);
    extern int		io_fopen(char *);
    extern int		io_init(int, struct fileblk *, char *);
    extern int		io_getc(int);
    extern void		io_fclose(int);
#   ifdef ESH
	extern histloc	hist_locate(int,int,int);
#   endif	/* ESH */

#ifdef __cplusplus
}
#endif
