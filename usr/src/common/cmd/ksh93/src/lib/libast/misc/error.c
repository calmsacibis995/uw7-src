#ident	"@(#)ksh93:src/lib/libast/misc/error.c	1.2"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * error and message formatter
 *
 *	level is the error level
 *	level >= error_info.core!=0 dumps core
 *	level >= ERROR_FATAL calls error_info.exit
 *	level < 0 is for debug tracing
 */

#include <ast.h>
#include <ctype.h>
#include <error.h>
#include <namval.h>
#include <sig.h>
#include <stk.h>
#include <times.h>

extern char	*gettxt();

#if _DLL_INDIRECT_DATA && !_DLL
static Error_info_t	error_info_data =
#else
Error_info_t		error_info =
#endif

{ 2, exit, write };

#if _DLL_INDIRECT_DATA && !_DLL
Error_info_t		error_info = &error_info_data;
#endif

#define OPT_CORE	1
#define OPT_FD		2
#define OPT_LIBRARY	3
#define OPT_MASK	4
#define OPT_SYSTEM	5
#define OPT_TIME	6
#define OPT_TRACE	7

static const Namval_t		options[] =
{
	"core",		OPT_CORE,
	"debug",	OPT_TRACE,
	"fd",		OPT_FD,
	"library",	OPT_LIBRARY,
	"mask",		OPT_MASK,
	"system",	OPT_SYSTEM,
	"time",		OPT_TIME,
	"trace",	OPT_TRACE,
	0,		0
};

/*
 * called by stropt() to set options
 */

static int
setopt(void* a, const void* p, register int n, register const char* v)
{
	NoP(a);
	if (p) switch (((Namval_t*)p)->value)
	{
	case OPT_CORE:
		if (n) switch (*v)
		{
		case 'e':
		case 'E':
			error_info.core = ERROR_ERROR;
			break;
		case 'f':
		case 'F':
			error_info.core = ERROR_FATAL;
			break;
		case 'p':
		case 'P':
			error_info.core = ERROR_PANIC;
			break;
		default:
			error_info.core = strtol(v, NiL, 0);
			break;
		}
		else error_info.core = 0;
		break;
	case OPT_FD:
		error_info.fd = n ? strtol(v, NiL, 0) : -1;
		break;
	case OPT_LIBRARY:
		if (n) error_info.set |= ERROR_LIBRARY;
		else error_info.clear |= ERROR_LIBRARY;
		break;
	case OPT_MASK:
		if (n) error_info.mask = strtol(v, NiL, 0);
		else error_info.mask = 0;
		break;
	case OPT_SYSTEM:
		if (n) error_info.set |= ERROR_SYSTEM;
		else error_info.clear |= ERROR_SYSTEM;
		break;
	case OPT_TIME:
		error_info.time = n ? 1 : 0;
		break;
	case OPT_TRACE:
		if (n) error_info.trace = -strtol(v, NiL, 0);
		else error_info.trace = 0;
		break;
	}
	return(0);
}

/*
 * print a name with optional delimiter, converting unprintable chars
 */

static void
print(register Sfio_t* sp, register char* name, char* delim)
{
	register int	c;

	while (c = *name++)
	{
		if (c & 0200)
		{
			c &= 0177;
			sfputc(sp, '?');
		}
		if (c < ' ')
		{
			c += 'A' - 1;
			sfputc(sp, '^');
		}
		sfputc(sp, c);
	}
	if (delim) sfputr(sp, delim, -1);
}

/*
 * print error context FIFO stack
 */

static void
context(register Sfio_t* sp, register Error_context_t* cp)
{
	if (cp->context) context(sp, cp->context);
	if (!(cp->flags & ERROR_SILENT))
	{
		if (cp->id) print(sp, cp->id, NiL);
		if (cp->line > ((cp->flags & ERROR_INTERACTIVE) != 0))
		{
			if (cp->file) sfprintf(sp, ERROR_translate(gettxt(":215",": \"%s\", line %d"),0), cp->file, cp->line);
			else sfprintf(sp, "[%d]", cp->line);
		}
		sfputr(sp, ": ", -1);
	}
}

void
error(int level, ...)
{
	va_list	ap;

	va_start(ap, level);
	sfsync((Sfio_t*)0);
	errorv(NiL, level, ap);
	sfsync((Sfio_t*)0);
	va_end(ap);
		/*
		 * The sfsync() calls in this routine provide a workaround for
		 * a known shell bug that occurs when a non-fatal error
		 * produces an error message in a situation where stdout and
		 * stderr are being directed to different files.
		 */
}

