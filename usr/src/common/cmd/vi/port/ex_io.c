/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_io.c	1.24.1.14"
#ident "$Header$"
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * File input/output, source, preserve and recover
 */

/*
 * Following remember where . was in the previous file for return
 * on file switching.
 */
int	altdot;
int	oldadot;
bool	wasalt;
short	isalt;

long	cntch;			/* Count of characters on unit io */
#ifndef VMUNIX
short	cntln;			/* Count of lines " */
#else
int	cntln;
#endif
long	cntnull;		/* Count of nulls " */
long	cntodd;			/* Count of non-ascii characters " */

static void chkmdln();

/*
 * Parse file name for command encoded by comm.
 * If comm is E then command is doomed and we are
 * parsing just so user won't have to retype the name.
 */
filename(comm)
	int comm;
{
	register int c = comm, d;
	register int i;

	d = getchar();
	if (endcmd(d)) {
		if (savedfile[0] == 0 && comm != 'f')
			error(EXmnofileid, EXmnofile);
		CP(file, savedfile);
		wasalt = (isalt > 0) ? isalt-1 : 0;
		isalt = 0;
		oldadot = altdot;
		if (c == 'e' || c == 'E')
			altdot = lineDOT();
		if (d == EOF)
			ungetchar(d);
	} else {
		ungetchar(d);
		getone();
		eol();
		if (savedfile[0] == 0 && c != 'E' && c != 'e') {
			c = 'e';
			edited = 0;
		}
		wasalt = strcmp((const char *)file, (const char *)altfile) == 0;
		oldadot = altdot;
		switch (c) {

		case 'f':
			edited = 0;
			/* fall into ... */

		case 'e':
			if (savedfile[0]) {
				altdot = lineDOT();
				CP(altfile, savedfile);
			}
			CP(savedfile, file);
			break;

		default:
			if (file[0]) {
				if (c != 'E')
					altdot = lineDOT();
				CP(altfile, file);
			}
			break;
		}
	}
	if (hush && comm != 'f' || comm == 'E')
		return;
	if (file[0] != 0) {
		lprintf("\"%s\"", file);
		if (comm == 'f') {
			if (value(vi_READONLY))
				gprintf(EXmbreadonlybid, EXmbreadonlyb);
			if (!edited)
				gprintf(":97", " [Not edited]");
			if (tchng)
				gprintf(":98", " [Modified]");
		}
		flush();
	} else
		gprintf(":99", "No file ");
	if (comm == 'f') {
		if (!(i = lineDOL()))
			i++;
		gprintf(":100", " line %d of %d --%ld%%--", lineDOT(), lineDOL(),
			(long) 100 * lineDOT() / i);
	}
}

/*
 * Get the argument words for a command into genbuf
 * expanding # and %.
 */
getargs()
{
	register int c;
	register unsigned char *cp, *fp;
	static unsigned char fpatbuf[32];	/* hence limit on :next +/pat */

	pastwh();
	if (peekchar() == '+') {
		for (cp = fpatbuf;;) {
			c = getchar();
			*cp++ = c;
			if (cp >= &fpatbuf[sizeof(fpatbuf)])
				error(":101", "Pattern too long");
			if (c == '\\' && isspace(peekchar()))
				c = getchar();
			if (c == EOF || isspace(c)) {
				ungetchar(c);
				*--cp = 0;
				firstpat = &fpatbuf[1];
				break;
			}
		}
	}
	if (skipend())
		return (0);
	CP(genbuf, "echo "); cp = &genbuf[5];
	for (;;) {
		c = getchar();
		if (endcmd(c)) {
			ungetchar(c);
			break;
		}
		switch (c) {

		case '\\':
			if (any(peekchar(), "#%|"))
				c = getchar();
			/* fall into... */

		default:
			if (cp > &genbuf[LBSIZE - 2])
flong:
				error(":102", "Argument buffer overflow");
			*cp++ = c;
			break;

		case '#':
			fp = (unsigned char *)altfile;
			if (*fp == 0)
				error(EXmnoaltfileid, EXmnoaltfile);
			goto filexp;

		case '%':
			fp = savedfile;
			if (*fp == 0)
error(":103", "No current filename|No current filename to substitute for %%");
filexp:
			while (*fp) {
				if (cp > &genbuf[LBSIZE - 2])
					goto flong;
				*cp++ = *fp++;
			}
			break;
		}
	}
	*cp = 0;
	return (1);
}

