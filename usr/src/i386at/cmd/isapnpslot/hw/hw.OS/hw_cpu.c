/*
 * File hw_cpu.c
 * Information handler for cpu
 *
 * @(#) hw_cpu.c 64.2 97/04/21 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ci/cidefs.h>	/* For CI_OWNER() */
#include <ctype.h>
#include <sys/utsname.h>
#include <sys/msr.h>

int __scoinfo(struct scoutsname *buf, int bufsize);

#include <sys/cpuid.h>

#include "hw_cpu.h"
#include "hw_util.h"

const char	* const callme_cpu[] =
{
    "cpu",
    "cpu_id",
    "cpu_type",
    NULL
};

const char	short_help_cpu[] = "CPU model and speed";

#ifdef FROM_INTEL	/* ## */

/*
 * Number of cycles needed to execute a single BSF 
 *   instruction. Note that processors below i386(tm) 
 *   are not supported.
 */

static ulong processor_cycles[] =
{
    00,
    00,
    00,
    115,	/* 386 */
    48,		/* 486 */
    43,		/* Pentium */
    38,		/* Pentium Pro */
    38,		/* 386 */
    38,		/* 386 */
    38,		/* 386 */
    38,
    38, 
    38,
    38,
    38,
    38,
    38,
    38,
    38,
    38,
    38
};

/*
 * Normalize raw clock frequency to one of these values.
 */

static ushort i386Speeds[] = { 16, 20, 25, 33, 40, EOA };
static ushort i486Speeds[] = { 25, 33, 50, 66, 75, 100, EOA };
static ushort iP5Speeds[] =
{
    60, 66, 75, 90, 100, 120, 133, 150, EOA
};
static ushort iP6Speeds[] =
{
    60, 66, 75, 90, 100, 120, 133, 150, 167, 200, EOA
};


#endif	/* FROM_INTEL */

/*
 * This program makes a guess at the clock speed of the CPU based
 * on the performance of a small loop performed during the kernel
 * boot process.
 *
 * Collateral test data
 *
 * cpu_id
 * family  cpu_family  speed  microdata  cpuname      X = microdata / speed
 * ------  ----------  -----  ---------  -----------  ----------------------
 *   --        *3*       ??      ??      386          ??
 *
 *   --        *4*       33      71      486          2.15151515151515151515
 *
 *    5         5        60      74      Pentium      1.23333333333333333333
 *    5         5        60      83      Pentium      1.38333333333333333333
 *
 *    5         5       100     124      Pentium      1.24000000000000000000
 *
 *    6         5       150     261      Pentium Pro  1.74000000000000000000
 *    6         5       150     260      Pentium Pro  1.73333333333333333333
 *
 *    6         5       200     349      Pentium Pro  1.74500000000000000000
 *
 *    6         5       200    1994      AMD-K6       9.97
 */

typedef struct
{
    u_long	microdata;
    const char	*descr;
} speed_ref_t;

static const speed_ref_t	speed_ref[] =	/* ## not yet used */
{
    { 71,	"33 Mhz 486" },

    { 74,	"60 Mhz Pentium" },
    { 83,	"60 Mhz Pentium 510\\60" },
    { 124,	"100 Mhz Pentium" },

    { 261,	"150 Mhz Pentium Pro" },
    { 260,	"150 Mhz Pentium Pro" },
    { 349,	"200 Mhz Pentium Pro" },
};

/*
 * cpu_id is bit encoded as:
 *  x x x x x x x x x x x x x x x x x x  x x  x x x x  x x x x  x x x x
 *  -----------------------------------  ---  -------  -------  -------
 *                   ^                    ^      ^        ^        ^
 *                   |                    |      |        |        |__ Stepping
 *                   |                    |      |        |___________ Model
 *                   |                    |      |____________________ Family
 *                   |                    |___________________________ Type
 *                   |________________________________________________ Reserved
 */

#define CPU_RSVD_MASK	0xffffc000
#define CPU_TYPE_MASK	0x00003000
#define CPU_FAMILY_MASK	0x00000f00
#define CPU_MODEL_MASK	0x000000f0
#define CPU_STEP_MASK	0x0000000f

#define QUAL_CPUID(cpu_id, x)	((cpu_id == NO_CPUID) ? NO_CPUID : (x))
#define CPU_TYPE(cpu_id)	QUAL_CPUID(cpu_id, (cpu_id >> 12) & 0x03)
#define CPU_FAMILY(cpu_id) ((u_char)QUAL_CPUID(cpu_id, (cpu_id >>  8) & 0x0f))
#define CPU_MODEL(cpu_id)	QUAL_CPUID(cpu_id, (cpu_id >>  4) & 0x0f)
#define CPU_STEP(cpu_id)	QUAL_CPUID(cpu_id,  cpu_id        & 0x0f)

typedef enum
{
    cpu_vend_unk,
    cpu_vend_intel,
    cpu_vend_amd,
    cpu_vend_cyrix
} cpu_vendor_t;

