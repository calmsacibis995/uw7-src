/*
 *	@(#)interpIfc.c	11.3	11/25/97	15:37:50
 *
 *	interpIfc.c - interpreter interface
 *	Routines:
 *		interpInit(opts) - prepare it to go
 *		VBiosINT10(preg, nreg) - call INT 10
 *		VBiosCall(seg, off, preg, nreg) - far call to BIOS
 *
 *	S002	Tue Oct 28 14:21:22 PST 1997	-	hiramc@sco.COM
 *	- moving all mmap and CON_MEM_MAPPED ioctl to MemConMap.c
 *	- and the function memconmap
 *	S001	Tue Apr 15 10:32:29 PDT 1997	-	hiramc@sco.COM
 *	- add CON_MEM_MAPPED ioctl
 */
#if defined(gemini)

#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/unistd.h>	/*	S001	*/
#include	<sys/mman.h>
#include <sys/inline.h>
#include <sys/file.h>
#include <sys/stream.h>
#include	<sys/kd.h>	/*	S001	*/
#include	<sys/ws/ws.h>	/*	S001	*/
#ifdef NOT
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <termio.h>
#include <sys/signal.h>
#endif

#include	"grafinfo.h"
#include	"interp.h"
#include	"v86opts.h"

typedef unsigned long	dword_t;

/*	decode_instructions is in interp.c (that is the interpreter)	*/
extern void decode_instructions(int, struct REGS *, struct REGS *);

/*	see v86opts.h for listing of option flags	*/

static int OptionFlags = 0;

#define Debug	(OptionFlags & OPT_DEBUG)

/* memconmap is in MemConMap.c				vvv	S002	*/
void *
memconmap( void *addr, size_t len, int prot, int flags, int fd,
          off_t off, int ioctlFd, int DoConMemMap, int DoZero );
/*							^^^	S002	*/

int interpInit(opts)
int opts;
{
	int fd;
	int videoFd;	/*	S001	*/
	caddr_t addr,addr1,addr2,addr3;
	unsigned short *ivec;
	struct REGS sregs,dregs;
	char text[128];
	unsigned char *tuchar;
	int cmmRet;			/*	S001	*/

	OptionFlags = opts;

	fd = open("/dev/pmem",O_RDWR);
	if (fd < 0) {
		if( Debug )
			ErrorF("interpInit: Cannot open /dev/pmem\n");
		return(FAILURE);
	}

	videoFd = open("/dev/video",O_RDWR);	/*	S001 vvv	*/
	if (videoFd < 0) {
		close(fd);
		if( Debug )
			ErrorF("interpInit: Cannot open %s\n", "/dev/video" );
		return(FAILURE);
	}

    addr3 = (caddr_t) memconmap( (void *) 0, (size_t) 3072,
	PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd,
	(off_t) 0, videoFd, 1, 0 );		/*	S002	*/

	if (addr3 == (caddr_t)-1) {
		if( Debug )
		    ErrorF("interpInit: Cannot mmap the 1st 3k of memory\n");
		close(videoFd);
		close(fd);
		return(FAILURE);
	}
	if( Debug ) {
		ErrorF("interpInit: First 3K of physical memory mapped to %#x\n", addr3 );
		ErrorF("interpInit: and INT 10 = %04x:%04x\n", *((unsigned short *)(((char *)addr3)+0x40)), *((unsigned short *)(((char *)addr3)+0x42)));
	}

    addr1 = (caddr_t) memconmap( (void *) 0xC0000, (size_t) 0x10000,
	PROT_READ, MAP_SHARED|MAP_FIXED, fd,
	(off_t) 0xC0000, videoFd, 1, 0 );		/*	S002	*/

	if (addr1 == (caddr_t)-1) {
		if( Debug )
			ErrorF("interpInit: Cannot mmap vga ROM at 0xc0000\n");
		close(videoFd);
		close(fd);
		return(FAILURE);
	}
	if( Debug ) {
		ErrorF("interpInit: VGA ROM memory mapped to %#x\n", addr1 );
		ErrorF("interpInit: VGA ROM magic: %#06x, size: %#06x\n", *((short *) 0xc0000), 512 * (*(((unsigned char *)0xc0000) + 2) ) );
	}

	if (OptionFlags & OPT_LDSYSROM) {

    addr1 = (caddr_t) memconmap( (void *) 0xE0000, (size_t) 0x20000,
	PROT_READ, MAP_SHARED|MAP_FIXED, fd,
	(off_t) 0xE0000, videoFd, 1, 0 );		/*	S002	*/

		if (addr1 == (caddr_t)-1) {
		if( Debug )
			ErrorF("interpInit: Cannot mmap SYS ROM 0xe0000\n");
		close(videoFd);
		close(fd);
		return(FAILURE);
		}

		if( Debug )
			ErrorF("SYS ROM memory mapped to %#x\n", addr1 );
	}

	close(videoFd);
	close(fd);

	return(SUCCESS);

}

static void
do_int10(struct REGS *sregs, struct REGS *dregs)
{
	unsigned short *ivec = (unsigned short *)0;


	sregs->Cs = *(ivec+0x21);
	sregs->Ip = *(ivec+0x20);		/* Initialize the video card */
	sregs->Ss = 0;
	sregs->Sp = 0;
	base_mem =  (unsigned char *)0;
	decode_instructions(OptionFlags, sregs, dregs);
}

