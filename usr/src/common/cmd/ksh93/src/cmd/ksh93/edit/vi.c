#ident	"@(#)ksh93:src/cmd/ksh93/edit/vi.c	1.1"
#pragma prototyped
/* Adapted for ksh by David Korn */
/*+	VI.C			P.D. Sullivan
 *
 *	One line editor for the shell based on the vi editor.
 *
 *	Questions to:
 *		P.D. Sullivan
 *		cbosgd!pds
-*/


#ifdef KSHELL
#   include	"defs.h"
#else
#   include	<ast.h>
#   include	"FEATURE/options"
#endif	/* KSHELL */
#include	<error.h>
#include	<ctype.h>
#include	"io.h"

#include	"history.h"
#include	"edit.h"
#include	"terminal.h"
#include	"FEATURE/time"

#ifdef SHOPT_OLDTERMIO
#   undef ECHOCTL
    extern char echoctl;
#else
#   ifdef ECHOCTL
#	define echoctl	ECHOCTL
#   else
#	define echoctl	0
#   endif /* ECHOCTL */
#endif /*SHOPT_OLDTERMIO */

#ifndef FIORDCHK
#   define NTICKS	5		/* number of ticks for typeahead */
#endif /* FIORDCHK */

#define	MAXCHAR	MAXLINE-2		/* max char per line */

#undef isblank
#ifdef SHOPT_MULTIBYTE
    static int bigvi;
#   define gencpy(a,b)	ed_gencpy(a,b)
#   define genncpy(a,b,n)	ed_genncpy(a,b,n)
#   define genlen(str)	ed_genlen(str)
#   define digit(c)	((c&~STRIP)==0 && isdigit(c))
#   define is_print(c)	((c&~STRIP) || isprint(c))
    static int isalph(int);
    static int ismetach(int);
    static int isblank(int);
#   include	"lexstates.h"
#else
#   define gencpy(a,b)	strcpy((char*)(a),(char*)(b))
#   define genncpy(a,b,n) strncpy((char*)(a),(char*)(b),n)
#   define genlen(str)	strlen(str)
#   define isalph(v)	((_c=virtual[v])=='_'||isalnum(_c))
#   define isblank(v)	isspace(virtual[v])
#   define ismetach(v)	ismeta(virtual[v])
#   define digit(c)	isdigit(c)
#   define is_print(c)	isprint(c)
#endif	/* SHOPT_MULTIBYTE */
#define fold(c)		((c)&~040)	/* lower and uppercase equivalent */

#undef putchar
#define putchar(c)	ed_putchar(c)

#define crallowed	editb.e_crlf
#define cur_virt	editb.e_cur		/* current virtual column */
#define cur_phys	editb.e_pcur	/* current phys column cursor is at */
#define curhline	editb.e_hline		/* current history line */
#define findchar	editb.e_fchar		/* last find char */
#define first_virt	editb.e_fcol		/* first allowable column */
#define first_wind	editb.e_globals[0]	/* first column of window */
#define	globals		editb.e_globals		/* local global variables */
#define histmin		editb.e_hismin
#define histmax		editb.e_hismax
#define last_phys	editb.e_peol		/* last column in physical */
#define last_virt	editb.e_eol		/* last column */
#define last_wind	editb.e_globals[1]	/* last column in window */
#define	lastmotion	editb.e_globals[2]	/* last motion */
#define lastrepeat	editb.e_mode	/* last repeat count for motion cmds */
#define	long_char	editb.e_globals[3]	/* line bigger than window */
#define	long_line	editb.e_globals[4]	/* line bigger than window */
#define lsearch		editb.e_search		/* last search string */
#define lookahead	editb.e_index		/* characters in buffer */
#define previous	editb.e_lbuf		/* lookahead buffer */
#define max_col		editb.e_llimit		/* maximum column */
#define	ocur_phys	editb.e_globals[5]   /* old current physical position */
#define	ocur_virt	editb.e_globals[6]	/* old last virtual position */
#define	ofirst_wind	editb.e_globals[7]	/* old window first col */
#define	o_v_char	editb.e_globals[8]	/* prev virtual[ocur_virt] */
#define Prompt		editb.e_prompt		/* pointer to prompt */
#define plen		editb.e_plen		/* length of prompt */
#define physical	editb.e_physbuf		/* physical image */
#define repeat		editb.e_repeat	    /* repeat count for motion cmds */
#define u_column	editb.e_ucol		/* undo current column */
#define U_saved		editb.e_saved		/* original virtual saved */
#define U_space		editb.e_Ubuf		/* used for U command */
#define u_space		editb.e_ubuf		/* used for u command */
#define usreof		editb.e_eof		/* user defined eof char */
#define usrerase	editb.e_erase		/* user defined erase char */
#define usrlnext	editb.e_lnext		/* user defined next literal */
#define usrkill		editb.e_kill		/* user defined kill char */
#define virtual		editb.e_inbuf	/* pointer to virtual image buffer */
#define	window		editb.e_window		/* window buffer */
#define	w_size		editb.e_wsize		/* window size */
#define	inmacro		editb.e_inmacro		/* true when in macro */
#define yankbuf		editb.e_killbuf		/* yank/delete buffer */


#define	ABORT	-2			/* user abort */
#define	APPEND	-10			/* append chars */
#define	BAD	-1			/* failure flag */
#define	BIGVI	-15			/* user wants real vi */
#define	CONTROL	-20			/* control mode */
#define	ENTER	-25			/* enter flag */
#define	GOOD	0			/* success flag */
#define	INPUT	-30			/* input mode */
#define	INSERT	-35			/* insert mode */
#define	REPLACE	-40			/* replace chars */
#define	SEARCH	-45			/* search flag */
#define	TRANSLATE	-50		/* translate virt to phys only */

#define	INVALID	(-1)			/* invalid column */

static genchar	_c;
static char addnl;			/* boolean - add newline flag */
static char last_cmd = '\0';		/* last command */
static char repeat_set;
static char nonewline;
static genchar *lastline;
static char paren_chars[] = "([{)]}";   /* for % command */

#ifdef FIORDCHK
    static clock_t typeahead;		/* typeahead occurred */
    extern int ioctl(int, int, ...);
#else
    static int typeahead;		/* typeahead occurred */
#endif	/* FIORDCHK */

static void	del_line(int);
static int	getcount(int);
static void	getline(int);
static int	getrchar(void);
static int	mvcursor(int);
static void	pr_string(const char*);
static void	putstring(int, int);
static void	refresh(int);
static void	replace(int, int);
static void	restore_v(void);
static void	save_last(void);
static void	save_v(void);
static int	search(int);
static void	sync_cursor(void);
static int	textmod(int,int);

/*+	VI_READ( fd, shbuf, nchar )
 *
 *	This routine implements a one line version of vi and is
 * called by _filbuf.c
 *
-*/