/*
 * Glob the argument words in genbuf, or if no globbing
 * is implied, just split them up directly.
 */
glob(gp)
	struct glob *gp;
{
	int pvec[2];
	register unsigned char **argv = gp->argv;
	register unsigned char *cp = gp->argspac;
	register int c;
	unsigned char ch;
	int nleft = NCARGS;

	gp->argc0 = 0;
	if (gscan() == 0) {
		register unsigned char *v = genbuf + 5;		/* strlen("echo ") */

		for (;;) {
			while (isspace(*v))
				v++;
			if (!*v)
				break;
			*argv++ = cp;
			while (*v && !isspace(*v))
				*cp++ = *v++;
			*cp++ = 0;
			gp->argc0++;
		}
		*argv = 0;
		return;
	}
	if (pipe(pvec) < 0)
		error(":104", "Cannot make pipe to glob");
	pid = fork();
	io = pvec[0];
	if (pid < 0) {
		close(pvec[1]);
		error(":105", "Cannot fork to do glob");
	}
	if (pid == 0) {
		int oerrno;

		close(1);
		dup(pvec[1]);
		close(pvec[0]);
		close(2);	/* so errors don't mess up the screen */
		open("/dev/null", 1);
		execlp((char *)svalue(vi_SHELL), "sh", "-c", genbuf, (char *)0);
		oerrno = errno; close(1); dup(2); errno = oerrno;
		filioerr(svalue(vi_SHELL));
	}
	close(pvec[1]);
	do {
		*argv = cp;
		for (;;) {
			if (read(io, &ch, 1) != 1) {
				close(io);
				c = -1;
			} else
				c = ch;
			if (c <= 0 || isspace(c))
				break;
			*cp++ = c;
			if (--nleft <= 0)
				error(EXmargtoolongid, EXmargtoolong);
		}
		if (cp != *argv) {
			--nleft;
			*cp++ = 0;
			gp->argc0++;
			if (gp->argc0 >= NARGS)
				error(EXmargtoolongid, EXmargtoolong);
			argv++;
		}
	} while (c >= 0);
	waitfor();
	if (gp->argc0 == 0)
		error(":106", "No match");
}

/*
 * Scan genbuf for shell metacharacters.
 * Set is union of v7 shell and csh metas.
 */
gscan()
{
	register unsigned char *cp;

	for (cp = genbuf; *cp; cp++)
		if (any(*cp, "~{[*?$`'\"\\"))
			return (1);
	return (0);
}

/*
 * Parse one filename into file.
 */
struct glob G;
getone()
{
	register unsigned char *str;

	if (getargs() == 0)
		error(":107", "Missing filename");
	glob(&G);
	if (G.argc0 > 1)
		error(":108", "Ambiguous|Too many file names");
	str = G.argv[G.argc0 - 1];
	if (str)
		if (strlen((const char *)str) > FNSIZE - 4)
			error(":109", "Filename too long");
samef:
	if (!str) str = (unsigned char *) "";
	CP(file, str);
}

/*
 * Read a file from the world.
 * C is command, 'e' if this really an edit (or a recover).
 */
