/*
 *	@(#) v86bios.c 12.2 95/10/31 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Sep 07 19:48:57 PDT 1992	buckm@sco.com
 *	- Created from working demo source supplied by Mike Davidson.
 *	  All the nice bits are his; any nasty bits are my fault.
 *	S001	Tue Oct 27 07:21:51 PST 1992	buckm@sco.com
 *	- Put in a few more debug printf's.
 *	S002	Mon Nov 02 09:12:20 PST 1992	buckm@sco.com
 *	- Some adjustments so this stuff can live in a daemon. 
 *	- Make v86opts an arg to VBiosInit().
 *	- Change all printf's to Eprint's.
 *	S003	Thu Nov 05 21:09:24 PST 1992	buckm@sco.com
 *	- Add more debug stuff to chase down wierd bios execution errors.
 *	- Adjust location of VBIOS_FLAGS.
 *	- Load at least 32k of adapter rom; some adapters leave a hole
 *	  from c6000 to c6800, and we don't want to miss the 2nd half.
 *	S004	Mon Nov 16 22:19:14 PST 1992	buckm@sco.com
 *	- Add v86opts flag to force loading of sysrom;
 *	  some adapters seem to make calls to INT 15 (System Services).
 *	S005	Mon Mar 29 00:16:32 PST 1993	buckm@sco.com
 *	- Add VBiosCall() for CallRom support.
 *	- Add seg and memfile args to VBiosInit().
 *	- Add timeout to terminate looping in v86 mode.
 *	- Add IOTRACE and TICKPRINT debug features.
 *	- Emulate CMOS access via ports 0x70 and 0x71.
 *	- Init xtss_off to 0 (unlimited users) to avoid
 *	  running afoul of vpix serialization.
 *	S006	Fri Oct 15 14:09:00 PDT 1993	buckm@sco.com
 *	- MCA machines don't mark extended sysrom with a magic number,
 *	  so always load from E0000 when loading sysrom.
 *	S007	Wed Feb 16 15:17:58 PST 1994	staceyc@sco.com
 *	- Added int N support, mainly for PCI BIOS execution, also moved
 *	opt defines into header file so I can use them in pciinfo source
 *	S008    Fri Jun 03 17:32:00 PST 1994    rogerv@sco.com
 *	- Added 32-bit I/O emulation (doubleword).  
 *	  Note that this is sccs ids 11.2 and 11.3.  For 11.2, did code
 *	  change, and for 11.3 put in this comment that was left out last
 *	  time.
 *	S009	Fri Feb  3 10:53:01 PST 1995	brianm@sco.com
 *	- added in a check to verify that the vid bios rom is starting at
 *	  0xC0000, if it is between 0xC0000 and 0xC8000, we try 0xC0000
 *	  to see if there is a valid ID, if so, we set up to get the BIOS
 *	  from here, if not we get it from where int 10 pointed to.
 *	S010	Tue Oct 31 11:53:25 PST 1995	brianm@sco.com
 *	- added in a bitmap initialization call V86MemInit, ifdefd
 *	  on V86_MAGIC1_PUM, which will call the v86init with an additional
 *	  argument, allowing us to not reserve the space between 
 *	  4K and 636K when running vbiosd.  Should not change any other 
 *	  programs behaviour.
 */

/*
 * v86bios.c	- v86 mode interface to video bios functions
 */

#include	<bool.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/immu.h>
#include	<sys/region.h>
#include	<sys/proc.h>
#include	<sys/tss.h>
#include	<sys/v86.h>
#include	<sys/sysi86.h>

#include	"v86bios.h"
#include	"v86opts.h"

/* Do some things differently if we are in the server */
#ifdef XSERVER
#define	Eprint		ErrorF
#define	perror		Error
#endif /* XSERVER */

/*
 * V86 mode TSS and extended TSS
 */
static struct tss386		*v86tss;
static struct v86xtss		*xtss;

static char *memfile = "/dev/mem";

/*
 * Low memory usage - try to stay within the 1st page.
 */
