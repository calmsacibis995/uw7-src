/*
 *      @(#) vbdintf.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Nov 02 15:06:38 PST 1992	buckm@sco.com
 *	- Created.
 *	S001	Thu Nov 05 20:54:42 PST 1992	buckm@sco.com
 *	- Pass along v86 status return from int10.
 *	S002	Wed Jan 20 00:13:54 PST 1993	buckm@sco.com
 *	- Because of a bug in the unix 3.2.4 keyboard driver,
 *	  NextScreen (ctrl-prntscrn) screen switching is initiated
 *	  by the kernel.  This means that if the user is leaning on
 *	  ctrl-prntscrn, we are likely to get a signal while hanging
 *	  in the read in getsts().  Just retry the read.
 *	S003	Sun Mar 28 23:46:36 PST 1993	buckm@sco.com
 *	- Pass more args to the daemon on the command line.
 *	- Add vbCallRom request.
 *	S004	Thu Apr  3 16:27:13 PST 1997	kylec@sco.com
 *	- Add i18n message support
 */

/*
 * vbdintf.c - video bios daemon interface
 */

#include	<errno.h>					/* S002 */
#include	"vbdintf.h"
#include	"xsrv_msgcat.h"					/* S004 */

/* Do some things differently if we are in the server */
#ifdef XSERVER
#define	Eprint		ErrorF
#define	perror		Error
#endif /* XSERVER */

#ifndef VBIOSDIR
#define VBIOSDIR "/usr/X11R6.1/lib/X11/vbios"
#endif

static char *daemon = VBIOSDIR "/vbiosd";

static int reqfd, stsfd;

static int
getsts(sts, size)
	char	*sts;
	int	size;
{
	int	r;

	do {							/* S002 */
		if ((r = read(stsfd, sts, size)) == size)
			return 0;
	} while ((r < 0) && (errno == EINTR));			/* S002 */

	if (r < 0)
		perror("read");
	Eprint(MSGGRAF(XGRAF_1,"Error reading from video bios daemon\n"));
	return -1;
}

static int
putreq(req, size)
	char	*req;
	int	size;
{
	int	r;

	if ((r = write(reqfd, req, size)) == size)
		return 0;

	if (r < 0)
		perror("write");
	Eprint(MSGGRAF(XGRAF_2,"Error writing to video bios daemon\n"));
	return -1;
}

/*
 * vbInit() - initialise video bios
 */
vbInit(opts, seg, memfile)
	int	opts;
	int	seg;
	char	*memfile;
{
	int	rfd[2];
	int	sfd[2];
	int	pid;

	/* setup two pipes: request and status */
	if ((pipe(rfd) < 0) || (pipe(sfd) < 0)) {
		perror("pipe");
		goto err;
	}

	/* now fork(); could add retries here */
	if ((pid = fork()) < 0) {
		perror("fork");
		goto err;
	}

	if (pid) {	/* parent */

		int	status;

		/* finish pipe setup */
		close(rfd[0]);		/* request read  is unused */
		reqfd = rfd[1];		/* request write */
		stsfd = sfd[0];		/* status  read  */
		close(sfd[1]);		/* status  write is unused */

		/* get initial status back from daemon */
		if (getsts(&status, sizeof status) || status)
			goto err;

	} else {	/* child */

		int	i;
		char	ostr[10];
		char	sstr[10];
		char	*argv[7];	/* bump if you add args */

		/* finish pipe setup */
		close(0);
		dup(rfd[0]);		/* request read  is stdin  */
		close(rfd[0]);

		close(1);
		dup(sfd[1]);		/* status  write is stdout */
		close(sfd[1]);

		close(rfd[1]);		/* request write is unused */
		close(sfd[0]);		/* status  read  is unused */

		/* leave stderr; close higher fd's */
		for (i = 3; i < 20; ++i)  /* probably nothing open over 20 */
			close(i);

		/* build args */
		i = 0;
		argv[i++] = "vbiosd";
		if (seg) {
			argv[i++] = "-s";
			sprintf(sstr, "%x", seg);
			argv[i++] = sstr;
		}
		if (memfile) {
			argv[i++] = "-m";
			argv[i++] = memfile;
		}
		sprintf(ostr, "%x", opts);
		argv[i++] = ostr;
		argv[i] = 0;

		/* exec the daemon */
		if (execv(daemon, argv) < 0) {
			perror(daemon);
			exit(1);
		}

		/* not reached */

	}

	return 0;

  err:
	Eprint(MSGGRAF(XGRAF_3,"vbInit: cannot init video bios\n"));
	return -1;
}

/*
 * vbInt10(preg, nreg) -	execute an INT 10 using the nreg registers
 *				pointed to by preg.
 */
vbInt10(preg, nreg)
	int			*preg;
	int			nreg;
{
	struct vb_int		nt;
	struct vb_int_sts	sts;

	nt.vb_req = VB_INT10;
	nt.vb_cnt = nreg;
	memcpy(nt.vb_reg, preg, nreg * sizeof(int));

	if (putreq(&nt, sizeof nt) || getsts(&sts, sizeof sts))
		return -1;

	memcpy(preg, sts.vb_reg, nreg * sizeof(int));

	return sts.vb_sts;
}

/*
 * vbCallRom(seg, off, preg, nreg) -	call into ROM at seg:off
 *					using the nreg registers
 *					pointed to by preg.
 */
vbCallRom(seg, off, preg, nreg)
	int			seg;
	int			off;
	int			*preg;
	int			nreg;
{
	struct vb_call		cl;
	struct vb_int_sts	sts;

	cl.vb_req = VB_CALL;
	cl.vb_seg = seg;
	cl.vb_off = off;
	cl.vb_cnt = nreg;
	memcpy(cl.vb_reg, preg, nreg * sizeof(int));

	if (putreq(&cl, sizeof cl) || getsts(&sts, sizeof sts))
		return -1;

	memcpy(preg, sts.vb_reg, nreg * sizeof(int));

	return sts.vb_sts;
}

/*
 * vbMemMap() - map memory addresses
 */
vbMemMap(addr, len)
	int		addr;
	int		len;
{
	struct vb_mio	mm;
	int		status;

	mm.vb_req  = VB_MEMMAP;
	mm.vb_base = addr;
	mm.vb_size = len;

	if (putreq(&mm, sizeof mm) ||
	    getsts(&status, sizeof status) || status)
		return -1;

	return 0;
}

/*
 * vbIOEnable()	- enable a range of i/o addresses
 */
vbIOEnable(addr, count)
	int		addr;
	int		count;
{
	struct vb_mio	io;
	int		status;

	io.vb_req  = VB_IOENB;
	io.vb_base = addr;
	io.vb_size = count;

	if (putreq(&io, sizeof io) ||
	    getsts(&status, sizeof status) || status)
		return -1;

	return 0;
}
