/*
 * File hw_util.c
 * Misc utilities
 *
 * @(#) hw_util.c 65.1 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <prototypes.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/immu.h>
#include <sys/disp.h>
#include <sys/region.h>
#include <limits.h>
#include <sys/immu.h>
#include <sys/ci/ciioctl.h>
#include <filehdr.h>
#include <dirent.h>
#include <sys/bootinfo.h>
#include <sys/conf.h>		/* struct cdevsw, struct bdevsw */
#include <stdlib.h>
#include <sys/msr.h>

int __scoinfo(struct scoutsname *buf, int bufsize);

#include <filehdr.h>
#include <ldfcn.h>

#undef n_name		/* Due to conflict with _n._n_name */
#include <nlist.h>

#include "hw_util.h"

FILE	*errorFd = stderr;

static int	is_cpu_specific(u_long kaddr);
static int	lock_cpu(int cpu);
static int	unlock_cpu(void);

const char	Yes[] = "Yes";		/* For bool messages */
const char	No[] =  "No ";

const char	Enabled[] = "Enabled";
const char	Disabled[] =  "Disabled ";

int		verbose = 0;
int		debug = 0;

const char	dflt_lib_dir[] = HWLIBDIR;
const char	lib_dir_env[] = "HWLIBDIR";
const char	*lib_dir = NULL;

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Print Routines				 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

void
debug_print(const char *fmt, ...)
{
    va_list	args;


    if (!debug || !fmt || !*fmt)
	return;

    fprintf(errorFd, "\nDEBUG: ");
    va_start(args, fmt);
    vfprintf(errorFd, fmt, args);
    va_end(args);
    putc('\n', errorFd);
}

void
error_print(const char *fmt, ...)
{
    va_list	args;


    if (!fmt || !*fmt)
	return;

    fprintf(errorFd, "ERROR: ");
    va_start(args, fmt);
    vfprintf(errorFd, fmt, args);
    va_end(args);
    putc('\n', errorFd);
}

#define DMPLEN 16