ed_viread(int fd, register char *shbuf, int nchar)
{
	register int i;			/* general variable */
	register int term_char;		/* read() termination character */
	char prompt[PRSIZE+2];		/* prompt */
	genchar Physical[2*MAXLINE];	/* physical image */
	genchar Ubuf[MAXLINE];	/* used for U command */
	genchar ubuf[MAXLINE];	/* used for u command */
	genchar Window[MAXWINDOW+10];	/* window image */
	int Globals[9];			/* local global variables */
	int esc_or_hang=0;		/* <ESC> or hangup */
	char cntl_char=0;		/* TRUE if control character present */
#ifdef SHOPT_RAWONLY
# 	define viraw	1
#else
	int viraw = (sh_isoption(SH_VIRAW) || sh.st.trap[SH_KEYTRAP]);
#endif /* SHOPT_RAWONLY */
#ifndef FIORDCHK
	clock_t oldtime, newtime;
	struct tms dummy;
#endif	/* FIORDCHK */
	
	/*** setup prompt ***/

	Prompt = prompt;
	ed_setup(fd);

	if(!viraw)
	{
		/*** Change the eol characters to '\r' and eof  ***/
		/* in addition to '\n' and make eof an ESC	*/
		if(tty_alt(ERRIO) < 0)
			return(ed_read(fd, shbuf, nchar));

#ifdef FIORDCHK
		ioctl(fd,FIORDCHK,&typeahead);
#else
		/* time the current line to determine typeahead */
		oldtime = times(&dummy);
#endif /* FIORDCHK */
#ifdef KSHELL
		/* abort of interrupt has occurred */
		if(sh.trapnote&SH_SIGSET)
			i = -1;
		else
#endif /* KSHELL */
		/*** Read the line ***/
		i = ed_read(fd, shbuf, nchar);
#ifndef FIORDCHK
		newtime = times(&dummy);
		typeahead = ((newtime-oldtime) < NTICKS);
#endif /* FIORDCHK */
	    if(echoctl)
	    {
		if( i <= 0 )
		{
			/*** read error or eof typed ***/
			tty_cooked(ERRIO);
			return(i);
		}
		term_char = shbuf[--i];
		if( term_char == '\r' )
			term_char = '\n';
		if( term_char=='\n' || term_char==ESC )
			shbuf[i--] = '\0';
		else
			shbuf[i+1] = '\0';
	    }
	    else
	    {
		register int c = shbuf[0];

		/*** Save and remove the last character if its an eol, ***/
		/* changing '\r' to '\n' */

		if( i == 0 )
		{
			/*** ESC was typed as first char of line ***/
			esc_or_hang = 1;
			term_char = ESC;
			shbuf[i--] = '\0';	/* null terminate line */
		}
		else if( i<0 || c==usreof )
		{
			/*** read error or eof typed ***/
			tty_cooked(ERRIO);
			if( c == usreof )
				i = 0;
			return(i);
		}
		else
		{
			term_char = shbuf[--i];
			if( term_char == '\r' )
				term_char = '\n';
#if !defined(VEOL2) && !defined(ECHOCTL)
			if(term_char=='\n')
				return(i+1);
#endif
			if( term_char=='\n' || term_char==usreof )
			{
				/*** remove terminator & null terminate ***/
				shbuf[i--] = '\0';
			}
			else
			{
				/** terminator was ESC, which is not xmitted **/
				term_char = ESC;
				shbuf[i+1] = '\0';
			}
		}
	    }
	}
	else
	{
		/*** Set raw mode ***/

#ifndef SHOPT_RAWONLY
		if( editb.e_ttyspeed == 0 )
		{
			/*** never did TCGETA, so do it ***/
			/* avoids problem if user does 'sh -o viraw' */
			tty_alt(ERRIO);
		}
#endif /* SHOPT_RAWONLY */
		if(tty_raw(ERRIO,0) < 0 )
			return(ed_read(fd, shbuf, nchar));
		i = INVALID;
	}

	/*** Initialize some things ***/

	virtual = (genchar*)shbuf;
#undef virtual
#define virtual		((genchar*)shbuf)
#ifdef SHOPT_MULTIBYTE
	shbuf[i+1] = 0;
	i = ed_internal(shbuf,virtual)-1;
#endif /* SHOPT_MULTIBYTE */
	globals = Globals;
	cur_phys = i + 1;
	cur_virt = i;
	first_virt = 0;
	first_wind = 0;
	last_virt = i;
	last_phys = i;
	last_wind = i;
	long_line = ' ';
	long_char = ' ';
	o_v_char = '\0';
	ocur_phys = 0;
	ocur_virt = MAXCHAR;
	ofirst_wind = 0;
	physical = Physical;
	u_column = INVALID - 1;
	U_space = Ubuf;
	u_space = ubuf;
	window = Window;
	window[0] = '\0';

#if KSHELL && (2*CHARSIZE*MAXLINE)<IOBSIZE
	yankbuf = shbuf + MAXLINE*sizeof(genchar);
#else
	if(yankbuf==0)
		yankbuf = (genchar*)malloc(sizeof(genchar)*(MAXLINE));
#endif
#if KSHELL && (3*CHARSIZE*MAXLINE)<IOBSIZE
	lastline = shbuf + (MAXLINE+MAXLINE)*sizeof(genchar);
#else
	if(lastline==0)
		lastline = (genchar*)malloc(sizeof(genchar)*(MAXLINE));
#endif
	if( last_cmd == '\0' )
	{
		/*** first time for this shell ***/

		last_cmd = 'i';
		findchar = INVALID;
		lastmotion = '\0';
		lastrepeat = 1;
		repeat = 1;
		*yankbuf = 0;
	}

	/*** fiddle around with prompt length ***/
	if( nchar+plen > MAXCHAR )
		nchar = MAXCHAR - plen;
	max_col = nchar - 2;

	if( !viraw )
	{
		int kill_erase = 0;
		for(i=(echoctl?last_virt:0); i<last_virt; ++i )
		{
			/*** change \r to \n, check for control characters, ***/
			/* delete appropriate ^Vs,			*/
			/* and estimate last physical column */

			if( virtual[i] == '\r' )
				virtual[i] = '\n';
		    if(!echoctl)
		    {
			register int c = virtual[i];
			if( c<=usrerase)
			{
				/*** user typed escaped erase or kill char ***/
				cntl_char = 1;
				if(is_print(c))
					kill_erase++;
			}
			else if( !is_print(c) )
			{
				cntl_char = 1;

				if( c == usrlnext )
				{
					if( i == last_virt )
					{
						/*** eol/eof was escaped ***/
						/* so replace ^V with it */
						virtual[i] = term_char;
						break;
					}

					/*** delete ^V ***/
					gencpy((&virtual[i]), (&virtual[i+1]));
					--cur_virt;
					--last_virt;
				}
			}
		    }
		}

		/*** copy virtual image to window ***/
		if(last_virt > 0)
			last_phys = ed_virt_to_phys(virtual,physical,last_virt,0,0);
		if( last_phys >= w_size )
		{
			/*** line longer than window ***/
			last_wind = w_size - 1;
		}
		else
			last_wind = last_phys;
		genncpy(window, virtual, last_wind+1);

		if( term_char!=ESC  && (last_virt==INVALID
			|| virtual[last_virt]!=term_char) )
		{
			/*** Line not terminated with ESC or escaped (^V) ***/
			/* eol, so return after doing a total update */
			/* if( (speed is greater or equal to 1200 */
			/* and something was typed) and */
			/* (control character present */
			/* or typeahead occurred) ) */

			tty_cooked(ERRIO);
			if( editb.e_ttyspeed==FAST && last_virt!=INVALID
				&& (typeahead || cntl_char) )
			{
				refresh(TRANSLATE);
				pr_string(Prompt);
				putstring(0, last_phys+1);
				if(echoctl)
					ed_crlf();
				else
					while(kill_erase-- > 0)
						putchar(' ');
			}

			if( term_char=='\n' )
			{
				if(!echoctl)
					ed_crlf();
				virtual[++last_virt] = '\n';
			}
			last_cmd = 'i';
			save_last();
#ifdef SHOPT_MULTIBYTE
			virtual[last_virt+1] = 0;
			last_virt = ed_external(virtual,shbuf);
			return(last_virt);
#else
			return(++last_virt);
#endif /* SHOPT_MULTIBYTE */
		}

		/*** Line terminated with escape, or escaped eol/eof, ***/
		/*  so set raw mode */

		if( tty_raw(ERRIO,0) < 0 )
		{
			tty_cooked(ERRIO);
			/*
			 * The following prevents drivers that return 0 on
			 * causing an infinite loop
			 */
			if(esc_or_hang)
				return(-1);
			virtual[++last_virt] = '\n';
#ifdef SHOPT_MULTIBYTE
			virtual[last_virt+1] = 0;
			last_virt = ed_external(virtual,shbuf);
			return(last_virt);
#else
			return(++last_virt);
#endif /* SHOPT_MULTIBYTE */
		}

		if(echoctl) /*** for cntl-echo erase the ^[ ***/
			pr_string("\b\b  \b\b");


		if(crallowed)
		{
			/*** start over since there may be ***/
			/*** a control char, or cursor might not ***/
			/*** be at left margin (this lets us know ***/
			/*** where we are ***/
			cur_phys = 0;
			window[0] = '\0';
			pr_string(Prompt);
			if( term_char==ESC && (last_virt<0 || virtual[last_virt]!=ESC))
				refresh(CONTROL);
			else
				refresh(INPUT);
		}
		else
		{
			/*** just update everything internally ***/
			refresh(TRANSLATE);
		}
	}
	else
		virtual[0] = '\0';

	/*** Handle usrintr, usrquit, or EOF ***/

	i = sigsetjmp(editb.e_env,0);
	if( i != 0 )
	{
		virtual[0] = '\0';
		tty_cooked(ERRIO);

		switch(i)
		{
		case UEOF:
			/*** EOF ***/
			return(0);

		case UINTR:
			/** interrupt **/
			return(-1);
		}
		return(-1);
	}

	/*** Get a line from the terminal ***/

	U_saved = 0;
	if(viraw)
		getline(APPEND);
	else if(last_virt>=0 && virtual[last_virt]==term_char)
		getline(APPEND);
	else
		getline(ESC);
	/*** add a new line if user typed unescaped \n ***/
	/* to cause the shell to process the line */
	tty_cooked(ERRIO);
	if( addnl )
	{
		virtual[++last_virt] = '\n';
		ed_crlf();
	}
	if( ++last_virt >= 0 )
	{
#ifdef SHOPT_MULTIBYTE
		if(bigvi)
		{
			bigvi = 0;
			shbuf[last_virt-1] = '\n';
		}
		else
		{
			virtual[last_virt] = 0;
			last_virt = ed_external(virtual,shbuf);
		}
#endif /* SHOPT_MULTIBYTE */
		return(last_virt);
	}
	else
		return(-1);
}


