/*
 *	@(#)vm86.c	11.2	11/11/97	17:22:23
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S001	Tue Nov 11 17:16:57 PST 1997	hiramc@sco.COM
 *	- use v86bios.h in place of v86.h
 *	- this causes a change for the names of some defined constants
 *      S000	Wed Jan 29 16:52:10 PST 1997	kylec@sco.com
 * 	- create
 */

#include <sys/types.h>
#include <sys/kd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef XSERVER
#include "os.h"
#include "grafinfo.h"
#include "y.tab.h"
#include "dyddx.h"
#include "commonDDX.h"
#endif /* XSERVER */

#include "v86opts.h"
#include <sys/v86bios.h>			/*	S001	*/

static int vm86InitDone = 0;
static int vm86DaemonInitDone = 0;
static int vm86_fd = -1;


#ifdef XSERVER

int
vm86DaemonInit()
{
  if( vm86DaemonInitDone )
    return 0;

  vm86DaemonInitDone = 1;

  return (vbInit(0, 0, 0));

}

int
vm86DaemonInt10(int *preg, int nreg)
{
  if (!vm86DaemonInitDone)
    return (-1);
  else
    return (vbInt10(preg, nreg));
}


int
vm86DaemonCallRom(int seg, int off, int *preg, int nreg)
{
  if (!vm86DaemonInitDone)
    return (-1);
  else
    return (vbCallRom(seg, off, preg, nreg));
}

#endif /* XSERVER */

int
vm86Init()
{
  if( vm86InitDone )
    return 0;

  vm86InitDone = 1;

  if (vm86_fd == -1)
    vm86_fd = open(V86BIOS_DEVICE, O_RDWR);	/*	S001	*/

  if (vm86_fd < 0)
    return -1;
  else
    return 0;

}

int
vm86Int10(int *preg, int nreg)
{
  intregs_t regs;
  extern int vm86_fd;

  regs.type = V86BIOS_INT16;			/*	S001	*/
  regs.entry.intval = VIDEO_BIOS_INT;		/*	S001	*/
  regs.error = 0;

  if (vm86_fd == -1)
    {
      return(-1);
    }

  switch (nreg)
    {
    case 8:		regs.es = preg[7];
    case 7:		regs.bp = preg[6];
    case 6:		regs.edi.word.di = preg[5];
    case 5:		regs.esi.word.si = preg[4];
    case 4:		regs.edx.word.dx = preg[3];
    case 3:		regs.ecx.word.cx = preg[2];
    case 2:		regs.ebx.word.bx = preg[1];
    case 1:		regs.eax.word.ax = preg[0];
    }
    
		/*	S001  vvv	*/
  if ((ioctl(vm86_fd, V86BIOS_CALL, &regs) < 0) || (regs.error != 0))
    {
      return(-1);
    }
    
  switch (nreg)
    {
    case 8:		preg[7] = regs.es;
    case 7:		preg[6] = regs.bp;
    case 6:		preg[5] = regs.edi.word.di;
    case 5:		preg[4] = regs.esi.word.si;
    case 4:		preg[3] = regs.edx.word.dx;
    case 3:		preg[2] = regs.ecx.word.cx;
    case 2:		preg[1] = regs.ebx.word.bx;
    case 1:		preg[0] = regs.eax.word.ax;
    }
    
  return(0);
}

int
vm86CallRom(int seg, int off, int *preg, int nreg)
{
  intregs_t regs;
  extern int vm86_fd;

  regs.type = V86BIOS_CALL;			/*	S001	*/
  regs.entry.farptr.v86_cs = (unsigned short)seg;
  regs.entry.farptr.v86_off = (unsigned short)off;
  regs.error = 0;

  if (vm86_fd == -1)
    {
      return(-1);
    }

  switch (nreg)
    {
    case 8:		regs.es = preg[7];
    case 7:		regs.bp = preg[6];
    case 6:		regs.edi.word.di = preg[5];
    case 5:		regs.esi.word.si = preg[4];
    case 4:		regs.edx.word.dx = preg[3];
    case 3:		regs.ecx.word.cx = preg[2];
    case 2:		regs.ebx.word.bx = preg[1];
    case 1:		regs.eax.word.ax = preg[0];
    default:		break;
    }
    
		/*	S001 vvvv	*/
  if ((ioctl(vm86_fd, V86BIOS_CALL, &regs) < 0) || (regs.error != 0))
    {
      return(-1);
    }
    
  switch (nreg)
    {
    case 8:		preg[7] = regs.es;
    case 7:		preg[6] = regs.bp;
    case 6:		preg[5] = regs.edi.word.di;
    case 5:		preg[4] = regs.esi.word.si;
    case 4:		preg[3] = regs.edx.word.dx;
    case 3:		preg[2] = regs.ecx.word.cx;
    case 2:		preg[1] = regs.ebx.word.bx;
    case 1:		preg[0] = regs.eax.word.ax;
    default:		break;
    }
    
  return(0);
}