rop(c)
	int c;
{
	register int i;
	struct stat stbuf;
	short magic;
	static int ovro;	/* old value(vi_READONLY) */
	static int denied;	/* 1 if READONLY was set due to file permissions */

	if (!one) {
		/* before init() ($EXINIT or $HOME/.exrc) */
		return;
	}

	io = open(file, 0);
	if (io < 0) {
		if (c == 'e' && errno == ENOENT) {
			edited++;
			/* If user has previously edited a R/O file, 
			 * cannel the vi_READONLY flag to prevent a 
			 * spurious error when he attempts to write
			 * his new file.  Check denied in case vi -R /
			 * view was the reason the flag is set, otherwise
			 * if he edits another file, the readonly flag
			 * won't be set any more.
			 */
			if (denied && seenprompt)
				value(vi_READONLY) = 0;
			/*
			 * If the user just did "ex foo" he is probably
			 * creating a new file.  Don't be an error, since
			 * this is ugly, and it messes up the + option.
			 */
			if (!seenprompt) {
				gprintf(EXmnewfileid, EXmnewfile);
				noonl();
				return;
			}
			/*
			 * If we were editing a readonly file,
			 * restore the old readonly value and 
			 * turn off the denied flag. So we won't
			 * think a new file is readonly later. 
			 */ 
			if (value(vi_READONLY) && denied) {
				value(vi_READONLY) = ovro;
				denied = 0;
			}
		}
		syserror(0);
	}
	if (fstat(io, &stbuf))
		syserror(0);
	switch (FTYPE(stbuf) & S_IFMT) {

	case S_IFBLK:
		error(":110", " Block special file");

	case S_IFCHR:
		if (isatty(io))
			error(":111", " Teletype");
		if (samei(&stbuf, "/dev/null"))
			break;
		error(":112", " Character special file");

	case S_IFDIR:
		error(":113", " Directory");

	}
	if (c != 'r') {
		if (value(vi_READONLY) && denied) {
			value(vi_READONLY) = ovro;
			denied = 0;
		}
		if ((FMODE(stbuf) & 0222) == 0 || access((char *)file, 2) < 0) {
			ovro = value(vi_READONLY);
			denied = 1;
			value(vi_READONLY) = 1;
		}
	}
	if (hush == 0 && value(vi_READONLY)) {
		gprintf(EXmbreadonlybid, EXmbreadonlyb);
		flush();
	}
	if (c == 'r')
		setdot();
	else
		setall();

	/* If it is a read command, then we must set dot to addr1
	 * (value of N in :Nr ).  In the default case, addr1 will
	 * already be set to dot.
	 * 
	 * Next, it is necessary to mark the beginning (undap1) and
	 * ending (undap2) addresses affected (for undo).  Note that
	 * rop2() and rop3() will adjust the value of undap2.
	 */
	if (FIXUNDO && inopen && c == 'r') {
		dot = addr1;
		undap1 = undap2 = dot + 1;
	}
	rop2();
	rop3(c);
}

rop2()
{
	line *first, *last, *a;

	deletenone();
	clrstats();
	first = addr2 + 1;
	(void)append(getfile, addr2);
	last = dot;
	if (value(vi_MODELINES))
		for (a=first; a<=last; a++) {
			if (a==first+5 && last-first > 10)
				a = last - 4;
			getline(*a);
				chkmdln(linebuf);
		}
}

rop3(c)
	int c;
{

	if (iostats() == 0 && c == 'e')
		edited++;
	if (c == 'e') {
		if (wasalt || firstpat) {
			register line *addr = zero + oldadot;

			if (addr > dol)
				addr = dol;
			if (firstpat) {
				globp = (*firstpat) ? firstpat : (unsigned char *)"$";
				commands(1,1);
				firstpat = 0;
			} else if (addr >= one) {
				if (inopen)
					dot = addr;
				markpr(addr);
			} else
				goto other;
		} else
other:
			if (dol > zero) {
				if (inopen)
					dot = one;
				markpr(one);
			}
		if(FIXUNDO)
			undkind = UNDNONE;
		if (inopen) {
			vcline = 0;
			vreplace(0, lines, lineDOL());
		}
	}
	if (laste) {
#ifdef VMUNIX
		tlaste();
#endif
		laste = 0;
		esync();
	}
}

/*
 * Are these two really the same inode?
 */
samei(sp, cp)
	struct stat *sp;
	unsigned char *cp;
{
	struct stat stb;

	if (stat((char *)cp, &stb) < 0) 
		return (0);
	return (IDENTICAL((*sp), stb));
}

/* Returns from edited() */
#define	EDF	0		/* Edited file */
#define	NOTEDF	-1		/* Not edited file */
#define	PARTBUF	1		/* Write of partial buffer to Edited file */

/*
 * Write a file.
 */