/*{	APPEND( char, mode )
 *
 *	This routine will append char after cur_virt in the virtual image.
 * mode	=	APPEND, shift chars right before appending
 *		REPLACE, replace char if possible
 *
}*/

#undef virtual
#define virtual		editb.e_inbuf	/* pointer to virtual image buffer */

static void append(int c, int mode)
{
	register int i,j;

	if( last_virt<max_col && last_phys<max_col )
	{
		if( mode==APPEND || (cur_virt==last_virt  && last_virt>=0))
		{
			j = (cur_virt>=0?cur_virt:0);
			for(i = ++last_virt;  i > j; --i)
				virtual[i] = virtual[i-1];
		}
		virtual[++cur_virt] = c;
	}
	else
		ed_ringbell();
	return;
}

/*{	BACKWORD( nwords, cmd )
 *
 *	This routine will position cur_virt at the nth previous word.
 *
}*/

static void backword(int nwords, register int cmd)
{
	register int tcur_virt = cur_virt;
	while( nwords-- && tcur_virt > first_virt )
	{
		if( !isblank(tcur_virt) && isblank(tcur_virt-1)
			&& tcur_virt>first_virt )
			--tcur_virt;
		else if(cmd != 'B')
		{
			register int last = iswalph(tcur_virt-1);
			register int cur = iswalph(tcur_virt);
			if((!cur && last) || (cur && !last))
				--tcur_virt;
		}
		while( isblank(tcur_virt) && tcur_virt>=first_virt )
			--tcur_virt;
		if( cmd == 'B' )
		{
			while( !isblank(tcur_virt) && tcur_virt>=first_virt )
				--tcur_virt;
		}
		else
		{
			if(iswalph(tcur_virt))
				while( iswalph(tcur_virt)
				   && tcur_virt>=first_virt )
					--tcur_virt;
			else
				while( !iswalph(tcur_virt) 
				    && !isblank(tcur_virt)
					&& tcur_virt>=first_virt )
					--tcur_virt;
		}
		cur_virt = ++tcur_virt;
	}
	return;
}

/*{	CNTLMODE()
 *
 *	This routine implements the vi command subset.
 *	The cursor will always be positioned at the char of interest.
 *
}*/

static int cntlmode(void)
{
	register int c;
	register int i;
	genchar tmp_u_space[MAXLINE];	/* temporary u_space */
	genchar *real_u_space;		/* points to real u_space */
	int tmp_u_column = INVALID;	/* temporary u_column */
	int was_inmacro;

	if(!U_saved)
	{
		/*** save virtual image if never done before ***/
		virtual[last_virt+1] = '\0';
		gencpy(U_space, virtual);
		U_saved = 1;
	}

	save_last();

	real_u_space = u_space;
	curhline = histmax;
	first_virt = 0;
	repeat = 1;
	if( cur_virt > INVALID )
	{
		/*** make sure cursor is at the last char ***/
		sync_cursor();
	}

	/*** Read control char until something happens to cause a ***/
	/* return to APPEND/REPLACE mode	*/

	while( c=ed_getchar(-1) )
	{
		repeat_set = 0;
		was_inmacro = inmacro;
		if( c == '0' )
		{
			/*** move to leftmost column ***/
			cur_virt = 0;
			sync_cursor();
			continue;
		}

		if( digit(c) )
		{
			lastrepeat = repeat;
			c = getcount(c);
			if( c == '.' )
				lastrepeat = repeat;
		}

		/*** see if it's a move cursor command ***/

		if(mvcursor(c))
		{
			sync_cursor();
			repeat = 1;
			continue;
		}

		/*** see if it's a repeat of the last command ***/

		if( c == '.' )
		{
			c = last_cmd;
			repeat = lastrepeat;
			i = textmod(c, c);
		}
		else
		{
			i = textmod(c, 0);
		}

		/*** see if it's a text modification command ***/

		switch(i)
		{
		case BAD:
			break;

		default:		/** input mode **/
			if(!was_inmacro)
			{
				last_cmd = c;
				lastrepeat = repeat;
			}
			repeat = 1;
			if( i == GOOD )
				continue;
			return(i);
		}

		switch( c )
		{
			/***** Other stuff *****/

		case cntl('L'):		/** Redraw line **/
			/*** print the prompt and ***/
			/* force a total refresh */
			if(nonewline==0)
				putchar('\n');
			nonewline = 0;
			pr_string(Prompt);
			window[0] = '\0';
			cur_phys = first_wind;
			ofirst_wind = INVALID;
			long_line = ' ';
			break;

		case cntl('V'):
		{
			register const char *p = &e_version[5];
			save_v();
			del_line(BAD);
			while(c = *p++)
				append(c,APPEND);
			refresh(CONTROL);
			ed_getchar(-1);
			restore_v();
			break;
		}

		case '/':		/** Search **/
		case '?':
		case 'N':
		case 'n':
			save_v();
			switch( search(c) )
			{
			case GOOD:
				/*** force a total refresh ***/
				window[0] = '\0';
				goto newhist;

			case BAD:
				/*** no match ***/
					ed_ringbell();

			default:
				if( u_column == INVALID )
					del_line(BAD);
				else
					restore_v();
				break;
			}
			break;

		case 'j':		/** get next command **/
		case '+':		/** get next command **/
			curhline += repeat;
			if( curhline > histmax )
			{
				curhline = histmax;
				goto ringbell;
			}
			else if(curhline==histmax && tmp_u_column!=INVALID )
			{
				u_space = tmp_u_space;
				u_column = tmp_u_column;
				restore_v();
				u_space = real_u_space;
				break;
			}
			save_v();
			cur_virt = INVALID;
			goto newhist;

		case 'k':		/** get previous command **/
		case '-':		/** get previous command **/
			if( curhline == histmax )
			{
				u_space = tmp_u_space;
				i = u_column;
				save_v();
				u_space = real_u_space;
				tmp_u_column = u_column;
				u_column = i;
			}

			curhline -= repeat;
			if( curhline <= histmin )
			{
				curhline += repeat;
				goto ringbell;
			}
			save_v();
			cur_virt = INVALID;
		newhist:
			if(curhline!=histmax || cur_virt==INVALID)
				hist_copy((char*)virtual, MAXLINE, curhline,-1);
			else
			{
				gencpy(virtual, u_space);
#ifdef SHOPT_MULTIBYTE
				ed_internal((char*)u_space,u_space);
#endif /* SHOPT_MULTIBYTE */
			}
#ifdef SHOPT_MULTIBYTE
			ed_internal((char*)virtual,virtual);
#endif /* SHOPT_MULTIBYTE */
			if((last_virt=genlen(virtual)-1) >= 0  && cur_virt == INVALID)
				cur_virt = 0;
			break;


		case 'u':		/** undo the last thing done **/
			restore_v();
			break;

		case 'U':		/** Undo everything **/
			save_v();
			if( virtual[0] == '\0' )
				goto ringbell;
			else
			{
				gencpy(virtual, U_space);
				last_virt = genlen(U_space) - 1;
				cur_virt = 0;
			}
			break;

#ifdef KSHELL
		case 'v':
			if(repeat_set==0)
				goto vcommand;
#endif /* KSHELL */

		case 'G':		/** goto command repeat **/
			if(repeat_set==0)
				repeat = histmin+1;
			if( repeat <= histmin || repeat > histmax )
			{
				goto ringbell;
			}
			curhline = repeat;
			save_v();
			if(c == 'G')
			{
				cur_virt = INVALID;
				goto newhist;
			}

#ifdef KSHELL
		vcommand:
			if(ed_fulledit()==GOOD)
				return(BIGVI);
			else
				goto ringbell;
#endif	/* KSHELL */

		case '#':	/** insert(delete) # to (no)comment command **/
			if( cur_virt != INVALID )
			{
				register genchar *p = &virtual[last_virt+1];
				*p = 0;
				/*** see whether first char is comment char ***/
				c = (virtual[0]=='#');
				while(p-- >= virtual)
				{
					if(*p=='\n' || p<virtual)
					{
						if(c) /* delete '#' */
						{
							if(p[1]=='#')
							{
								last_virt--;
								gencpy(p+1,p+2);
							}
						}
						else
						{
							cur_virt = p-virtual;
							append('#', APPEND);
						}
					}
				}
				if(c)
				{
					curhline = histmax;
					cur_virt = 0;
					break;
				}
				refresh(INPUT);
			}

		case '\n':		/** send to shell **/
			return(ENTER);

		default:
		ringbell:
			ed_ringbell();
			repeat = 1;
			continue;
		}

		refresh(CONTROL);
		repeat = 1;
	}
/* NOTREACHED */
}