#define	VBIOS_VECS	0x0000	/* bios vectors/data from 0x0000 thru 0x04FF */
#define	VBIOS_SS	0x0050	/* stack from 0x0500 thru 0x06FB */
#define	VBIOS_SP	0x01FC
#define	VBIOS_FLAGS	0x06FC	/* cli/sti, pushf/popf emulation */
#define	VBIOS_CS	0x0070	/* code  from 0x0700 thru 0x07FF */
#define	VBIOS_IP	0x0000
#define	VBIOS_DS	0x0080	/* data  from 0x0800 thru 0x1000 */

/*
 * Time limit for a single v86 invocation.
 */
#define	MAX_TICKS	(60 * 20)	/* 20 ticks/second; sixty seconds */

static int v86opts = 0;

#define	debug	if (v86opts & OPT_DEBUG) Eprint
#define	iotrace	if (v86opts & OPT_IOTRACE) Eprint


/*
 * VBiosInit() - initialise video BIOS
 */
VBiosInit(opts, seg, dbgmemfile)
    int			opts;
    int			seg;
    char		*dbgmemfile;
{
    int			r;
    int			memfd;
    unsigned long	v, s;
    byte_t		hdr[3];

    v86opts = opts;

    /*
     * initialise the V86 mode environment
     */
    if ((r = V86Init()) != 0)
    {
	perror("V86Init");
	return r;
    }

    /*
     * open the memory file
     */
    if (dbgmemfile)
    {
	memfile = dbgmemfile;
	debug("using memfile '%s'\n", memfile);
    }
    if ((memfd = open(memfile, 0)) < 0)
    {
	perror(memfile);
	return -1;
    }

    /*
     * load the real-mode interrupt vectors and BIOS data area
     */
    if (VBiosMemLoad(memfd, 0x0, 0x0, 0x500) < 0)
    {
	perror("VBiosMemLoad lowmem");
	goto err;
    }

    /*
     * if seg was passed in, use it as the video ROM location,
     * else use the INT 10 vector to locate the video BIOS
     */
    if (seg)
    {
	v = seg << 4;
	debug("ROM seg = %04x\n", seg);
    }
    else
    {
	fptr_t f;

	if (v86opts & OPT_LDPCIROM)
		f = ((fptr_t *) 0)[0x1a];
	else
		f = ((fptr_t *) 0)[0x10];

	v = SEGMENT(f) << 4;
	debug("INT %s = %04x:%04x\n", (v86opts & OPT_LDPCIROM) ? "1A" : "10",
	    SEGMENT(f), OFFSET(f));
    }

    /*
     * load the video BIOS
     */
    hdr[0] = hdr[1] = 0;
    if (v < 0xE0000)			/* should be in adapter ROM */
    {
	if (v > 0xC0000 && v < 0xC8000)	/* vvv S009 start */
	{
		VBiosMemLoad(memfd, 0xC0000, hdr, 3);
		if (*(word_t *)hdr == 0xAA55)	/* good magic */
		{
			debug("alternate " );
			v = 0xC0000;
		}
	}				/* ^^^ S009 end */
	VBiosMemLoad(memfd, v, hdr, 3);
	debug("adprom magic=%04x size=%x\n", *(word_t *)hdr, hdr[2] * 512);
	if (*(word_t *)hdr != 0xAA55)	/* bad magic */
	{
	    goto err;
	}
	s = hdr[2] * 512;
	if (s < 0x8000)
		s = 0x8000;		/* load at least 32k */
	if (VBiosMemLoad(memfd, v, v, s) < 0)
	{
	    perror("VBiosMemLoad adprom");
	    goto err;
	}
	if (VBiosChecksum(v) < 0)
	{
	    Eprint("VBiosChecksum bad");
	    goto err;
	}
    }

    /*
     * load system ROM if needed
     */
    if ((v >= 0xE0000) || (v86opts & OPT_LDSYSROM))
    {
	debug("loading sysrom\n");
	v = 0xE0000;
	s = 0x20000;
	if (VBiosMemLoad(memfd, v, v, s) < 0)
	{
	    perror("VBiosMemLoad sysrom");
	    goto err;
	}
    }

    close(memfd);
    return 0;

  err:
    close(memfd);
    return -1;
}

