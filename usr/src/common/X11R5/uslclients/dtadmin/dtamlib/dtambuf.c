/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dtamlib/dtambuf.c	1.6"
#endif
/*
 *	facility for backgrounding shell commands, with stdout/stderr directed
 *	to temporary files that are read periodically in the XtMainLoop.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <X11/Intrinsic.h>

struct	 {	int	t_fd;
		char	*t_buf;
		char	*t_name;
	XtIntervalId	t_intid;
		int	t_timeout;
		int	t_count;
		Boolean	t_flag;
} DtamBuf[3] = {{-1, NULL, NULL, 0, 0, FALSE},
		{-1, NULL, NULL, 0, 0, FALSE},
		{-1, NULL, NULL, 0, 0, FALSE}};

#define	NAMESIZE	20

void	_DtamUnlink()
{
	int	n;

	for (n = 0; n <= (int)fileno(stderr); n++)
		if (DtamBuf[n].t_name) {
			close(DtamBuf[n].t_fd);
			unlink(DtamBuf[n].t_name);
		}
}

void	_DtamReadTO(buf_index, interval)
	int		*buf_index;
	XtIntervalId	interval;
{
extern	int	errno;
	int	i, n, r;
	char	*ptr;

	i = *buf_index;
	if ((n = BUFSIZ - DtamBuf[i].t_count - 1) > 0) {
		ptr = DtamBuf[i].t_buf+DtamBuf[i].t_count;
		if ((r = read(DtamBuf[i].t_fd, ptr, n)) > 0) {
			ptr[r] = 0;
			DtamBuf[i].t_count += r;
			if (DtamBuf[i].t_flag && i > 0)
				fputs(ptr, (i == 1 ? stdout : stderr));
		}
	}
	if (r > 0 || r == -1 && errno == EAGAIN)
		DtamBuf[i].t_intid = XtAddTimeOut(DtamBuf[i].t_timeout,
					(XtTimerCallbackProc)_DtamReadTO,
					(XtPointer)buf_index);
}

char	*_DtamGetline(stdfile, lineflag)
	FILE	*stdfile;
	Boolean	lineflag;
{
	int	m, n = fileno(stdfile);
	char	*ptr;

	if (DtamBuf[n].t_count == 0)
		return NULL;
	else {
		m = DtamBuf[n].t_count;
		if (lineflag) {
			if ((ptr = strchr(DtamBuf[n].t_buf, '\n')) == NULL)
				return NULL;
			m = ptr - DtamBuf[n].t_buf + 1;
		}
		ptr = XtMalloc(m+1);
		strncpy(ptr, DtamBuf[n].t_buf, m);
		ptr[m] = 0;
		if (m < DtamBuf[n].t_count)
			strcpy(DtamBuf[n].t_buf, DtamBuf[n].t_buf+m);
		DtamBuf[n].t_count -= m;
		return ptr;
	}
}

Boolean _DtamSetbuf (stdfile, timeout, flag)
	FILE	*stdfile;
	int	timeout;
	Boolean	flag;
{
	int	n;

	if ((n=fileno(stdfile)) > (int)fileno(stderr))
		return FALSE;
	DtamBuf[n].t_flag = flag;
	DtamBuf[n].t_timeout = (timeout > 0 ? timeout : 100);
	if (DtamBuf[n].t_name == NULL) {
		DtamBuf[n].t_buf = XtMalloc(BUFSIZ);
		DtamBuf[n].t_name = XtMalloc(NAMESIZE);
		sprintf(DtamBuf[n].t_name, "/tmp/std%s.%d",
					(n==1 ? "out" : "err"), getpid());
	}
	return TRUE;
}

Boolean	_DtamBackground(cmd)
	char	*cmd;
{
static	int	findex[3] = {0,1,2};
	char	cmdbuf[BUFSIZ];
	FILE	*fptr;
	int	n;

	strcpy(cmdbuf, cmd);
	if (DtamBuf[0].t_buf != NULL)
		strcat(strcat(cmdbuf," <"),DtamBuf[0].t_name);
	if (DtamBuf[1].t_buf != NULL) {
		if (DtamBuf[1].t_fd == -1)
			strcat(cmdbuf," >");
		else
			strcat(cmdbuf," >>");
		strcat(cmdbuf, DtamBuf[1].t_name);
	}
	if (DtamBuf[2].t_buf != NULL) {
		if (DtamBuf[2].t_fd == -1)
			strcat(cmdbuf," 2>");
		else
			strcat(cmdbuf," 2>>");
		strcat(cmdbuf, DtamBuf[2].t_name);
	}
	strcat(cmdbuf,"&");
/*
 * open the files, if new, and register the TimeOut procs.
 */
	for (n = 0; n <= (int)fileno(stderr); n++) {
		if (DtamBuf[n].t_buf != NULL && DtamBuf[n].t_fd == -1) {
			fptr = fopen(DtamBuf[n].t_name,"r");
			DtamBuf[n].t_fd = fileno(fptr);
			if (fcntl(fileno(fptr),F_SETFL,O_NDELAY) == -1)
				return FALSE;
			DtamBuf[n].t_intid = XtAddTimeOut(DtamBuf[n].t_timeout,
					(XtTimerCallbackProc)_DtamReadTO,
					(XtPointer)&findex[n]);
		}
	}
	system(cmdbuf);
	return TRUE;
}
