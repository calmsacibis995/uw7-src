/*
 * File hw_ram.c
 * Information handler for ram
 *
 * @(#) hw_ram.c 61.1 97/02/24 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/bootinfo.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>

#include "hw_ram.h"
#include "hw_util.h"

const char	* const callme_ram[] =
{
    "ram",
    "mem",
    "memory",
    NULL
};

const char	short_help_ram[] = "System memory statistics";

static const double	oneK = 1024;
static const double	oneM = 1024 * 1024;

static void showKernelName(FILE *out);
static void showBootFlags(FILE *out, u_long bootflags);
static void showBootInfo(FILE *out, struct bootmem *bm, long cnt, int nswapb,
								    int used);
static const char *mem_size_str(u_long len);

int
have_ram(void)
{
    return 1;	/* I HOPE we always have RAM */
}

/*
 * This routine computes the memory size by looking for the
 * bootinfo structure and adding up the sizes of all of the
 * memory segments.
 */

void
report_ram(FILE *out)
{
    u_char		buffer[BOOTINFO_LOC+512];
    struct bootinfo	*bp;
    int			swplob = 0;
    int			nswapb = 0;
    int			i;

    report_when(out, "memory");

    if ((i = read_mem(swplob, buffer, sizeof(buffer))) == -1)
    {
	error_print("Kernel memory read failed");
	return;
    }

    if ((i != sizeof(buffer)) || (nswapb && (nswapb < sizeof(buffer))))
    {
	error_print("memory read was too short");
	return;
    }

    /* debug_dump("Kernel info", buffer, sizeof(buffer)); */

    bp = (struct bootinfo *)&buffer[BOOTINFO_LOC];
    if (bp->magic != (u_long)BOOTINFOMAGIC)
    {
	error_print("memory read invalid checksum");
	return;
    }

    /* debug_dump("bootinfo", (u_char *)bp, sizeof(*bp)); */

    showBootFlags(out, bp->bootflags);
    fprintf(out, "    BaseMem:\t %s\n", mem_size_str(bp->basemem));

    if (bp->extmem != 0xfff00000UL)	/* fail value from BIOS */
	fprintf(out, "    ExtMem:\t %s\n", mem_size_str(bp->extmem));

    fprintf(out, "    BootStr:\t%s\n", bp->bootstr);
    showKernelName(out);

    fprintf(out, "\n\t\t\tAvailable Memory\n\n");
    showBootInfo(out, bp->memavail, bp->memavailcnt, nswapb, 0);

    fprintf(out, "\n\n\t\t\tMemory used by Kernel\n\n");
    showBootInfo(out, bp->memused, bp->memusedcnt, nswapb, 1);
}

static void
showKernelName(FILE *out)
{
    const char	*kernel;
    struct stat	stbuf;
    char	*name;


    if (!(kernel = GetKernelName()))
	return;

    fprintf(out, "    Kernel:\t%s", kernel);

    if (!lstat(kernel, &stbuf) &&
	S_ISLNK(stbuf.st_mode) &&
	((name = malloc(stbuf.st_size + 1)) != NULL))
    {
	int	n;

	if ((n = readlink(kernel, name, stbuf.st_size + 1)) != -1)
	{
	    name[n] = '\0';
	    fprintf(out, " -> %s", name);
	}

	free(name);
    }

    fprintf(out, "\n");
}

static void
showBootFlags(FILE *out, u_long bootflags)
{
    typedef struct
    {
	u_long		mask;
	const char	*use;
    } bootFlags_t;

    static const bootFlags_t	flags[] =
    {
	{ B_AT,		"AT"		},
	{ B_MC,		"MicroChannel"	},
	{ B_EISA,	"EISA"		},
	{ B_I486,	"Intel 80486"	},
	{ B_PCI,	"PCI"		},
	{ B_APM,	"APM"		},
	{ B_BOOTP,	"Bootp"		},

	{ 0,		NULL		}	/* End of table */
    };
    const bootFlags_t	*fp;
    int			first;


    fprintf(out, "    BootFlags:\t");

    if (verbose)
    {
	fprintf(out, "0x%.8lx", bootflags);
	first = 0;
    }
    else
	first = 1;

    for (fp = flags; fp->use; ++fp)
	if (bootflags & fp->mask)
	{
	    if (first)
		first = 0;
	    else
		fprintf(out, "\n\t\t");

	    fprintf(out, "%s", fp->use);
	}

    fprintf(out, "\n\n");
}