typedef struct
{
    const char		*id;
    cpu_vendor_t	vendor;
} vendor_id_t;

const vendor_id_t	vendors[] =
{
    { "GenuineIntel",	cpu_vend_intel	},
    { "AuthenticAMD",	cpu_vend_amd	},
    { "CyrixInstead",	cpu_vend_cyrix	}
};

#define ID_LEN	(sizeof(long)*3)

typedef struct
{
    u_char		cpu_family;
    u_char		cpu_id_family;
    u_long		cpu_id_model;
    cpu_vendor_t	cpu_vendor;
    double		cpu_factor;	/* ## take this out */
    const char		*cpu_name;
} cpu_info_t;

/*
 * FAMILY_GE5
 *	In some versions of the kernel, cpu_family may not know
 *	about all CPU types.  For instance, cpu_family may show
 *	a P6 or P7 as a P5 since P6 and P7 were not known at the
 *	time (OpenServer prior to Commet bl6).  For that reason,
 *	multiple entries with lesser cpu_family values may be
 *	needed.
 *
 * ANY_FAMILY
 *	Used to support CPU models that do not have the CPUID
 *	instruction.
 *
 * ANY_MODEL
 *	Allows an "other" grouping for unknown models
 */

#define ANY_TYPE	((u_char)-1)
#define FAMILY_GE5	((u_char)-2)
#define ANY_FAMILY	((u_char)-1)
#define ANY_MODEL	((u_long)-1)
#define ANY_STEP	((u_long)-1)
#define NO_CPUID	((u_long)-1)

static const char	Generic386[] =	"386";

static const char	I486DX[] =	"486 - Intel 486DX";
static const char	I486SX[] =	"486 - Intel 486SX";
static const char	I486DX2[] =	"486 - Intel 487 or Intel DX2";
static const char	I486SL[] =	"486 - Intel 486SL";
static const char	I486SX2[] =	"486 - Intel SX2";
static const char	I486DX4[] =	"486 - Intel DX4";
static const char	AMD486e[] =	"486 - AMD Am5x86 Write-through mode";
static const char	AMD486f[] =	"486 - AMD Am5x86 Write-back mode";
static const char	AMD486[] =	"486 - AMD Am5x86";
static const char	CRX486[] =	"486 - Cyrix";
static const char	Generic486[] =	"486";

static const char	Pentium510[] =	"Intel Pentium (510\\60, 567\\66)";
static const char	Pentium735[] =	"Intel Pentium (735\\90, 815\\100)";
static const char	Pentium[] =	"Intel Pentium";
static const char	AMD_K5[] =	"Pentium Compatible - AMD-K5";
static const char	CrxP5[] =	"Pentium Compatible - Cyrix";
static const char	GenericP5[] =	"Pentium Compatible";

static const char	PentiumPro[] =	"Intel Pentium Pro";
static const char	AMD_K6[] =	"Pentium Pro Compatible - AMD-K6";
static const char	CrxP6[] =	"Pentium Pro Compatible - Cyrix";
static const char	GenericP6[] =	"Pentium Pro Compatible";

static const char	PentiumII[] =	"Intel Pentium II";

/*
 * This table is scanned in linear order.  More generic entries
 * should follow more specific ones.
 */

cpu_info_t	cpu_info[] =
{
    { 3, ANY_FAMILY,	ANY_MODEL,	cpu_vend_unk,	2.50, Generic386 },

    { 4, 4,		0,		cpu_vend_intel,	2.15, I486DX },
    { 4, 4,		1,		cpu_vend_intel,	2.15, I486DX },
    { 4, 4,		2,		cpu_vend_intel,	2.15, I486SX },
    { 4, 4,		3,		cpu_vend_intel,	2.15, I486DX2 },
    { 4, 4,		4,		cpu_vend_intel,	2.15, I486SL },
    { 4, 4,		5,		cpu_vend_intel,	2.15, I486SX2 },
    { 4, 4,		8,		cpu_vend_intel,	2.15, I486DX4 },
    { 4, 4,		0xe,		cpu_vend_amd,	2.15, AMD486e },
    { 4, 4,		0xf,		cpu_vend_amd,	2.15, AMD486f },
    { 4, 4,		ANY_MODEL,	cpu_vend_amd,	2.15, AMD486 },
    { 4, 4,		ANY_MODEL,	cpu_vend_cyrix,	2.15, CRX486 },
    { 4, ANY_FAMILY,	ANY_MODEL,	cpu_vend_unk,	2.15, Generic486 },

    { 5, 5,		1,		cpu_vend_intel,	1.32, Pentium510 },
    { 5, 5,		2,		cpu_vend_intel,	1.32, Pentium735 },
    { 5, 5,		ANY_MODEL,	cpu_vend_intel,	1.32, Pentium },

    { 5, 5,		0,		cpu_vend_amd,	1.32, AMD_K5 },
    { 5, 5,		1,		cpu_vend_amd,	1.32, AMD_K5 },
    { 5, 5,		2,		cpu_vend_amd,	1.32, AMD_K5 },
    { 5, 5,		3,		cpu_vend_amd,	1.32, AMD_K5 },

    { 5, 5,		6,		cpu_vend_amd,	9.97, AMD_K6 },

    { 5, 5,		ANY_MODEL,	cpu_vend_cyrix,	1.32, CrxP5 },
    { 5, 5,		ANY_MODEL,	cpu_vend_unk,	1.32, GenericP5 },

    { FAMILY_GE5, 6,	1,		cpu_vend_intel,	1.73, PentiumPro },
    { FAMILY_GE5, 6,	2,		cpu_vend_intel,	1.73, PentiumPro },
    { FAMILY_GE5, 6,	3,		cpu_vend_intel,	1.73, PentiumII },
    { FAMILY_GE5, 6,	ANY_MODEL,	cpu_vend_intel,	1.73, PentiumII },

    { FAMILY_GE5, 6,	ANY_MODEL,	cpu_vend_cyrix,	1.73, CrxP6 },
    { FAMILY_GE5, 6,	ANY_MODEL,	cpu_vend_unk,	1.73, GenericP6 },
};
static const int	num_cpu_info = sizeof(cpu_info)/sizeof(*cpu_info);

