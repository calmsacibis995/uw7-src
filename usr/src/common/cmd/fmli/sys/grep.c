/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/grep.c	1.10.3.5"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
/*
 * fmlgrep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		TRUE - ok, and some matches
 *		FALSE - no matches or error 
 */

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <regexpr.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "wish.h"
#include "ctl.h"
#include "eval.h"
#include "moremacros.h"
#include "message.h"

struct { const char *id, *msg; } errstr[] = {
	":251", "Range endpoint too large" ,
	":252", "Bad number" ,
	":253", "`\\digit' out of range" ,
	":254", "No remembered search string" ,
	":255", "'\\( \\)' imbalance" ,
	":256", "Too many `\\(' s" ,
	":257", "More than two numbers given in '\\{ \\}'" ,
	":258", "'\\}' expected" ,
	":259", "First number exceeds second in '\\{ \\}'" ,
	":260", "'[ ]' imbalance" ,
	":261", "Regular expression overflow" ,
	":262", "Illegal byte sequence",
	":263", "Regular expression error",
};


/*
 * Macros for FMLI i/o and FMLI error messages (rjk)
 */ 
static	char	tmpbuf[BUFSIZ];		/* for formatting purposes */

#define PRINTF(x, y)	{ \
				 sprintf(tmpbuf, (x), (y)); \
				 putastr(tmpbuf, Outstr); \
				}

/* changed mess_tmp to putastr in errmsg below.  abs s14 */
#define	errmsg(msg, arg1, arg2)	{ \
				 sprintf(tmpbuf, (msg), (arg1), (arg2)); \
				 putastr(tmpbuf, Errstr); \
				}

static int execute();
static int succeed();



extern char	*strrchr();
static int	temp;
static long	lnum;
static char	linebuf[2*BUFSIZ];
static char	prntbuf[2*BUFSIZ];
static int	nflag;
static int	bflag;
static int	lflag;
static int	cflag;
static int	vflag;
static int	sflag;
static int	iflag;
static int	hflag;
static int	errflg;
static int	nfile;
static long	tln;
static int	nsucc;
static int	nlflag;
static char	*ptr, *ptrend;
static char *expbuf;
static char *pref_s, *pref_ld;

static IOSTRUCT *Instr;
static IOSTRUCT *Outstr;
static IOSTRUCT *Errstr;


static int bytes_read; /* used in fmlilseek() */

static int
_fmliread (fd, buf, nr)
register int fd;
register char *buf;
register int nr;
{
register int i = 0;
register int result = 0;
    
#ifdef DEBUG
    fprintf (stderr, "_fmliread (%d, 0x%x, %d)\n", fd, buf, nr);
#endif
    if ( fd )
      {
	while ( nr > 0 && (i = read(fd, buf, nr)) > 0 )
	  {
	    nr -= i;
	    buf += i;
	    result += i;
	  }
	if ( result == 0 )
	    return (i);
	else
	  {
#ifdef DEBUG
	    fprintf (stderr, "_fmliread return %d\n", result);
#endif
	    return (result);
	  }
      } /* END OF IF (fd) */
    else
      { /* reading from stdin
	** in fmli this means reading from Instr 
	*/
	while ( nr > 0 && getastr(buf, nr, Instr) &&
		(i = strlen (buf)) > 0 )
	  {
	    nr -= i;
	    buf += i;
	    result += i;
	  }
	bytes_read += result;
#ifdef DEBUG
        fprintf (stderr, "_fmliread return %d\n", result);
#endif
	return (result);
      }
}

static long
_fmlilseek (fd, off, from)
int fd;
long off;
int from;
{
#ifdef DEBUG
    fprintf (stderr, "_fmlilseek(%d, %d, %d)\n", fd, off, from);
#endif
    if ( fd )
	return (lseek(fd, off, from));
    else
      {
	if ( off != 0L && from != SEEK_SET)
	    return (-1);
	switch (from)
	  {
	    case SEEK_CUR:
		return (bytes_read);
	    case SEEK_SET:
		io_seek(Instr, off);
		return (off);
	    default:
		return (-1);
	  } /* END OF SWITCH (off) */
      }
}


cmd_grep(argc, argv, instr, outstr, errstr)
int	argc;
char	*argv[];
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;
{
	register	c;
	register char	*arg;
	extern int	optind;
	extern int      opterr, optopt;		/* abs s14 */
	void		regerr();

#ifdef DEBUG
	fprintf (stderr, "cmd_grep(): argc=%d\n", argc);
	for (c = 0 ; c < argc ; ++c )
	      fprintf (stderr, "argv[%d]=\"%s\"\n", c, argv[c]);
#endif

				/* moved from below.  abs s15 */
	Instr = instr;		/* rjk */
	Outstr = outstr;	/* rjk */
	Errstr = errstr;	/* abs s14 */

	prntbuf[0] = linebuf[0] = '\0';
	nflag = bflag = lflag = cflag = vflag = 0;
	sflag = iflag = hflag = errflg = nfile = nsucc = 0;
	lnum = tln = 0;
	ptr = ptrend = expbuf = pref_s = pref_ld = NULL;
	optind = 1;
	opterr = 0;             		/* abs s14 */
	bytes_read = 0;

	while((c=getopt(argc, argv, "hblcnsviy")) != -1)
		switch(c) {
		case 'h':
			hflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'b':
			bflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'y':
		case 'i':
			iflag++;
			break;
		case '?':
			errflg++;
		}

	if(errflg || (optind >= argc)) {
		/*
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":0:Incorrect usage\n");
		*/
		errmsg (gettxt (":264", "Usage: fmlgrep -hblcnsvi pattern file . . .\n"), (char*)NULL, (char*)NULL);
		/* exit(2); */
		return (FAIL);
	}


	argv = &argv[optind];
	argc -= optind;
	nfile = argc - 1;

	if (strrchr(*argv,'\n'))
	  {
		regerr(41);
		return (FAIL);
	  }

	if (iflag) {
		for(arg = *argv; *arg != NULL; ++arg)
			*arg = (char)tolower((int)((unsigned char)*arg));
	}


	expbuf = compile(*argv, (char *)0, (char *)0);
	if(regerrno)
		regerr(regerrno);

	if (--argc == 0)
		execute((char *)NULL);
	else
		while (argc-- > 0)
			execute(*++argv);

	free (expbuf);
	/* exit(nsucc == 2 ? 2 : nsucc == 0); */
	return((nsucc == 2 || nsucc == 0) ? FAIL : SUCCESS);
}