/*{	CURSOR( new_current_physical )
 *
 *	This routine will position the virtual cursor at
 * physical column x in the window.
 *
}*/

static void cursor(register int x)
{
	register int delta;
	int tmpctr, tmpctr2;

#ifdef SHOPT_MULTIBYTE
	while(physical[x]==MARKER)
		x++;
#endif /* SHOPT_MULTIBYTE */
	delta = x - cur_phys;

	if( delta == 0 )
		return;

	if( delta > 0 )
	{
		/*** move to right ***/
		putstring(cur_phys, delta);
	}
	else
	{
		/*** move to left ***/

		delta = -delta;

		/*** attempt to optimize cursor movement ***/
		if(!crallowed)
		{
			/* I'm not sure about the following -- jv */
			tmpctr = cur_phys - 1;
			do {
				if ((physical[tmpctr] == 0x100) &&
				   (tmpctr2 = icharset(physical[(tmpctr-1)]))) {
					delta -= (1 + tmpctr2);
					tmpctr-= (1 + tmpctr2); 
				} else {
					delta--;
					tmpctr--;
				}
				putchar('\b');
			} while (delta > 0);
		}
		else
		{
			pr_string(Prompt);
			putstring(first_wind, x - first_wind);
		}
	}
	cur_phys = x;
	return;
}

/*{	DELETE( nchars, mode )
 *
 *	Delete nchars from the virtual space and leave cur_virt positioned
 * at cur_virt-1.
 *
 *	If mode	= 'c', do not save the characters deleted
 *		= 'd', save them in yankbuf and delete.
 *		= 'y', save them in yankbuf but do not delete.
 *
}*/

static void cdelete(register int nchars, int mode)
{
	register int i;
	register genchar *vp;

	if( cur_virt < first_virt )
	{
		ed_ringbell();
		return;
	}
	if( nchars > 0 )
	{
		vp = virtual+cur_virt;
		o_v_char = vp[0];
		if( (cur_virt-- + nchars) > last_virt )
		{
			/*** set nchars to number actually deleted ***/
			nchars = last_virt - cur_virt;
		}

		/*** save characters to be deleted ***/

		if( mode != 'c' )
		{
			i = vp[nchars];
			vp[nchars] = 0;
			gencpy(yankbuf,vp);
			vp[nchars] = i;
		}

		/*** now delete these characters ***/

		if( mode != 'y' )
		{
			gencpy(vp,vp+nchars);
			last_virt -= nchars;
		}
	}
	return;
}

/*{	DEL_LINE( mode )
 *
 *	This routine will delete the line.
 *	mode = GOOD, do a save_v()
 *
}*/
static void del_line(int mode)
{
	if( last_virt == INVALID )
		return;

	if( mode == GOOD )
		save_v();

	cur_virt = 0;
	first_virt = 0;
	cdelete(last_virt+1, BAD);
	refresh(CONTROL);

	cur_virt = INVALID;
	cur_phys = 0;
	findchar = INVALID;
	last_phys = INVALID;
	last_virt = INVALID;
	last_wind = INVALID;
	first_wind = 0;
	o_v_char = '\0';
	ocur_phys = 0;
	ocur_virt = MAXCHAR;
	ofirst_wind = 0;
	window[0] = '\0';
	return;
}

/*{	DELMOTION( motion, mode )
 *
 *	Delete thru motion.
 *
 *	mode	= 'd', save deleted characters, delete
 *		= 'c', do not save characters, change
 *		= 'y', save characters, yank
 *
 *	Returns 1 if operation successful; else 0.
 *
}*/

static int delmotion(int motion, int mode)
{
	register int begin, end, delta;
	/* the following saves a register */

	if( cur_virt == INVALID )
		return(0);
	if( mode != 'y' )
		save_v();
	begin = cur_virt;

	/*** fake out the motion routines by appending a blank ***/

	virtual[++last_virt] = ' ';
	end = mvcursor(motion);
	virtual[last_virt--] = 0;
	if(!end)
		return(0);

	end = cur_virt;
	if( mode=='c' && end>begin && strchr("wW", motion) )
	{
		/*** called by change operation, user really expects ***/
		/* the effect of the eE commands, so back up to end of word */
		while( end>begin && isblank(end-1) )
			--end;
		if( end == begin )
			++end;
	}

	delta = end - begin;
	if( delta >= 0 )
	{
		cur_virt = begin;
		if( strchr("eE;,TtFf%", motion) )
			++delta;
	}
	else
	{
		delta = -delta;
	}

	cdelete(delta, mode);
	if( mode == 'y' )
		cur_virt = begin;
	return(1);
}


/*{	ENDWORD( nwords, cmd )
 *
 *	This routine will move cur_virt to the end of the nth word.
 *
}*/

