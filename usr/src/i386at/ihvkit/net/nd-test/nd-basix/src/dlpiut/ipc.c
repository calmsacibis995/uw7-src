/*
 * File ipc.c
 * ipc primitive isolation routines
 * named pipe version
 * messages are sent through the pipes as a single write that contains
 * the data prefixed by a short int that has the length of the message.
 * new to this version is the mrdchk call (works like rdchk)
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 *
 * 2/18/88 kurth
 */

#pragma comment(exestr, "@(#) ipc.c 11.1 95/05/01 SCOINC")

#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

#define PIPENAME	"/tmp/dlpi."

#define FNAMELEN 60

#define MAXIPC	33

#define IPC_BUFSIZ	32002

static int ipcmaxlen;

static int	mbufget(int size);
static void	mkqname(char *s, char *n);
static int	valq(int fd);

struct ipcdef {
	int	i_flg;		/* non zero indicates slot in use */
	char	i_name[15];	/* first char zero indicates creator */
};

typedef struct ipcdef ipc;

static ipc i[MAXIPC];

static char *mp;	/* the local message buffer */

/*
 * mcreat
 * create an ipc queue
 */

int
mcreat(char *qname)
{
	register ipc *ip;
	char buf[FNAMELEN];
	int fd;
	int j;

	signal(SIGPIPE, SIG_IGN);
	if (strlen(qname) > 14)
		return(-1);
	mkqname(buf, qname);
	fd = open(buf, O_NDELAY|O_WRONLY);
	if (fd >= 0) {
		close(fd);
		return(-2);
	}
	unlink(buf);
	fd = mknod(buf, 010666, 0);
	if (fd >= 0)
		fd = open(buf, O_RDWR);
	else {
		/*j = fcntl(fd, F_GETFL, 0);		/* get modes */
		/*fcntl(fd, F_SETFL, j & (~O_NDELAY));	/* clear ndelay */
	}
	if (fd < 0) {
		perror("second ipc open failed?\n");
		return(-1);
	}
	if (mbufget(IPC_BUFSIZ) < 0) {
		close(fd);
		return(-1);
	}
	ip = &i[fd];
	ip->i_flg = 1;
	strcpy(ip->i_name, qname);
	return(fd);
}


/*
 * mopen
 * open an already existing ipc queue.
 */

int
mopen(char *qname)
{
	register ipc *ip;
	char buf[FNAMELEN];
	int fd;
	int j;

	signal(SIGPIPE, SIG_IGN);
	if (strlen(qname) > 14)
		return(-1);
	mkqname(buf, qname);
	fd = open(buf, O_NDELAY|O_WRONLY);
	if (fd < 0) {
		return(-1);
	}
	j = fcntl(fd, F_GETFL, 0);		/* get modes */
	fcntl(fd, F_SETFL, j & (~O_NDELAY));	/* open ok, clear ndelay */
	if (mbufget(IPC_BUFSIZ) < 0) {
		close(fd);
		return(-1);
	}
	ip = &i[fd];
	ip->i_flg = 1;
	strcpy(ip->i_name, qname);
	return(fd);
}

/*
 * close an ipc queue, used by mopen callers
 */

int
mclose(int fd)
{
	register ipc *ip;

	if (valq(fd) < 0)
		return(-1);
	ip = &i[fd];
	close(fd);
	ip->i_flg = 0;
	return(0);
}

/*
 * delete an ipc queue, used by mcreat callers
 */

int
mdelete(int fd)
{
	register ipc *ip;
	char buf[FNAMELEN];

	if (valq(fd) < 0)
		return(-1);
	ip = &i[fd];
	mkqname(buf, ip->i_name);
	close(fd);
	unlink(buf);
	ip->i_flg = 0;
	return(0);
}

/*
 * receive messages from the named pipe
 * truncate (flush) messages that are too long for the users buffer
 */

int
mrecv(int fd, char *buf, int mlen)
{
	register ipc *ip;
	short len;
	int j;
	int k;

	if (valq(fd) < 0)
		return(-1);
	ip = &i[fd];
	if (read(fd, &len, sizeof(short)) != sizeof(short))
		return(-1);
	j = len;
	while (j > 0) {
		k = read(fd, buf, j);
		if (k < 0)
			return(-1);
		j -= k;
		buf += k;
	}
	return(len);
}

/*
 * send messages to the ipc queue
 */

int
msend(int fd, char *buf, int mlen)
{
	register ipc *ip;

	if (valq(fd) < 0)
		return(-1);
	if (mlen > ipcmaxlen)
		return(-1);
	if (mlen == 0)
		abort();
	ip = &i[fd];
	memcpy(mp + sizeof(short), buf, mlen);
	*((short *)mp) = mlen;
	mlen += sizeof(short);
	if (write(fd, mp, mlen) < 0)
		return(-1);
	return(mlen);
}

/*
 * works like rdchk()
 */

#ifdef NOT_USED
int
mrdchk(int fd)
{
	register ipc *ip;

	if (valq(fd) < 0)
		return(-1);
	ip = &i[fd];
	return(rdchk(fd));
}
#endif	/* NOT_USED */

/*
 *	define the buffer size (must be called before the first mopen or mcreat)
 */


#ifdef NOT_USED
int
msetbuf(int size)
{
	if (mbufget(size) < 0)
		return(-1);
	return(0);
}
#endif	/* NOT_USED */

/*
 * allocate the buffer if it needs to allocated
 */

static int
mbufget(int size)
{
	if (mp)
		return(0);
	mp = (char *)malloc(size + sizeof(short));
	ipcmaxlen = size;
	if (mp == (char *)0)
		return(-1);
	return(0);
}


/*
 * create a fully qualified name
 */

static void
mkqname(char *s, char *n)
{
	strcpy(s, PIPENAME);
	strcat(s, n);
}

/*
 * validate a token passed in by the user
 */

static int
valq(int fd)
{
	if ((fd < 0) || (fd >= MAXIPC))
		return(-1);
	if (i[fd].i_flg == 0)
		return(-1);
	return(1);
}

