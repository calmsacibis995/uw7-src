/*
 * File hw_mp.c
 * Information handler for mp
 *
 * @(#) hw_mp.c 67.2 97/10/09 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sys/bootinfo.h>

#include "hw_mp.h"
#include "hw_util.h"
#include "hw_cpu.h"	/* for show_num_cpu() */
#include "pci_data.h"

/*
 * Feature byte 2 definitions
 * See also usr/src/uts/i386/corollary/vendors/intel/pcmp.h
 */
#define FEAT_MCLK	0x40	/* multiple processor clock sources	*/
#define FEAT_IMCR	0x80	/* system has an IMCR register		*/

/*
 * Definition of MP Configuration Table header.
 */

typedef struct
{
    u_int	signature;	/* signature (the string "PCMP") */
    u_short	length;		/* total table length (bytes) */
    u_char	spec_rev;	/* PC+MP version number */
    u_char	checksum;	/* all bytes to add up to zero */
    char	oemid[8];	/* OEM string */
    char	productid[12];	/* product string */
    paddr_t	oem_addr;	/* OEM table physical address */
    u_short	oem_size;	/* OEM table size (bytes) */
    u_short	entry_count;	/* number of entries */
    paddr_t	local_addr;	/* physical address of Local APIC */
    u_int	res1;		/* reserved */
} header_t;

/*
 * The following was extracted from civendor.h
 */

#define	CI_AT_BRIDGE	1		/* Corollary 2-board set	 */
#define	MP_Z1000	2		/* Corollary 3-board set - Z1000 */
#define	MP_MITAC	3		/* Corollary 2-board set - Mitac */
#define	MP_COMPAQ	4		/* Compaq Systempro		 */
#define	MP_APRICOT	5		/* Apricot Voyager		 */
#define	MP_NX		6		/* Reserved			 */
#define	MP_MPAX		7		/* Chips & Technologies M/Pax 	 */
#define	MED_AT_BRIDGE	8		/* Corollary 2-board set - Medidata */
#define	MP_CBUS_PLUS	9		/* Corollary C-bus II		 */
#define	MP_RES1	       10		/* Reserved			 */
#define	MP_RES2	       11		/* Reserved			 */
#define	MP_TPF	       12		/* Tricord PowerFrame		 */
#define	MP_RES3	       13		/* Reserved			 */
#define MP_PCE	       14		/* SNI PCE			 */
#define MP_AST	       15		/* AST Manhattan SMP		 */
#define MP_IMPS        16		/* Intel Multiprocessor Spec	 */

#define	MP_CRLLRYISA	0x01		/* Corollary ISA architecture	 */
#define	MP_CRLLRYEISA	0x02		/* Corollary EISA architecture	 */
#define	MP_CPQCLASS	0x04		/* Compaq Systempro or clone     */
#define	MP_APRCLASS	0x08		/* Apricot Voyager or clone      */
#define	MP_NXCLASS	0x10		/* Reserved			 */
#define	MP_MPAXCLASS	0x20		/* M/Pax or clone      	 	 */
#define	MP_CBPEISA	0x40		/* Corollary C-bus II EISA arch. */
#define	MP_RES1CLASS    0x80		/* Reserved			 */
#define	MP_RES2CLASS   0x100		/* Reserved			 */
#define	MP_TPFCLASS    0x200		/* Tricord PowerFrame		 */
#define	MP_RES3CLASS   0x400		/* Reserved			 */
#define MP_SNICLASS    0x800		/* SNI PCE			 */
#define MP_ASTCLASS   0x1000		/* AST Manhattan SMP		 */
#define MP_IMPSCLASS  0x2000		/* Intel Multiprocessor Spec	 */
#define	MP_CBPPCI     0x4000		/* Corollary C-bus II PCI arch.  */

/*
 * This module is not combined with the CPU report because
 * command inquiries about the existence of a CPU will always
 * return TRUE and inquiries about the existence of MP may not.
 */

const char	* const callme_mp[] =
{
    "mp",
    "gpi",
    NULL
};

const char	short_help_mp[] = "Multiprocessor information";

static u_long	*pcmp_list = NULL;
static u_long	pcmp_cnt = 0;

static void	gpi_mpvendor(FILE *out);
static void	gpi_mpvendorclass(FILE *out);

static void	pcmp_hdwr_info(FILE *out);

static void	a15k_gpi_info(FILE *out);
static void	attack_gpi_info(FILE *out);
static void	cbp_gpi_info(FILE *out);
static void	ncrmp_gpi_info(FILE *out);
static void	pce_gpi_info(FILE *out);
static void	pcmp_gpi_info(FILE *out);
static void	systempro_gpi_info(FILE *out);
static void	tpf_gpi_info(FILE *out);
static void	olimpx_gpi_info(FILE *out);
static void	olimpxp5_gpi_info(FILE *out);
static void	sysproxl_gpi_info(FILE *out);
static void	wyse_gpi_info(FILE *out);

