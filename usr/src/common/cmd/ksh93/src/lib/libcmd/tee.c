#ident	"@(#)ksh93:src/lib/libcmd/tee.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * tee
 */

static const char id[] = "\n@(#)tee (AT&T Bell Laboratories) 04/01/93\0\n";

#include <cmdlib.h>

#include <ls.h>
#include <sig.h>

struct tee
{
	Sfdisc_t	disc;
	int		fd[1];
};

/*
 * This discipline writes to each file in the list given in handle
 */

static int tee_write(Sfio_t* fp, const void* buf, int n, Sfdisc_t* handle)
{
	register const char*	bp;
	register const char*	ep;
	register int*		hp = ((struct tee*)handle)->fd;
	register int		fd = sffileno(fp);
	register int		r;

	do
	{
		bp = (const char*)buf;
		ep = bp + n;
		while (bp < ep)
		{
			if ((r = write(fd, bp, ep - bp)) <= 0)
				return(-1);
			bp += r;
		}
	} while ((fd = *hp++) >= 0);
	return(n);
}

static Sfdisc_t tee_disc = { 0, tee_write, 0, 0, 0 };

int
b_tee(int argc, register char** argv)
{
	register struct tee*	tp = 0;
	register int		oflag = O_WRONLY|O_TRUNC|O_CREAT;
	register int		n;
	register int*		hp;
	register char*		cp;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "ai [file...]")) switch (n)
	{
	case 'a':
		oflag &= ~O_TRUNC;
		oflag |= O_APPEND;
		break;
	case 'i':
		signal(SIGINT, SIG_IGN);
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	if(error_info.errors)
		error(ERROR_usage(2), optusage(NiL));
	argv += opt_info.index;
	argc -= opt_info.index;

	/*
	 * for backward compatibility
	 */

	if (*argv && streq(*argv, "-"))
	{
		signal(SIGINT, SIG_IGN);
		argv++;
		argc--;
	}
	if (argc > 0)
	{
		if (!(tp = (struct tee*)stakalloc(sizeof(struct tee) + argc * sizeof(int))))
			error(ERROR_exit(1), gettxt(":311","no space"));
		tp->disc = tee_disc;
		hp = tp->fd;
		while (cp = *argv++)
		{
			if ((*hp = open(cp, oflag, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) < 0)
				error(ERROR_system(0), gettxt(":20","%s: cannot create"), cp);
			else hp++;
		}
		if (hp == tp->fd) tp = 0;
		else
		{
			*hp = -1;
			sfdisc(sfstdout, &tp->disc);
		}
	}
	if (sfmove(sfstdin, sfstdout, SF_UNBOUND, -1) < 0 || !sfeof(sfstdin) || sferror(sfstdout))
		error(ERROR_system(1), gettxt(":278","cannot copy"));

	/*
	 * close files and free resources
	 */

	if (tp)
	{
		sfdisc(sfstdout, NiL);
		for(hp = tp->fd; (n = *hp) >= 0; hp++)
			close(n);
	}
	return(error_info.errors);
}