/*
 * VBiosINT10(preg, nreg) -	execute an INT 10 using the nreg registers
 *				pointed to by preg.
 */
VBiosINT10(preg, nreg)
    register dword_t		*preg;
    int				nreg;
{
    register struct tss386	*t = v86tss;
    register byte_t		*p;
    int				r;

    /*
     * load parameters
     */
    switch (nreg)
    {
	case 8:		ES(t)  = preg[7];
	case 7:		EBP(t) = preg[6];
	case 6:		EDI(t) = preg[5];
	case 5:		ESI(t) = preg[4];
	case 4:		EDX(t) = preg[3];
	case 3:		ECX(t) = preg[2];
	case 2:		EBX(t) = preg[1];
	case 1:		EAX(t) = preg[0];
    }

    /*
     * load code and stack registers
     */
    CS(t) = VBIOS_CS;
    IP(t) = VBIOS_IP;
    SS(t) = VBIOS_SS;
    SP(t) = VBIOS_SP;

    /*
     * put an INT 10 followed by a HLT instruction in the code segment
     */
    p = (byte_t *) ((VBIOS_CS << 4) | VBIOS_IP);
    p[0] = 0xCD;
    p[1] = 0x10;
    p[2] = 0xF4;

    r = V86Run();
    
    /*
     * save results
     */
    switch (nreg)
    {
	case 8:		preg[7] = ES(t);
	case 7:		preg[6] = EBP(t);
	case 6:		preg[5] = EDI(t);
	case 5:		preg[4] = ESI(t);
	case 4:		preg[3] = EDX(t);
	case 3:		preg[2] = ECX(t);
	case 2:		preg[1] = EBX(t);
	case 1:		preg[0] = EAX(t);
    }

    return (r == ERR_V86_HALT) ? 0 : r;
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
    register struct tss386	*t = v86tss;
    register byte_t		*p;
    int				r;

    /*
     * load parameters
     */
    switch (nreg)
    {
	case 8:		ES(t)  = preg[7];
	case 7:		EBP(t) = preg[6];
	case 6:		EDI(t) = preg[5];
	case 5:		ESI(t) = preg[4];
	case 4:		EDX(t) = preg[3];
	case 3:		ECX(t) = preg[2];
	case 2:		EBX(t) = preg[1];
	case 1:		EAX(t) = preg[0];
    }

    /*
     * load code and stack registers
     */
    CS(t) = VBIOS_CS;
    IP(t) = VBIOS_IP;
    SS(t) = VBIOS_SS;
    SP(t) = VBIOS_SP;

    /*
     * put an INT vec followed by a HLT instruction in the code segment
     */
    p = (byte_t *) ((VBIOS_CS << 4) | VBIOS_IP);
    p[0] = 0xCD;
    p[1] = vec;
    p[2] = 0xF4;

    r = V86Run();
    
    /*
     * save results
     */
    switch (nreg)
    {
	case 8:		preg[7] = ES(t);
	case 7:		preg[6] = EBP(t);
	case 6:		preg[5] = EDI(t);
	case 5:		preg[4] = ESI(t);
	case 4:		preg[3] = EDX(t);
	case 3:		preg[2] = ECX(t);
	case 2:		preg[1] = EBX(t);
	case 1:		preg[0] = EAX(t);
    }

    return (r == ERR_V86_HALT) ? 0 : r;
}

/*
 * VBiosCall(seg, off, preg, nreg) -	call into ROM at seg:off
 *					using the nreg registers
 *					pointed to by preg.
 */