typedef struct
{
    double	low;
    double	high;
    u_long	nominal;
} cpu_range_t;

static const cpu_range_t	cpu_range[] =
{
    {  22.0,  28.0,  25 },
    {  30.0,  36.0,  33 },
    {  47.0,  55.0,  50 },
    {  55.1,  62.9,  60 },
    {  63.0,  69.0,  66 },
    {  71.0,  78.0,  75 },
    {  92.0, 103.0, 100 },
    { 117.0, 123.0, 120 },
    { 197.0, 203.0, 200 },
};
static const int num_cpu_range = sizeof(cpu_range)/sizeof(*cpu_range);

static void show_cpu_features(FILE *out, int cpu,
				u_char cpu_family, cpu_vendor_t cpu_vendor);
static void show_msr(FILE *out, int cpu,
				u_char cpu_family, cpu_vendor_t cpu_vendor);

static cpu_vendor_t get_cpu_vendor(const char *cpu_vendorid);
static const char *GetMfgrStep(cpu_vendor_t vendor, u_long cpu_id);
static void MsrPrintFirst(FILE *out);
static void MsrPrintGeneric(FILE *out);
static const char *MsrGenericName(u_long addr);
static int MsrPrintDebug(FILE *out, int cpu, u_long end_addr);
static void MsrPrint32(FILE *out, const char *format,
	    u_long value, u_long mask, int shift, const char * const *boolNm);

int
have_cpu(void)
{
    return 1;	/* I HOPE we always have at least one */
}