typedef struct
{
    const char	*name;
    void	(*gpi_info)(FILE *out);
    void	(*hdwr_info)(FILE *out);
} gpi_info_t;

static gpi_info_t	gpi_info[] =
{
    { "a15k",		a15k_gpi_info,		NULL },
    { "smp",		attack_gpi_info,	NULL },
    { "smpz",		attack_gpi_info,	NULL },
    { "smpmitac",	attack_gpi_info,	NULL },
    { "cbus",		cbp_gpi_info,		NULL },
    { "ncrmp",		ncrmp_gpi_info,		NULL },
    { "pce",		pce_gpi_info,		NULL },
    { "pcmp",		pcmp_gpi_info,		pcmp_hdwr_info },
    { "cpq",		systempro_gpi_info,	NULL },
    { "tpf",		tpf_gpi_info,		NULL },
    { "oli",		olimpx_gpi_info,	NULL },
    { "olip5",		olimpxp5_gpi_info,	NULL },
    { "sp_xl",		sysproxl_gpi_info,	NULL },
    { "wyse",		wyse_gpi_info,		NULL },
};

/*
 * Definitions of MP Configuration Table entries.
 */

#pragma pack(1)

/*
 * Processor entry
 */

typedef struct
{
    u_char	type;		/* Entry type (0) */
    u_char	local_id;	/* Local APIC id */
    u_char	local_version;	/* Local APIC version */
    u_char	bits1;		/* reserved.Bootstrap.Enable */
    u_short	fms;		/* reserved.family.model.stepping */
    u_short	res3;		/* reserved */
    u_int	features;	/* processor features */
    u_int	res4[2];	/* reserved */
} pcmp_proc_entry_t;

/*
 * Bus entry
 */

typedef struct
{
    u_char	type;		/* Entry type (1) */
    u_char	bus_id;		/* Bus id */
    char	bus[6];		/* Bus string */
} pcmp_bus_entry_t;

/*
 * I/O APIC entry
 */

typedef struct
{
    u_char	type;		/* Entry type (2) */
    u_char	io_id;		/* I/O APIC id */
    u_char	io_version;	/* I/O APIC version */
    u_char	enable;		/* reserved.Enable_flag */
    paddr_t	io_addr;	/* I/O APIC physical address */
} pcmp_ioapic_entry_t;

/*
 * Local or I/O APIC Interrupt Assignment entry
 */

typedef struct
{
    u_char	type;		/* Entry type (3 or 4) */
    u_char	intr_type;	/* Interrupt type */
    u_char	bits;		/* reserved.Edge/Level.Polarity */
    u_char	reserved;
    u_char	source_bus;	/* Source Bus Id */
    u_char	source_irq;	/* Source Bus IRQ */
    u_char	apic_id;	/* Destination APIC id */
    u_char	apic_pin;	/* Destination APIC pin */
} pcmp_intr_entry_t;

#pragma pack()

#define	PCMP_PROC_TYPE		0
#define	PCMP_BUS_TYPE		1
#define	PCMP_IOAPIC_TYPE	2
#define	PCMP_INTR0_TYPE		3
#define	PCMP_INTR1_TYPE		4

static void	pcmp_proc_dump(FILE *out, void *ep, u_long addr);
static void	pcmp_bus_dump(FILE *out, void *ep, u_long addr);
static void	pcmp_ioapic_dump(FILE *out, void *ep, u_long addr);
static void	pcmp_intr_dump(FILE *out, void *ep, u_long addr);

typedef struct
{
    int		entry_type;
    int		structsz;
    void	(*func)(FILE *out, void *ep, u_long addr);
} pcmp_entry_table_t;

static const pcmp_entry_table_t	pcmp_entry_table[] =
{
    { PCMP_PROC_TYPE,	sizeof(pcmp_proc_entry_t),	pcmp_proc_dump },
    { PCMP_BUS_TYPE,	sizeof(pcmp_bus_entry_t),	pcmp_bus_dump },
    { PCMP_IOAPIC_TYPE,	sizeof(pcmp_ioapic_entry_t),	pcmp_ioapic_dump },
    { PCMP_INTR0_TYPE,	sizeof(pcmp_intr_entry_t),	pcmp_intr_dump },
    { PCMP_INTR1_TYPE,	sizeof(pcmp_intr_entry_t),	pcmp_intr_dump },
    { -1,		0,				NULL }
};