VBiosCall(seg, off, preg, nreg)
    int				seg;
    int				off;
    register dword_t		*preg;
    int				nreg;
{
    register struct tss386	*t = v86tss;
    byte_t			*p;
    int				r;

    /*
     * load parameters
     */
    switch (nreg)
    {
	case 8:		ES(t)  = preg[7];
	case 7:		EBP(t) = preg[6];
	case 6:		EDI(t) = preg[5];
	case 5:		ESI(t) = preg[4];
	case 4:		EDX(t) = preg[3];
	case 3:		ECX(t) = preg[2];
	case 2:		EBX(t) = preg[1];
	case 1:		EAX(t) = preg[0];
    }

    /*
     * load code and stack registers
     */
    CS(t) = VBIOS_CS;
    IP(t) = VBIOS_IP;
    SS(t) = VBIOS_SS;
    SP(t) = VBIOS_SP;

    /*
     * put a far CALL to seg:off followed by a HLT instruction
     * in the code segment
     */
    p = (byte_t *) ((VBIOS_CS << 4) | VBIOS_IP);
    *p++ = 0x9A;
    *((word_t *) p)++ = off;
    *((word_t *) p)++ = seg;
    *p = 0xF4;

    r = V86Run();
    
    /*
     * save results
     */
    switch (nreg)
    {
	case 8:		preg[7] = ES(t);
	case 7:		preg[6] = EBP(t);
	case 6:		preg[5] = EDI(t);
	case 5:		preg[4] = ESI(t);
	case 4:		preg[3] = EDX(t);
	case 3:		preg[2] = ECX(t);
	case 2:		preg[1] = EBX(t);
	case 1:		preg[0] = EAX(t);
    }

    return (r == ERR_V86_HALT) ? 0 : r;
}

/*
 * VBiosMemLoad() - load V86 address space from a file
 */
VBiosMemLoad(fd, off, virt, len)
    int		fd;
    long	off;
    long	virt;
    long	len;
{

    if (lseek(fd, off, 0) != off || read(fd, (char *) virt, len) != len)
	return -1;

    return 0;
}

/*
 * VBiosChecksum() - check the integrity of the video BIOS
 */
VBiosChecksum(b)
    register unsigned char *b;
{
    register int c;
    int	 n;

    n	= b[2] * 512;
    c	= 0;
    while (--n >= 0)
	c += *b++;

    return (c & 0xff);
}


#ifdef	V86_MAGIC1_PUM	/* S010 Begin */
static char *V86PageUseMap = NULL;

V86MemInit( ptr )
char *ptr;
{
    V86PageUseMap = ptr;
}

#endif			/* S010 End */

/*
 * V86 mode i/o permission bitmap
 *
 * v86init() requires that the xtss structure including
 * the i/o bitmap be contained within a single 4K page.
 * TSS_IO_BITMAP_SIZE is 0x80 which allows i/o addresses
 * up to 0x3ff to be handled directly through the TSS.
 * i/o accesses above 0x3ff always trap into the V86
 * instruction emulation code which uses a full 8K bitmap
 * to keep track of the entire 16 bit i/o space.
 */
#define	MAX_IO_ADDR		0xFFFF
#define	IO_BITMAP_SIZE		8192

#define	TSS_MAX_IO_ADDR		0x3FF
#define TSS_IO_BITMAP_SIZE	((TSS_MAX_IO_ADDR + 8) / 8)

static unsigned char		*io_bitmap;

/*
 * V86Init() - initialise the v86 environment
 */