static void endword(int nwords, register int cmd)
{
	register int tcur_virt = cur_virt;
	while( nwords-- )
	{
		if( !isblank(tcur_virt) && tcur_virt<=last_virt )
			++tcur_virt;
		while( isblank(tcur_virt) && tcur_virt<=last_virt )
			++tcur_virt;	
		if( cmd == 'E' )
		{
			while( !isblank(tcur_virt) && tcur_virt<=last_virt )
				++tcur_virt;
		}
		else
		{
			/* if( isalph(tcur_virt) ) */
			if( iswalph(tcur_virt) )
				/* while( isalph(tcur_virt) && tcur_virt<=last_virt ) */
				while( iswalph(tcur_virt)
				   && tcur_virt<=last_virt )
					++tcur_virt;
			else
				/* while( !isalph(tcur_virt) && !isblank(tcur_virt) */
				while( !iswalph(tcur_virt)
				    && !isblank(tcur_virt)
					&& tcur_virt<=last_virt )
					++tcur_virt;
		}
		if( tcur_virt > first_virt )
			tcur_virt--;
	}
	cur_virt = tcur_virt;
	return;
}

/*{	FORWARD( nwords, cmd )
 *
 *	This routine will move cur_virt forward to the next nth word.
 *
}*/

static void forward(register int nwords, int cmd)
{
	register int tcur_virt = cur_virt;
	while( nwords-- )
	{
		if( cmd == 'W' )
		{
			while( !isblank(tcur_virt) && tcur_virt < last_virt )
				++tcur_virt;
		}
		else
		{
			/* if( isalph(tcur_virt) ) */
			if( iswalph(tcur_virt) )
			{
				/* while( isalph(tcur_virt) && tcur_virt<last_virt ) */
				while( iswalph(tcur_virt) && 
				   tcur_virt<last_virt )
					++tcur_virt;
			}
			else
			{
				/* while( !isalph(tcur_virt) && !isblank(tcur_virt) */
				while( !iswalph(tcur_virt) && 
				   !isblank(tcur_virt)
					&& tcur_virt < last_virt )
					++tcur_virt;
			}
		}
		while( isblank(tcur_virt) && tcur_virt < last_virt )
			++tcur_virt;
	}
	cur_virt = tcur_virt;
	return;
}



/*{	GETCOUNT(c)
 *
 *	Set repeat to the user typed number and return the terminating
 * character.
 *
}*/

static int getcount(register int c)
{
	register int i;

	/*** get any repeat count ***/

	if( c == '0' )
		return(c);

	repeat_set++;
	i = 0;
	while( digit(c) )
	{
		i = i*10 + c - '0';
		c = ed_getchar(-1);
	}

	if( i > 0 )
		repeat *= i;
	return(c);
}


/*{	GETLINE( mode )
 *
 *	This routine will fetch a line.
 *	mode	= APPEND, allow escape to cntlmode subroutine
 *		  appending characters.
 *		= REPLACE, allow escape to cntlmode subroutine
 *		  replacing characters.
 *		= SEARCH, no escape allowed
 *		= ESC, enter control mode immediately
 *
 *	The cursor will always be positioned after the last
 * char printed.
 *
 *	This routine returns when cr, nl, or (eof in column 0) is
 * received (column 0 is the first char position).
 *
}*/

static void getline(register int mode)
{
	register int c;
	register int tmp;

	addnl = 1;

	if( mode == ESC )
	{
		/*** go directly to control mode ***/
		goto escape;
	}

	for(;;)
	{
		if( (c=ed_getchar(mode==SEARCH?1:-2)) == usreof )
			c = UEOF;
		else if( c == usrerase )
			c = UERASE;
		else if( c == usrkill )
			c = UKILL;
		else if( c == editb.e_werase )
			c = UWERASE;
		else if( c == usrlnext )
			c = ULNEXT;

		if( c == ULNEXT)
		{
			/*** implement ^V to escape next char ***/
			c = ed_getchar(2);
			append(c, mode);
			refresh(INPUT);
			continue;
		}

		switch( c )
		{
		case ESC:		/** enter control mode **/
			if( mode == SEARCH )
			{
				ed_ringbell();
				continue;
			}
			else
			{
	escape:
				if( mode == REPLACE )
					--cur_virt;
				tmp = cntlmode();
				if( tmp == ENTER || tmp == BIGVI )
				{
#ifdef SHOPT_MULTIBYTE
					bigvi = (tmp==BIGVI);
#endif /* SHOPT_MULTIBYTE */
					return;
				}
				if( tmp == INSERT )
				{
					mode = APPEND;
					continue;
				}
				mode = tmp;
			}
			break;

		case UERASE:		/** user erase char **/
				/*** treat as backspace ***/

		case '\b':		/** backspace **/
			if( virtual[cur_virt] == '\\' )
			{
				cdelete(1, BAD);
				append(usrerase, mode);
			}
			else
			{
				if( mode==SEARCH && cur_virt==0 )
				{
					first_virt = 0;
					cdelete(1, BAD);
					return;
				}
				cdelete(1, BAD);
			}
			break;

		case UWERASE:		/** delete back word **/
			if( cur_virt > first_virt && 
				!isblank(cur_virt) &&
				!ispunct(virtual[cur_virt]) &&
				isblank(cur_virt-1) )
			{
				cdelete(1, BAD);
			}
			else
			{
				tmp = cur_virt;
				backword(1, 'W');
				cdelete(tmp - cur_virt + 1, BAD);
			}
			break;

		case UKILL:		/** user kill line char **/
			if( virtual[cur_virt] == '\\' )
			{
				cdelete(1, BAD);
				append(usrkill, mode);
			}
			else
			{
				if( mode == SEARCH )
				{
					cur_virt = 1;
					delmotion('$', BAD);
				}
				else if(first_virt)
				{
					tmp = cur_virt;
					cur_virt = first_virt;
					cdelete(tmp - cur_virt + 1, BAD);
				}
				else
					del_line(GOOD);
			}
			break;

		case UEOF:		/** eof char **/
			if( cur_virt != INVALID )
				continue;
			addnl = 0;

		case '\n':		/** newline or return **/
			if( mode != SEARCH )
				save_last();
			refresh(INPUT);
			return;

		default:
			if( mode == REPLACE )
			{
				if( cur_virt < last_virt )
				{
					replace(c, 1);
					continue;
				}
				cdelete(1, BAD);
				mode = APPEND;
			}
			append(c, mode);
			break;
		}
		refresh(INPUT);

	}
}

/*{	MVCURSOR( motion )
 *
 *	This routine will move the virtual cursor according to motion
 * for repeat times.
 *
 * It returns GOOD if successful; else BAD.
 *
}*/