void
report_cpu(FILE *out)
{
    u_char	cpu_family;
    u_long	cpu_id;
    u_long	microdata;
    int		cpu_ix;
    int		n;
    double	cpu_speed;
    int		num_cpu;
    int		cpu;


    report_when(out, "cpu");

    num_cpu = show_num_cpu(out);
    for (cpu = 0; cpu < num_cpu; ++cpu)
    {
	char			cpu_vendorid[ID_LEN+1];
	cpu_vendor_t		cpu_vendor;

 	/*
	 * Just to clarify
	 *	cpu = 0 is the base processor
	 *	cpu = 1 is first aux processor
	 *	cpu = 2 is next
	 */

	if (cpu > 0)
	    fprintf(out, "\n");

	if (read_kvar_d("microdata", &microdata) ||
	    read_cpu_kvar_b(cpu, "cpu_family", &cpu_family))
	{
	    if (num_cpu <= 1)
		return;		/* Nothing yet reported, exit quietly */

	    fprintf(out, "    Information on CPU %d is unavailable\n", cpu+1);
	    continue;
	}

	if (read_cpu_kvar_d(cpu, "cpu_id", &cpu_id) || (cpu_id == 0))
	    cpu_id = NO_CPUID;

	/*
	 * Identify the vendor
	 *
	 * Prior to Commet bl6, there was no cpu_vendorid, only
	 * base_cpu_vendorid.  If we fail, try the old way.
	 */

	if (!read_cpu_kvar(cpu, "cpu_vendorid", cpu_vendorid, ID_LEN) ||
	    ((cpu == 0) &&
	     !read_kvar("base_cpu_vendorid", cpu_vendorid, ID_LEN)))
	{
	    const char		*tp;
	    int			len;

	    cpu_vendorid[ID_LEN] = '\0';	/* Terminate the string */

	    len = 0;
	    for (tp = cpu_vendorid; *tp; ++tp)
	    {
		if (!isprint(*tp))
		    break;

		++len;
	    }

	    if (len != ID_LEN)
		cpu_vendorid[0] = '\0';		/* Invalid vendor ID */
	}
	else
	    cpu_vendorid[0] = '\0';		/* No vendor ID */

	cpu_vendor = get_cpu_vendor(cpu_vendorid);

	/*
	 * Locate the info on this CPU type
	 */

	for (cpu_ix = 0; cpu_ix < num_cpu_info; ++cpu_ix)
	    if (((cpu_family == cpu_info[cpu_ix].cpu_family) ||
		 ((cpu_info[cpu_ix].cpu_family == FAMILY_GE5) &&
		  (cpu_family >= 5))) &&

		((cpu_info[cpu_ix].cpu_id_family == ANY_FAMILY) ||
		 (cpu_info[cpu_ix].cpu_id_family == CPU_FAMILY(cpu_id))) &&

		((cpu_info[cpu_ix].cpu_id_model == ANY_MODEL) ||
		 (cpu_info[cpu_ix].cpu_id_model == CPU_MODEL(cpu_id))) &&

		((cpu_info[cpu_ix].cpu_vendor == cpu_vend_unk) ||
		 (cpu_info[cpu_ix].cpu_vendor == cpu_vendor)))
		    break;

	if (cpu_ix >= num_cpu_info)
	{
	    if (num_cpu <= 1)
		fprintf(out, "\tThe CPU type is unknown\n");
	    else
		fprintf(out, "\tThe type of CPU %d is unknown\n", cpu+1);

	    fprintf(out, "\t    cpu_family: 0x%.2x\n", cpu_family);

	    if (cpu_id != NO_CPUID)
		fprintf(out, "\t    cpu_id:     0x%.8x\n", cpu_id);

	    if (cpu == 0)
		fprintf(out, "\t    microdata:  0x%.8x\n", microdata);

	    continue;
	}

	/*
	 * Calculate our guess of the correct speed.  If we can find
	 * it in our range of nominal speeds, use the nominal; 
	 * otherwise just use the calculated value.
	 */

	cpu_speed = microdata / cpu_info[cpu_ix].cpu_factor;

	for (n = 0; n < num_cpu_range; ++n)
	    if ((cpu_speed >= cpu_range[n].low) &&
		(cpu_speed <= cpu_range[n].high))
		    break;

	if (num_cpu <= 1)
	    fprintf(out, "    The CPU performs like a %ldMhz %s\n",
					(n < num_cpu_range)
					    ? cpu_range[n].nominal
					    : (u_long)(cpu_speed + 0.5),
					cpu_info[cpu_ix].cpu_name);
	else
	    fprintf(out,
		"    CPU %d performs like a %ldMhz %s\n",
					cpu+1,
					(n < num_cpu_range)
					    ? cpu_range[n].nominal
					    : (u_long)(cpu_speed + 0.5),
					cpu_info[cpu_ix].cpu_name);

	if (verbose)
	{
	    u_long	processor;

	    fprintf(out, "\n");

	    if (!read_cpu_kvar_d(cpu, "processor", &processor))
		fprintf(out, "\tProcessor:     %d (0x%.2lx)\n",
					    is_mp_kernel()
						? (CI_OWNER(processor))
						: (processor + 1),
						processor);

	    if (cpu_vendorid[0])
		fprintf(out, "\tVendor ID:     %s\n", cpu_vendorid);

	    fprintf(out, "\tcpu_family:    %d\n", cpu_family);

	    if (cpu_id != NO_CPUID)
	    {
		const char	*ms;

		fprintf(out,
		    "\tcpu_id:        0x%.8lx\n"
		    "\t    type:        %lu\n"
		    "\t    family:      %u\n"
		    "\t    model:       %lu\n"
		    "\t    stepping:    %lu\n",
					    cpu_id,
						CPU_TYPE(cpu_id),
						(u_int)CPU_FAMILY(cpu_id),
						CPU_MODEL(cpu_id),
						CPU_STEP(cpu_id));

		if ((ms = GetMfgrStep(cpu_vendor, cpu_id)) != NULL)
		    fprintf(out, "\t    mfgr step:   %s\n", ms);
	    }

	    show_cpu_features(out, cpu, cpu_family, cpu_vendor);

	    if (cpu == 0)
		fprintf(out, "\tmicrodata:     %lu\n", microdata);

	    fprintf(out, "\tderived speed: %lg MHz\n", cpu_speed);
	}

	show_msr(out, cpu, cpu_family, cpu_vendor);
    }

    fprintf(out, "\n");
}

static cpu_vendor_t
get_cpu_vendor(const char *cpu_vendorid)
{
    int		n;

    if (!cpu_vendorid || !*cpu_vendorid)
	return cpu_vend_unk;

    for (n = 0; n < sizeof(vendors)/sizeof(*vendors); ++n)
	if (strncmp(cpu_vendorid, vendors[n].id, ID_LEN) == 0)
	    return vendors[n].vendor;
    
    return cpu_vend_unk;
}