V86Init()
{
    struct v86parm	v86parms;
    int			i;

    /*
     * set up parameters for v86init()
     */
    v86parms.magic[0]	= V86_MAGIC0;
    v86parms.magic[1]	= V86_MAGIC1;
    v86parms.xtssp	= 0;
    v86parms.szxtss	= sizeof(struct v86xtss);
    v86parms.szbitmap	= (v86opts & OPT_IOTRACE) ? 0 : TSS_IO_BITMAP_SIZE + 1;
    v86parms.xtss_off	= 0;	/* unlimited users */

#ifdef	V86_MAGIC1_PUM		/* S010 Begin */
    if (V86PageUseMap != NULL)
    {
        v86parms.pageusemap = V86PageUseMap;
        v86parms.magic[1] = V86_MAGIC1_PUM;
    }
    if (sysi86(SI86V86, V86SC_INIT, &v86parms) != 0)
    {
        v86parms.pageusemap = NULL;
        v86parms.magic[1]   = V86_MAGIC1;
        if (sysi86(SI86V86, V86SC_INIT, &v86parms) != 0)
	     return -1;
    }
#else
    if (sysi86(SI86V86, V86SC_INIT, &v86parms) != 0)
	return -1;
#endif				/* S010 End */

    /*
     * v86init() returns a pointer to the xtss structure
     */
    xtss	= v86parms.xtssp;
    v86tss	= &xtss->xt_tss;

    /*
     * set up fields in the xtss that are used by the kernel
     * emulation code
     */

    /*
     * this is the location that will be used by the kernel
     * for cli/sti/popf emulation to keep track of the state
     * of the interrupt mask bit in the flags register
     */
    xtss->xt_vflptr	= (unsigned int *) VBIOS_FLAGS;

    /*
     * xt_vimask, xt_vitoect and xt_viflag are bitmasks used by the
     * kernel for exception handling as follows
     *
     * xt_vimask	is set in user mode and read by the kernel
     *			it defines which exceptions will wake up
     *			a process sleeping in v86sleep()
     * xt_vitoect	is set in user mode and read by the kernel
     *			it defines which exceptions should cause a
     *			task switch to 386 mode
     * xt_viflag	is set by the kernel and cleared in user mode
     *			it defines which exceptions are currently pending
     */

    /*
     * not using v86sleep() so set vi_mask to V86VI_NONE
     */
    xtss->xt_vimask	= V86VI_NONE;

    /*
     * switch to 386 mode to handle processor traps in V86 mode and
     * signals that arrive when running in V86 mode
     */
    xtss->xt_vitoect	= V86VI_TIMER		/* virtual timer	*/
			| V86VI_DIV0		/* divide by 0		*/
			| V86VI_SGLSTP		/* single step		*/
			| V86VI_BRKPT		/* breakpoint		*/
			| V86VI_OVERFLOW	/* overflow		*/
			| V86VI_BOUND		/* bounds trap		*/
			| V86VI_INVOP		/* illegal instruction	*/
			| V86VI_SIGHDL;		/* signal pending	*/

    /*
     * xt_imaskbits is a bitmap of 256 bits that define whether
     * particular 8086 software interrupts should be emulated
     * by the kernel general protection trap handler or should
     * trap to user mode emulation. Bits set to 0 are directly
     * emulated in the kernel, bits set to 1 cause a task switch
     * to 386 mode for emulation
     */
    for (i = 0; i < V86_IMASKBITS; i++)
	xtss->xt_imaskbits[i]	= 0;	

    xtss->xt_tss.t_bitmapbase	= v86parms.szxtss << 16;
    io_bitmap			= (unsigned char *)xtss + v86parms.szxtss;
    memset(io_bitmap, 0xff, IO_BITMAP_SIZE + 1);

    return 0;
}

/*
 * V86MemMap() - map physical addresses into the V86 address space
 */
V86MemMap(phys, virt, len)
    paddr_t	phys;
    caddr_t	virt;
    int		len;
{
    struct v86memory	v86mem;
    int			r;

    v86mem.vmem_cmd	= V86MEM_MAP;
    v86mem.vmem_physaddr= phys;
    v86mem.vmem_membase	= virt;
    v86mem.vmem_memlen	= len;
    if ((r = sysi86(SI86V86, V86SC_MEMFUNC, &v86mem)) < 0)
	perror("V86MemMap");
    return r;
}

#if 0	/* UNUSED */
/*
 * V86MemUnmap() - unmap physical addresses from the V86 address space
 */
V86MemUnmap(phys, virt, len)
    paddr_t	phys;
    caddr_t	virt;
    int		len;
{
    struct v86memory	v86mem;
    int			r;

    v86mem.vmem_cmd	= V86MEM_UNMAP;
    v86mem.vmem_physaddr= phys;
    v86mem.vmem_membase	= virt;
    v86mem.vmem_memlen	= len;
    if ((r = sysi86(SI86V86, V86SC_MEMFUNC, &v86mem)) < 0)
	perror("V86MemUnmap");
    return r;
}
#endif