#define	NBUS	16

struct
{
    int		bus_id;
    char	bus_name[8];
} pcmp_bus_type[NBUS];

static void	pcmp_cfgtable(FILE *out, u_long addr);
static void	pcmp_header_dump(FILE *out, header_t *hp, u_long addr);
static void	pcmp_pci_info(FILE *out, int devnum);
static void	pcmp_add_bus_entry(pcmp_bus_entry_t *ep);

int
have_mp(void)
{
    return is_mp_kernel();
}

void
report_mp(FILE *out)
{
    u_long		mpswp;
    const char		*np;
    static const char	PresStr1[] = "pres";
    static const char	PresStr2[] = "presence";	/* wyse */
    static size_t	buflen = 0;
    static char		*buf = NULL;
    char		*tp;
    int			len;
    const char		*xp;
    int			n;
    struct
    {
	u_long		mp_pres;
	char		*mp_id;
    } mpsw_stub;

    report_when(out, "MP");

    /*
     * If someone has asked for the same list again, start from
     * an empty state.
     */

    if (pcmp_list)
    {
	free(pcmp_list);
	pcmp_list = NULL;
	pcmp_cnt = 0;
    }

    /*
     * Generic system data
     */

    show_num_cpu(out);

    /*
     * Determine the GPI
     *
     * On a uni kernel:
     *	mpx_kernel == 0
     *	mpswp does not exist
     *
     * On an mp kernel:
     *	mpx_kernel != 0
     *	mpswp does exist
     *	    mpswp will == &mpswnull if no GPI is installed
     */

    if (read_kvar_d("mpswp", &mpswp))
    {
	fprintf(out, "    This is a non-MP kernel\n");
	return;
    }

    debug_print("mpswp: 0x%.8lx\n", mpswp);

    if (read_kmem(mpswp, &mpsw_stub, sizeof(mpsw_stub)) != sizeof(mpsw_stub))
    {
	error_print("cannot get kernel values");
	return;
    }

    debug_print("mp_pres:  0x%.8lx\n", mpsw_stub.mp_pres);

    if (mpsw_stub.mp_pres == 0)
    {
	fprintf(out, "    This is an MP kernel running on a non-MP system\n");
	return;
    }

    if ((np = get_ksym((void *)mpsw_stub.mp_pres)) == NULL)
    {
	fprintf(out, "    Unknown MP system type\n");
	return;
    }

    debug_print("mp_pres is: %s\n", np);

    /*
     * Trim leading "pres" or "presence" and leading "_".
     * We probably do not have this problem but it is easy to do
     * and shortens the string before doing the copy.
     */

    if (strncmp(np, PresStr1, sizeof(PresStr1)-1) == 0)
	np += sizeof(PresStr1)-1;

    if (strncmp(np, PresStr2, sizeof(PresStr2)-1) == 0)
	np += sizeof(PresStr2)-1;

    while (*np == '_')
	++np;

    /*
     * Copy to our buffer so that we can edit it.  np is const
     * since we should not trash the table for next time.
     */

    len = strlen(np);
    if (!buflen || !buf || (len > buflen))
    {
	if (buf)
	    free(buf);

	buflen = len + 256;
	if (buflen < 1024)
	    buflen = 1024;

	buf = Fmalloc(buflen);
    }

    strcpy(buf, np);

    /*
     * Trim the trailing "pres" or "presence" and "_"
     */

    if (((tp = strstr(buf, PresStr1)) != NULL) && !tp[sizeof(PresStr1)-1])
	*tp = '\0';

    if (((tp = strstr(buf, PresStr2)) != NULL) && !tp[sizeof(PresStr2)-1])
	*tp = '\0';

    while (((len = strlen(buf)) > 0) && (buf[len-1] == '_'))
	buf[len-1] = '\0';

    if (!*buf)
    {
	error_print("cannot determine GPI type");
	free(buf);
	return;
    }

    /*
     * Generic GPI data
     */

    fprintf(out, "    Your system is using the `%s' GPI\n", buf);

    gpi_mpvendor(out);
    gpi_mpvendorclass(out);

    if (((xp = read_k_string((u_long)mpsw_stub.mp_id)) != NULL) &&
	(*xp != '\0'))
	    fprintf(out, "        MP Version:   %s\n", xp);

    /*
     * GPI specific data
     */

    for (n = 0; n < sizeof(gpi_info)/sizeof(*gpi_info); ++n)
	if ((strcmp(buf, gpi_info[n].name) == 0) && gpi_info[n].gpi_info)
	    gpi_info[n].gpi_info(out);

    free(buf);

    /*
     * Hardware detection of features
     */

    for (n = 0; n < sizeof(gpi_info)/sizeof(*gpi_info); ++n)
	if (gpi_info[n].hdwr_info)
	    gpi_info[n].hdwr_info(out);
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				Generic GPI data			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

static void
gpi_mpvendor(FILE *out)
{
    typedef struct
    {
	int		vend;
	const char	*name;
    } mp_vendor_t;
    static const mp_vendor_t	mp_vendor[] =
    {
	{ CI_AT_BRIDGE,		"Corollary 2-board set" },
	{ MP_Z1000,		"Corollary 3-board set - Z1000" },
	{ MP_MITAC,		"Corollary 2-board set - Mitac" },
	{ MP_COMPAQ,		"Compaq Systempro" },
	{ MP_APRICOT,		"Apricot Voyager" },
	{ MP_NX,		"Reserved NX" },
	{ MP_MPAX,		"Chips & Technologies M/Pax" },
	{ MED_AT_BRIDGE,	"Corollary 2-board set - Medidata" },
	{ MP_CBUS_PLUS,		"Corollary C-bus II" },
	{ MP_RES1,		"Reserved 1" },
	{ MP_RES2,		"Reserved 2" },
	{ MP_TPF,		"Tricord PowerFrame" },
	{ MP_RES3,		"Reserved 3" },
	{ MP_PCE,		"SNI PCE" },
	{ MP_AST,		"AST Manhattan SMP" },
	{ MP_IMPS,		"Intel Multiprocessor Specification" },
    };
    int		mpvendor;
    int		n;

    if (read_kvar_d("mpvendor", (u_long *)&mpvendor))
	return;

    fprintf(out, "        MP Vendor:   ");

    for (n = 0; n < sizeof(mp_vendor)/sizeof(*mp_vendor); ++n)
	if (mp_vendor[n].vend == mpvendor)
	    break;

    if (verbose || (n >= sizeof(mp_vendor)/sizeof(*mp_vendor)))
	fprintf(out, " 0x%.4x", mpvendor);

    if (n < sizeof(mp_vendor)/sizeof(*mp_vendor))
	fprintf(out, " %s", mp_vendor[n].name);

    fprintf(out, "\n");
}

static void
gpi_mpvendorclass(FILE *out)
{
    typedef struct
    {
	int		vend;
	const char	*name;
    } mp_vendorclass_t;
    static const mp_vendorclass_t	mp_vendorclass[] =
    {
	{ MP_CRLLRYISA,		"Corollary ISA architecture" },
	{ MP_CRLLRYEISA,	"Corollary EISA architecture" },
	{ MP_CPQCLASS,		"Compaq Systempro or clone" },
	{ MP_APRCLASS,		"Apricot Voyager or clone" },
	{ MP_NXCLASS,		"Reserved NX" },
	{ MP_MPAXCLASS,		"M/Pax or clone" },
	{ MP_CBPEISA,		"Corollary C-bus II EISA arch." },
	{ MP_RES1CLASS,		"Reserved 1" },
	{ MP_RES2CLASS,		"Reserved 2" },
	{ MP_TPFCLASS,		"Tricord PowerFrame" },
	{ MP_RES3CLASS,		"Reserved 3" },
	{ MP_SNICLASS,		"SNI PCE" },
	{ MP_ASTCLASS,		"AST Manhattan SMP" },
	{ MP_IMPSCLASS,		"Intel Multiprocessor Specification" },
	{ MP_CBPPCI,		"Corollary C-bus II PCI arch." },
    };
    int		mpvendorclass;
    int		n;

    if (read_kvar_d("mpvendorclass", (u_long *)&mpvendorclass))
	return;

    fprintf(out, "        Vendor class:");

    for (n = 0; n < sizeof(mp_vendorclass)/sizeof(*mp_vendorclass); ++n)
	if (mp_vendorclass[n].vend == mpvendorclass)
	    break;

    if (verbose || (n >= sizeof(mp_vendorclass)/sizeof(*mp_vendorclass)))
	fprintf(out, " 0x%.4x", mpvendorclass);

    if (n < sizeof(mp_vendorclass)/sizeof(*mp_vendorclass))
	fprintf(out, " %s", mp_vendorclass[n].name);

    fprintf(out, "\n");
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				a15k GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The a15k driver has been detected.  Display information
 * specific to this driver.
 */

static void
a15k_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				attack GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The attack driver has been detected.  Display information
 * specific to this driver.
 */

static void
attack_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				cbp GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The cbp driver has been detected.  Display information
 * specific to this driver.
 */

static void
cbp_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				ncrmp GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The ncrmp driver has been detected.  Display information
 * specific to this driver.
 */

static void
ncrmp_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				pce GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The pce driver has been detected.  Display information
 * specific to this driver.
 */

static void
pce_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				pcmp GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The following was stolen from pcmp.h
 */

typedef struct floating_ptr
{
    unsigned	signature;	/* signature (the string "_MP_") */
    paddr_t	config_addr;	/* physical address of Config Table */
    u_char	length;		/* structure length in para's (01h) */
    u_char	spec_rev;	/* PC+MP version number */
    u_char	checksum;	/* checksum causing all bytes to sum zero */
    u_char	feature1;	/* MP feature byte 1 */
    u_char	feature2;	/* MP feature byte 2 */
    u_char	feature3;	/* MP feature byte 3 */
    u_char	feature4;	/* MP feature byte 4 */
    u_char	feature5;	/* MP feature byte 5 */
} floating_t;

#define PCMP_EBDA_PTR           0x40e		/* 40:0e */
#define PCMP_SEARCH_LEN         1024
#define PCMP_BIOS_ROM           0xF0000         /* BIOS ROM area */
#define PCMP_BIOS_ROM_SIZE      0x10000         /* BIOS ROM size */

#define FLOAT_SIGNATURE  0x5F504D5F		/* "_MP_" */

/*
 * Our local routines
 */

static int	valid_floating_structure(floating_t *fp);
static u_char	calc_checksum(const void *addr, size_t nbytes);
static void	displayFloating(FILE *out, u_long floating_addr,
							const char *event);

/*
 * This routine is called prior to detecting the GPI driver.  We
 * will search for hardware belonging to the pcmp GPI and
 * display the information without the assumption that the pcmp
 * driver is linked in to the kernel.
 *
 * The 1.4 version of the Intel MultiProcessor Specification
 * specifies the following areas as proper places to put the
 * floating structure, and specified that the OS will examine
 * these areas in the following order:
 *
 *	1. In the first 1K of the Extended BIOS Data Area (EBDA),
 *
 *	2. Within the last 1K of system base memory if the
 *	   EBDA is undefined, or
 *
 *	3. In the BIOS ROM address space between 0xF0000 and 0xFFFFF
 *
 * This routine attempts to find all such.
 */

static void
pcmp_hdwr_info(FILE *out)
{
    u_short		aShort;
    u_long		offset;
    u_char		scan_buf[PCMP_SEARCH_LEN];
    u_char		boot_buf[BOOTINFO_LOC+512];
    int			n;
    int			swplob = 0;
    int			nswapb = 0;
    u_long		chunk;
    static const size_t	scan_buf_sz = sizeof(scan_buf);
    const char		hdwrFound[] = "hardware found";

    /*
     * First, find the EBDA, and look there.
     */

    if (read_mem(PCMP_EBDA_PTR, &aShort, sizeof(aShort)) == sizeof(aShort))
    {
	u_long		ebdap;

	ebdap = (u_long)aShort << 4;	/* seg:off to linear physical	*/

	if (read_mem(ebdap, scan_buf, scan_buf_sz) == scan_buf_sz)
	    for (offset = 0; offset < scan_buf_sz; offset += 16)
		if (valid_floating_structure((floating_t *)&scan_buf[offset]))
		    displayFloating(out, ebdap + offset, hdwrFound);
    }

    /*
     * Try the last 1K of base memory.
     */

    n = read_mem(swplob, boot_buf, sizeof(boot_buf));
    if ((n == sizeof(boot_buf)) && (!nswapb || (nswapb >= sizeof(boot_buf))))
    {
	struct bootinfo	*bp;

	bp = (struct bootinfo *)&boot_buf[BOOTINFO_LOC];
	if ((bp->magic == (u_long)BOOTINFOMAGIC) &&
	    (read_mem(bp->basemem, scan_buf, scan_buf_sz) == scan_buf_sz))
	{
	    for (offset = 0; offset < scan_buf_sz; offset += 16)
		if (valid_floating_structure((floating_t *)&scan_buf[offset]))
		    displayFloating(out, bp->basemem + offset, hdwrFound);
	}
    }

    /*
     * Ok, it wasn't in the top 1K of base memory, either. Look
     * in the BIOS ROM area. This area is too big to map all at
     * once (things aren't quite yet fully set up), so map in a
     * page at a time
     */

    for (chunk = 0; chunk < PCMP_BIOS_ROM_SIZE; chunk += scan_buf_sz)
    {
	if (read_mem(PCMP_BIOS_ROM+chunk, scan_buf, scan_buf_sz) != scan_buf_sz)
	    continue;

	for (offset = 0 ; offset < scan_buf_sz; offset += 16)
	    if (valid_floating_structure((floating_t *)&scan_buf[offset]))
		displayFloating(out, PCMP_BIOS_ROM + chunk + offset, hdwrFound);
    }
}

/*
 * Check whether the virtual address passed points to a valid Floating
 * pointer structure.
 */

static int
valid_floating_structure(floating_t *fp)
{
    if (fp->signature != FLOAT_SIGNATURE)
	return 0;		/* bad sig */

    /*
     * Verify the checksum.
     */

    if (calc_checksum(fp, sizeof(floating_t)) != 0)
	return 0;

    return 1;
}

/*
 * Routine to calculate the checksum byte required to make a
 * region checksum to zero.  A zero value indicates that a
 * region checksumed okay, a non-zero value indicates the
 * checksum byte required to make the region checksum to zero.
 */

static u_char
calc_checksum(const void *addr, register size_t nbytes)
{
    u_char		sum;
    register u_char	*ptr = (u_char *)addr;

    sum = 0;
    while (nbytes-- > 0)
	sum += *ptr++;

    return -sum;
}

static void
displayFloating(FILE *out, u_long floating_addr, const char *event)
{
    floating_t		fp;
    int			n;
    u_long		*up;

    /*
     * The listing of this structure is quite long.  Keep a list
     * of those that we have seen and do not do the same ones again.
     */

    if (pcmp_list)
	for (n = 0; n < pcmp_cnt; ++n)
	    if (pcmp_list[n] == floating_addr)
		return;

    up = Fmalloc(sizeof(u_long)*(pcmp_cnt+1));

    fprintf(out,
	    "    Intel MPS %s:\n"
	    "\n"
	    "\tFloating Structure", event);
    if (verbose)
	fprintf(out, ": 0x%.8lx", floating_addr);
    fprintf(out, "\n");

    if (pcmp_list)
    {
	for (n = 0; n < pcmp_cnt; ++n)
	    up[n] = pcmp_list[n];

	free(pcmp_list);
    }

    pcmp_list = up;
    pcmp_list[pcmp_cnt++] = floating_addr;

    /*
     * Dump the structure.
     */

    if (read_mem(floating_addr, &fp, sizeof(fp)) != sizeof(fp))
	return;

    fprintf(out, "\t    signature:      ");
    if (fp.signature == FLOAT_SIGNATURE)
	fprintf(out, "\"%.*s\"\n", sizeof(fp.signature), (char *)&fp.signature);
    else
	fprintf(out, "0x%.8x\n", fp.signature);

    fprintf(out, "\t    config_addr:    0x%.8x\n", fp.config_addr);
    fprintf(out, "\t    length:         %d\n", (u_int)fp.length);

    fprintf(out, "\t    spec_rev:       %d     %ssupported\n",
				(u_int)fp.spec_rev,
				(fp.spec_rev >= 1) ? "" : "NOT ");

    fprintf(out, "\t    checksum:       0x%.2x\n", (u_int)fp.checksum);
    fprintf(out, "\t    feature1:       0x%.2x  ", (u_int)fp.feature1);
    if (fp.feature1)
	fprintf(out, "Use default config #%d\n", fp.feature1);
    else
	fprintf(out, "Configuration structure present\n");

    fprintf(out, "\t    feature2:       0x%.2x  ", (u_int)fp.feature2);

    if (fp.feature2 & FEAT_IMCR)
	fprintf(out, "PIC");
    else
	fprintf(out, "Virtual Wire");
    fprintf(out, " Mode\n\t\t\t\t  ");

    if (fp.feature2 & FEAT_MCLK)
	fprintf(out, "Multiple Processor Clock Sources");
    else
	fprintf(out, "Shared Processor Clock Source");
    fprintf(out, "\n");

    fprintf(out, "\t    feature3:       0x%.2x\n", (u_int)fp.feature3);
    fprintf(out, "\t    feature4:       0x%.2x\n", (u_int)fp.feature4);
    fprintf(out, "\t    feature5:       0x%.2x\n", (u_int)fp.feature5);

    if (fp.config_addr)
	pcmp_cfgtable(out, fp.config_addr);

    fprintf(out, "\n");
}

static void
pcmp_cfgtable(FILE *out, u_long addr)
{
    char	cfgtable[4096];
    header_t	*hp;
    char	*curptr;
    u_short	i;
    u_short	j;

    if (read_mem(addr, cfgtable, sizeof(cfgtable)) != sizeof(cfgtable))
	return;

    pcmp_header_dump(out, hp = (header_t *)cfgtable, addr);

    /*
     * find all the bus entries, and save them away
     */

    curptr = &cfgtable[sizeof(header_t)];
    for (i = 0; i < hp->entry_count; i++)
	for (j = 0; pcmp_entry_table[j].entry_type != -1; j++)
	    if (*curptr == pcmp_entry_table[j].entry_type)
	    {
		if (pcmp_entry_table[j].entry_type == PCMP_BUS_TYPE) 
		    pcmp_add_bus_entry((pcmp_bus_entry_t *)curptr);

		curptr += pcmp_entry_table[j].structsz;
		break;
	    }

    curptr = &cfgtable[sizeof(header_t)];
    addr += sizeof(header_t);
    for (i = 0; i < hp->entry_count; i++)
	for (j = 0; pcmp_entry_table[j].entry_type != -1; j++)
	    if (*curptr == pcmp_entry_table[j].entry_type)
	    {
		(*pcmp_entry_table[j].func)(out, (void *)curptr, addr);
		curptr += pcmp_entry_table[j].structsz;
		addr += pcmp_entry_table[j].structsz;
		break;
	    }
}

static void
pcmp_header_dump(FILE *out, header_t *hp, u_long addr)
{
    int	i;
    
    fprintf(out, "\n\tHeader Structure");
    if (verbose)
	fprintf(out, ":   0x%.8lx", addr);
    fprintf(out, "\n");

    fprintf(out, "\t    signature:      ");
    if (verbose)
	fprintf(out, "0x%x  ", hp->signature);
    fprintf(out, "\"%.4s\"\n", &hp->signature);

    fprintf(out, "\t    length:         0x%x\n", hp->length);
    fprintf(out, "\t    spec_rev:       0x%x\n", hp->spec_rev);
    fprintf(out, "\t    checksum:       0x%x\n", hp->checksum);
    
    fprintf(out, "\t    oemid:          \"");
    for (i = 0; i < 8; i++)
	    fprintf(out, "%c", hp->oemid[i]);
    fprintf(out, "\"\n");

    fprintf(out, "\t    productid       \"");
    for (i = 0; i < 12; i++)
	    fprintf(out, "%c", hp->productid[i]);
    fprintf(out, "\"\n");

    fprintf(out, "\t    oem_addr:       0x%x\n", hp->oem_addr);
    fprintf(out, "\t    oem_size:       0x%x\n", hp->oem_size);
    fprintf(out, "\t    entry_count:    0x%x\n", hp->entry_count);
    fprintf(out, "\t    local_addr:     0x%x\n", hp->local_addr);
}

static void
pcmp_proc_dump(FILE *out, void *ep, u_long addr)
{
    pcmp_proc_entry_t	*pp = (pcmp_proc_entry_t *)ep;
    
    fprintf(out, "\n\tProcessor Entry");
    if (verbose)
	fprintf(out, ":    0x%.8lx", addr);
    fprintf(out, "\n");


    fprintf(out, "\t    local_id:       0x%x\n",  pp->local_id);
    fprintf(out, "\t    local_version:  0x%x\n",  pp->local_version);
    fprintf(out, "\t    usable:         %d\n",    pp->bits1 & 0x01);
    fprintf(out, "\t    bootstrap:      %d\n",	  (pp->bits1 >> 1) & 0x01);
    fprintf(out, "\t    family:         %d\n",    (pp->fms >> 8) & 0x0f);
    fprintf(out, "\t    model:          %d\n",    (pp->fms >> 4) & 0x0f);
    fprintf(out, "\t    stepping:       %d\n",    pp->fms & 0x0f);
    fprintf(out, "\t    features:       0x%.4x\n",  pp->features);
}

static void
pcmp_bus_dump(FILE *out, void *ep, u_long addr)
{
    pcmp_bus_entry_t	*bp = (pcmp_bus_entry_t *)ep;
    int			i;
    
    fprintf(out, "\n\tBus Entry");
    if (verbose)
	fprintf(out, ":          0x%.8lx", addr);
    fprintf(out, "\n");

    fprintf(out, "\t    bus_id:         0x%x\n", bp->bus_id);

    fprintf(out, "\t    bus:            \"");
    for (i = 0; i < 6; i++)
	fprintf(out, "%c", bp->bus[i]);
    fprintf(out, "\"\n");
}

static void
pcmp_ioapic_dump(FILE *out, void *ep, u_long addr)
{
    pcmp_ioapic_entry_t	*io = (pcmp_ioapic_entry_t *)ep;

    fprintf(out, "\n\tI/O Apic Entry");
    if (verbose)
	fprintf(out, ":     0x%.8lx", addr);
    fprintf(out, "\n");

    fprintf(out, "\t    io_id:          0x%x\n",	io->io_id);
    fprintf(out, "\t    io_version:     0x%x\n",	io->io_version);
    fprintf(out, "\t    usable:         %d\n",		io->enable & 0x01);
    fprintf(out, "\t    io_addr:        0x%x\n",	io->io_addr);
}

static void
pcmp_intr_dump(FILE *out, void *ep, u_long addr)
{
    static const char	*polarity[] =
    {
	"Bus Default",
	"Active High",
	"Reserved",
	"Active Low"
    };
    static const char	*trigger[] =
    {
	"Bus Default",
	"Edge",
	"Reserved",
	"Level"
    };
    static const char	*intr_type[] =
    {
	"APIC Vectored",
	"NMI",
	"SMI",
	"External Vectored"
    };
    pcmp_intr_entry_t	*in = (pcmp_intr_entry_t *)ep;
    int			i;

    fprintf(out, "\n\t");
    if (in->type == 4)
	fprintf(out, "Local Interrupt");
    else
	fprintf(out, "Interrupt Entry");
    if (verbose)
	fprintf(out, ":    0x%.8lx", addr);
    fprintf(out, "\n");
    
    fprintf(out, "\t    intr_type:      0x%.2x  %s\n",
					    in->intr_type,
					    intr_type[in->intr_type]);
    fprintf(out, "\t    polarity:       0x%.2x  %s\n",
					    in->bits & 0x03,
					    polarity[in->bits & 0x03]);
    fprintf(out, "\t    trigger:        0x%.2x  %s\n",
					    (in->bits >> 2) & 0x03,
					    trigger[(in->bits >> 2) & 0x03]);
    fprintf(out, "\t    source_bus:     0x%.2x  ", in->source_bus);

    for (i = 0; (i < NBUS) && pcmp_bus_type[i].bus_name[0]; i++)
	if (in->source_bus == pcmp_bus_type[i].bus_id)
	{
	    fprintf(out, "%s\n", pcmp_bus_type[i].bus_name);
	    fprintf(out, "\t    source_irq:     0x%.2x", in->source_irq);

	    if (strncmp(pcmp_bus_type[i].bus_name, "PCI", 3) == 0)
	    {
		/*
		 * PCI intr signal is two lower bits
		 * PCI device number is bits 2 thru 6
		 */

		int	devnum = (in->source_irq >> 2) & 0x1f;

		fprintf(out, "  PCI INT_%c#\n", 'A' + (in->source_irq & 0x3));
		pcmp_pci_info(out, devnum);
	    }
	    else
	    {
		/*
		 * not PCI
		 */
		fprintf(out, "\n");
	    }

	    break;
	}
    
    fprintf(out, "\t    apic_id:        0x%.2x\n", in->apic_id);
    fprintf(out, "\t    apic_pin:       0x%.2x\n", in->apic_pin);
}

static void
pcmp_pci_info(FILE *out, int devnum)
{
#ifdef NOT_YET	/* ## */
    static int	init = 0;

    if (!init)
    {
	load_pci_data();
	more
	init = 1;
    }
#endif

    fprintf(out, "\t\t\t\t  PCI DeviceNum %d\n", devnum);

    /* ## find the device in PCI space and print VendorId, DeviceId, IRQ */
}

static void
pcmp_add_bus_entry(pcmp_bus_entry_t *ep)
{
    int	i;
    
    for (i = 0; i < NBUS; i++)
	if (pcmp_bus_type[i].bus_name[0] == '\0')
	{
	    /*
	     * found an empty entry
	     */

	    pcmp_bus_type[i].bus_id = ep->bus_id;
	    strncpy(pcmp_bus_type[i].bus_name, ep->bus, 6);
	    break;
	}
}

static void
pcmp_gpi_info(FILE *out)
{
    u_long	pcmp_floating_addr;

    if (!read_kvar_d("pcmp_floating_addr", &pcmp_floating_addr))
    {
	fprintf(out, "\n");
	displayFloating(out, pcmp_floating_addr, "driver data");
    }
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				systempro GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The systempro driver has been detected.  Display information
 * specific to this driver.
 */

static void
systempro_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				tpf GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The tpf driver has been detected.  Display information
 * specific to this driver.
 */

static void
tpf_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				olimpx GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The olimpx driver has been detected.  Display information
 * specific to this driver.
 */

static void
olimpx_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				olimpxp5 GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The olimpxp5 driver has been detected.  Display information
 * specific to this driver.
 */

static void
olimpxp5_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				sysproxl GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The sysproxl driver has been detected.  Display information
 * specific to this driver.
 */

static void
sysproxl_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *									 *
 *				wyse GPI Specific			 *
 *									 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

/*
 * The wyse driver has been detected.  Display information
 * specific to this driver.
 */

static void
wyse_gpi_info(FILE *out)
{
    /* ## Not Yet */
}