VBiosINT10(preg, nreg)
    register dword_t		*preg;
    int				nreg;
{
	struct REGS sregs,dregs;

	if ( Debug )
		ErrorF("VBiosINT10 nreg %d\n", nreg );

        if ( Debug )
            switch (nreg)
            {
              case 8:		ErrorF("reg 7 %#010x\n", preg[7]);
              case 7:		ErrorF("reg 6 %#010x\n", preg[6]);
              case 6:		ErrorF("reg 5 %#010x\n", preg[5]);
              case 5:		ErrorF("reg 4 %#010x\n", preg[4]);
              case 4:		ErrorF("reg 3 %#010x\n", preg[3]);
              case 3:		ErrorF("reg 2 %#010x\n", preg[2]);
              case 2:		ErrorF("reg 1 %#010x\n", preg[1]);
              case 1:		ErrorF("reg 0 %#010x\n", preg[0]);
            }


	switch (nreg)
	{
	case 8:		sregs.Es = preg[7];
	case 7:		sregs.Bp = preg[6];
	case 6:		sregs.Di = preg[5];
	case 5:		sregs.Si = preg[4];
	case 4:		sregs.Dx = preg[3];
	case 3:		sregs.Cx = preg[2];
	case 2:		sregs.Bx = preg[1];
	case 1:		sregs.Ax = preg[0];
	}

	do_int10(&sregs,&dregs);

	switch (nreg)
	{
	case 8:		preg[7] = dregs.Es;
	case 7:		preg[6] = dregs.Bp;
	case 6:		preg[5] = dregs.Di;
	case 5:		preg[4] = dregs.Si;
	case 4:		preg[3] = dregs.Dx;
	case 3:		preg[2] = dregs.Cx;
	case 2:		preg[1] = dregs.Bx;
	case 1:		preg[0] = dregs.Ax;
	}

	if ( Debug )
		ErrorF("VBiosINT10 returning\n" );

	return SUCCESS;
}


#ifdef NOT_USED
static int
do_intx(int inum,struct REGS *sregs, struct REGS *dregs)
{
	unsigned short *ivec = (unsigned short *)0;

	sregs->Ip = *(ivec+inum*2);
	sregs->Cs = *(ivec+inum*2+1);
	sregs->Ss = sregs->Sp = 0;
	base_mem = (unsigned char *)0;
	decode_instructions(OptionFlags, sregs, dregs);
}

/*
 * VBiosINTn(preg, nreg) -	execute an INT N using the nreg registers
 *				pointed to by preg.
 */
VBiosINTn(preg, nreg, vec)
    register dword_t		*preg;
    int				nreg;
    int				vec;
{
	struct REGS sregs,dregs;

	switch (nreg)
	{
	case 8:		sregs.Es = preg[7];
	case 7:		sregs.Bp = preg[6];
	case 6:		sregs.Di = preg[5];
	case 5:		sregs.Si = preg[4];
	case 4:		sregs.Dx = preg[3];
	case 3:		sregs.Cx = preg[2];
	case 2:		sregs.Bx = preg[1];
	case 1:		sregs.Ax = preg[0];
	}

/*	Need to figure out what to do with the vec	*/
	do_intx(&sregs,&dregs);

	switch (nreg)
	{
	case 8:		preg[7] = dregs.Es;
	case 7:		preg[6] = dregs.Bp
	case 6:		preg[5] = dregs.Di;
	case 5:		preg[4] = dregs.Si;
	case 4:		preg[3] = dregs.Dx;
	case 3:		preg[2] = dregs.Cx;
	case 2:		preg[1] = dregs.Bx;
	case 1:		preg[0] = dregs.Ax;
	}
}

#endif /*	NOT_USED	*/

static void
call_far(struct REGS *sregs, struct REGS *dregs, unsigned short cseg, unsigned short ip)
{
	unsigned short *ivec = (unsigned short *)0;

	sregs->Ip = ip;
	sregs->Cs = cseg;
	sregs->Ss = sregs->Sp = 0;
	base_mem = (unsigned char *)0;
	decode_instructions(OptionFlags, sregs, dregs);
}

VBiosCall(seg, off, preg, nreg)
int			seg;
int			off;
register dword_t	*preg;
int			nreg;
{
	struct REGS sregs,dregs;

	if ( Debug )
		ErrorF("VBiosCall nreg %d\n", nreg );

	switch (nreg)
	{
	case 8:		sregs.Es = preg[7];
	case 7:		sregs.Bp = preg[6];
	case 6:		sregs.Di = preg[5];
	case 5:		sregs.Si = preg[4];
	case 4:		sregs.Dx = preg[3];
	case 3:		sregs.Cx = preg[2];
	case 2:		sregs.Bx = preg[1];
	case 1:		sregs.Ax = preg[0];
	}

	call_far(&sregs,&dregs, (unsigned short) seg, (unsigned short) off);

	switch (nreg)
	{
	case 8:		preg[7] = dregs.Es;
	case 7:		preg[6] = dregs.Bp;
	case 6:		preg[5] = dregs.Di;
	case 5:		preg[4] = dregs.Si;
	case 4:		preg[3] = dregs.Dx;
	case 3:		preg[2] = dregs.Cx;
	case 2:		preg[1] = dregs.Bx;
	case 1:		preg[0] = dregs.Ax;
	}

	if ( Debug )
		ErrorF("VBiosCall returning\n" );

	return SUCCESS;
}

#endif	/*	gemini	*/