/*
 * V86IOEnable() - enable a range of i/o addresses
 */
V86IOEnable(ioaddr, n)
    unsigned	ioaddr;
    int		n;
{
    if ((ioaddr + n) > MAX_IO_ADDR)
	return -1;

    while (--n >= 0)
    {
	int	index	= ioaddr >> 3;
	int	mask	= ~(1 << (ioaddr & 7));

	io_bitmap[index] &= mask;
	ioaddr++;
    }

    return 0;
}

#if 0	/* UNUSED */
/*
 * V86IODisable() - disable a range of i/o addresses
 */
V86IODisable(ioaddr, n)
    unsigned	ioaddr;
    int		n;
{
    if ((ioaddr + n) > MAX_IO_ADDR)
	return -1;

    while (--n >= 0)
    {
	int	index	= ioaddr >> 3;
	int	mask	= 1 << (ioaddr & 7);

	io_bitmap[index] |= mask;
	ioaddr++;
    }

    return 0;
}
#endif

#define	IOEnabled(addr, mask)	! ( *(unsigned short *)(io_bitmap+(addr>>3)) \
				   & (mask << (addr & 7)) )

#define	IO_BYTE_MASK		0x01
#define	IO_WORD_MASK		0x03
#define IO_DOUBLEWORD_MASK	0x0f

#define OPERAND_SIZE_OVERRIDE_PREFIX   0x66

/*
 * V86Run() - switch to V86 mode
 */
V86Run()
{
    register struct v86xtss	*x	= xtss;
    register struct tss386	*t	= v86tss;
    int		stat;

    x->xt_timer_count	= 0;
    x->xt_timer_bound	= 1000;
    x->xt_viflag	= 0;	/* clear any old exception flags	*/

    do
    {
	x->xt_magicstat	= XT_MSTAT_PROCESS;

	EnterV86Mode();

	stat		= x->xt_magicstat;
	x->xt_magicstat	= XT_MSTAT_NOPROCESS;

	if (stat == XT_MSTAT_OPSAVED)
	    *(byte_t *)((CS(t) << 4) + IP(t)) = x->xt_magictrap;

	/*
	 * check if there are any exceptions that we care about
	 */
	if (x->xt_viflag & x->xt_vitoect)
	    stat = V86Exception();
	else
	    stat = V86Emulate();

    } while (stat == 0);

    return stat;
}


/*
 * V86Exception() - handle V86 mode exceptions
 */
V86Exception()
{
    register struct v86xtss	*x	= xtss;
    int				viflag	= x->xt_viflag & x->xt_vitoect;

    if (viflag & V86VI_SIGHDL)		/* signal handler	*/
    {
	x->xt_viflag &= ~V86VI_SIGHDL;
	(*x->xt_hdlr)(x->xt_signo);
	/*
	 * set xt_signo to 0 to tell the kernel that there
	 * is no longer a pending signal
	 */
	x->xt_signo	= 0;
    }

    if (viflag & V86VI_TIMER)		/* virtual clock tick */
    {
	x->xt_viflag &= ~V86VI_TIMER;
	if (v86opts & OPT_TICKPRINT)
	    Eprint("tick=%03d  pc=%04x:%04x\n",
		    x->xt_timer_count, CS(v86tss), IP(v86tss));
	if (x->xt_timer_count >= MAX_TICKS)
	    return ERR_V86_TIMEOUT;
    }

    if ((viflag &= ~(V86VI_SIGHDL | V86VI_TIMER)) == 0)
	return 0;

    debug("exception: viflag=0x%x\n", viflag);

    if (viflag & V86VI_DIV0)
	return ERR_V86_DIV0;

    if (viflag & V86VI_SGLSTP)
	return ERR_V86_SGLSTP;

    if (viflag & V86VI_BRKPT)
	return ERR_V86_BRKPT;

    if (viflag & V86VI_OVERFLOW)
	return ERR_V86_OVERFLOW;

    if (viflag & V86VI_BOUND)
	return ERR_V86_BOUND;

    if (viflag & V86VI_INVOP)
	return ERR_V86_ILLEGAL_OP;

    return -1;
}

