/*	copyright	"%c%"	*/

#ident	"@(#)grep:grep.c	1.22.3.14"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>

static const char badopen[] = ":4:Cannot open %s: %s\n";

static void *
#ifdef __STDC__
alloc(void *p, size_t sz)
#else
alloc(p, sz)void *p; size_t sz;
#endif
{
	if ((p = realloc(p, sz)) == 0)
	{
		pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n",
			strerror(errno));
		exit(2);
	}
	return p;
}

struct list
{
	char		*str;
	struct list	*next;
};

static struct list *flist, *slist;
static size_t totlen;

static void
#ifdef __STDC__
regexp(char *str, int isfile)
#else
regexp(str)char *str; int isfile;
#endif
{
	struct list *lp = (struct list *)alloc((void *)0, sizeof(struct list));

	lp->str = str;
	if (isfile)
	{
		lp->next = flist;
		flist = lp;
	}
	else
	{
		totlen += strlen(str) + 1;
		lp->next = slist;
		slist = lp;
	}
}

static unsigned long Nmax = ULONG_MAX;	/* close enough to infinity */

static int prflags;
#define PRF_BLOCK	0x001	/* print block number */
#define PRF_NUMBER	0x002	/* print line number */
#define PRF_CONTENT	0x004	/* print the line */

static int optflags;
#define OPT_EMPTYPAT	0x001	/* empty pattern specified */
#define OPT_JUSTCOUNT	0x002	/* print count of matches only */
#define OPT_JUSTFILE	0x004	/* print filenames only */
#define OPT_NEGATE	0x008	/* invert match sense */
#define OPT_FILENAME	0x010	/* filenames printed */
#define OPT_QUIET	0x020	/* no regular output */
#define OPT_SILENT	0x040	/* no file access errors */
#define OPT_ENTIRE	0x080	/* must match entire line */
#define OPT_FGREP	0x100	/* exec fgrep */
#define OPT_NMAX	0x200	/* given a maximum number of output lines */

static int regflags;

static size_t
#ifdef __STDC__
move(char *d, const char *s)
#else
move(d, s)char *d; const char *s;
#endif
{
	char *p = d;

	/*
	* Special handling for the initial newline(s) since there's
	* no previous newline to trigger the regular test below.
	*/
	while (*s == '\n')
	{
		optflags |= OPT_EMPTYPAT;
		s++;
	}
	if (*s == '\0')
	{
		optflags |= OPT_EMPTYPAT;
		return 0;
	}
	do
	{
		if (s[0] == '\n' && s[1] == '\n')
		{
			optflags |= OPT_EMPTYPAT;
			continue;
		}
		*p++ = *s;
	} while (*++s != '\0');
	if (s[-1] != '\n')
		*p++ = '\n';
	return p - d;
}

static regex_t	engine;

static void
#ifdef __STDC__
reerr(int err)
#else
reerr(err)int err;
#endif
{
	char errmsg[BUFSIZ];

	regerror(err, &engine, errmsg, sizeof(errmsg));
	pfmt(stderr, MM_ERROR, ":1230:RE error: %s\n", errmsg);
}

static int
#ifdef __STDC__
compile(void)
#else
compile()
#endif
{
	char *p, *restr;
	struct stat stbuf;
	struct list *lp;
	size_t relen, sz;
	int fd, err;

	restr = 0;
	relen = 0;
	/*
	* Start with the command list REs first.
	*/
	if (totlen != 0)
	{
		restr = (char *)alloc((void *)0, totlen);
		p = restr;
		lp = slist;
		do
		{
			sz = move(p, lp->str);
			p += sz;
			relen += sz;
		} while ((lp = lp->next) != 0);
	}
	/*
	* Bring in the files last.
	*/
	while ((lp = flist) != 0)
	{
		flist = flist->next;
		if ((fd = open(lp->str, O_RDONLY)) < 0)
		{
			pfmt(stderr, MM_ERROR, badopen,
				lp->str, strerror(errno));
			return -1;
		}
		if (fstat(fd, &stbuf) != 0)
		{
			pfmt(stderr, MM_ERROR,
				"uxes:65:stat failed for file \"%s\", %s\n",
				lp->str, strerror(errno));
			return -1;
		}
		if (stbuf.st_size == 0)
			optflags |= OPT_EMPTYPAT;
		else
		{
			restr = p = (char *)alloc((void *)restr,
				relen + stbuf.st_size + 1);
			p += relen;
			if (read(fd, p, stbuf.st_size) != stbuf.st_size)
			{
				pfmt(stderr, MM_ERROR,
					"uxcore:205:Cannot read %s: %s\n",
					lp->str, strerror(errno));
				return -1;
			}
			p[stbuf.st_size] = '\0';
			relen += move(p, p);
		}
		close(fd);
	}
	/*
	* Trim off final \n and compile it.
	* After it's compiled, it's no longer needed.
	*/
	err = 0;
	if (relen > 1)
	{
		restr[relen - 1] = '\0';
		if (optflags & OPT_ENTIRE)
			regflags |= REG_ONESUB | REG_NLALT;
		else
			regflags |= REG_NOSUB | REG_NLALT;
		if ((err = regcomp(&engine, restr, regflags)) != 0)
			reerr(err);
	}
	free((void *)restr);
	return err;
}

