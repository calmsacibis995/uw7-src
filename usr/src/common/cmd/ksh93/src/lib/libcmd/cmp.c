#ident	"@(#)ksh93:src/lib/libcmd/cmp.c	1.1"
#pragma prototyped
/*
 * David Korn
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * cmp
 */

static const char id[] = "\n@(#)cmp (AT&T Bell Laboratories) 07/17/94\0\n";

#include <cmdlib.h>
#include <ls.h>

#define CMP_VERBOSE	1
#define CMP_SILENT	2

/*
 * compare two files
 */

static int
cmp(const char* file1, Sfio_t* f1, const char* file2, Sfio_t* f2, int flags)
{
	register int		c1;
	register int		c2;
	register unsigned char*	p1 = 0;
	register unsigned char*	p2 = 0;
	register int		lines = 1;
	register unsigned char*	e1 = 0;
	register unsigned char*	e2 = 0;
	unsigned long		pos = 0;
	int			ret = 0;
	unsigned char*		last;

	for (;;)
	{
		if ((c1 = e1 - p1) <= 0)
		{
			if (!(p1 = (unsigned char*)sfreserve(f1, SF_UNBOUND, 0)) || (c1 = sfslen()) <= 0)
			{
				if ((e2 - p2) > 0 || sfreserve(f2, SF_UNBOUND, 0) && sfslen() > 0)
				{
					ret = 1;
					if (!(flags & CMP_SILENT))
						error(ERROR_exit(1), gettxt(":287","%s: EOF"), file1);
				}
				return(ret);
			}
			e1 = p1 + c1;
		}
		if ((c2 = e2 - p2) <= 0)
		{
			if (!(p2 = (unsigned char*)sfreserve(f2, SF_UNBOUND, 0)) || (c2 = sfslen()) <= 0)
			{
				if (!(flags & CMP_SILENT))
					error(ERROR_exit(1), gettxt(":287","%s: EOF"), file2);
				return(1);
			}
			e2 = p2 + c2;
		}
		if (c1 > c2)
			c1 = c2;
		pos += c1;
		if (flags & CMP_SILENT)
		{
			if (memcmp(p1, p2, c1))
				return(1);
			p1 += c1;
			p2 += c1;
		}
		else
		{
			last = p1 + c1;
			while (p1 < last)
			{
				if ((c1 = *p1++) != *p2++)
				{
					if (flags)
					{
						ret = 1;
						sfprintf(sfstdout, "%6ld %3o %3o\n", pos - (last - p1), c1, *(p2 - 1));
					}
					else
					{
						sfprintf(sfstdout, "%s %s differ: char %ld, line %d\n", file1, file2, pos - (last - p1), lines);
						return(1);
					}
				}
				if (c1 == '\n')
					lines++;
			}
		}
	}
}

int
b_cmp(int argc, register char** argv)
{
	char*		s;
	char*		e;
	Sfio_t*		f1 = 0;
	Sfio_t*		f2 = 0;
	char*		file1;
	char*		file2;
	int		n;
	off_t		o1 = 0;
	off_t		o2 = 0;
	struct stat	s1;
	struct stat	s2;

	int		flags = 0;

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, gettxt(":389", "ls file1 file2 [skip1] [skip2]"))) switch (n)
	{
	case 'l':
		flags |= CMP_VERBOSE;
		break;
	case 's':
		flags |= CMP_SILENT;
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if (error_info.errors || !(file1 = *argv++) || !(file2 = *argv++))
		error(ERROR_usage(2), optusage(NiL));
	n = 2;
	if (streq(file1, "-"))
		f1 = sfstdin;
	else if (!(f1 = sfopen(NiL, file1, "r")))
	{
		if (!(flags & CMP_SILENT))
			error(ERROR_system(0), gettxt(":27","%s: cannot open"), file1);
		goto done;
	}
	if (streq(file2, "-"))
		f2 = sfstdin;
	else if (!(f2 = sfopen(NiL, file2, "r")))
	{
		if (!(flags & CMP_SILENT))
			error(ERROR_system(0), gettxt(":27","%s: cannot open"), file2);
		goto done;
	}
	if (s = *argv++)
	{
		o1 = strtol(s, &e, 0);
		if (*e)
		{
			error(ERROR_exit(0), gettxt(":288","%s: %s: invalid skip"), file1, s);
			goto done;
		}
		if (sfseek(f1, o1, 0) != o1)
		{
			if (!(flags & CMP_SILENT))
				error(ERROR_exit(0), gettxt(":287","%s: EOF"), file1);
			n = 1;
			goto done;
		}
		if (s = *argv++)
		{
			o2 = strtol(s, &e, 0);
			if (*e)
			{
				error(ERROR_exit(0), gettxt(":288","%s: %s: invalid skip"), file2, s);
				goto done;
			}
			if (sfseek(f2, o2, 0) != o2)
			{
				if (!(flags & CMP_SILENT))
					error(ERROR_exit(0), gettxt(":287","%s: EOF"), file2);
				n = 1;
				goto done;
			}
		}
		if (*argv)
		{
			error(ERROR_usage(0), optusage(NiL));
			goto done;
		}
	}
	if (fstat(sffileno(f1), &s1))
		error(ERROR_system(0), gettxt(":289","%s: cannot stat"), file1);
	else if (fstat(sffileno(f2), &s2))
		error(ERROR_system(0), gettxt(":289","%s: cannot stat"), file1);
	else if (s1.st_ino == s2.st_ino && s1.st_dev == s2.st_dev && o1 == o2)
		n = 0;
	else n = ((flags & CMP_SILENT) && S_ISREG(s1.st_mode) && S_ISREG(s2.st_mode) && (s1.st_size - o1) != (s2.st_size - o2)) ? 1 : cmp(file1, f1, file2, f2, flags);
 done:
	if (f1 && f1 != sfstdin) sfclose(f1);
	if (f2 && f2 != sfstdin) sfclose(f2);
	return(n);
}