void
errorv(const char* lib, int level, va_list ap)
{
	register int	n;
	int		fd;
	int		flags;
	char*		s;
	char*		format;

	int		line;
	char*		file;

	if (!error_info.init)
	{
		error_info.init = 1;
		stropt(getenv("ERROR_OPTIONS"), options, sizeof(*options), setopt, NiL);
	}
	if (level > 0)
	{
		flags = level & ~ERROR_LEVEL;
		level &= ERROR_LEVEL;
	}
	else flags = 0;
	if (level < error_info.trace || lib && (error_info.clear & ERROR_LIBRARY) || level < 0 && error_info.mask && !(error_info.mask & (1<<(-level - 1))))
	{
		if (level >= ERROR_FATAL) (*error_info.exit)(level - 1);
		return;
	}
	if (error_info.trace < 0) flags |= ERROR_LIBRARY|ERROR_SYSTEM;
	flags |= error_info.set | error_info.flags;
	flags &= ~error_info.clear;
	if (!lib || !*lib) flags &= ~ERROR_LIBRARY;
	fd = (flags & ERROR_OUTPUT) ? va_arg(ap, int) : error_info.fd;
	if (error_info.write)
	{
		long	off;
		char*	bas;

		bas = stkptr(stkstd, 0);
		if (off = stktell(stkstd)) stkfreeze(stkstd, 0);
		file = error_info.id;
		if (flags & ERROR_USAGE)
		{
			if (flags & ERROR_NOID) sfprintf(stkstd, "       ");
			else sfprintf(stkstd, ERROR_translate(gettxt(":216","Usage: "),0));
			if (file || opt_info.argv && (file = opt_info.argv[0])) print(stkstd, file, " ");
		}
		else
		{
			if (level && !(flags & ERROR_NOID))
			{
				if (error_info.context && level > 0) context(stkstd, error_info.context);
				if (file) print(stkstd, file, (flags & ERROR_LIBRARY) ? " " : ": ");
				if (flags & ERROR_LIBRARY) sfprintf(stkstd, ERROR_translate(gettxt(":217","[%s library]: "),0), lib);
			}
			if (level > 0 && error_info.line > ((flags & ERROR_INTERACTIVE) != 0))
			{
				if (error_info.file && *error_info.file) sfprintf(stkstd, "\"%s\", ", error_info.file);
				sfprintf(stkstd, ERROR_translate(gettxt(":218","line %d: "), 0), error_info.line);
			}
		}
		switch (level)
		{
		case 0:
			break;
		case ERROR_WARNING:
			error_info.warnings++;
			sfprintf(stkstd, ERROR_translate(gettxt(":219","warning: "), 0));
			break;
		case ERROR_PANIC:
			error_info.errors++;
			sfprintf(stkstd, ERROR_translate(gettxt(":220","panic: "), 0));
			break;
		default:
			if (level < 0)
			{
				if (error_info.trace < -1) sfprintf(stkstd, ERROR_translate(gettxt(":221","debug%d:%s"),0), level, level > -10 ? " " : "");
				else sfprintf(stkstd, ERROR_translate(gettxt(":222","debug: "),0));
				if (error_info.time)
				{
					long		d;
					struct tms	us;

					if (error_info.time == 1 || (d = times(&us) - error_info.time) < 0)
						d = error_info.time = times(&us);
					sfprintf(stkstd, " %05lu.%05lu.%05lu ", d, (unsigned long)us.tms_utime, (unsigned long)us.tms_stime);
				}
				for (n = 0; n < error_info.indent; n++)
				{
					sfputc(stkstd, ' ');
					sfputc(stkstd, ' ');
				}
			}
			else error_info.errors++;
			break;
		}
		if (flags & ERROR_SOURCE)
		{
			/*
			 * source ([version], file, line) message
			 */

			file = va_arg(ap, char*);
			line = va_arg(ap, int);
			if (error_info.version) sfprintf(stkstd, ERROR_translate(gettxt(":223","(%s: \"%s\", line %d) "),0), error_info.version, file, line);
			else sfprintf(stkstd, ERROR_translate(gettxt(":224","(\"%s\", line %d) "),0), file, line);
		}
		format = va_arg(ap, char*);
		sfvprintf(stkstd, format, ap);
		if (!(flags & ERROR_PROMPT))
		{
			if (error_info.auxilliary && !(*error_info.auxilliary)(stkstd, level, flags))
			{
				stkset(stkstd, bas, off);
				return;
			}
			if ((flags & ERROR_SYSTEM) && errno && errno != error_info.last_errno)
			{
				sfprintf(stkstd, gettxt(":338"," [%s]"), fmterror(errno));
				if (error_info.set & ERROR_SYSTEM) errno = 0;
				error_info.last_errno = (level >= 0) ? 0 : errno;
			}
			sfputc(stkstd, '\n');
		}
		n = stktell(stkstd);
		s = stkptr(stkstd, 0);
		if (fd == sffileno(sfstderr) && error_info.write == write) sfwrite(sfstderr, s, n);
		else (*error_info.write)(fd, s, n);
		stkset(stkstd, bas, off);
	}
	if (level >= error_info.core && error_info.core)
	{
#ifndef SIGABRT
#ifdef	SIGQUIT
#define SIGABRT	SIGQUIT
#else
#ifdef	SIGIOT
#define SIGABRT	SIGIOT
#endif
#endif
#endif
#ifdef	SIGABRT
		signal(SIGABRT, SIG_DFL);
		kill(getpid(), SIGABRT);
		pause();
#else
		abort();
#endif
	}
	if (level >= ERROR_FATAL) (*error_info.exit)(level - ERROR_FATAL + 1);
}