static unsigned long curloc;	/* for PRF_BLOCK */
static size_t nleft;		/* no. of valid bytes left in buffer */

static char *
#ifdef __STDC__
newline(int fd)
#else
newline(fd)int fd;
#endif
{
	static char *buf, *cur = "";
	static size_t buflen;
	size_t filled, len;
	ssize_t n;
	char *p;

	while ((cur = memchr(p = cur, '\n', nleft)) == 0)
	{
		if ((len = nleft) != 0 && p != buf)
			memmove(buf, p, len);
		if ((n = (buflen - len) >> 10) == 0)
		{
			buf = (char *)alloc((void *)buf,
				buflen += 9 * 1024 - 1);
			n = 8;
		}
		cur = p = &buf[len];
		len = n << 10;
		while ((n = read(fd, p, len)) > 0)
		{
			p += n;
			if ((len -= n) < 1024)
				break;
		}
		if ((filled = p - buf) == 0)
		{
			cur = buf;
			*cur = '\0';
			nleft = 0;
			return 0;
		}
		if (filled == nleft)
		{
			cur = p;
			p = buf;
			nleft++; /* forces nleft to be zero below */
			break;
		}
		nleft = filled;
		/*
		* Scan for any null characters, temporarily changing them
		* to newlines.  This allows scanning of binaries without
		* too much fuss, although the "line numbers" will be off.
		*/
		filled -= cur - buf;
		while ((p = memchr(cur, '\0', filled)) != 0)
		{
			*p++ = '\n';
			filled -= p - cur;
			cur = p;
		}
		cur = buf;
	}
	*cur++ = '\0';
	len = cur - p;
	nleft -= len;
	curloc += len;
	return p;
}

static const char *filename;	/* current filename */

static int
#ifdef __STDC__
execute(int fd)
#else
execute(fd)int fd;
#endif
{
	static const char stdinput[] = "(standard input)";
	static const char stdinnum[] = ":1168";
	unsigned long cnt, lno;
	regmatch_t entire;
	int ans, want;
	size_t nmat;
	char *cur;

	want = 0;
	if (optflags & OPT_NEGATE)
		want = REG_NOMATCH;
	nmat = 0;
	if ((optflags & (OPT_ENTIRE | OPT_EMPTYPAT)) == OPT_ENTIRE)
		nmat = 1;
	curloc = 0;
	cnt = 0;
	lno = 0;
	while ((cur = newline(fd)) != 0)
	{
		lno++;
		if (optflags & OPT_EMPTYPAT)
			ans = 0;
		else if ((ans = regexec(&engine, cur, nmat, &entire, 0)) != 0)
		{
			if (ans != REG_NOMATCH)
			{
				reerr(ans);
				nleft = 0;
				return -1;
			}
		}
		else if (nmat != 0) /* OPT_ENTIRE: must match the whole line */
		{
			if (entire.rm_so != 0 || cur[entire.rm_eo] != '\0')
				ans = REG_NOMATCH;
		}
		if (ans != want)
			continue;
		/*
		* Got a match.  Decide what to do now.
		*/
		cnt++;
		if (prflags == 0)
		{
			if (optflags & OPT_QUIET)
				exit(0);
			if (optflags & OPT_JUSTFILE)
			{
				if (filename == 0)
					filename = gettxt(stdinnum, stdinput);
				puts(filename);
				nleft = 0;
				return 1;
			}
		}
		else
		{
			if (optflags & OPT_FILENAME)
			{
				if (filename == 0)
					filename = gettxt(stdinnum, stdinput);
				printf("%s:", filename);
			}
			if (prflags & PRF_BLOCK)
				printf("%ld:", (curloc - 1) / 512);
			if (prflags & PRF_NUMBER)
				printf("%ld:", lno);
			puts(cur);
			if (--Nmax == 0)
				return 2;
		}
	}
	if (optflags & OPT_JUSTCOUNT)
	{
		if (optflags & OPT_FILENAME)
		{
			if (filename == 0)
				filename = gettxt(stdinnum, stdinput);
			printf("%s:", filename);
		}
		printf("%ld\n", cnt);
	}
	if (cnt != 0)
		return 1;
	return 0;	/* no match, but no fails */
}

static const char usestr[] = ":1465:Usage:\n\
    [-E|-F] [-c|-l|-q|-Nmax] [-bhinsvx] pattern [file ...]\n\
    [-E|-F] [-c|-l|-q|-Nmax] [-bhinsvx] -e pattern ... [-f file ...] [file ...]\n\
    [-E|-F] [-c|-l|-q|-Nmax] [-bhinsvx] -f file ... [-e pattern ...] [file ...]\n";