static void
showBootInfo(FILE *out, struct bootmem *bm, long cnt, int nswapb, int used)
{
    paddr_t		prev;
    u_long		RAMsize;
    u_long		AddrSize;
    double		mRAMsize;
    u_long		iRAMsize;
    long		i;

    typedef struct
    {
	u_long		mask;
	const char	*msg;
    } bm_flags_t;

    /*
     * These are sorted so that the most usefull message is
     * printed first whenever more than one flag is set.
     */
    static const bm_flags_t	flags[] =
    {
	{ B_MEM_RSRVD,		"Kernel reserved" },
	{ B_MEM_KTEXT,		"Kernel text" },
	{ B_MEM_KDATA,		"Kernel data" },
	{ B_MEM_KBSS,		"Kernel bss" },
	{ B_MEM_BIOS,		"BIOS private area - Reserved" },
	{ B_MEM_EISA,		"Where EISA BIOS data is stored" },
	{ B_MEM_PRELOAD,	"Pre-loaded by /boot" },
	{ B_MEM_SPECIAL,	"Shadow ram & other special mem" },
	{ B_MEM_DOWN,		"Size this extent from the top down" },
	{ B_MEM_CANTDMA,	"Non-ISA-DMAable" },
    };


    prev = -1;
    RAMsize = AddrSize = 0;
    for (i = 0; i < cnt; ++i, ++bm)
    {
	int	j;
	int	first;

	if ((prev + 1) < bm->base)
	{
	    u_long	ROMsize = bm->base - prev - 1;
	    fprintf(out, "    0x%.8x-0x%.8x   %s\t%s\n",
					prev + 1,
					bm->base - 1,
					mem_size_str(ROMsize),
					used ? "Unused" : "Non-RAM");
	    AddrSize += ROMsize;
	}

	fprintf(out, "    0x%.8x-0x%.8x   %s",
					bm->base,
					prev = bm->base + bm->extent - 1,
					mem_size_str(bm->extent));
	if (verbose)
	{
	    fprintf(out, "\tFlags 0x%l.8x", bm->flags);
	    first = 0;
	}
	else
	    first = 1;

	for (j = 0; j < sizeof(flags)/sizeof(*flags); ++j)
	    if (bm->flags & flags[j].mask)
	    {
		if (first)
		    first = 0;
		else
		    fprintf(out, "\n\t\t\t\t");
		fprintf(out, "\t%s", flags[j].msg);
	    }

	fprintf(out, "\n");
	RAMsize += bm->extent;
	AddrSize += bm->extent;
    }

    if (nswapb && (RAMsize > nswapb))
	RAMsize = nswapb;

    fprintf(out, "\t\t\t    ---------\n");
    fprintf(out, "\t\t\t    %s\tRAM total", mem_size_str(RAMsize));
    mRAMsize = RAMsize / oneM;
    iRAMsize = (u_long)mRAMsize;
    if ((double)iRAMsize < mRAMsize)
	fprintf(out, "  (%ld Mb - %g Kb)",
				iRAMsize + 1,
				oneK - ((mRAMsize - iRAMsize) * oneK));
    fprintf(out, "\n");
    fprintf(out, "\t\t\t    %s\tMemory total\n", mem_size_str(AddrSize));
}

static const char *
mem_size_str(u_long len)
{
    static char		buf[64];
    const char		*unit;
    double		mult;
    u_long		sz;

    if (len < oneK)
    {
	mult = 1;
	unit = "";
    }
    else if (len < oneM)
    {
	mult = oneK;
	unit = "K";
    }
    else
    {
	mult = oneM;
	unit = "M";
    }

    sz = (u_long)((len / mult * 100.0) + .5);	/* Round to two digits */
    sprintf(buf, "%6g %sb", (double)sz / 100.0, unit);
    return buf;
}