void
debug_dump(const void *buf, u_long len, u_long rel_adr, const char *fmt, ...)
{
    register int	x, y;
    u_char		text[DMPLEN];
    va_list		args;
    int			state;
    const u_char	*adr = (u_char *)buf;


    if (!debug)
	return;

    va_start(args, fmt);
    vfprintf(errorFd, fmt, args);
    va_end(args);
    fprintf(errorFd, " length: %lu", len);

    if (!len)
    {
	fprintf(errorFd, "\n");
	return;
    }

    state = 0;			/* We have not yet printed anything */
    while (len)
    {
	if ((state > 0) &&
	    (len >= DMPLEN) &&
	    (memcmp(adr, &adr[-DMPLEN], DMPLEN) == 0))
	{
	    if (state == 1)
	    {
		fprintf(errorFd, "\n  *");
		state = 2;	/* We are marking SAME */
	    }

	    adr += DMPLEN;
	    rel_adr += DMPLEN;
	    len -= DMPLEN;
	    continue;
	}

	state = 1;		/* There is a buffer to look back at */

	fprintf(errorFd, "\n  %4.4lx ", rel_adr);
	rel_adr += DMPLEN;

	for(x=0; x < DMPLEN && len; x++, len--)
		fprintf(errorFd, " %2.2x", (u_int)(text[x] = *adr++));
	for(y=x; y < DMPLEN; y++)
		fprintf(errorFd, "   ");
	fprintf(errorFd, " :");
	for(y=0; y < x; y++)
	{
	    if ( isprint(text[y]) )
		fprintf(errorFd, "%c", text[y]);
	    else
		fprintf(errorFd, ".");
	}
	fprintf(errorFd, ":");
    }

    if (state == 2)
	fprintf(errorFd, "\n  %4.4lx ", rel_adr);

    fprintf(errorFd, "\n");
    fflush(errorFd);
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Port I/O Routines			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * These routines assume that the byte sex of the port I/O will
 * be the same as the byte sex of the memory I/O.
 *
 *	union
 *	{
 *	    u_long	value;
 *	    u_char	byte[sizeof(u_long)];
 *	} eisa_id;
 */

/*
 * Devices for the mm driver
 */

#define MINOR_OF_MEM	0
#define MINOR_OF_KMEM	1
#define MINOR_OF_NULL	2
#define MINOR_OF_IOB	3
#define MINOR_OF_IOW	4
#define MINOR_OF_IOD	5
#define MINOR_OF_PANIC	6
#define MINOR_OF_MSR	7

static void	open_iob(void);
static void	open_iow(void);
static void	open_iod(void);
static int	major_of_mem(void);

/* #define DEV_IO_DEBUG	/* Normal users should not see this debug */

static int	IobFd = -1;
static int	IowFd = -1;
static int	IodFd = -1;
static int	MsrFd = -1;

u_char
io_inb(u_long addr)
{
    u_char	value;

    open_iob();

    lseek(IobFd, addr, SEEK_SET);
    read(IobFd, &value, sizeof(value));
    return value;
}

u_short
io_inw(u_long addr)
{
    u_short	value;

    open_iow();

    lseek(IowFd, addr, SEEK_SET);
    read(IowFd, (u_char *)&value, sizeof(value));
    return value;
}

u_long
io_ind(u_long addr)
{
    u_long	value;

    open_iod();

    lseek(IodFd, addr, SEEK_SET);
    read(IodFd, (u_char *)&value, sizeof(value));
    return value;
}

void
io_outb(u_long addr, u_char value)
{
    open_iob();

    lseek(IobFd, addr, SEEK_SET);
    write(IobFd, &value, sizeof(value));
}

void
io_outw(u_long addr, u_short value)
{
    open_iow();

    lseek(IowFd, addr, SEEK_SET);
    write(IowFd, (u_char *)&value, sizeof(value));
}

void
io_outd(u_long addr, u_long value)
{
    open_iod();

    lseek(IodFd, addr, SEEK_SET);
    write(IodFd, (u_char *)&value, sizeof(value));
}

static void
open_iob()
{
    if ((IobFd == -1) &&
	((IobFd = open_cdev(major_of_mem(), MINOR_OF_IOB, O_RDWR)) == -1))
	exit(errno);
}

static void
open_iow()
{
    if ((IowFd == -1) &&
	((IowFd = open_cdev(major_of_mem(), MINOR_OF_IOW, O_RDWR)) == -1))
	exit(errno);
}

static void
open_iod()
{
    if ((IodFd == -1) &&
	((IodFd = open_cdev(major_of_mem(), MINOR_OF_IOD, O_RDWR)) == -1))
	exit(errno);
}

static int
major_of_mem()
{
    struct stat	stat_buf;
    static int	maj = -1;


    if (maj == -1)
    {
	if (stat("/dev/mem", &stat_buf) && stat("/dev/kmem", &stat_buf))
	{
	    error_print("Unable to stat /dev/mem or /dev/kmem");
	    return -1;
	}

#ifdef DEV_IO_DEBUG
	debug_print("mem stat returned: dev(%04x) rdev(%04x) mode(%08x)",
							stat_buf.st_dev,
							stat_buf.st_rdev,
							stat_buf.st_mode);
#endif

	if (!S_ISCHR(stat_buf.st_mode))
	{
	    error_print("/dev/mem or /dev/kmem not char special");
	    errno = ENOTTY;
	    return -1;
	}

	maj = major(stat_buf.st_rdev);
    }

    return maj;
}

int
open_cdev(int maj_dev, int min_dev, mode_t mode)
{
    char	*dev_name;
    int		dev;
    int		fd;


    dev = makedev(maj_dev, min_dev);
#ifdef DEV_IO_DEBUG
    debug_print("temp file will be dev %d,%d", major(dev), minor(dev));
#endif

    if (!(dev_name = tmpnam(NULL)))
    {
	error_print("Unable to allocate a temporary file name");
	return -1;
    }

#ifdef DEV_IO_DEBUG
    debug_print("temp file name is %s", dev_name);
#endif

    unlink(dev_name);
    if (mknod(dev_name, S_IFCHR | S_IRUSR, dev))
    {
	error_print("Unable to create temporary file %s", dev_name);
	return -1;
    }

    if ((fd = open(dev_name, mode)) == -1)
    {
	/*
	 * In some cases we will be making devices that we do
	 * not KNOW are good.  It is expected that there will be
	 * some failures opening devices.
	 */

	debug_print("unable to open temporary cdev %d,%d: %s",
					maj_dev, min_dev, strerror(errno));

	unlink(dev_name);
	return -1;
    }

    unlink(dev_name);
    return fd;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *			Model Specific Register Routines		 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int	open_msr(void);

/*
 * The kernel support for this may not exsist in all versions of
 * the kernel.  Be prepared for failure.
 *
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 *
 * If return is true, errno is set.
 * If return is set and errno is zero, EOF was detected.
 */

int
read_msr(int cpu, u_long addr, msr_t *value)
{
    int		n;

    if (open_msr())
    {
	errno = ENODEV;
	return -1;
    }

    if (lock_cpu(cpu))
	return -1;

    lseek(MsrFd, addr, SEEK_SET);
    if ((n = read(MsrFd, (void *)value, sizeof(*value))) != sizeof(*value))
    {
	/*
	 * read() does not set errno if return is not -1.
	 * read_msr() must set errno for all error returns.
	 */

	if (n != -1)
	{
	    if (n == 0)
		errno = 0;	/* End of file detected */
	    else
		errno = EIO;
	}

#ifdef MSR_DEBUG
	debug_print("Msr read fail: cpu=%d, addr=%lu, ret=%d errno=%d",
							cpu, addr, n, errno);
#endif
	return -1;
    }

    unlock_cpu();
    return 0;
}

#ifdef THIS_IS_UNUSED

int
write_msr(int cpu, u_long addr, msr_t *value)
{
    int		n;

    if (open_msr())
    {
	errno = ENODEV;
	return -1;
    }

    if (lock_cpu(cpu))
	return -1;

    lseek(MsrFd, addr, SEEK_SET);
    if ((n = write(MsrFd, (void *)value, sizeof(*value))) != sizeof(*value))
    {
	/*
	 * write() does not set errno if return is not -1.
	 * write_msr() must set errno for all error returns.
	 */

	if (n != -1)
	{
	    if (n == 0)
		errno = 0;	/* End of file detected */
	    else
		errno = EIO;
	}

#ifdef MSR_DEBUG
	debug_print("Msr write fail: cpu=%d, addr=%lu, ret=%d errno=%d",
							cpu, addr, n, errno);
#endif
	return -1;
    }

    unlock_cpu();
    return 0;
}

#endif	/* THIS_IS_UNUSED */

/*
 * This one may actually open /dev/null on older kernels
 */

static int
open_msr()
{
    if (MsrFd == -2)
	return -1;

    if ((MsrFd == -1) &&
	((MsrFd = open_cdev(major_of_mem(), MINOR_OF_MSR, O_RDWR)) == -1))
    {
	debug_print("Cannot open msr");
	MsrFd = -2;
	return -1;
    }

    return 0;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				  CPU Features				 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * CPUID is permitted in ring 3.  It may not however be supported.
 */

#ifdef NOT_YET	/* ## */

static int	gotSigIll;

static void
handleSigIll(int sig)
{
    gotSigIll = 1;
}

typedef struct cpu_feat_s
{
    ##
} cpu_feat_t;

static const cpu_feat_t *
readCPUfeatures(int cpu)
{
    static cpu_feat_t	features;

    if (lock_cpu(cpu))
	return NULL;

    gotSigIll = 0;
    ## catch SIGILL
    if ## sigaction(SIGILL, #))

    ##

    ## uncatch SIGILL

    unlock_cpu();
    return &features;
}

#endif	/* ## */

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				CPU Lock Routines			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int	open_cpuFd(int cpu);
static int	ValidAtdev(int fd);
static int	major_of_cdev(const char *prefix);
static int	major_of_bdev(const char *prefix);

#define BAD_PID		((pid_t)-1)
#define NO_LOCKED_CPU	(-1)

static int	lockedCPU = NO_LOCKED_CPU;

/*
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 *
 * See also lockpid(ADM)
 * cidriver
 * Notice:
 *	The ACPU_LOCK/ACPU_UNLOCK ioctl()s expect the address of
 *	an int as myPID.  A pid_t is a short.  There will be
 *	trouble if myPID is a pid_t.
 *
 * These routines should not normally issue messages.  It may be
 * normal for locking to fail.  The cpu may be disabled.
 */

static int
lock_cpu(int cpu)
{
    int		Fd;
    int		myPID = 0;	/* Zero implies my pid */

    if (get_num_cpu() <= 1)
    {
	if (cpu == 0)
	    return 0;		/* uni */

	return -1;
    }

    if (lockedCPU == cpu)
	return 0;	/* Already locked */

    if (unlock_cpu())
	return -1;

    if ((Fd = open_cpuFd(cpu)) == -1)
	return -1;

    if (ioctl(Fd, ACPU_LOCK, &myPID) < 0)
	return -1;

    lockedCPU = cpu;
    return 0;
}

static int
unlock_cpu()
{
    int		Fd;
    int		myPID = 0;	/* Zero implies my pid */

    if (lockedCPU == NO_LOCKED_CPU)
	return 0;		/* This also covers uni */

    if ((Fd = open_cpuFd(lockedCPU)) == -1)
	return -1;

    if (ioctl(Fd, ACPU_UNLOCK, &myPID) < 0)
	return -1;

    lockedCPU = NO_LOCKED_CPU;
    return 0;
}

/*
 * cpu 0 base processor		/dev/at1
 * cpu 1 first aux processor	/dev/at2
 * cpu 2 next aux processor	/dev/at3
 *				...
 *				/dev/at9
 *				/dev/at10
 *				...
 *				/dev/at14
 */

static int
open_cpuFd(int cpu)
{
    static int	maxcpus = 0;
    static int	*cpuFd = NULL;
    int		n;

    /*
     * Maintain a list of open files.  Grow the list as required.
     */

    if (cpu >= maxcpus)
    {
	int	ncpu;

	if ((ncpu = get_num_cpu()) < 1)
	    return -1;

	if (ncpu <= cpu)
	    ncpu = cpu + 1;

	if (cpuFd)
	{
	    int		*tp;

	    tp = (int *)Fmalloc(sizeof(int) * ncpu);
	    for (n = 0; n < ncpu; ++n)
		if (n < maxcpus)
		    tp[n] = cpuFd[n];
		else
		    tp[n] = -1;

	    free(cpuFd);
	    cpuFd = tp;
	}
	else
	{
	    cpuFd = (int *)Fmalloc(sizeof(int) * ncpu);
	    for (n = 0; n < ncpu; ++n)
		cpuFd[n] = -1;
	}

	maxcpus = ncpu;
    }

    if (cpuFd[cpu] == -1)
    {
	static char	atdev[] = "/dev/at?????";
	static int	SpclMajor = 0;

	if (SpclMajor == -1)
	    return -1;	/* No need try again */

	if (!SpclMajor)
	{
	    struct stat	stat;

	    sprintf(&atdev[7], "%d", cpu+1);

	    if (((cpuFd[cpu] = open(atdev, O_RDWR)) != -1) &&
		!fstat(cpuFd[cpu], &stat))
	    {
		/*
		 * If it is a good major, we no longer need
		 * device names in /dev.
		 */

		if (S_ISCHR(stat.st_mode) && ValidAtdev(cpuFd[cpu]))
		    SpclMajor = major(stat.st_rdev);
		else
		{
		    /*
		     * Whatever this is; we do not want.
		     */

		    close(cpuFd[cpu]);
		    cpuFd[cpu] = -1;
		}
	    }

	    debug_print("open_cpuFd(cpu=%d) %s maj=%d Fd=%d",
					cpu+1, atdev, SpclMajor, cpuFd[cpu]);
	}

	/*
	 * If the device does not open, we may not have the
	 * correct major for /dev/at*.  This happens if the
	 * environment has not been rebuilt.  These drivers
	 * are in cidriver,  The minor is the same as our
	 * cpu number (zero based).
	 */

	if (cpuFd[cpu] == -1)
	{
	    if (!SpclMajor && ((SpclMajor = major_of_cdev("ci__")) == -1))
		return -1;

	    cpuFd[cpu] = open_cdev(SpclMajor, cpu, O_RDWR);

	    debug_print("open_cpuFd(cpu=%d) maj=%d Fd=%d",
						cpu+1, SpclMajor, cpuFd[cpu]);
	}
    }

    return cpuFd[cpu];
}

static int
ValidAtdev(int fd)
{
    u_long	num_ACPUs;

    if (ioctl(fd, ACPU_GETNUM, &num_ACPUs) < 0)
    {
	debug_print("ValidAtdev() failed ACPU_GETNUM");
	return 0;	/* A good driver would have liked this request */
    }

    if ((num_ACPUs + 1) != (u_long)get_num_cpu())
    {
	debug_print("ValidAtdev() failed: num_ACPUs=%u, num_cpu=%d",
						num_ACPUs, get_num_cpu());
	return 0;
    }

    return 1;	/* Looks OK */
}

/*
 * Given the driver prefix, we should be able to find the major
 */

static int
major_of_cdev(const char *prefix)
{
    static u_long		cdevsw_addr = 0;
    static u_long		cdevcnt = 0;
    static u_long		p_open;
    static u_long		p_close;
    static u_long		p_read;
    static u_long		p_write;
    static u_long		p_ioctl;
    static struct cdevsw	a_cdevsw;
    static struct
    {
	const char	*name;
	u_long		*addr;
	u_long		*value;
    } my_addrs[] =
    {
	{ "open",	&p_open,	(u_long *)&a_cdevsw.d_open },
	{ "close",	&p_close,	(u_long *)&a_cdevsw.d_close },
	{ "read",	&p_read,	(u_long *)&a_cdevsw.d_read },
	{ "write",	&p_write,	(u_long *)&a_cdevsw.d_write },
	{ "ioctl",	&p_ioctl,	(u_long *)&a_cdevsw.d_ioctl }
    };
    int		n;
    int		maj;

    /*
     * Locate the device switch
     */

    if (!cdevsw_addr)
    {
	if (get_kaddr("cdevsw", &cdevsw_addr))
	    return -1;

	debug_print("cdevsw is at 0x%.8x", cdevsw_addr);;
    }

    if (!cdevcnt)
    {
	if (read_kvar_d("cdevcnt", &cdevcnt))
	    return -1;

	debug_print("cdevcnt = %d", cdevcnt);;
    }

    /*
     * Find the address of this drivers routines
     */

    for (n = 0; n < sizeof(my_addrs)/sizeof(*my_addrs); ++n)
    {
	char	buf[128];

	sprintf(buf, "%s%s", prefix, my_addrs[n].name);

	if (get_kaddr(buf, my_addrs[n].addr))
	    *my_addrs[n].addr = 0;
    }

    /*
     * Run the switch for looking for my routines
     */

    for (maj = 0; maj < cdevcnt; ++maj)
    {
	static const int	cdevcnt_len = sizeof(a_cdevsw);
	u_long			addr = cdevsw_addr + (maj * cdevcnt_len);

	if (read_kmem(addr, &a_cdevsw, cdevcnt_len) == cdevcnt_len)
	    for (n = 0; n < sizeof(my_addrs)/sizeof(*my_addrs); ++n)
		if (*my_addrs[n].addr &&
		    (*my_addrs[n].addr == *my_addrs[n].value))
			return maj;
    }

    return -1;	/* Not found */
}

static int
major_of_bdev(const char *prefix)
{
    static u_long		bdevsw_addr = 0;
    static u_long		bdevcnt = 0;
    static u_long		p_open;
    static u_long		p_close;
    static u_long		p_strategy;
    static u_long		p_print;
    static struct bdevsw	a_bdevsw;
    static struct
    {
	const char	*name;
	u_long		*addr;
	u_long		*value;
    } my_addrs[] =
    {
	{ "open",	&p_open,	(u_long *)&a_bdevsw.d_open },
	{ "close",	&p_close,	(u_long *)&a_bdevsw.d_close },
	{ "strategy",	&p_strategy,	(u_long *)&a_bdevsw.d_strategy },
	{ "print",	&p_print,	(u_long *)&a_bdevsw.d_print }
    };
    int		n;
    int		maj;

    /*
     * Locate the device switch
     */

    if (!bdevsw_addr)
    {
	if (get_kaddr("bdevsw", &bdevsw_addr))
	    return -1;

	debug_print("bdevsw is at 0x%.8x", bdevsw_addr);;
    }

    if (!bdevcnt)
    {
	if (read_kvar_d("bdevcnt", &bdevcnt))
	    return -1;

	debug_print("bdevcnt = %d", bdevcnt);;
    }

    /*
     * Find the address of this drivers routines
     */

    for (n = 0; n < sizeof(my_addrs)/sizeof(*my_addrs); ++n)
    {
	char	buf[128];

	sprintf(buf, "%s%s", prefix, my_addrs[n].name);

	if (get_kaddr(buf, my_addrs[n].addr))
	    *my_addrs[n].addr = 0;
    }

    /*
     * Run the switch for looking for my routines
     */

    for (maj = 0; maj < bdevcnt; ++maj)
    {
	static const int	bdevcnt_len = sizeof(a_bdevsw);
	u_long			addr = bdevsw_addr + (maj * bdevcnt_len);

	if (read_kmem(addr, &a_bdevsw, bdevcnt_len) == bdevcnt_len)
	    for (n = 0; n < sizeof(my_addrs)/sizeof(*my_addrs); ++n)
		if (*my_addrs[n].addr &&
		    (*my_addrs[n].addr == *my_addrs[n].value))
			return maj;
    }

    return -1;	/* Not found */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *			Kernel memory I/O Routines			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int	MemFd = -1;
static int	KmemFd = -1;

static LDFILE	*ldFd = NULL;
static int	ksym_cnt = 0;
static SYMENT	*kernel_symbols = NULL;

static int	SetKernelName(void);
static int	IsBootedKernel(const char *kernel);
static int	open_mem(void);
static int	open_kmem(void);
static SYMENT	*match_ksym(void *kaddr);
static void	read_ksymtab(void);
static int	find_last_text(void);

int
read_mem(u_long paddr, void *buf, size_t len)
{
    if (open_mem())
	return -1;

    lseek(MemFd, paddr, SEEK_SET);
    return read(MemFd, (char *)buf, len);
}

int
read_kmem(u_long kaddr, void *buf, size_t len)
{
    if (open_kmem())
	return -1;

    lseek(KmemFd, kaddr, SEEK_SET);
    return read(KmemFd, (char *)buf, len);
}

/*
 * Return the kernel virtual address for the symbol name given.
 *
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to get its address.
 */

int
get_kaddr(const char *name, u_long *kaddr)
{
    struct nlist	k_name[2];

    if (SetKernelName())
	return -1;

    memset(k_name, 0, sizeof(k_name));
    k_name[0].n_name = (char *)name;

    nlist(KernelName, k_name);
    if (k_name[0].n_value == 0)
    {
	debug_print("get_kaddr(%s) fail", name);
	return -1;
    }

    *kaddr = k_name[0].n_value;
    debug_print("get_kaddr(%s) 0x%.8x", name, *kaddr);
    return 0;
}

const char *
GetKernelName()
{
    SetKernelName();

    return KernelName;
}

static int
SetKernelName()
{
    static const char	dflt_kernel[] = "/unix";
    static const char	bootDir[] = "/stand";
    static int		beenHere = 0;
    DIR			*dirp;

    if (KernelName != NULL)
	return 0;	/* Already set */

    if (IsBootedKernel(dflt_kernel))
    {
	KernelName = dflt_kernel;
	return 0;
    }

    if (beenHere)
	return -1;

    beenHere = 1;

    /*
     * If it is not the default, look at everything in /stand. 
     * It will be there if it is anywhere.
     */

    if ((dirp = opendir(bootDir)) != NULL)
    {
	struct dirent	*dp;

	while ((dp = readdir(dirp)) != NULL)
	{
	    char	name[sizeof(bootDir)+MAXNAMLEN+2];

	    if (dp->d_name[0] == '.')
	    {
		if (dp->d_name[1] == '\0')
		    continue;

		if ((dp->d_name[1] == '.') &&
		    (dp->d_name[2] == '\0'))
		    continue;
	    }

	    strcpy(name, bootDir);
	    strcat(name, "/");
	    strcat(name, dp->d_name);
	    if (IsBootedKernel(name))
	    {
		KernelName = strdup(name);
		closedir(dirp);
		return 0;
	    }
	}

	closedir(dirp);
    }

    error_print("Cannot locate booted kernel");
    return errno = ENODEV;
}

static int
IsBootedKernel(const char *kernel)
{
    static struct nlist nl[] =
    {
	{ "scoutsname" },
	{ "bootmisc" },
	{ "" }
    };
#define NL_SCOUTSNAME	0
#define NL_BOOTMISC	1

    struct scoutsname		memsco;
    struct bootmisc		miscbuf;
    struct stat			stbuf;
    static struct scoutsname	syssco;
    static int			initDone = 0;


    debug_print("Checking for booted kernel in %s", kernel);

    if (!initDone)
    {
	if (__scoinfo(&syssco, sizeof(syssco)))
	    return 0;

	initDone = 1;
    }

    if (stat(kernel, &stbuf))
	return 0;		/* Cannot stat */

    if (!S_ISREG(stbuf.st_mode))
	return 0;		/* Not a regular file */

    /*
     * We cannot do get_kaddr() here since it calls here for the
     * kernel name.
     */

    if (nlist(kernel, nl))
	return 0;		/* Cannot get namelist */

    if (nl[NL_SCOUTSNAME].n_value == 0)
	return 0;		/* Not a kernel */

    /*
     * Equivalent to traditional ps(C); checks that
     * __scoinfo(S)'s returned data matches what's read from
     * /dev/kmem
     */

    if (read_kmem(nl[NL_SCOUTSNAME].n_value, &memsco, sizeof(memsco)) !=
								sizeof(memsco))
	return 0;

    if (memcmp(&syssco, &memsco, sizeof(memsco)))
	return 0;

    /*
     * In 3.2v5 and later systems, boot(HW) passes to the kernel
     * the i-node number of the booted kernel (for C2 Auditing
     * purposes).  We can use that as another (presumably rather
     * strong) check.
     */

    if (nl[NL_BOOTMISC].n_value == 0)
	return 0;		/* apparently prior to 3.2v4.0 */

    if (read_kmem(nl[NL_BOOTMISC].n_value, &miscbuf, sizeof(miscbuf)) !=
								sizeof(miscbuf))
	return 0;

    if (!valid_field(miscbuf, kinode) || (miscbuf.kinode != stbuf.st_ino))
	return 0;

    /*
     * If it passed all of these tests, it probably is the
     * booted kernel.
     */

    debug_print("Booted kernel found: %s", kernel);
    return 1;
}

/*
 * Return the symbol name for the kernel virtual address given.
 *
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has a symbol for this address
 * by attempting to get its symbol.
 */

const char *
get_ksym(void *kaddr)
{
    const char		*np;
    SYMENT		*sp;

    if (!(sp = match_ksym(kaddr)))
    {
	debug_print("symbol not found for address: 0x%.8x", kaddr);
	return NULL;
    }

    np = ldgetname(ldFd, sp);
    debug_print("np: %s", np ? np : "(null)");

    return np;
}

static SYMENT *
match_ksym(void *kaddr)
{
    SYMENT	*sp;
    SYMENT	*match_sp = NULL;
    u_long	match_value = 0;

    read_ksymtab();
    if (!kernel_symbols || !ksym_cnt)
	return NULL;

    for (sp = kernel_symbols; sp < &kernel_symbols[ksym_cnt]; sp++)
	if ((sp->n_value <= (u_long)kaddr) && (sp->n_value > match_value))
	{
	    match_value = sp->n_value;
	    match_sp = sp;
	}

    return(match_sp);
}

static void
read_ksymtab()
{
    SYMENT	sp;
    int		n_text;
    u_long	j;
    size_t	sym_size;

    if (SetKernelName())
	return;

    if (kernel_symbols && ksym_cnt)
	return;

    if (!ldFd && (!(ldFd = ldopen(KernelName, NULL))))
    {
	error_print("unable to open kernel symbols: %s", KernelName);
	return;
    }

    debug_print("ldFd: 0x%x", ldFd);

#ifdef i286
    if ((TYPE(ldFd) != I286SMAGIC) && (TYPE(ldFd) != I286LMAGIC))
    {
	error_print("not a valid iAPX286 file");
	return;
    }
#endif /* i286 */
#ifdef i386
    if (TYPE(ldFd) != I386MAGIC)
    {
	error_print("not a valid iAPX386 file");
	return;
    }
#endif /* i386 */

    if (!(n_text = find_last_text()))
	return;

    debug_print("n_text: 0x%x", n_text);

    if (ldtbseek(ldFd) == FAILURE)
    {
	error_print("symbol table seek failure");
	return;
    }

    sym_size = HEADER(ldFd).f_nsyms * SYMESZ;
    debug_print("sym_size: %d", sym_size);
    kernel_symbols = (SYMENT *)Fmalloc(sym_size);

    for (j = 0; j < HEADER(ldFd).f_nsyms; j++)
    {
	if (FREAD((char *)&sp, SYMESZ, 1, ldFd) == FAILURE)
	{
	    error_print("symbol table read failure");
	    free(kernel_symbols);
	    kernel_symbols = NULL;
	    return;
	}

	if ((sp.n_sclass == C_EXT) && (sp.n_scnum <= n_text))
	{
	    if (ksym_cnt == sym_size)
	    {
		error_print("too many text symbols");
		free(kernel_symbols);
		kernel_symbols = NULL;
		return;
	    }

	    kernel_symbols[ksym_cnt++] = sp;
	}
    }
}

static int
find_last_text()
{
    SCNHDR	sh;
    u_short	i;
    int		maxsect = 1;

    for (i = 1; i <= HEADER(ldFd).f_nscns; i++)
    {
	if (ldshread(ldFd,i, &sh) == FAILURE)
	{
	    error_print("symbol table read failure");
	    return 0;
	}

	if (strcmp(sh.s_name, ".text") == 0)
	    maxsect = i;
    }

    return(maxsect);
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 */

int
read_kvar(const char *name, void *value, size_t len)
{
    u_long	kaddr;

    if (get_kaddr(name, &kaddr) ||
	(read_kmem(kaddr, value, len) != len))
	    return -1;

    return 0;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 */

int
read_kvar_b(const char *name, u_char *value)
{
    if (read_kvar(name, value, sizeof(*value)))
	return -1;

    debug_print("read_kvar_b(%s) 0x%.2x", name, *value);
    return 0;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 */

int
read_kvar_w(const char *name, u_short *value)
{
    if (read_kvar(name, value, sizeof(*value)))
	return -1;

    debug_print("read_kvar_w(%s) 0x%.4x", name, *value);
    return 0;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 */

int
read_kvar_d(const char *name, u_long *value)
{
    if (read_kvar(name, value, sizeof(*value)))
	return -1;

    debug_print("read_kvar_d(%s) 0x%.8x", name, *value);
    return 0;
}

const char *
read_k_string(u_long kaddr)
{
    static char		*buf = NULL;
    static int		buflen = 0;
    char		*bp;
    static const int	first_len = 2048;
    static const int	grow_len = 2048;

    /*
     * Get a buffer that is probably big enough.
     */

    if ((!buflen || !buf) && !(buf = malloc(buflen = first_len)))
    {
	buflen = 0;
	return NULL;
    }

    for (bp = buf; ; )
    {
	if ((read_kmem(kaddr++, bp, 1) != 1) || !*bp)
	    break;

	/*
	 * Grow the buffer as required
	 */

	if ((++bp - buf) >= buflen)
	{
	    char	*tp;

#define GROW_LIMIT	(30 * 1024)
#ifdef GROW_LIMIT
	    if (buflen >= GROW_LIMIT)
	    {
		error_print("read_k_string: string too long");
		return NULL;
	    }
#endif

	    tp = Fmalloc(buflen + grow_len);
	    memcpy(tp, buf, buflen);
	    free(buf);
	    buf = tp;
	    bp = &tp[buflen];
	    buflen += grow_len;

	    debug_print("read_k_string: Buffer grows to %d bytes", buflen);
	}
    }

    *bp = '\0';
    return buf;
}

static int
open_mem()
{
    if (MemFd != -1)
	return 0;

    if ((MemFd = open("/dev/mem", O_RDONLY)) == -1)
    {
	error_print("unable to open memory");
	return errno;
    }

    return 0;
}

static int
open_kmem()
{
    if (KmemFd != -1)
	return 0;

    if ((KmemFd = open("/dev/kmem", O_RDONLY)) == -1)
    {
	error_print("unable to open kernel memory");
	return errno;
    }

    return 0;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				CPU specific data			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static int	num_cpu = 0;

/*
 * On a uni kernel:
 *	mpx_kernel == 0
 *	mpswp does not exist
 *
 * On an mp kernel:
 *	mpx_kernel != 0
 *	mpswp does exist
 *	    mpswp will == &mpswnull if no GPI is installed
 */

int
is_mp_kernel()
{
    static u_long	status = (u_long)-1;

    if (status == (u_long)-1)
    {
	u_long	mpswp;

	status = get_kaddr("mpswp", &mpswp) ? 0 : 1;
    }

    return status;
}

/*
 * The value in scoutsname is the number of licensed CPUs; that
 * is, the number that are functioning.  There may be others
 * that exist.  num_ACPUs is the number of aux processors.
 */

int
get_num_cpu()
{
    u_long	num_ACPUs;

    if (num_cpu == 0)
	num_cpu = read_kvar_d("num_ACPUs", &num_ACPUs)
					? -1 : ((int)num_ACPUs + 1);

    return num_cpu;
}

/*
 * Find out if this is in the CPU specific area.
 *
 * The CPU specific area is defined in vuifile to be at fixed
 * offsets from KV_crllry. Various code makes the explicit
 * assumption that the first item within the KV_crllry area is
 * "processor" and the last item is "crllry_info".
 *
 * crllry_info is a structure of size known only to
 * kernel private header files.  The memory is allocated
 * in even pages.  So just round up to a page boundry.
 *
 * The system maintains two tables of virtual memory segments per CPU:
 *
 *  o The Global Descriptor Table (GDT) defines the objects used
 *    by the kernel, including its data and text segments
 *
 *  o The Interrupt Descriptor Table (IDT) defines where trap
 *    and interrupt handler routines may be found in the
 *    kernel's text segment
 *
 * > gdt
 * iAPX386 GDT
 * CPU SLOT     SELECTOR OFFSET   TYPE       DPL  ACCESSBITS
 *   0    1     f01d0114 000002d0 DSEG         0  ACCS'D R&W
 *   0    2     f01cf114 00000800 DSEG         0  R&W
 *   0    3     00000500 0000005f LDT          0
 *   0    4     00000640 00000067 TSS386       0
 *   0   40     e0001600 00000807 LDT          0
 *   0   41     e0001514 000000e7 TSS386       0
 *   0   42     f01d0450 00000068 TSS386       0
 *   0   43     00000000 fffff000 XSEG         0  ACCS'D R&X DFLT G4096
 *   0   44     00000000 fffff000 DSEG         0  ACCS'D R&W BIG  G4096
 *   0   45     f01d04b8 00000068 TSS386       0
 *   0   46     dffeb514 000000e7 TSS386       0
 *   0   49     00110000 000001e0 TSS386       0
 *   0   50     00000000 000fffff XSEG         0  R&X DFLT
 *   0   55     00000000 0000ffff XSEG         0  R&X DFLT
 *   0   56     00000000 0000ffff XSEG         0  R&X
 *   0   57     00000000 0000ffff DSEG         0  R&W BIG
 *   1    1     f0804800 000002d0 DSEG         0  ACCS'D R&W
 *   1    2     f0804000 00000800 DSEG         0  R&W
 *   1    3     00000500 0000005f LDT          0
 *   1    4     00000640 00000067 TSS386       0
 *   1   40     e0001600 00000807 LDT          0
 *   1   41     e0001514 000000e7 TSS386       0
 *   1   42     f02d50e8 00000068 TSS386       0
 *   1   43     00000000 fffff000 XSEG         0  ACCS'D R&X DFLT G4096
 *   1   44     00000000 fffff000 DSEG         0  ACCS'D R&W BIG  G4096
 *   1   45     f02d5154 00000068 TSS386       0
 *   1   46     dffeb514 000000e7 TSS386       0
 *   1   49     00110000 00000000 TSS386       0
 *   1   50     00000000 000fffff XSEG         0  R&X DFLT
 *   1   55     00000000 0000ffff XSEG         0  R&X DFLT
 *   1   56     00000000 0000ffff XSEG         0  R&X
 *   1   57     00000000 0000ffff DSEG         0  R&W BIG
 *
 * See also:
 *	<sys/seg.h>
 *	scodb vuifile
 */

static int
is_cpu_specific(u_long kaddr)
{
    static u_long	begin_KV_crllry = 0;
    static u_long	end_KV_crllry = 0;

    if (!begin_KV_crllry && get_kaddr("processor", &begin_KV_crllry))
	return 0;	/* Bounds not known therefore not CPU specific */

    if (!end_KV_crllry)
    {
	if (get_kaddr("crllry_info", &end_KV_crllry))
	    return 0;	/* Bounds not known therefore not CPU specific */

	end_KV_crllry = ((end_KV_crllry + PAGESIZE) & ~(PAGESIZE - 1)) - 1;
	debug_print("KV_crllry: 0x%.8.x-0x%.8lx  %ld",
				begin_KV_crllry, end_KV_crllry,
				end_KV_crllry - begin_KV_crllry + 1);
    }

    return((kaddr >= begin_KV_crllry) && (kaddr <= end_KV_crllry));
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 *
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 */

int
read_cpu_kvar(int cpu, const char *name, void *value, size_t len)
{
    u_long	kaddr;
    int		lock_done;
    int		status;

    if (get_num_cpu() <= 1)
	return read_kvar(name, value, len);

    if (get_kaddr(name, &kaddr))
	return -1;

    if (is_cpu_specific(kaddr))
    {
	if (lock_cpu(cpu))
	    return -1;

	lock_done = 1;
    }
    else
	lock_done = 0;

    status = (read_kmem(kaddr, value, len) == len) ? 0 : -1;

    if (lock_done)
	unlock_cpu();

    return status;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 *
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 */

int
read_cpu_kvar_b(int cpu, const char *name, u_char *value)
{
    if (read_cpu_kvar(cpu, name, value, sizeof(*value)))
	return -1;

    debug_print("read_cpu_kvar_b(%s) 0x%.2x", name, *value);
    return 0;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 *
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 */

int
read_cpu_kvar_w(int cpu, const char *name, u_short *value)
{
    if (read_cpu_kvar(cpu, name, value, sizeof(*value)))
	return -1;

    debug_print("read_cpu_kvar_w(%s) 0x%.4x", name, *value);
    return 0;
}

/*
 * This routine should normally be silent on failure.  It may be
 * normal to inquire if the kernel has such a variable by
 * attempting to read its value.
 *
 * cpu 0 base processor
 * cpu 1 first aux processor
 * cpu 2 next aux processor
 */

int
read_cpu_kvar_d(int cpu, const char *name, u_long *value)
{
    if (read_cpu_kvar(cpu, name, value, sizeof(*value)))
	return -1;

    debug_print("read_cpu_kvar_d(%s) 0x%.8x", name, *value);
    return 0;
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Misc utilities				 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

const char *
get_lib_file_name(const char *file)
{
    static char	*filename = NULL;
    static int	buflen = 0;
    const char	*ep;
    int		len;


    if (!file || !*file)
	return NULL;

    len = strlen(lib_dir ? lib_dir : dflt_lib_dir)+strlen(file)+5;
    if (!filename || (len > buflen))
    {
	if (filename)
	    free(filename);

	if (!(filename = malloc(buflen = len + 128)))
	{
	    buflen = 0;
	    errno = ENOMEM;
	    return NULL;
	}
    }

    /*
     * If the user has specified the directory, by command line
     * or environment, just take it as so.  In fact it might be
     * desired that the files not be found.
     */

    if (lib_dir)
    {
	sprintf(filename, "%s/%s", lib_dir, file);
	return filename;
    }

    if (((ep = getenv(lib_dir_env)) != NULL) && *ep)
    {
	sprintf(filename, "%s/%s", ep, file);
	return filename;
    }

    /*
     * If nothing was found, look arround a little.
     *    ./<file>
     *    ./<dflt_lib_dir>/<file>
     *    <dflt_lib_dir>/<file>
     */

    sprintf(filename, "%s", file);
    if (!eaccess(filename, R_OK))
	return filename;

    sprintf(filename, "%s/%s", &dflt_lib_dir[1], file);
    if (!eaccess(filename, R_OK))
	return filename;

    sprintf(filename, "%s/%s", dflt_lib_dir, file);
    return filename;
}

/*
 * This routine must not return an invalid string.
 */

void
report_when(FILE *out, const char *what)
{
    char	buf[256];
    char	*tp;
    time_t	clock = time(NULL);

    strcpy(buf, " for ");
    if (gethostname(&buf[5], sizeof(buf)-6))
	buf[0] = '\0';

    if ((tp = ctime(&clock)) != NULL)
    {
	strcat(buf, " on ");
	tp[strlen(tp)-1] = '\0';	/* Trim the \n */
	strcat(buf, tp);
    }

    fprintf(out, "\nReport about %s%s\n\n", what, buf);
}

/*
 * This is a malloc that does not require error checking.  If
 * malloc() fails, it never returns.
 */

void *
Fmalloc(size_t len)
{
    void	*tp;

    if (!(tp = malloc(len)))
    {
	fprintf(stderr, "Cannot allocate memory\n");
	exit(ENOMEM);
    }

    return tp;
}

const char *
spaces(size_t len)
{
    static char	*buf = NULL;
    static int	buflen = 0;


    if (len <= 0)
	return "";

    if ((buflen <= len) || !buf)
    {
	if (buf)
	    free(buf);

	buflen = len+80;
	if (buflen < 160)
	    buflen = 160;
	buf = Fmalloc(buflen+1);
	memset(buf, ' ', buflen);
	buf[buflen] = '\0';
    }

    return &buf[buflen-len];
}