int
#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)int argc; char **argv;
#endif
{
	extern char *optarg;
	extern int optind;
	int EFcnt, efcnt, clqNcnt;
	int ch, ret, fd;
	const char *p;
	char *after;

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	/*
	* Check how invoked.
	*/
	if ((p = strrchr(argv[0], '/')) != 0)
		p++;
	else
		p = &argv[0][0];
	regflags = REG_ANGLES; /* POSIX.2 permits \< and \> */
	if (*p == 'e')
	{
		regflags = REG_EXTENDED | REG_MTPARENBAD;
		p = "UX:egrep";
	}
	else
		p = "UX:grep";
	setlabel(p);
	EFcnt = 0;
	efcnt = 0;
	clqNcnt = 0;
	ret = 0;
	/*
	* Gather options.
	*/
	while ((ch = getopt(argc, argv, "EFN:bce:f:hilnqsvxy")) >= 0)
	{
		switch (ch)
		{
		usage:;
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		default:
			pfmt(stderr, MM_ACTION, usestr);
			return 2;
		case 'E':	/* behave like egrep */
			EFcnt++;
			regflags |= REG_EXTENDED | REG_MTPARENBAD;
			break;
		case 'F':	/* behave like fgrep */
			EFcnt++;
			optflags |= OPT_FGREP;
			break;
		case 'N':	/* stop after max number of lines printed */
			clqNcnt++;
			optflags |= OPT_NMAX;
			Nmax = strtoul(optarg, &after, 0);
			if (Nmax == 0 || after == optarg)
			{
				pfmt(stderr, MM_ERROR, ":0:Expecting" /*CAT*/
					" a nonzero number for -N\n");
				goto usage;
			}
			break;
		case 'b':	/* include block number */
			prflags |= PRF_BLOCK;
			break;
		case 'c':	/* report number of matches */
			clqNcnt++;
			optflags |= OPT_JUSTCOUNT;
			break;
		case 'e':	/* register pattern */
			efcnt++;
			regexp(optarg, 0);
			break;
		case 'f':	/* register pattern file */
			efcnt++;
			regexp(optarg, 1);
			break;
		case 'h':	/* do not include filename */
			ret = 1;
			break;
		case 'y':	/* egrep compatibility */
		case 'i':	/* caseless matching */
			regflags |= REG_ICASE;
			break;
		case 'l':	/* filename(s), not matching line(s) */
			clqNcnt++;
			optflags |= OPT_JUSTFILE;
			break;
		case 'n':	/* include linenumber */
			prflags |= PRF_NUMBER;
			break;
		case 'q':	/* no output */
			clqNcnt++;
			optflags |= OPT_QUIET;
			break;
		case 's':	/* no errors about file access */
			optflags |= OPT_SILENT;
			break;
		case 'v':	/* invert selection */
			optflags |= OPT_NEGATE;
			break;
		case 'x':	/* match entire input lines */
			optflags |= OPT_ENTIRE;
			break;
		}
	}
	/*
	* Check for bad invocations.
	*/
	if (EFcnt > 1 || clqNcnt > 1)
		goto usage;
	if (efcnt == 0)
	{
		if (optind >= argc)
			goto usage;
		regexp(argv[optind++], 0);
	}
	/*
	* Do -F now (don't bother checking the files).
	*/
	if (optflags & OPT_FGREP)
	{
		static const char fgrep[] = "/usr/bin/fgrep";

		if ((p = getenv("FGREP")) == 0)
			p = fgrep;
		argv[0] = (char *)p;
		execv(p, argv);
		pfmt(stderr, MM_ERROR, ":788:exec of %s failed: %s\n", p,
			strerror(errno));
		return 2;
	}
	/*
	* Finish setting flags.
	*/
	if (ret == 0 && optind + 1 < argc)
		optflags |= OPT_FILENAME;
	if (clqNcnt == 1 && (optflags & OPT_NMAX) == 0)
		prflags = 0;
	else
		prflags |= PRF_CONTENT; /* just so that it's nonzero */
	/*
	* Turn the patterns into one big matching engine
	* and then run through each of the files.
	*/
	if (compile() != 0)
		return 2;
	if (optind == argc)
	{
		filename = 0;
		ret = execute(STDIN_FILENO);
	}
	else /* once through each file */
	{
		ret = 0; /* assume no matches or failures */
		do
		{
			filename = argv[optind];
			if (filename[0] == '-' && filename[1] == '\0')
			{
				filename = 0;
				fd = STDIN_FILENO;
			}
			else if ((fd = open(filename, O_RDONLY)) < 0)
			{
				if ((optflags & OPT_SILENT) == 0)
				{
					pfmt(stderr, MM_ERROR, badopen,
						filename, strerror(errno));
				}
				ret = -1;
				ch = 0;
				continue;
			}
			if ((ch = execute(fd)) < 0 || ret == 0)
				ret = ch;
			if (fd != STDIN_FILENO)
				close(fd);
		} while (ch < 2 && ++optind < argc);
	}
	if (ret < 0)
		ret = 2; /* errors detected */
	else if (ret > 0)
		ret = 0; /* at least one match found */
	else
		ret = 1; /* no errors; no matches */
	return ret;
}
