#ident	"@(#)ksh93:src/cmd/ksh93/include/edit.h	1.1"
#pragma prototyped
#ifndef SEARCHSIZE
/*
 *  edit.h -  common data structure for vi and emacs edit options
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 */

#define SEARCHSIZE	80

#include	"FEATURE/options"
#if !defined(SHOPT_VSH) && !defined (SHOPT_ESH)
#   define ed_winsize()	(SEARCHSIZE)
#else

#ifndef KSHELL
#   include	<setjmp.h>
#   include	<sig.h>
#   include	<ctype.h>
#endif /* KSHELL */

#include	"FEATURE/setjmp"

#ifdef SHOPT_SEVENBIT
#   define STRIP	0177
#else
#   define STRIP	0377
#endif /* SHOPT_SEVENBIT */
#define LOOKAHEAD	80

#ifdef SHOPT_MULTIBYTE
#   ifndef ESS_MAXCHAR
#   include	"national.h"
#   endif /* ESS_MAXCHAR */

typedef wchar_t genchar;

#   define CHARSIZE	3
#else
    typedef char genchar;
#   define CHARSIZE	1
#endif /* SHOPT_MULTIBYTE */

#define TABSIZE	8
#define PRSIZE	80
#define MAXLINE	502		/* longest edit line permitted */

struct edit
{
	int	e_kill;
	int	e_erase;
	int	e_werase;
	int	e_eof;
	int	e_lnext;
	int	e_fchar;
	char	e_plen;		/* length of prompt string */
	char	e_crlf;		/* zero if cannot return to beginning of line */
	sigjmp_buf e_env;
	int	e_llimit;	/* line length limit */
	int	e_hline;	/* current history line number */
	int	e_hloff;	/* line number offset for command */
	int	e_hismin;	/* minimum history line number */
	int	e_hismax;	/* maximum history line number */
	int	e_raw;		/* set when in raw mode or alt mode */
	int	e_cur;		/* current line position */
	int	e_eol;		/* end-of-line position */
	int	e_pcur;		/* current physical line position */
	int	e_peol;		/* end of physical line position */
	int	e_mode;		/* edit mode */
	int	e_index;	/* index in look-ahead buffer */
	int	e_repeat;
	int	e_saved;
	int	e_fcol;		/* first column */
	int	e_ucol;		/* column for undo */
	int	e_wsize;	/* width of display window */
	char	*e_outbase;	/* pointer to start of output buffer */
	char	*e_outptr;	/* pointer to position in output buffer */
	char	*e_outlast;	/* pointer to end of output buffer */
	genchar	*e_inbuf;	/* pointer to input buffer */
	char	*e_prompt;	/* pointer to buffer containing the prompt */
	genchar	*e_ubuf;	/* pointer to the undo buffer */
	genchar	*e_killbuf;	/* pointer to delete buffer */
	genchar	e_search[SEARCHSIZE];	/* search string */
	genchar	*e_Ubuf;	/* temporary workspace buffer */
	genchar	*e_physbuf;	/* temporary workspace buffer */
	int	e_lbuf[LOOKAHEAD];/* pointer to look-ahead buffer */
	int	e_fd;		/* file descriptor */
	int	e_ttyspeed;	/* line speed, also indicates tty parms are valid */
	int	*e_globals;	/* global variables */
	genchar	*e_window;	/* display window  image */
	char	e_inmacro;	/* processing macro expansion */
#ifndef KSHELL
	char	e_prbuff[PRSIZE]; /* prompt buffer */
#endif /* KSHELL */
};

#undef MAXWINDOW
#define MAXWINDOW	160	/* maximum width window */
#define FAST	2
#define SLOW	1
#define ESC	033
#define	UEOF	-2			/* user eof char synonym */
#define	UINTR	-3			/* user intr char synonym */
#define	UERASE	-4			/* user erase char synonym */
#define	UKILL	-5			/* user kill char synonym */
#define	UWERASE	-6			/* user word erase char synonym */
#define	ULNEXT	-7			/* user next literal char synonym */

#define	cntl(x)		(x&037)

#ifndef KSHELL
#   define STRIP	0377
#   define GMACS	1
#   define EMACS	2
#   define VIRAW	4
#   define EDITVI	8
#   define NOHIST	16
#   define EDITMASK	15
#   define is_option(m)	(opt_flag&(m))
    extern char opt_flag;
#   ifdef SYSCALL
#	define read(fd,buff,n)	syscall(3,fd,buff,n)
#   else
#	define read(fd,buff,n)	rEAd(fd,buff,n)
#   endif /* SYSCALL */
#endif	/* KSHELL */

extern struct edit editb;

extern void	ed_crlf(void);
extern void	ed_putchar(int);
extern void	ed_ringbell(void);
extern void	ed_setup(int);
extern void	ed_flush(void);
extern int	ed_getchar(int);
extern int	ed_virt_to_phys(genchar*,genchar*,int,int,int);
extern int	ed_window(void);
extern void	ed_ungetchar(int);
extern int	ed_viread(int, char*, int);
extern int	ed_read(int, char*, int);
extern int	ed_emacsread(int, char*, int);
#ifdef KSHELL
	extern int	ed_macro(int);
	extern int	ed_expand(char[],int*,int*,int);
	extern int	ed_fulledit(void);
#endif /* KSHELL */
#   ifdef SHOPT_MULTIBYTE
	extern int ed_internal(const char*, genchar*);
	extern int ed_external(const genchar*, char*);
	extern void ed_gencpy(genchar*,const genchar*);
	extern void ed_genncpy(genchar*,const genchar*,int);
	extern int ed_genlen(const genchar*);
	extern int ed_setwidth(const char*);
#  endif /* SHOPT_MULTIBYTE */

extern const char	e_runvi[];
#ifndef KSHELL
   extern const char	e_version[];
#endif /* KSHELL */

#endif
#endif