static int mvcursor(register int motion)
{
	register int count;
	register int tcur_virt;
	register int incr = -1;
	register int bound = 0;
	static int last_find = 0;	/* last find command */

	switch(motion)
	{
		/***** Cursor move commands *****/

	case '0':		/** First column **/
		tcur_virt = 0;
		break;

	case '^':		/** First nonblank character **/
		tcur_virt = first_virt;
		while( isblank(tcur_virt) && tcur_virt < last_virt )
			++tcur_virt;
		break;

	case '|':
		tcur_virt = repeat-1;
		if(tcur_virt <= last_virt)
			break;
		/* fall through */

	case '$':		/** End of line **/
		tcur_virt = last_virt;
		break;

	case 'h':		/** Left one **/
	case '\b':
		motion = first_virt;
		goto walk;

	case ' ':
	case 'l':		/** Right one **/
		motion = last_virt;
		incr = 1;
	walk:
		tcur_virt = cur_virt;
		if( incr*tcur_virt < motion)
		{
			tcur_virt += repeat*incr;
			if( incr*tcur_virt > motion)
				tcur_virt = motion;
		}
		else
			return(0);
		break;

	case 'B':
	case 'b':		/** back word **/
		tcur_virt = cur_virt;
		backword(repeat, motion);
		if( cur_virt == tcur_virt )
			return(0);
		return(1);

	case 'E':
	case 'e':		/** end of word **/
		tcur_virt = cur_virt;
		if(tcur_virt >=0)
			endword(repeat, motion);
		if( cur_virt == tcur_virt )
			return(0);
		return(1);

	case ',':		/** reverse find old char **/
	case ';':		/** find old char **/
		switch(last_find)
		{
		case 't':
		case 'f':
			if(motion==';')
			{
				bound = last_virt;
				incr = 1;
			}
			goto find_b;

		case 'T':
		case 'F':
			if(motion==',')
			{
				bound = last_virt;
				incr = 1;
			}
			goto find_b;

		default:
			return(0);
		}


	case 't':		/** find up to new char forward **/
	case 'f':		/** find new char forward **/
		bound = last_virt;
		incr = 1;

	case 'T':		/** find up to new char backward **/
	case 'F':		/** find new char backward **/
		last_find = motion;
		if((findchar=getrchar())==ESC)
			return(1);
find_b:
		tcur_virt = cur_virt;
		count = repeat;
		while( count-- )
		{
			while( incr*(tcur_virt+=incr) <= bound
				&& virtual[tcur_virt] != findchar );
			if( incr*tcur_virt > bound )
			{
				return(0);
			}
		}
		if( fold(last_find) == 'T' )
			tcur_virt -= incr;
		break;

        case '%':
	{
		int nextmotion;
		int nextc;
		tcur_virt = cur_virt;
		while( tcur_virt <= last_virt
			&& strchr(paren_chars,virtual[tcur_virt])==(char*)0)
				tcur_virt++;
		if(tcur_virt > last_virt )
			return(0);
		nextc = virtual[tcur_virt];
		count = strchr(paren_chars,nextc)-paren_chars;
		if(count < 3)
		{
			incr = 1;
			bound = last_virt;
			nextmotion = paren_chars[count+3];
		}
		else
			nextmotion = paren_chars[count-3];
		count = 1;
		while(count >0 &&  incr*(tcur_virt+=incr) <= bound)
		{
		        if(virtual[tcur_virt] == nextmotion)
		        	count--;
		        else if(virtual[tcur_virt]==nextc)
		        	count++;
		}
		if(count)
			return(0);
		break;
	}

	case 'W':
	case 'w':		/** forward word **/
		tcur_virt = cur_virt;
		forward(repeat, motion);
		if( tcur_virt == cur_virt )
			return(0);
		return(1);

	default:
		return(0);
	}
	cur_virt = tcur_virt;

	return(1);
}

/*
 * print a string
 */

static void pr_string(register const char *sp)
{
	/*** copy string sp ***/
	register char *ptr = editb.e_outptr;
	while(*sp)
		*ptr++ = *sp++;
	editb.e_outptr = ptr;
	return;
}

/*{	PUTSTRING( column, nchars )
 *
 *	Put nchars starting at column of physical into the workspace
 * to be printed.
 *
}*/

static void putstring(register int col, register int nchars)
{
	while( nchars-- )
		putchar(physical[col++]);
	return;
}

/*{	REFRESH( mode )
 *
 *	This routine will refresh the crt so the physical image matches
 * the virtual image and display the proper window.
 *
 *	mode	= CONTROL, refresh in control mode, ie. leave cursor
 *			positioned at last char printed.
 *		= INPUT, refresh in input mode; leave cursor positioned
 *			after last char printed.
 *		= TRANSLATE, perform virtual to physical translation
 *			and adjust left margin only.
 *
 *		+-------------------------------+
 *		|   | |    virtual	  | |   |
 *		+-------------------------------+
 *		  cur_virt		last_virt
 *
 *		+-----------------------------------------------+
 *		|	  | |	        physical	 | |    |
 *		+-----------------------------------------------+
 *			cur_phys			last_phys
 *
 *				0			w_size - 1
 *				+-----------------------+
 *				| | |  window		|
 *				+-----------------------+
 *				cur_window = cur_phys - first_wind
}*/

static void refresh(int mode)
{
	register int p;
	register int regb;
	register int first_w = first_wind;
	int p_differ;
	int new_lw;
	int ncur_phys;
	int opflag;			/* search optimize flag */

	/* for editing screen image with multibyte characters */
	int char_length, extra_byte_cnt;  

#	define	w	regb
#	define	v	regb

	/*** find out if it's necessary to start translating at beginning ***/

	if(lookahead>0)
	{
		p = previous[lookahead-1];
		if(p != ESC && p != '\n' && p != '\r')
			mode = TRANSLATE;
	}
	v = cur_virt;
	if( v<ocur_virt || ocur_virt==INVALID
		|| ( v==ocur_virt
			&& (!is_print(virtual[v]) || !is_print(o_v_char))) )
	{
		opflag = 0;
		p = 0;
		v = 0;
	}
	else
	{
		opflag = 1;
		p = ocur_phys;
		v = ocur_virt;
		if( !is_print(virtual[v]) )
		{
			/*** avoid double ^'s ***/
			++p;
			++v;
		}
	}
	virtual[last_virt+1] = 0;
	ncur_phys = ed_virt_to_phys(virtual,physical,cur_virt,v,p);
	p = genlen(physical);
	if( --p < 0 )
		last_phys = 0;
	else
		last_phys = p;

	/*** see if this was a translate only ***/

	if( mode == TRANSLATE )
		return;

	/*** adjust left margin if necessary ***/

	if( ncur_phys<first_w || ncur_phys>=(first_w + w_size) )
	{
		cursor(first_w);
		first_w = ncur_phys - (w_size>>1);
		if( first_w < 0 )
			first_w = 0;
		first_wind = cur_phys = first_w;
	}

	/*** attempt to optimize search somewhat to find ***/
	/*** out where physical and window images differ ***/

	if( first_w==ofirst_wind && ncur_phys>=ocur_phys && opflag==1 )
	{
		p = ocur_phys;
		w = p - first_w;
	}
	else
	{
		p = first_w;
		w = 0;
	}

	for(; (p<=last_phys && w<=last_wind); ++p, ++w)
	{
		if( window[w] != physical[p] )
			break;
	}
	p_differ = p;

	if( (p>last_phys || p>=first_w+w_size) && w>last_wind
		&& cur_virt==ocur_virt )
	{
		/*** images are identical ***/
		return;
	}

	/*** copy the physical image to the window image ***/

	extra_byte_cnt = 0;
	if( last_virt != INVALID )
	{
		while( p <= last_phys && w < w_size )
		{
			if((char_length = (wcwidth(physical[p]))) > 1)
				extra_byte_cnt += (char_length - 1);
			window[w++] = physical[p++];
		}
	}
	new_lw = w;

	/*** erase trailing characters if needed ***/

	while( w <= (last_wind + extra_byte_cnt) && w < w_size)
		window[w++] = ' ';
	last_wind = --w;

	p = p_differ;

	/*** move cursor to start of difference ***/

	cursor(p);

	/*** and output difference ***/

	w = p - first_w;
	while( w <= last_wind )
		putchar(window[w++]);

	cur_phys = w + first_w;
	last_wind = --new_lw;

	if( last_phys >= w_size )
	{
		if( first_w == 0 )
			long_char = '>';
		else if( last_phys < (first_w+w_size) )
			long_char = '<';
		else
			long_char = '*';
	}
	else
		long_char = ' ';

	if( long_line != long_char )
	{
		/*** indicate lines longer than window ***/
		while( w++ < w_size )
		{
			putchar(' ');
			++cur_phys;
		}
		putchar(long_char);
		++cur_phys;
		long_line = long_char;
	}

	ocur_phys = ncur_phys;
	ocur_virt = cur_virt;
	ofirst_wind = first_w;

	if( mode==INPUT && cur_virt>INVALID )
		++ncur_phys;

	cursor(ncur_phys);
	ed_flush();
	return;
}