wop(dofname)
bool dofname;	/* if 1 call filename, else use savedfile */
{
	register int c, exclam, nonexist;
	line *saddr1, *saddr2;
	struct stat stbuf;

	c = 0;
	exclam = 0;
	if (dofname) {
		if (peekchar() == '!')
			exclam++, ignchar();
		(void)skipwh();
		while (peekchar() == '>')
			ignchar(), c++, (void)skipwh();
		if (c != 0 && c != 2)
			error(":114", "Write forms are 'w' and 'w>>'");
		filename('w');
	} else {
		if (savedfile[0] == 0)
			error(EXmnofileid, EXmnofile);
		saddr1=addr1;
		saddr2=addr2;
		addr1=one;
		addr2=dol;
		CP(file, savedfile);
		if (inopen) {
			vclrech(0);
			splitw++;
		}
		lprintf("\"%s\"", file);
	}
	nonexist = stat((char *)file, &stbuf);
	switch (c) {

	case 0:
		if (!exclam && (!value(vi_WRITEANY) || value(vi_READONLY)))
		switch (edfile()) {
		
		case NOTEDF:
			if (nonexist)
				break;
			if (ISCHR(stbuf)) {
				if (samei(&stbuf, "/dev/null"))
					break;
				if (samei(&stbuf, "/dev/tty"))
					break;
			}
			io = open(file, 1);
			if (io < 0)
				syserror(0);
			if (!isatty(io))
				serror(":115", 
					" File exists| File exists - use \"w! %s\" to overwrite",
					file);
			close(io);
			break;

		case EDF:
			if (value(vi_READONLY))
				error(EXmronlyid, EXmronly);
			break;

		case PARTBUF:
			if (value(vi_READONLY))
				error(EXmronlyid, EXmronly);
			error(":116", " Use \"w!\" to write partial buffer");
		}
cre:
/*
		synctmp();
*/
		io = creat(file, 0666);
		if (io < 0)
			syserror(0);
		writing = 1;
		if (hush == 0)
			if (nonexist)
				gprintf(EXmnewfileid, EXmnewfile);
			else if (value(vi_WRITEANY) && edfile() != EDF)
				gprintf(":117", " [Existing file]");
		break;

	case 2:
		io = open(file, 1);
		if (io < 0) {
			if (exclam || value(vi_WRITEANY))
				goto cre;
			syserror(0);
		}
		lseek(io, 0l, 2);
		break;
	}
	if (write_quit && inopen && (argc == 0 || morargc == argc))
		setty(normf);
	putfile(0);
	(void)iostats();
	if (c != 2 && addr1 == one && addr2 == dol) {
		if (eq(file, savedfile))
			edited = 1;
		esync();
	}
	if (!dofname) {
		addr1 = saddr1;
		addr2 = saddr2;
	}
	writing = 0;
}

/*
 * Is file the edited file?
 * Work here is that it is not considered edited
 * if this is a partial buffer, and distinguish
 * all cases.
 */
edfile()
{

	if (!edited || !eq(file, savedfile))
		return (NOTEDF);
	return (addr1 == one && addr2 == dol ? EDF : PARTBUF);
}

/*
 * Extract the next line from the io stream.
 */
unsigned char *nextip;

getfile()
{
	register short c;
	register unsigned char *lp; 
	register unsigned char *fp;

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			ninbuf = read(io, genbuf, LBSIZE) - 1;
			if (ninbuf < 0) {
				if (lp != linebuf) {
					lp++;
					gprintf(":118", 
						" [Incomplete last line]");
					break;
				}
				return (EOF);
			}
			if(crflag == -1) {
				if(isencrypt(genbuf, ninbuf + 1))
					crflag = 2;
				else
					crflag = -2;
			}
			if (crflag > 0 && run_crypt(cntch, genbuf, ninbuf+1, perm) == -1) {
smerror(":119", "Cannot decrypt block of text\n");
					break;
			}
			fp = genbuf;
			cntch += ninbuf+1;
		}
		if (lp >= &linebuf[LBSIZE]) {
			error(":120", " Line too long");
		}
		c = *fp++;
		if (c == 0) {
			cntnull++;
			continue;
		}
		*lp++ = c;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	cntln++;
	return (0);
}

/*
 * Write a range onto the io stream.
 */
