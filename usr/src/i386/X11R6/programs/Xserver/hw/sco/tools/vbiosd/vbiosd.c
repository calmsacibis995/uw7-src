/*
 *	@(#) vbiosd.c 11.2 97/11/12
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Nov 03 12:58:21 PST 1992	buckm@sco.com
 *	- Created.
 *	S001	Sun Nov 08 18:44:06 PST 1992	buckm@sco.com
 *	- Ignore SIGTERM; xinit might send us one.
 *	We will go away when the server goes away (pipe closures).
 *	S002	Mon Mar 29 01:18:10 PST 1993	buckm@sco.com
 *	- Add CallRom support.
 *	- Get seg and memfile args off the cmdline.
 *	S003	Tue Oct 31 11:50:39 PST 1995	brianm@sco.com
 *	- added in a call, ifdefd on V86_MAGIC1_PUM, to initialize
 *	  the bitmap for the virtual 8086 process.
 *	S004	Wed Nov 12 17:44:01 PST 1997	hiramc@sco.COM
 *	- using v86bios.h instead of v86.h
 *	- (actually probably none of this is needed, is this used in
 *	- gemini ?)
 */

/*
 * vbiosd.c - video bios daemon
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/region.h>
#include <sys/proc.h>

#include <signal.h>						/* S001 */
#include <stdio.h>

#if defined(usl)
#include <sys/v86bios.h>			/*	S004	*/
#else
#include "v86.h"
#endif
#include "vbdintf.h"


#define	REQFD	0	/* read requests from stdin */
#define	STSFD	1	/* write status   to stdout */

union	{
  struct vb_mio		mio;
  struct vb_int		ntr;
  struct vb_call		cll;
  struct vb_int_sts	nts;
} req;

char *myname;


main(ac, av)
     int ac;
     char **av;
{
  int opts = 0;
  int seg = 0;
  char *memfile = NULL;
  int status;
  int r, c;
  extern char *optarg;
  extern int optind;

  myname = av[0];

  signal(SIGTERM, SIG_IGN);

  /* process args */
  while ((c = getopt(ac, av, "m:s:")) != -1)
    switch (c)
      {
      case 'm':
        memfile = optarg;
        break;
      case 's':
        seg = strtol(optarg, 0, 16);
        break;
      }

  if (optind < ac)
    opts = strtol(av[optind], 0, 16);

  /* init video bios and send status */
  status = vm86Init();
  putsts(&status, sizeof status);

  /* loop reading requests and sending status */
  while ((r = getreq(&req, sizeof req)) > 0)
    {
      switch (req.mio.vb_req)
        {
        case VB_MEMMAP:
          if (r != sizeof req.mio)
            goto badsize;
          status = 0;           /* do nothing? */
          putsts(&status, sizeof status);
          break;

        case VB_IOENB:
          if (r != sizeof req.mio)
            goto badsize;
          status = 0;           /* do nothing? */
          putsts(&status, sizeof status);
          break;

        case VB_INT10:
          if (r != sizeof req.ntr)
            goto badsize;
          req.nts.vb_sts = vm86Int10(req.ntr.vb_reg, req.ntr.vb_cnt);
          putsts(&req.nts, sizeof req.nts);
          break;

        case VB_CALL:
          if (r != sizeof req.cll)
            goto badsize;
          req.nts.vb_sts =
            vm86CallRom(req.cll.vb_seg, req.cll.vb_off,
                        req.cll.vb_reg, req.cll.vb_cnt);
          putsts(&req.nts, sizeof req.nts);
          break;

        default:
          Error("unknown request %d\n", req.mio.vb_req);
          exit(2);

        badsize:
          Error("bad size %d for request %d\n", r, req.mio.vb_req);
          exit(3);
        }
    }

  exit(0);
}

getreq(req, size)
     char *req;
     int size;
{
  int r;

  if ((r = read(REQFD, req, size)) >= 0)
    return r;

  perror("read");
  Error("error reading request\n");
  exit(1);
}

putsts(sts, size)
     char *sts;
     int size;
{
  int r;

  if ((r = write(STSFD, sts, size)) == size)
    return;

  if (r < 0)
    perror("write");
  Error("error sending status\n");
  exit(1);
}

Error(f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
     char *f;
     char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
  fprintf(stderr, "%s: ", myname);
  fprintf(stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
}