/*{	REPLACE( char, increment )
 *
 *	Replace the cur_virt character with char.  This routine attempts
 * to avoid using refresh().
 *
 *	increment	= 1, increment cur_virt after replacement.
 *			= 0, leave cur_virt where it is.
 *
}*/

static void replace(register int c, register int increment)
{
	register int cur_window;

	if( cur_virt == INVALID )
	{
		/*** can't replace invalid cursor ***/
		ed_ringbell();
		return;
	}
	cur_window = cur_phys - first_wind;
	if( ocur_virt == INVALID || !is_print(c)
		|| !is_print(virtual[cur_virt])
		|| !is_print(o_v_char)
#ifdef SHOPT_MULTIBYTE
		|| icharset(c) || out_csize(icharset(o_v_char))>1
		|| icharset(virtual[cur_virt])
#endif /* SHOPT_MULTIBYTE */
		|| (increment && (cur_window==w_size-1)
			|| !is_print(virtual[cur_virt+1])) )
	{
		/*** must use standard refresh routine ***/

		cdelete(1, BAD);
		append(c, APPEND);
		if( increment && cur_virt<last_virt )
			++cur_virt;
		refresh(CONTROL);
	}
	else
	{
		virtual[cur_virt] = c;
		physical[cur_phys] = c;
		window[cur_window] = c;
		putchar(c);
		if(increment)
		{
			c = virtual[++cur_virt];
			++cur_phys;
		}
		else
		{
			putchar('\b');
		}
		o_v_char = c;
		ed_flush();
	}
	return;
}

/*{	RESTORE_V()
 *
 *	Restore the contents of virtual space from u_space.
 *
}*/

static void restore_v(void)
{
	register int tmpcol;
	genchar tmpspace[MAXLINE];

	if( u_column == INVALID-1 )
	{
		/*** never saved anything ***/
		ed_ringbell();
		return;
	}
	gencpy(tmpspace, u_space);
	tmpcol = u_column;
	save_v();
	gencpy(virtual, tmpspace);
	cur_virt = tmpcol;
	last_virt = genlen(tmpspace) - 1;
	ocur_virt = MAXCHAR;	/** invalidate refresh optimization **/
	return;
}

/*{	SAVE_LAST()
 *
 *	If the user has typed something, save it in last line.
 *
}*/

static void save_last(void)
{
	register int i;

	if( (i = cur_virt - first_virt + 1) > 0 )
	{
		/*** save last thing user typed ***/
		if(i >= MAXLINE)
			i = MAXLINE-1;
		genncpy(lastline, (&virtual[first_virt]), i);
		lastline[i] = '\0';
	}
	return;
}

/*{	SAVE_V()
 *
 *	This routine will save the contents of virtual in u_space.
 *
}*/

static void save_v(void)
{
	if(!inmacro)
	{
		virtual[last_virt + 1] = '\0';
		gencpy(u_space, virtual);
		u_column = cur_virt;
	}
	return;
}

/*{	SEARCH( mode )
 *
 *	Search history file for regular expression.
 *
 *	mode	= '/'	require search string and search new to old
 *	mode	= '?'	require search string and search old to new
 *	mode	= 'N'	repeat last search in reverse direction
 *	mode	= 'n'	repeat last search
 *
}*/

/*
 * search for <string> in the current command
 */
static int curline_search(const char *string)
{
	register int len=strlen(string);
	register const char *dp,*cp=string, *dpmax;
#ifdef SHOPT_MULTIBYTE
	ed_external(u_space,(char*)u_space);
#endif /* SHOPT_MULTIBYTE */
	for(dp=(char*)u_space,dpmax=dp+wcslen(dp)-len; dp<=dpmax; dp++)
	{
		if(*dp==*cp && memcmp(cp,dp,len)==0)
			return(dp-(char*)u_space);
	}
#ifdef SHOPT_MULTIBYTE
	ed_internal((char*)u_space,u_space);
#endif /* SHOPT_MULTIBYTE */
	return(-1);
}

static int search(register int mode)
{
	register int new_direction;
	register int oldcurhline;
	register int i;
	static int direction = -1;
	Histloc_t  location;

	if( mode == '/' || mode == '?')
	{
		/*** new search expression ***/
		del_line(BAD);
		append(mode, APPEND);
		refresh(INPUT);
		first_virt = 1;
		getline(SEARCH);
		first_virt = 0;
		virtual[last_virt + 1] = '\0';	/*** make null terminated ***/
		direction = mode=='/' ? -1 : 1;
	}

	if( cur_virt == INVALID )
	{
		/*** no operation ***/
		return(ABORT);
	}

	if( cur_virt==0 ||  fold(mode)=='N' )
	{
		/*** user wants repeat of last search ***/
		del_line(BAD);
		wcscpy( ((char*)virtual)+1, lsearch);
#ifdef SHOPT_MULTIBYTE
		*((char*)virtual) = '/';
		ed_internal((char*)virtual,virtual);
#endif /* SHOPT_MULTIBYTE */
	}

	if( mode == 'N' )
		new_direction = -direction;
	else
		new_direction = direction;


	/*** now search ***/

	oldcurhline = curhline;
#ifdef SHOPT_MULTIBYTE
	ed_external(virtual,(char*)virtual);
#endif /* SHOPT_MULTIBYTE */
	if(mode=='?' && (i=curline_search(((char*)virtual)+1))>=0)
	{
		location.hist_command = curhline;
		location.hist_char = i;
	}
	else
	{
		i = INVALID;
		if( new_direction==1 && curhline >= histmax )
			curhline = histmin + 1;
		location = hist_find(sh.hist_ptr,((char*)virtual)+1, curhline, 1, new_direction);
	}
	cur_virt = i;
	wcsncpy(lsearch, ((char*)virtual)+1, SEARCHSIZE);
	if( (curhline=location.hist_command) >=0 )
	{
		ocur_virt = INVALID;
		return(GOOD);
	}

	/*** could not find matching line ***/

	curhline = oldcurhline;
	return(BAD);
}

/*{	SYNC_CURSOR()
 *
 *	This routine will move the physical cursor to the same
 * column as the virtual cursor.
 *
}*/

static void sync_cursor(void)
{
	register int p;
	register int v;
	register int c;
	int new_phys;

	if( cur_virt == INVALID )
		return;

	/*** find physical col that corresponds to virtual col ***/

	new_phys = 0;
	if(first_wind==ofirst_wind && cur_virt>ocur_virt && ocur_virt!=INVALID)
	{
		/*** try to optimize search a little ***/
		p = ocur_phys + 1;
#ifdef SHOPT_MULTIBYTE
		while(physical[p]==MARKER)
			p++;
#endif /* SHOPT_MULTIBYTE */
		v = ocur_virt + 1;
	}
	else
	{
		p = 0;
		v = 0;
	}
	for(; v <= last_virt; ++p, ++v)
	{
#ifdef SHOPT_MULTIBYTE
		int d;
		c = virtual[v];
		/* if(d = icharset(c)) */
		if((d = wcwidth(c)) > 1)
		{
			if( v != cur_virt )
				/* p += (out_csize(d)-1); */
				p += (d-1);
		}
		else
#else
		c = virtual[v];
#endif	/* SHOPT_MULTIBYTE */
		/* if( !isprint(c) ) */
		if( !iswprint(c) )
		{
			if( c == '\t' )
			{
				p -= ((p+editb.e_plen)%TABSIZE);
				p += (TABSIZE-1);
			}
		}
		if( v == cur_virt )
		{
			new_phys = p;
			break;
		}
	}

	if( new_phys < first_wind || new_phys >= first_wind + w_size )
	{
		/*** asked to move outside of window ***/

		window[0] = '\0';
		refresh(CONTROL);
		return;
	}

	cursor(new_phys);
	ed_flush();
	ocur_phys = cur_phys;
	ocur_virt = cur_virt;
	o_v_char = virtual[ocur_virt];

	return;
}