static const char *
GetMfgrStep(cpu_vendor_t vendor, u_long cpu_id)
{
    static const struct
    {
	cpu_vendor_t	vendor;
	u_long		type;
	u_long		family;
	u_long		model;
	u_long		step;
	const char	*mfgstep;
    } stepmap[] =
    {
	/*      vendor        type   family   model    step    mfgstep	*/
	{ cpu_vend_intel,	0,	5,	1,	3,	"B1"	},
	{ cpu_vend_intel,	0,	5,	1,	5,	"C1"	},
	{ cpu_vend_intel,	0,	5,	1,	7,	"D1"	},
	{ cpu_vend_intel,	0,	5,	2,	1,	"B1"	},
	{ cpu_vend_intel,	2,	5,	2,	1,	"B1"	},
	{ cpu_vend_intel,	0,	5,	2,	2,	"B3"	},
	{ cpu_vend_intel,	2,	5,	2,	2,	"B3"	},
	{ cpu_vend_intel,	0,	5,	2,	4,	"B5"	},
	{ cpu_vend_intel,	2,	5,	2,	4,	"B5"	},
	{ cpu_vend_intel,	0,	5,	2,	5,	"C2"	},
	{ cpu_vend_intel,	2,	5,	2,	5,	"C2"	},
	{ cpu_vend_intel,	0,	5,	2,	0xB,	"cB1"	},
	{ cpu_vend_intel,	2,	5,	2,	0xB,	"cB1"	},
	{ cpu_vend_intel,	0,	5,	2,	0xC,	"cC0"	},
	{ cpu_vend_intel,	2,	5,	2,	0xC,	"cC0"	},
	{ cpu_vend_intel,	0,	5,	7,	0,	"mA4"	},
	{ cpu_vend_intel,	0,	5,	2,	0xC,	"mcC0"	},
	{ cpu_vend_intel,	0,	5,	2,	6,	"E0"	},	
	{ cpu_vend_intel,	2,	5,	2,	6,	"E0"	},
	{ cpu_vend_intel,	0,	6,	1,	1,	"B0"	},
	{ cpu_vend_intel,	0,	6,	1,	2,	"C0"	},
	{ cpu_vend_intel,	0,	6,	1,	6,	"sA0"	},
	{ cpu_vend_intel,	0,	6,	1,	7,	"sA1"	},
    };
    int		n;


    if (cpu_id == NO_CPUID)
	return NULL;

    for (n = 0; n < sizeof(stepmap)/sizeof(*stepmap); ++n)
	if ((stepmap[n].vendor == vendor) &&

	    ((stepmap[n].type == ANY_TYPE) ||
	     (stepmap[n].type == CPU_TYPE(cpu_id))) &&

	    ((stepmap[n].family == ANY_FAMILY) ||
	     ((stepmap[n].family == FAMILY_GE5) &&
	      (CPU_FAMILY(cpu_id) >= 5)) ||
	     (stepmap[n].family == CPU_FAMILY(cpu_id))) &&

	    ((stepmap[n].model == ANY_MODEL) ||
	     (stepmap[n].model == CPU_MODEL(cpu_id))) &&

	    ((stepmap[n].step == ANY_STEP) ||
	     (stepmap[n].step == CPU_STEP(cpu_id))))

		return stepmap[n].mfgstep;

    return NULL;
}

/*
 * The value returned from get_num_cpu() is the physical number
 * of CPUs.  The value in scoutsname is the number of licensed
 * CPUs; that is, the number that are functioning.
 */

int
show_num_cpu(FILE *out)
{
    int		num_cpu;

    /*
     * For purposes of reality, there is always at least one CPU
     * on every system.
     */

    if ((num_cpu = get_num_cpu()) < 0)
	num_cpu = 1;

    if (num_cpu <= 1)
	fprintf(out, "    There is one CPU on this system.\n");
    else
    {
	fprintf(out, "    There are %d CPUs on this system.\n", num_cpu);

	if (verbose)
	{
	    struct scoutsname	suts;
	    u_long		value;

	    if (!read_kvar_d("max_ACPUs", &value)) /* maximum MPX aux CPUs */
		fprintf(out, "\tMaximum:\t%lu\n", value+1);

	    if (!__scoinfo(&suts, sizeof(suts)) && (suts.numcpu >= 1))
		fprintf(out, "\tConfigured:\t%lu\n", (u_long)suts.numcpu);

	    if (!read_kvar_d("live_ACPUs", &value))
		fprintf(out, "\tActive:\t\t%lu\n", value+1);

	    /* ##
		cpu_dead
		mpx_rap
	    */
	}
    }

    fprintf(out, "\n");
    return num_cpu;
}

typedef struct
{
    u_long	mask;
    const char	*mnemonic;
    const char	*name;
} feat_bits_t;

#define END_OF_FEAT_BITS	{ 0, NULL, NULL }