putfile(isfilter)
int isfilter;
{
	line *a1;
	register unsigned char *lp;
	register unsigned char *fp;
	register int nib;
	register bool ochng = chng;

	chng = 1;		/* set to force file recovery procedures in */
				/* the event of an interrupt during write   */
	a1 = addr1;
	clrstats();
	cntln = addr2 - a1 + 1;
	nib = BUFSIZ;
	fp = genbuf;
	while (a1 <= addr2) {
		getline(*a1++);
		lp = linebuf;
		for (;;) {
			if (--nib < 0) {
				nib = fp - genbuf;
                		if(kflag && !isfilter)
                                        if (run_crypt(cntch, genbuf, nib, perm) == -1)
					  wrerror();
				if (write(io, genbuf, nib) != nib) {
					if(!isfilter)
						wrerror();
					return;
				}
				cntch += nib;
				nib = BUFSIZ - 1;
				fp = genbuf;
			}
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	}
	nib = fp - genbuf;
	if(kflag && !isfilter)
		if (run_crypt(cntch, genbuf, nib, perm) == -1)
			wrerror();
	if ((cntch == 0) && (nib == 1)) {
		cntln = 0;
		return;
	}
	if (write(io, genbuf, nib) != nib) {
		if(!isfilter)
			wrerror();
		return;
	}
	cntch += nib;
	chng = ochng;			/* reset chng to original value */
}

/*
 * A write error has occurred;  if the file being written was
 * the edited file then we consider it to have changed since it is
 * now likely scrambled.
 */
wrerror()
{

	if (eq(file, savedfile) && edited)
		change();
	syserror(1);
}

/*
 * Source command, handles nested sources.
 * Traps errors since it mungs unit 0 during the source.
 */
short slevel;
short ttyindes;

source(fil, okfail, sb)
	unsigned char *fil;
	bool okfail;
	struct stat *sb;
{
	jmp_buf osetexit;
	register int saveinp, ointty, oerrno;
	unsigned char *saveglobp;
	short savepeekc;

	/* 
	 * Check that the .exrc file is owned by the user and
	 * is only writable by the user.
	 */
	if (sb != NULL) {
		if (sb->st_uid != geteuid()) {
			werror(":286", "%s not owned by you - ignoring\n",fil);
			return(0);
		}

		if (sb->st_mode & (S_IWGRP|S_IWOTH)) {
			werror(":287", "%s writable by others - ignoring\n",fil);
			return(0);
		}
	}

	signal(SIGINT, SIG_IGN);
	saveinp = dup(0);
	savepeekc = peekc;
	saveglobp = globp;
	peekc = 0; globp = 0;
	if (saveinp < 0)
		error(":121", "Too many nested sources");
	if (slevel <= 0)
		ttyindes = saveinp;
	close(0);
	if (open(fil, 0) < 0) {
		oerrno = errno;
		setrupt();
		dup(saveinp);
		close(saveinp);
		errno = oerrno;
		if (!okfail)
			filioerr(fil);
		return;
	}
	slevel++;
	ointty = intty;
	intty = isatty(0);
	oprompt = value(vi_PROMPT);
	value(vi_PROMPT) &= intty;
	getexit(osetexit);
	setrupt();
	if (setexit() == 0)
		commands(1, 1);
	else if (slevel > 1) {
		close(0);
		dup(saveinp);
		close(saveinp);
		slevel--;
		resexit(osetexit);
		reset();
	}
	intty = ointty;
	value(vi_PROMPT) = oprompt;
	close(0);
	dup(saveinp);
	close(saveinp);
	globp = saveglobp;
	peekc = savepeekc;
	slevel--;
	resexit(osetexit);
}

/*
 * Clear io statistics before a read or write.
 */
clrstats()
{

	ninbuf = 0;
	cntch = 0;
	cntln = 0;
	cntnull = 0;
	cntodd = 0;
}

/*
 * Io is finished, close the unit and print statistics.
 */
iostats()
{

	close(io);
	io = -1;
	if (hush == 0) {
		if (value(vi_TERSE))
			gprintf(":122", " %d/%ld", cntln, cntch);
		else {
			if (cntln == 1)
				gprintf(":123", " 1 line, ");
			else
				gprintf(":124", " %d lines, ", cntln);
			if (cntch == 1)
				gprintf(":125", "1 character");
			else
				gprintf(":126", "%ld characters", cntch);
		}
		if (cntnull || cntodd) {
			printf(" (");
			if (cntnull) {
				gprintf(":127", "%ld null", cntnull);
				if (cntodd)
					printf(", ");
			}
			if (cntodd)
				gprintf(":128", "%ld non-ASCII", cntodd);
			putchar(')');
		}
		noonl();
		flush();
	}
	return (cntnull != 0 || cntodd != 0);
}


static void chkmdln(aline)
unsigned char *aline;
{
	unsigned char *beg, *end;
	unsigned char cmdbuf[1024];
	char *strchr(), *strrchr();
	bool savetty;
	int  savepeekc;

	beg = (unsigned char *)strchr((char *)aline, ':');
	if (beg == NULL)
		return;
	if (!((beg[-2] == 'e' && beg[-1] == 'x')
	||    (beg[-2] == 'v' && beg[-1] == 'i')))
		 return;

	strncpy((char *)cmdbuf, (const char *)(beg+1), sizeof cmdbuf);
	end = (unsigned char *)strrchr(cmdbuf, ':');
	if (end == NULL)
		return;
	*end = 0;
	globp = cmdbuf;
	savepeekc = peekc;
	peekc = 0;
	savetty = intty;
	intty = 0;
	commands(1, 1);
	peekc = savepeekc;
	globp = 0;
	intty = savetty;
}