/*{	TEXTMOD( command, mode )
 *
 *	Modify text operations.
 *
 *	mode != 0, repeat previous operation
 *
}*/

static int textmod(register int c, int mode)
{
	register int i;
	register genchar *p = lastline;
	register int trepeat = repeat;
	static int lastmacro;
	genchar *savep;

	if(mode && (fold(lastmotion)=='F' || fold(lastmotion)=='T')) 
		lastmotion = ';';

	if( fold(c) == 'P' )
	{
		/*** change p from lastline to yankbuf ***/
		p = yankbuf;
	}

addin:
	switch( c )
	{
			/***** Input commands *****/

#ifdef KSHELL
	case '*':		/** do file name expansion in place **/
	case '\\':		/** do file name completion in place **/
		if( cur_virt == INVALID )
			return(BAD);
	case '=':		/** list file name expansions **/
		save_v();
		i = last_virt;
		++last_virt;
		virtual[last_virt] = 0;
		if( ed_expand((char*)virtual, &cur_virt, &last_virt, c) )
		{
			last_virt = i;
			ed_ringbell();
		}
		else if(c == '=')
		{
			last_virt = i;
			nonewline++;
			ed_ungetchar(cntl('L'));
			return(GOOD);
		}
		else
		{
			--cur_virt;
			--last_virt;
			ocur_virt = MAXCHAR;
			return(APPEND);
		}
		break;

	case '@':		/** macro expansion **/
		if( mode )
			c = lastmacro;
		else
			if((c=getrchar())==ESC)
				return(GOOD);
		if(!inmacro)
			lastmacro = c;
		if(ed_macro(c))
		{
			save_v();
			inmacro++;
			return(GOOD);
		}
		ed_ringbell();
		return(BAD);

#endif	/* KSHELL */
	case '_':		/** append last argument of prev command **/
		save_v();
		{
			genchar tmpbuf[MAXLINE];
			if(repeat_set==0)
				repeat = -1;
			p = (genchar*)hist_word((char*)tmpbuf,MAXLINE,repeat);
#ifndef KSHELL
			if(p==0)
			{
				ed_ringbell();
				break;
			}
#endif	/* KSHELL */
#ifdef SHOPT_MULTIBYTE
			ed_internal((char*)p,tmpbuf);
			p = tmpbuf;
#endif /* SHOPT_MULTIBYTE */
			i = ' ';
			do
			{
				append(i,APPEND);
			}
			while(i = *p++);
			return(APPEND);
		}

	case 'A':		/** append to end of line **/
		cur_virt = last_virt;
		sync_cursor();

	case 'a':		/** append **/
		if( fold(mode) == 'A' )
		{
			c = 'p';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
		{
			first_virt = cur_virt + 1;
			cursor(cur_phys + 1);
			ed_flush();
		}
		return(APPEND);

	case 'I':		/** insert at beginning of line **/
		cur_virt = first_virt;
		sync_cursor();

	case 'i':		/** insert **/
		if( fold(mode) == 'I' )
		{
			c = 'P';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
 		{
 			o_v_char = virtual[cur_virt];
			first_virt = cur_virt--;
  		}
		return(INSERT);

	case 'C':		/** change to eol **/
		c = '$';
		goto chgeol;

	case 'c':		/** change **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(ed_getchar(-1));
chgeol:
		lastmotion = c;
		if( c == 'c' )
		{
			del_line(GOOD);
			return(APPEND);
		}

		if(!delmotion(c, 'c'))
			return(BAD);

		if( mode == 'c' )
		{
			c = 'p';
			trepeat = 1;
			goto addin;
		}
		first_virt = cur_virt + 1;
		return(APPEND);

	case 'D':		/** delete to eol **/
		c = '$';
		goto deleol;

	case 'd':		/** delete **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(ed_getchar(-1));
deleol:
		lastmotion = c;
		if( c == 'd' )
		{
			del_line(GOOD);
			break;
		}
		if(!delmotion(c, 'd'))
			return(BAD);
		if( cur_virt < last_virt )
			++cur_virt;
		break;

	case 'P':
		if( p[0] == '\0' )
			return(BAD);
		if( cur_virt != INVALID )
		{
			i = virtual[cur_virt];
			if(!is_print(i))
				ocur_virt = INVALID;
			--cur_virt;
		}

	case 'p':		/** print **/
		if( p[0] == '\0' )
			return(BAD);

		if( mode != 's' && mode != 'c' )
		{
			save_v();
			if( c == 'P' )
			{
				/*** fix stored cur_virt ***/
				++u_column;
			}
		}
		if( mode == 'R' )
			mode = REPLACE;
		else
			mode = APPEND;
		savep = p;
		for(i=0; i<trepeat; ++i)
		{
			while(c= *p++)
				append(c,mode);
			p = savep;
		}
		break;

	case 'R':		/* Replace many chars **/
		if( mode == 'R' )
		{
			c = 'P';
			goto addin;
		}
		save_v();
		if( cur_virt != INVALID )
			first_virt = cur_virt;
		return(REPLACE);

	case 'r':		/** replace **/
		if( mode )
			c = *p;
		else
			if((c=getrchar())==ESC)
				return(GOOD);
		*p = c;
		save_v();
		while(trepeat--)
			replace(c, trepeat!=0);
		return(GOOD);

	case 'S':		/** Substitute line - cc **/
		c = 'c';
		goto chgeol;

	case 's':		/** substitute **/
		save_v();
		cdelete(repeat, BAD);
		if( mode )
		{
			c = 'p';
			trepeat = 1;
			goto addin;
		}
		first_virt = cur_virt + 1;
		return(APPEND);

	case 'Y':		/** Yank to end of line **/
		c = '$';
		goto yankeol;

	case 'y':		/** yank thru motion **/
		if( mode )
			c = lastmotion;
		else
			c = getcount(ed_getchar(-1));
yankeol:
		lastmotion = c;
		if( c == 'y' )
		{
			gencpy(yankbuf, virtual);
		}
		else if(!delmotion(c, 'y'))
		{
			return(BAD);
		}
		break;

	case 'x':		/** delete repeat chars forward - dl **/
		c = 'l';
		goto deleol;

	case 'X':		/** delete repeat chars backward - dh **/
		c = 'h';
		goto deleol;

	case '~':		/** invert case and advance **/
		if( cur_virt != INVALID )
		{
			save_v();
			i = INVALID;
			while(trepeat-->0 && i!=cur_virt)
			{
				i = cur_virt;
				c = virtual[cur_virt];
#ifdef SHOPT_MULTIBYTE
				if((c&~STRIP)==0)
#endif /* SHOPT_MULTIBYTE */
				if( isupper(c) )
					c = tolower(c);
				else if( islower(c) )
					c = toupper(c);
				replace(c, 1);
			}
			return(GOOD);
		}
		else
			return(BAD);

	default:
		return(BAD);
	}
	refresh(CONTROL);
	return(GOOD);
}


#ifdef SHOPT_MULTIBYTE
    static int isalph(register int c)
    {
	register int v = virtual[c];
	return((v&~STRIP) || isalnum(v) || v=='_');
    }

    static int iswalph(register int c)
    {
	return(iswalnum(virtual[c]) || virtual[c]=='_');
    }


    static int isblank(register int c)
    {
	register int v = virtual[c];
	return((v&~STRIP)==0 && isspace(v));
    }

    static int ismetach(register int c)
    {
	register int v = virtual[c];
	return((v&~STRIP)==0 && ismeta(v));
    }

#endif	/* SHOPT_MULTIBYTE */

/*
 * get a character, after ^V processing
 */
static int getrchar()
{
	register int c;
	if((c=ed_getchar(1))== usrlnext)
		c = ed_getchar(2);
	return(c);
}