static 
execute(file)
register char *file;
{
	register char *lbuf, *p;
	int count, count1;
	
	if (file == NULL)
		temp = 0;
	else if ((temp = open(file, 0)) == -1) {
		if (!sflag)
			errmsg (gettxt (":265", "fmlgrep: Cannot open %s: %s\n"), 
				file, strerror(errno));
		nsucc = 2;
		return;
	}
	/* read in first block of bytes */
	if((count = _fmliread(temp, prntbuf, BUFSIZ)) <= 0) {
		if (temp != 0)
			close(temp);

		if (cflag) {
			if (nfile>1 && !hflag && file)
				PRINTF (pref_s ? pref_s :
					(pref_s = gettxt(":266", "%s:")), file);
			PRINTF ("%ld\n", tln);
		}
		return;
	}
		
	lnum = 0;
	tln = 0;
	ptr = prntbuf;
	for(;;) {
		/* look for next newline */
		if((ptrend = memchr(ptr, '\n', prntbuf + count - ptr)) == NULL) {
			count = prntbuf + count - ptr;
			if(count <= BUFSIZ) {
				/* 
				 * shift end of block to beginning of buffer
				 * if necessary
				 * and fill up buffer until newline 
				 * is found 
				 */
				if(ptr != prntbuf)
				/* assumes memcpy copies correctly with overlap */
					memmove(prntbuf, ptr, count);
				p = prntbuf + count;
				ptr = prntbuf;
			} else {
				/*
				 * No newline in current block.
				 * Throw it away and get next
				 * block.
				 */
				count = 0;
				ptr = p = prntbuf;
			}
			if((count1 = _fmliread(temp, p, BUFSIZ)) > 0) {
				count += count1;
				continue;
			}
			/* end of file - last line has no newline */
			ptrend = ptr + count;
			nlflag = 0;
		} else
			nlflag = 1;
		lnum++;
		*ptrend = '\0';
		if (iflag) {
			p = ptr;
			for(lbuf=linebuf; p < ptrend; )
				*lbuf++ = (char)tolower((int)(unsigned char)*p++);
			*lbuf = '\0';
			lbuf = linebuf;
		} else
			lbuf = ptr;

		if((step(lbuf, expbuf) ^ vflag) && succeed(file) == 1)
			break;	/* lflag only once */
		if(!nlflag)
			break;
		ptr = ptrend + 1;
		if(ptr >= prntbuf + count) {
			/* at end of block; read in another block */
			ptr = prntbuf;
			if((count = _fmliread(temp, prntbuf, BUFSIZ)) <= 0)
				break;
		}
	}
	if (temp)
	    close(temp);

	if (cflag) {
		if (nfile>1 && !hflag && file)
			PRINTF (pref_s ? pref_s : 
					  (pref_s = gettxt(":266", "%s:")),
				file);
		PRINTF ("%ld\n", tln);
	}
	return;
}

static
succeed(f)
register char *f;
{
	int nchars;
	nsucc = (nsucc == 2) ? 2 : 1;
	if (f == NULL)
		f = "<stdin>";
	if (cflag) {
		tln++;
		return(0);
	}
	if (lflag) {
		PRINTF ("%s\n", f);
		return(1);
	}

	if (nfile > 1 && !hflag)	/* print filename */
		PRINTF (pref_s ? pref_s : (pref_s = gettxt(":266", "%s:")), f);

	if (bflag)	/* print block number */
		PRINTF (pref_ld ? pref_ld : (pref_ld = gettxt(":267", "%ld:")),
			(_fmlilseek(temp, 0L, 1)-1)/BUFSIZ);

	if (nflag)	/* print line number */
		PRINTF (pref_ld ? pref_ld : (pref_ld = gettxt(":267", "%ld:")), lnum);
	if(nlflag) {
		/* newline at end of line */
		*ptrend = '\n';
		nchars = ptrend - ptr + 1;
	} else
		nchars = ptrend - ptr;
#ifdef notdef
	/* old code of unix grep command */
	fwrite(ptr, 1, nchars, stdout);
#else
	/* new for fmlgrep */
	{
	register char *p;

	p = ptr;
	while ( nchars-- > 0 )
	      putac (*p++, Outstr);
	}
#endif
	return(0);
}

static void
regerr(err)
register err;
{
	switch(err) {
		case 11:
			err = 0;
			break;
		case 16:
			err = 1;
			break;
		case 25:
			err = 2;
			break;
		case 41:
			err = 3;
			break;
		case 42:
			err = 4;
			break;
		case 43:
			err = 5;
			break;
		case 44:
			err = 6;
			break;
		case 45:
			err = 7;
			break;
		case 46:
			err = 8;
			break;
		case 49:
			err = 9;
			break;
		case 50:
			err = 10;
			break;
		case 67:
			err = 11;
			break;
		default:
			err = 12;
			break;
	}
	errmsg (gettxt (":268", "RE error %d: %s\n"), err,
		gettxt(errstr[err].id, errstr[err].msg));

	/* exit(2); */
}
