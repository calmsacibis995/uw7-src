#ident	"@(#)debugger:libedit/common/ksh_fc.C	1.7"

#include "Interface.h"
#include "Parser.h"
#include "Buffer.h"
#include "Input.h"
#include "utility.h"
#include "global.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include "history.h"
#include "io.h"
#include "national.h"


#define	E_FLAG		01
#define	L_FLAG		02
#define	N_FLAG		04
#define	R_FLAG		010
#define BAD_FLAG	-1

static int 		hist_subst(int, char *);
static void		sh_substitute(const char *, const char *,
				char *, Buffer *);
static int		flagset(char *);
static int		argnum;

int 
ksh_fc(char *cmd)
{
	int	i;
	int 	flag = 0;
	int	fdo;
	int	index2;
	int	indx = -1; /* used as subscript for range */
	int	incr;
	int	range[2];	/* upper and lower range of commands */
	int	lflag = 0;
	int	nflag = 0;
	int	rflag = 0;
	int	exit_val = 1;
	char	*a1;
	char	*comarray[10];	/* arg ptrs */
	char	**com;		/* arg ptrs */
	char	*cptr1;
	char	*edit = NULL;		/* name of editor */
	char	*replace = NULL;		/* replace old=new */
	char	fname[TMPSIZ];
	struct 	history *fp;
	histloc location;

	com = comarray;
	argnum = 0;
	/* break command up into strings */
	cptr1 = cmd;
	i = 0;
	while(cptr1 && *cptr1 && i < 10)
	{
		char	*cptr2;

		com[i++] = cptr1;
		cptr2 = strchr(cptr1, ' ');
		if (cptr2)
		{
			*cptr2 = 0;
			cptr1 = cptr2 + 1;
		}
		else
			break;
	}
	com[i] = 0;
	if (!hist_open())
	{
		return 0;
	}
	fp = hist_ptr;
	while((a1 = com[0]) && *a1 == '-')
	{
		argnum = -1;
		if ((flag = flagset(a1)) == BAD_FLAG)
			return 0;
		if (flag == 0)
		{
			flag = fp->fixind - argnum-1;
			if (flag <= 0)
				flag = 1;
			range[++indx] = flag;
			argnum = 0;
			if (indx == 1)
				break;
		}
		else
		{
			if (flag & E_FLAG)
			{
				/* name of editor specified */
				com++;
				if ((edit = com[0]) == NULL)
				{
					printe(ERR_fc_syntax, E_ERROR);
					return 0;
				}
			}
			if (flag & N_FLAG)
				nflag++;
			if (flag & L_FLAG)
				lflag++;
			if (flag & R_FLAG)
				rflag++;
		}
		com++;
	}
	flag = indx;
	while (flag < 1 && (a1 = com[0]))
	{
		/* look for old=new argument */
		if (replace == NULL && strchr(a1+1, '='))
		{
			replace = a1;
			com++;
			continue;
		}
		else if (isdigit(*a1) || *a1 == '-')
		{
			/* see if completely numeric */
			do	a1++;
			while(isdigit(*a1));
			if (*a1 == 0)
			{
				a1 = com[0];
				range[++flag] = atoi(a1);
				if (*a1 == '-')
					range[flag] += (fp->fixind-1);
				com++;
				continue;
			}
		}
		/* search for last line starting with string */
		location = hist_find(com[0], fp->fixind-1, 0, -1);
		if ((range[++flag] = location.his_command) < 0)
		{
			printe(ERR_fc_found, E_ERROR);
			return 0;
		}
		com++;
	}
	if (flag <0)
	{
		/* set default starting range */
		if (lflag)
		{
			flag = fp->fixind-16;
			if (flag < 1)
				flag = 1;
		}
		else
			flag = fp->fixind-2;
		range[0] = flag;
		flag = 0;
	}
	if (flag == 0)
		/* set default termination range */
		range[1] = (lflag ? fp->fixind-1 : range[0]);
	if ((index2 = fp->fixind - fp->fixmax) <= 0)
		index2 = 1;
	/* check for valid ranges */
	for (flag = 0; flag < 2; flag++)
	{
		if (range[flag] < index2 ||
			range[flag] >= (fp->fixind- (lflag == 0)))
			{
				printe(ERR_fc_range, E_ERROR);
				return 0;
			}
	}
	if (edit && *edit =='-' && range[0] != range[1])
	{
		printe(ERR_fc_range, E_ERROR);
		return 0;
	}
	/* now list commands from range[rflag] to range[1-rflag] */
	incr = 1;
	flag = rflag > 0;
	if (range[1-flag] < range[flag])
		incr = -1;
	if (lflag)
	{
		fdo = 1; /* stdout */
		a1 = "\n\t";
	}
	else
	{
		fdo = io_mktmp(fname);
		a1 = "\n";
		nflag++;
	}
	p_setout(fdo);
	while(1)
	{
		/* write commands to stdout or temp file */
		if (nflag==0)
			p_num(range[flag],'\t');
		else if (lflag)
			p_char('\t');
		hist_list(hist_position(range[flag]), 0, a1);
		if (range[flag] == range[1-flag])
			break;
		range[flag] += incr;
	}
	p_setout(ERRIO);
	if (lflag)
		return 1;
	io_fclose(fdo);
	hist_eof();
	a1 = edit;
	if (a1 == NULL && (a1 = getenv("FCEDIT")) == NULL)
		a1 = "/usr/bin/ed";
	if (*a1 != '-')
	{
		/* now edit the commands */
		char	*ecmd;
		ecmd = new(char[strlen(a1) + strlen(fname) + 2]);
		sprintf(ecmd, "%s %s", a1, fname);
		exit_val = do_shell(ecmd, 0);
		delete(ecmd);
	}
	fdo = io_fopen(fname);
	/* don't history fc itself */
	hist_cancel();
	if (replace != NULL)
	{
		exit_val = hist_subst(fdo, replace);
	}
	else if (exit_val == 1)
	{
		/* read in and run the commands */
		InputFile(fdo, 0, 1);
		doscript();
		CloseInput();
		exit_val = (cmd_result == 0)  ? 1 : 0;
	}
	io_fclose(fdo);
	unlink(fname);
	return exit_val;
}