/*
 * V86Emulate() -	called to handle emulation of instructions
 *			that could not be directly executed in V86 mode
 *
 *			also handles the emulation of i/o accesses to i/o
 *			addresses beyond the limit (0x3ff) of the i/o
 *			bitmap in the TSS.
 */
V86Emulate()
{
    register struct tss386	*t	= v86tss;
    byte_t			*codeseg;
    unsigned			ip;
    byte_t			op;
    unsigned			addr;
    int				doubleword_io = FALSE;

    codeseg	= (byte_t *)(CS(t) << 4);
    ip		= IP(t);

    /*
     * If trapped on an operand size prefix byte, then collect next
     * byte for opcode...
     * QUESTION: Is there any case where we don't want to suck up
     *		 the prefix byte?
     */
    op = codeseg[ip++];
    if (op == OPERAND_SIZE_OVERRIDE_PREFIX) {
	doubleword_io = TRUE;
	iotrace("Operand size override prefix found, code %x %x\n",
		 op, codeseg[ip]); /* let's see what instructions
				      we're getting */
	op = codeseg[ip++];	/* get the actual requested op code */
    }

    switch (op)
    {
	case 0xe4:		/*	in	al, imm8	*/
	case 0xec:		/*	in	al, dx		*/
	    addr = (op & 0x08) ? DX(t) : codeseg[ip++];
	    if (IOEnabled(addr, IO_BYTE_MASK))
	    {
		AL(t) = inp(addr);
		iotrace("inb(%x) = %x\n", addr, AL(t));
	    }
	    else if (V86IOEmulate(op, addr))
		return ERR_V86_ILLEGAL_IO;
	    break;

	case 0xe5:		/*	in	[e]ax, imm8	*/
	case 0xed:		/*	in	[e]ax, dx	*/
	    addr = (op & 0x08) ? DX(t) : codeseg[ip++];
	    if (!doubleword_io && IOEnabled(addr, IO_WORD_MASK)) {
		AX(t) = inpw(addr);
		iotrace("inw(%x) = %x\n", addr, AX(t));
	    } else if (doubleword_io && IOEnabled(addr, IO_DOUBLEWORD_MASK)) {
		EAX(t) = inpd(addr);
                iotrace("ind(%x) = %x\n", addr, EAX(t));
	    } else if (V86IOEmulate(op, addr))
			/* assumes that no other trapped requests will validly
			   have an operand size override prefix, since we
			   would have sucked it up earlier...and doubleword_io
			   is not passed down... */
		return ERR_V86_ILLEGAL_IO;
	    break;

	case 0xe6:		/*	out	imm8, al	*/
	case 0xee:		/*	out	dx, al		*/
	    addr = (op & 0x08) ? DX(t) : codeseg[ip++];
	    if (IOEnabled(addr, IO_BYTE_MASK))
	    {
		outp(addr, AL(t));
		iotrace("outb(%x, %x)\n", addr, AL(t));
	    }
	    else if (V86IOEmulate(op, addr))
		return ERR_V86_ILLEGAL_IO;
	    break;

	case 0xe7:		/*	out	imm8, [e]ax	*/
	case 0xef:		/*	out	dx, [e]ax	*/
	    addr = (op & 0x08) ? DX(t) : codeseg[ip++];
	    if (!doubleword_io && IOEnabled(addr, IO_WORD_MASK)) {
		outpw(addr, AX(t));
		iotrace("outw(%x, %x)\n", addr, AX(t));
	    } else if (doubleword_io && IOEnabled(addr, IO_DOUBLEWORD_MASK)) {
		outpd(addr, EAX(t));
		iotrace("outd(%x, %x)\n", addr, EAX(t));
	    } else if (V86IOEmulate(op, addr))
			/* assumes that there are no other trapped requests
			   that can validly have operand size override
			   prefix byte, since we would have sucked it up
			   earlier...and doubleword_io is not passed down... */
		return ERR_V86_ILLEGAL_IO;
	    break;

	case 0xf4:		/* hlt				*/
	    return ERR_V86_HALT;

	default:
	    if (v86opts & OPT_DEBUG)
	    {
		Eprint("illegal op %02x at %04x:%04x\n", op, CS(t), IP(t));
		V86PrintRegs();
	    }
	    return ERR_V86_ILLEGAL_OP;
    } /* end opcode switch */

    /*
     * since everything was emulated OK update IP in the TSS
     * and return 0 to indicate that it is OK to go back to
     * V86 mode
     */

    IP(t)	= ip;
    return 0;
}