static const feat_bits_t	intel_p6_feature_bits[] =
{
    {  CPUF_INTEL_APIC,	"APIC",	"APIC on Chip" },
    {  CPUF_INTEL_CMOV,	"CMOV",	"Conditional Move and Compare Instructions" },
    {  CPUF_CX8,	"CXS",	"CMPXCHG8B instruction" },
    {  CPUF_DE,		"DE",	"Debugging Extensions" },
    {  CPUF_FPU,	"FPU",	"FPU on chip" },
    {  CPUF_INTEL_MCA,	"MCA",	"Machine Check Architecture" },
    {  CPUF_MCE,	"MCE",	"Machine Check Exception" },
    {  CPUF_INTEL_MMX,	"MMX",	"MMX Technology Supported" },
    {  CPUF_MSR,	"MSR",	"RDMSR and WRMSR Support" },
    {  CPUF_INTEL_MTTR,	"MTRR",	"Memory Type Range Registers" },
    {  CPUF_INTEL_PAE,	"PAE",	"Physical Address Extensions" },
    {  CPUF_INTEL_PGE,	"PGE",	"PTE Global Flag" },
    {  CPUF_PSE,	"PSE",	"Page Size Extensions" },
    {  CPUF_TSC,	"TSC",	"Time Stamp Counter" },
    {  CPUF_VME,	"VME",	"Virtual 8086 Mode Enhancement" },

    END_OF_FEAT_BITS
};

static const feat_bits_t	amd_p5_feature_bits[] =
{
    {  CPUF_CX8,	"CXS",	"CMPXCHG8B instruction" },
    {  CPUF_DE,		"DE",	"I/O Breakpoints" },
    {  CPUF_FPU,	"FPU",	"FPU on chip" },
    {  CPUF_AMD_GPE,	"GPE",	"Global Paging Extensions" },
    {  CPUF_MCE,	"MCE",	"Machine Check Exception" },
    {  CPUF_MSR,	"MSR",	"K86 Model-Specific Registers" },
    {  CPUF_PSE,	"PSE",	"4-Mbyte Pages" },
    {  CPUF_TSC,	"TSC",	"Time Stamp Counter" },
    {  CPUF_VME,	"VME",	"Virtual 8086 Mode Enhancement" },

    END_OF_FEAT_BITS
};

static const feat_bits_t	cyrix_p5_feature_bits[] =
{
    {  CPUF_FPU,	"FPU",	"FPU on chip" },

    END_OF_FEAT_BITS
};

typedef struct
{
    cpu_vendor_t	vendor;
    u_char		family;
    const feat_bits_t	*mp;
} feat_model_t;

static const feat_model_t	feat_model[] =
{
    { cpu_vend_intel,	5, intel_p6_feature_bits },
    { cpu_vend_intel,	6, intel_p6_feature_bits },

    { cpu_vend_amd,	4, amd_p5_feature_bits },
    { cpu_vend_amd,	5, amd_p5_feature_bits },

    { cpu_vend_cyrix,	5, cyrix_p5_feature_bits },
};

static void
show_cpu_features(FILE *out,int cpu, u_char cpu_family, cpu_vendor_t cpu_vendor)
{
    u_long		cpu_features;
    int			n;
    register size_t	len;
    size_t		tab;
    const feat_bits_t	*mp;

    /*
     * Figure out which feat_bits_t to use
     */

    for (n = 0; n < sizeof(feat_model)/sizeof(*feat_model); ++n)
	if (((feat_model[n].vendor == cpu_vend_unk) ||
	     (feat_model[n].vendor == cpu_vendor)) &&
	    ((feat_model[n].family == ANY_FAMILY) ||
	     (feat_model[n].family == cpu_family)))
	    break;

    if (n >= sizeof(feat_model)/sizeof(*feat_model))
	return;		/* No table found */
    
    mp = feat_model[n].mp;

    /*
     * Display all we know
     */

    if (read_cpu_kvar_d(cpu, "cpu_features", &cpu_features))
	return;		/* Probably no such variable */

    fprintf(out, "\tcpu_features:");
    if (verbose)
	fprintf(out, "  0x%.8x\n", cpu_features);
    else
	fprintf(out, "\n");

    tab = 5;	/* Minimum field width */
    for (n = 0; mp[n].name; ++n)
	if ((len = strlen(mp[n].mnemonic)) > tab)
	    tab = len;

    for (n = 0; mp[n].name; ++n)
	fprintf(out, "\t    %s:%s %s   %s\n",
				    mp[n].mnemonic,
				    spaces(tab-strlen(mp[n].mnemonic)),
				    (cpu_features & mp[n].mask) ? Yes : No,
				    mp[n].name);
}

/*
 * MSR display formats
 */

static const char Msr64bitHex[] = "0x%.8lx%.8lx"; /* Use for 61-64 bit HEX */
static const char Msr32bitHex[] = "0x%.8lx";	  /* Use for 29-32 bit HEX */
static const char Msr12bitHex[] = "0x%.3lx";	  /* Use for 9-12 bit HEX */
static const char Msr8bitHex[] = "0x%.2lx";	  /* Use for 5-8 bit HEX */
static const char Msr4bitHex[] = "0x%lx";	  /* Use for 1-4 bit HEX */

static const char Msr32bitDec[] = "%lu";	/* Use for 1-32 bit Decimal */