static const char flgchar[] = "elnr";
static const int flgval[] = {E_FLAG,L_FLAG,N_FLAG, R_FLAG };
/*
 * process option flags for built-ins
 * flagmask are the invalid options
 */

static int
flagset(char *flaglist)
{
	int	flag = 0;
	int	c;
	char	*cp, *sp;
	int	numset = 0;
	int	flagmask = ~(E_FLAG|L_FLAG|N_FLAG|R_FLAG);

	for(cp = flaglist+1; c = *cp; cp++)
	{
		if (isdigit(c))
		{
			if (argnum < 0)
			{
				argnum = 0;
				numset = -100;
			}
			else
				numset++;
			argnum = 10*argnum + (c - '0');
		}
		else if (sp = strchr(flgchar, c))
			flag |= flgval[sp- (char *)flgchar];
		else if (c != *flaglist)
		{
			printe(ERR_fc_syntax, E_ERROR);
			return BAD_FLAG;
		}
	}
	if (numset > 0 && flag == 0)
	{
		printe(ERR_fc_syntax, E_ERROR);
		return BAD_FLAG;
	}
	if ((flag & flagmask) == 0)
		return(flag);

	printe(ERR_fc_syntax, E_ERROR);
	return BAD_FLAG;
}

/*
 * given a file containing a command and a string of the form old=new,
 * execute the command with the string old replaced by new
 */

static int 
hist_subst(int fd, char *replace)
{
	char		*newstr = replace;
	char		*sp;
	int		c;
	struct fileblk	fb;
	char		inbuff[IOBSIZE+1];
	char		*string;
	Buffer		*buf1 = buf_pool.get();
	Buffer		*buf2 = buf_pool.get();

	while(*++newstr != '=')
		; /* skip to '=' */
	io_init(fd, &fb, inbuff);
	buf1->clear();

	while ((c = io_getc(fd)) != EOF)
		buf1->add(c);
	string = (char *)*buf1;
	io_fclose(fd);
	*newstr++ =  0;
	sh_substitute(string, replace, newstr, buf2);
	buf_pool.put(buf1);
	if (buf2->size() == 0)
	{
		printe(ERR_fc_found, E_ERROR);
		buf_pool.put(buf2);
		return 0;
	}
	sp = (char *)*buf2;
	*(newstr-1) =  '=';
	p_setout(hist_ptr->fixfd);
	p_str(sp, 0);
	hist_flush();
	p_setout(ERRIO);
	printm(MSG_input_line, Pprompt, sp);
	parse_and_execute(sp);
	buf_pool.put(buf2);
	return((cmd_result == 0) ? 1 : 0);
}

/*
  look for the substring <old> in <string> and replace with <newstr>
	assume string!=NULL && old!=NULL && new!=NULL;
	return x satisfying x==NULL ||
		strlen(x)==(strlen(in string)+strlen(in new)-strlen(in old));
*/

static void
sh_substitute(const char *string, const char *old, 
	char *newstr, Buffer *buf)
{
	const char	*sp = string;
	const char	*cp;
	const char	*savesp = NIL;

	buf->clear();
	if (*sp == 0)
		return;
	if (*(cp = old) == 0)
		goto found;
	do
	{
	/* skip to first character which matches start of old */
		while(*sp && (savesp == sp || *sp != *cp))
		{
#ifdef MULTIBYTE
			/* skip a whole character at a time */
			int c = *sp;
			c = echarset(c);
			c = in_csize(c) + (c >= 2);
			while(c-- > 0)
#endif /* MULTIBYTE */
			buf->add(*sp++);
		}
		if (*sp == 0)
		{
			buf->clear();
			return;
		}
		savesp = sp;
	        for(; *cp; cp++)
		{
			if (*cp != *sp++)
				break;
		}
		if (*cp == 0)
		/* match found */
			goto found;
		sp = savesp;
		cp = old;
	}
	while(*sp);
	buf->clear();
	return;
found:
	/* copy new */
	buf->add(newstr);
	buf->add(sp);
}