/*
 * V86IOEmulate() -	i/o emulation
 *
 *			this is the common handler for all
 *			illegal i/o requests
 */
V86IOEmulate(op, addr)
    byte_t	op;
    unsigned	addr;
{
    register struct tss386	*t	= v86tss;

    if ((addr & ~1) == 0x70)
    {
	V86CMOSEmulate(op, addr);
	return 0;
    }

    if (v86opts & OPT_IOPRINT)
	Eprint("%04x:%04x  op = %02x  illegal BIOS i/o to port %x\n",
	    CS(t), IP(t), op, addr);

    return (v86opts & OPT_IOERROR);
}

/*
 * V86CMOSEmulate() -	CMOS i/o emulation
 *
 *			maintain our own soft copy of CMOS RAM
 */
V86CMOSEmulate(op, addr)
    byte_t	op;
    unsigned	addr;
{
    static short		cmos[128];
    static int 			index	= -1;
    register struct tss386	*t	= v86tss;

    if (index < 0)
    {
	for (index = 128; --index >= 0; )
	    cmos[index] = -1;
	index = 0;
    }

    if (! (addr & 1))		/* index port involved */
    {
	if (op & 2)		    /* out */
	    index = AL(t) & 0x7F;	/* high bit is NMI mask */
	else			    /* in  */
	    AL(t) = index;		/* return 0xFF like hardware ? */
    }

    if ((addr | op) & 1)	/* data port involved */
    {
	if (op & 2)		    /* out */
	{
	    cmos[index] = (addr & 1) ? AL(t) : AH(t);
	    debug("write CMOS[%x] = %x\n", index, cmos[index]);
	}
	else			    /* in  */
	{
	    if (cmos[index] < 0)
	    {
		outp(0x70, index);
		/*
		 * the kernel could screw us up right here
		 * by interrupting and changing port 0x70,
		 * but we really don't want to start using
		 * cli/sti in user-mode code 
		 */
		cmos[index] = inp(0x71);
	    }
	    if (addr & 1) 
		AL(t) = cmos[index];
	    else
		AH(t) = cmos[index];
	    debug("read CMOS[%x] = %x\n", index, cmos[index]);
	}
    }
}

/*
 * V86PrintRegs() -	print some v86 registers and a bit of stack
 */
V86PrintRegs()
{
    register struct tss386	*t	= v86tss;
    register word_t		*sptr;
    register int		i;
    word_t			sp;

    Eprint("Registers:\n");
    Eprint("AX=%04x  BX=%04x  CX=%04x  DX=%04x  ", AX(t), BX(t), CX(t), DX(t));
    Eprint("SI=%04x  DI=%04x  BP=%04x\n",	   SI(t), DI(t), BP(t));
    Eprint("DS=%04x  ES=%04x  FL=%04x  CS=%04x  ", DS(t), ES(t), FL(t), CS(t));
    Eprint("IP=%04x  SS=%04x  SP=%04x\n",	   IP(t), SS(t), SP(t));

    Eprint("Stack:");
    sp = SP(t) - 0x10;
    sptr = (word_t *)((SS(t) << 4) + sp);
    for (i = 0; i < 40; ++i)
    {
	if ((i & 7) == 0)
	{
	    Eprint("\n%04x:%04x ", SS(t), sp);
	    sp += 0x10;
	}
	Eprint(" %04x", *sptr++);
    }
    Eprint("\n");
}