static const char Msr16bitBool[] = "%s";
static const char *Msr16bitYesNo[] = { Yes, No };
static const char *Msr16bitEnbl[] = { "Enabled", "Disabled" };
static const char *Msr16bitDsbl[] = { "Disabled", "Enabled" };
static const char *Msr16bitEccPar[] = { "ECC", "Parity" };

/*
 * The order of the mask and shift arrays is not the normal
 * Intel byte ordering.  It does however improve the readability
 * of msr_vals_t arrays.  Use the macros.
 */

#define MsrHiMask(mp)	(mp)->mask[0]
#define MsrLowMask(mp)	(mp)->mask[1]

#define MsrHiShift(mp)	(mp)->shift[0]
#define MsrLowShift(mp)	(mp)->shift[1]

typedef struct
{
    u_long	addr;
    u_int	level;
    u_long	mask[2];	/* mask[0] is for Hi dword, mask[1] is Low */
    int		shift[2];	/* shift[0] is for Hi dword, shift[1] is Low */
    const char	*format;	/* Table scan ends if this is NULL */
    const char	**boolNm;
    const char	*name;		/* Table scan ends if this is NULL */
} msr_vals_t;

#define END_OF_MSR_VALS	{0, 0, {0x00000000, 0x00000000}, {0, 0}, NULL,NULL,NULL}

/*
 * Intel Pentium
 */

static const msr_vals_t	intel_p5_msr[] =
{
    {
	MSR_P5_MC_ADDR, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"Machine check exception address"
    },
    {
	MSR_P5_MC_TYPE, 0,
	{ 0x00000000, 0xffffffff }, { 0, 0 }, Msr32bitHex, NULL,
	"Machine check exception type"
    },
    {
	2, 0,
	{ 0x00000000, 0xffffffff }, { 0, 0 }, Msr32bitHex, NULL,
	"TR1 parity reversal test register"
    },

    /* ## more */

    END_OF_MSR_VALS
};

/*
 * Intel Pentium Pro
 */

static const msr_vals_t	intel_p6_msr[] =
{
    {
	MSR_TSC, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"TSC"
    },
    {
	MSR_APICBASE, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"APICBASE"
    },
    {
	MSR_APICBASE, 1,
	{ 0x00000000, 0x00000100 }, { 0, 8 }, Msr16bitBool, Msr16bitYesNo,
	"BootStrap Processor"
    },
    {
	MSR_APICBASE, 1,
	{ 0x00000000, 0x00000800 }, { 0, 11 }, Msr16bitBool, Msr16bitEnbl,
	"APIC Global Enable"
    },
    {
	MSR_EBL_CR_POWERON, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"EBL_CR_POWERON"
    },
    {
	MSR_EBL_CR_POWERON, 1,
	{ 0x00000000, 0x00000001 }, { 0, 0 }, Msr16bitBool, Msr16bitEccPar,
	"Data bus error policy"
    },
    {
	MSR_EBL_CR_POWERON, 1,
	{ 0x00000000, 0x01c00000 }, { 0, 22 }, Msr32bitDec, NULL,
	"Clock Frequency Ratio"
    },
    {
	MSR_MISC_ADDR33, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"MISC_ADDR33"
    },
    {
	MSR_MISC_ADDR33, 1,
	{ 0x00000000, 0x40000000 }, { 0, 30 }, Msr16bitBool, Msr16bitDsbl,
	"Instruction Streaming Buffers"
    },

    /* ## more */

    END_OF_MSR_VALS
};

/*
 * AMD 5k86
 */

static const msr_vals_t	amd_p5_msr[] =
{
    {
	MSR_P5_MC_ADDR, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"MCAR: Machine-Check Address Register"
    },
    {
	MSR_P5_MC_TYPE, 0,
	{ 0x00000000, 0x0000001f }, { 0, 0 }, Msr8bitHex, NULL,
	"MCTR: Machine-Check Type Register"
    },
    {
	MSR_TSC, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"TSC: Time Stamp Counter"
    },
    {
	0x82, 0,
	{ 0xffffffff, 0xffffffff }, { 0, 0 }, Msr64bitHex, NULL,
	"AAR: Array Access Register"
    },
    {
	0x83, 0,
	{ 0x00000000, 0x000001ff }, { 0, 0 }, Msr12bitHex, NULL,
	"HWDR: Hardware Configuration Register"
    },

    END_OF_MSR_VALS
};

typedef struct
{
    cpu_vendor_t	vendor;
    u_char		family;
    const msr_vals_t	*mp;
    u_long		max_msr;
} msr_model_t;

static const msr_model_t	msr_model[] =
{
    { cpu_vend_intel,	5, intel_p5_msr, 1050 },
    { cpu_vend_intel,	6, intel_p6_msr, 1050 },
    { cpu_vend_amd,	4, amd_p5_msr,   1050 },
    { cpu_vend_amd,	5, amd_p5_msr,   1050 },
};

static int	first_msr = 1;
static int	msr_tab = 1;
static int	msr_valid = 0;
static u_long	msr_addr = (u_long)-1;
static msr_t	msr_value;
static u_long	max_debug_msr;

static void
show_msr(FILE *out, int cpu, u_char cpu_family, cpu_vendor_t cpu_vendor)
{
    const msr_vals_t	*mp;
    const msr_vals_t	*tp;
    int			n;

    /*
     * Figure out which msr_vals to use
     */

    for (n = 0; n < sizeof(msr_model)/sizeof(*msr_model); ++n)
	if (((msr_model[n].vendor == cpu_vend_unk) ||
	     (msr_model[n].vendor == cpu_vendor)) &&
	    ((msr_model[n].family == ANY_FAMILY) ||
	     (msr_model[n].family == cpu_family)))
	    break;

    if (n >= sizeof(msr_model)/sizeof(*msr_model))
	return;		/* No table found */
    
    mp = msr_model[n].mp;
    max_debug_msr = msr_model[n].max_msr;

    /*
     * Calculate the tab position
     */

    msr_tab = debug ? strlen(MsrGenericName((u_long)-1)) : 1;
    for (tp = mp; tp->name; ++tp)
    {
	int	len;

	if ((len = strlen(tp->name)) > msr_tab)
	    msr_tab = len;
    }

    /*
     * Display all we know
     */

    first_msr = 1;
    msr_valid = 0;
    msr_addr = (u_long)-1;
    for ( ; mp->name && mp->format; ++mp)
    {
	if (!MsrLowMask(mp) && !MsrHiMask(mp))
	    continue;

	if (!msr_valid || (msr_addr != mp->addr))
	{
	    if ((mp->addr > 0) &&
		MsrPrintDebug(out, cpu, mp->addr-1))
		    return;		/* End of file */

	    if (read_msr(cpu, mp->addr, &msr_value))
	    {
		if (errno == 0)
		    return;	/* End of file */
		continue;
	    }

	    msr_valid = 1;
	    msr_addr = mp->addr;

	    if (mp->level != 0)
		MsrPrintGeneric(out);
	}

	MsrPrintFirst(out);
	fprintf(out, "%s\t    %s: %s",
				    spaces(mp->level * 4),
				    mp->name,
				    spaces(msr_tab-strlen(mp->name)));

	if (MsrLowMask(mp) && MsrHiMask(mp))
	    fprintf(out, mp->format,
		(MsrHiVal(&msr_value)  &  MsrHiMask(mp)) >> MsrHiShift(mp),
		(MsrLowVal(&msr_value) & MsrLowMask(mp)) >> MsrLowShift(mp));
	else if (MsrHiMask(mp))
	    MsrPrint32(out, mp->format,
		MsrHiVal(&msr_value), MsrHiMask(mp), MsrHiShift(mp),
								mp->boolNm);
	else /* (MsrLowMask(mp)) */
	    MsrPrint32(out, mp->format,
		MsrLowVal(&msr_value), MsrLowMask(mp), MsrLowShift(mp),
								mp->boolNm);

	fprintf(out, "\n");
    }

    MsrPrintDebug(out, cpu, (u_long)-1);
}

/*
 * Since /dev/msr may be /dev/null on some kernels, we do not
 * display our headding until we get at least one successful
 * data read.
 */

static void
MsrPrintFirst(FILE *out)
{
    if (!first_msr)
	return;

    first_msr = 0;
    fprintf(out, "\n\tModel Specific registers\n");
}

static int
MsrPrintDebug(FILE *out, int cpu, u_long end_addr)
{
    if (!debug)
	return 0;

    if (end_addr >= max_debug_msr)
	end_addr = max_debug_msr;

    msr_valid = 0;
    for (++msr_addr; msr_addr <= end_addr; ++msr_addr)
    {
	if (read_msr(cpu, msr_addr, &msr_value))
	{
	    if (errno == 0)
		return -1;	/* End of file */
	    msr_valid = 0;
	}
	else
	{
	    msr_valid = 1;
	    MsrPrintGeneric(out);
	}

	if (msr_addr == (u_long)-1)
	    break;	/* Prevent numeric wrap */
    }

    return 0;
}

static void
MsrPrintGeneric(FILE *out)
{
    const char	*name;

    MsrPrintFirst(out);

    name = MsrGenericName(msr_addr);
    fprintf(out, "\t    %s: %s0x%.8lx%.8lx\n",
			name,
			spaces(msr_tab-strlen(name)),
			MsrHiVal(&msr_value), MsrLowVal(&msr_value));
}

static const char *
MsrGenericName(u_long addr)
{
    static char	buf[128];

    sprintf(buf, "MSR 0x%.2lx", addr);
    return buf;
}

static void
MsrPrint32(FILE *out, const char *format, u_long value, u_long mask, int shift,
						    const char * const *boolNm)
{
    if (format == Msr16bitBool)
	fprintf(out, Msr16bitBool, (value & mask) ? boolNm[0] : boolNm[1]);
    else
	fprintf(out, format, (value & mask) >> shift);
}

