/*
 * File hw_isa.c
 * Information handler for isa
 *
 * @(#) hw_isa.c 67.2 97/10/27 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/pnp.h>
#include <malloc.h>

#include "hw_util.h"
#include "hw_eisa.h"
#include "hw_isa.h"
#include "eisa_data.h"

#define MAX_DISPL_RAW_LARGE_RES		64

#define IRQ_CNT		2
#define DMA_CNT		2
#define IO_CNT		8
#define MEM_CNT		4
#define MEM32_CNT	4

const char	* const callme_isa[] =
{
    "isa",
    "isa_bus",
    "isa_buss",
    "pnp",
    "isa_pnp",
    NULL
};

const char		short_help_isa[] = "Info on ISA Plug-and-Play devices";
static int		pnpFd = -1;
static PnP_busdata_t	pnpBus;
static int		endFound = 0;
static int		functionNumber = -1;
static int		deviceNumber = -1;

static int	open_PnP(void);
static const char *devTab(void);
static void	ResCommon(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq);
static void	CurrentSettings(FILE *out, const PnP_TagReq_t *tagReq);
static void	SmallRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq);
static void	LargeRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq);
static const char *SmallResName(const u_char *tag);
static const char *LargeResName(const u_char *tag);
static void	PnPvendorID(FILE *out, const char *tab, const u_char *id);


int
have_isa(void)
{
    static int	gotBus = 0;

    if (!gotBus)
    {
	memset(&pnpBus, 0, sizeof(pnpBus));
	pnpBus.BusType = pnp_bus_none;
	gotBus = 1;

	if (open_PnP())
	    return 0;

	if (ioctl(pnpFd, PNP_BUS_PRESENT, &pnpBus) == -1)
	{
	    debug_print("PNP_BUS_PRESENT fail: %s", strerror(errno));
	    return 0;
	}
    }

    return pnpBus.BusType != pnp_bus_none;
}

void
report_isa(FILE *out)
{
    u_char		*tag;
    u_long		maxTagLen;
    PnP_TagReq_t	tagReq;
    u_int		node;

    report_when(out, "ISA Plug-and-Play");

    if (!have_isa())
    {
	fprintf(out, "    The ISA Plug-and-Play driver was not found\n");
	return;
    }

    load_eisa_data();	/* EISA and PnP share vendor identification */

    fprintf(out, "    PnP Type:    ");
    switch (pnpBus.BusType)
    {
	case pnp_bus_bios:
	    fprintf(out, "BIOS\n");
	    break;

	case pnp_bus_os:
	    fprintf(out, "OS\n");
	    break;

	case pnp_bus_none:
	default:
	    fprintf(out, "None\n");
    }

    fprintf(out, "    Nodes:       %lu\n", pnpBus.NumNodes);
    fprintf(out, "    NodeSize:    %lu\n", pnpBus.NodeSize);

    if (pnpBus.NodeSize < 5)
	maxTagLen = USHRT_MAX + 3;
    else
	maxTagLen = pnpBus.NodeSize + 5;

    if (!(tag = malloc(maxTagLen)))
    {
	error_print("Out of memory");
	return;
    }

    if (verbose)
    {
	u_long	addr;
	union
	{
	    pnp_bios_t	bios;
	    u_char	buf[PNPSIGLEN];
	} ident;


	if (!read_kvar_d("PnP_BIOS_entry", &addr))
	{
	    fprintf(out, "    BIOS_entry:  ");
	    if (addr)
		fprintf(out, "0x%.4x\n", addr);
	    else
		fprintf(out, "Not used\n");
	}

	for (addr = 0x0f0000UL; addr < 0x0fffffUL; addr += 16)
	{
	    u_char		sum;
	    const u_char	*tp;

	    if (read_mem(addr, ident.buf, PNPSIGLEN) != PNPSIGLEN)
	    {
		fprintf(out, "read_mem: %s\n", strerror(errno));
		continue;
	    }

	    if ((ident.bios.sig.dword != PNPSIG) ||
		(ident.bios.len < PNPSIGLEN))
		    continue;

	    sum = 0;
	    for (tp = ident.buf; tp < &ident.buf[PNPSIGLEN]; ++tp)
		sum += *tp;

	    if (sum == 0)
		break;
	}

	if (addr < 0x0fffffU)
	{
	    fprintf(out, "    Inst Check:  0x%.4x\n", addr);
	    fprintf(out, "\tSignature:        %4.4s\n", ident.bios.sig.byte);
	    fprintf(out, "\tRev:              %u.%u\n",
						(u_int)ident.bios.rev >> 4,
						(u_int)ident.bios.rev & 0x0f);
	    fprintf(out, "\tLength:           %u bytes\n",
						    (u_int)ident.bios.len);

	    fprintf(out, "\tControl:          0x%4.4hx    ", ident.bios.ctrl);
	    switch (ident.bios.ctrl & 0x03)
	    {
		case 0x00:
		    fprintf(out, "Event notification not supported");
		    break;

		case 0x01:
		    fprintf(out, "Poll event notification");
		    break;

		case 0x02:
		    fprintf(out, "Asynchronous event notification");
		    break;

		case 0x03:
		    fprintf(out, "Asynchronous and poll event notification");
		    break;
	    }
	    fprintf(out, "\n");

	    fprintf(out, "\tChecksum:         0x%2.2x\n",
						    (u_int)ident.bios.chksum);
	    fprintf(out, "\tEvent:            0x%.4x\n", ident.bios.event);
	    fprintf(out, "\tOEM:              0x%.4lx\n", ident.bios.oem);

	    fprintf(out, "\t16 bit Real mode\n");
	    fprintf(out, "\t    Text:         0x%4.4hx:%4.4hx\n",
						ident.bios.rm16seg,
						ident.bios.rm16offset);
	    fprintf(out, "\t    Data:         0x%4.4hx:0000\n",
						ident.bios.rm16dataseg);

	    /*
	     * The value of base would go into the GDT as base
	     * and the address would be the GDT index << 3.  We
	     * ignore these and give a representation of the
	     * information that is usable if not strictly
	     * correct.
	     */

	    fprintf(out, "\t16 bit Prot mode\n");
	    fprintf(out, "\t    Text:         0x%.4lx:%4.4hx\n",
						ident.bios.pm16codebase,
						ident.bios.pm16offset);
	    fprintf(out, "\t    Data:         0x%.4lx:0000\n",
						ident.bios.pm16database);
	}
    }

    for (node = 1; node <= (u_int)pnpBus.NumNodes; ++node)
    {
	PnP_findUnit_t	up;
	u_long		resNum;
	u_long		totalLength = 0;
	u_char		chkSum = 0;

	up.unit.vendor = PNP_ANY_VENDOR;
	up.unit.serial = PNP_ANY_SERIAL;
	up.unit.unit = PNP_ANY_UNIT;
	up.unit.node = node;

	if (ioctl(pnpFd, PNP_FIND_UNIT, &up) == -1)
	{
	    debug_print("PNP_FIND_UNIT fail: %s", strerror(errno));
	    break;
	}

	fprintf(out, "\n    Node:        %d\n\n", up.unit.node);

	PnPvendorID(out, "\t", (u_char *)&up.unit.vendor);

	fprintf(out, "        Serial:           ");
	if (up.unit.serial == 0xffffffffUL)
	    fprintf(out, "NOT Supported\n");
	else
	    fprintf(out, "0x%8.8x\n", up.unit.serial);

	fprintf(out, "\tUnit:             %d\n", up.unit.unit);
	fprintf(out, "\tNodeSize:         %d\n", up.NodeSize);
	fprintf(out, "\tResuorces:        %d\n", up.ResCnt);
	fprintf(out, "\tDevices:          %d\n", up.devCnt);

	/*
	 * Read and display all resources
	 */

	deviceNumber = -1;
	functionNumber = -1;
	for (endFound = resNum = 0; !endFound; ++resNum)
	{
	    memset(tag, 0, maxTagLen);

	    tagReq.unit = up.unit;
	    tagReq.tagLen = maxTagLen;
	    tagReq.tagPtr = tag;
	    tagReq.resNum = resNum;

	    if (ioctl(pnpFd, PNP_READ_TAG, &tagReq) == -1)
	    {
		debug_print("PNP_READ_TAG fail: %s", strerror(errno));
		break;
	    }

	    if (verbose)
	    {
		u_long	n;

		totalLength += tagReq.tagLen;

		for (n = 0; n < tagReq.tagLen; ++n)
		    chkSum += tag[n];
	    }

	    if (!(tag[0] & 0x80U))
		SmallRes(out, resNum, &tagReq);
	    else
		LargeRes(out, resNum, &tagReq);
	}

	if (verbose && totalLength)
	{
	    fprintf(out, "\n\tTotal length:         %lu\n", totalLength);
	    fprintf(out, "\tChecksum:             0x%2.2x  %s\n",
						chkSum,
						chkSum ? "ERROR" : "correct");
	}
    }

    free(tag);

    if (!node)
	fprintf(out, "    No ISA Plug-and-Play devices found\n");
}

static const char *
devTab()
{
    if (deviceNumber >= 0)
	return (functionNumber >= 0) ? "\t\t" : "\t    ";

    return "\t";
}

static void
ResCommon(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq)
{
    const u_char	*tag = tagReq->tagPtr;
    int			largeReq = tag[0] & 0x80U;

    fprintf(out, "\n%sResource: %3lu     %s\n",
			    devTab(),
			    resNum,
			    largeReq ? LargeResName(tag) : SmallResName(tag));

    if (verbose)
    {
	int	n;

	fprintf(out, "%sResource type:    %s\n",
					    devTab(),
					    largeReq ? "large" : "small");
	fprintf(out, "%sResource length:  %d\n", devTab(), tagReq->tagLen);

	/*
	 * Show the raw data
	 */

	if (largeReq)
	{
	    /*
	     * Since this is a large resource, there may be as
	     * much as 65538 bytes total.
	     */

	    u_long	len = tagReq->tagLen;
	    u_long	cnt;

	    if (len > MAX_DISPL_RAW_LARGE_RES)
	    {
		fprintf(out, "%sP/O raw data:    ", devTab());
		len = MAX_DISPL_RAW_LARGE_RES;
	    }
	    else
		fprintf(out, "%sRaw data:        ", devTab());

	    cnt = 0;
	    for (n = 0; n < len; ++n)
	    {
		if (cnt >= 8)
		{
		    fprintf(out, "\n%s                 ", devTab());
		    cnt = 0;
		}
		fprintf(out, " 0x%2.2x", tag[n]);
		++cnt;
	    }
	}
	else
	{
	    /*
	     * Since this is a small resource, there can only be
	     * a maximum of 8 bytes total.
	     */

	    fprintf(out, "%sRaw data:        ", devTab());
	    for (n = 0; n < tagReq->tagLen; ++n)
		fprintf(out, " 0x%2.2x", tag[n]);
	}

	fprintf(out, "\n");
    }
}

# define DEV_OFS	0x40

static void
CurrentSettings(FILE *out, const PnP_TagReq_t *tagReq)
{
    PnP_RegReq_t	regReq;
    u_char		regBuf[0x100U-DEV_OFS];
    int			n;
    u_short		value;
    u_long		addr;
    u_long		limit;
    u_char		control;
    int			found = 0;

    regReq.unit = tagReq->unit;
    regReq.devNum = deviceNumber;
    regReq.regNum = DEV_OFS;
    regReq.regCnt = sizeof(regBuf);
    regReq.regBuf = regBuf;

    if (ioctl(pnpFd, PNP_READ_REG, &regReq) == -1)
	return;

    fprintf(out, "\n%sCurrent settings\n", devTab());
    debug_dump(regBuf, sizeof(regBuf), DEV_OFS, "Registers");

    for (n = 0; n < MEM_CNT; ++n)	/* 24 bit Memory */
    {
	addr = ((u_long)regBuf[0x40-DEV_OFS+(n*0x08U)] << 16) |
	       ((u_long)regBuf[0x41-DEV_OFS+(n*0x08U)] << 8);

	if (addr)
	{
	    control = regBuf[0x42-DEV_OFS+(n*0x08U)];

	    limit = ((u_long)regBuf[0x43-DEV_OFS+(n*0x08U)] << 16) |
		    ((u_long)regBuf[0x44-DEV_OFS+(n*0x08U)] << 8);

	    if (control & 0x01)
		limit = addr + limit - 1;	/* Range -> Limit */

	    fprintf(out, "%s    MEM[%1d]:       0x%8.8lx - 0x%8.8lx   %d bit\n",
						    devTab(), n,
						    addr, limit,
						    control & 0x02U ? 16 : 8);
	    found = 1;
	}
    }

    for (n = 0; n < MEM32_CNT; ++n)	/* 32 bit Memory */
    {
	int		j = n ? 0x70 : 0x76;

	addr = ((u_long)regBuf[j+0-DEV_OFS+(n*0x10U)] << 24) |
	       ((u_long)regBuf[j+1-DEV_OFS+(n*0x10U)] << 16) |
	       ((u_long)regBuf[j+2-DEV_OFS+(n*0x10U)] << 8) |
		(u_long)regBuf[j+3-DEV_OFS+(n*0x10U)];

	if (addr)
	{
	    control = regBuf[j+4-DEV_OFS+(n*0x10U)];

	    limit = ((u_long)regBuf[j+5-DEV_OFS+(n*0x10U)] << 24) |
		    ((u_long)regBuf[j+6-DEV_OFS+(n*0x10U)] << 16) |
		    ((u_long)regBuf[j+7-DEV_OFS+(n*0x10U)] << 8) |
		     (u_long)regBuf[j+8-DEV_OFS+(n*0x10U)];

	    if (control & 0x01)
		limit = addr + limit - 1;	/* Range -> Limit */

	    control = (control >> 1) & 0x03;
	    fprintf(out, "%s    MEM32[%1d]:     0x%8.8lx - 0x%8.8lx   %s\n",
				    devTab(), n,
				    addr, limit,
				    control == 0 ? "8 bit" :
					(control == 1 ? "16 bit" :
					(control == 3 ? "32 bit" : "")));
	    found = 1;
	}
    }

    for (n = 0; n < IO_CNT; ++n)	/* I/O */
    {
	value = ((u_short)regBuf[0x60-DEV_OFS+(n*2)] << 8) |
		 (u_short)regBuf[0x61-DEV_OFS+(n*2)];

	/* Note: a range is not recorded for this */

	if (value)
	{
	    fprintf(out, "%s    I/O[%1d]:       0x%4.4hx\n",
						    devTab(),
						    n,
						    value);
	    found = 1;
	}
    }

    for (n = 0; n < IRQ_CNT; ++n)	/* IRQ */
    {
	value = regBuf[0x70-DEV_OFS+(n*2)];

	if (value)
	{
	    u_char	irqType = regBuf[0x71-DEV_OFS+(n*2)];

	    fprintf(out,
		    "%s    IRQ[%1d]:       %d      Active %s, %s trigger\n",
			    devTab(),
			    n,
			    value,
			    irqType & 0x02U ? "high" : "low",
			    irqType & 0x01U ? "level" : "edge");
	    found = 1;
	}
    }

    for (n = 0; n < DMA_CNT; ++n)	/* DMA */
    {
	value = regBuf[0x74-DEV_OFS+n] & 0x07U;

	if (value != 4)
	{
	    fprintf(out, "%s    DMA[%1d]:       %d\n", devTab(), n, value);
	    found = 1;
	}
    }

    if (!found)
	fprintf(out, "%s    No resources assigned\n", devTab());
}

static void
SmallRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq)
{
    const u_char	*tag = tagReq->tagPtr;
    int			more;
    u_short		value;
    int			n;
    PnP_Active_t	act;

    /*
     * Show the known specifics
     */

    switch ((tag[0] >> 3) & 0x0fU)
    {
	case 0x01:	/* Plug-and-Play version */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sVersion:          %u.%u\n",
							devTab(),
							tag[1] >> 4,
							tag[1] & 0x07U);
	    fprintf(out, "%sVendor specific:  0x%2.2x\n",
							devTab(),
							tag[2]);
	    break;

	case 0x02:	/* Logical device ID */
	    fprintf(out, "\n\tDevice number:        %lu\n", ++deviceNumber);
	    functionNumber = -1;

	    CurrentSettings(out, tagReq);
	    ResCommon(out, resNum, tagReq);

	    PnPvendorID(out, devTab(), &tag[1]);
	    fprintf(out, "%sSupports boot:    %s\n",
						devTab(),
						(tag[5] & 0x80U) ? Yes : No);
	    fprintf(out, "%sFlags 0x31-0x37:  0x%2.2x\n",
							devTab(),
							tag[5] & 0x7fU);
	    if (tagReq->tagLen >= 7)
		fprintf(out, "%sFlags 0x38-0x3f:  0x%2.2x\n", devTab(), tag[6]);

	    act.unit = tagReq->unit;
	    act.device = deviceNumber;
	    if (!ioctl(pnpFd, PNP_READ_ACTIVE, &act))
		fprintf(out, "%sActive:           %s\n",
					devTab(),
					act.active ? Yes : No);
	    break;

	case 0x03:	/* Compatible device ID */
	    ResCommon(out, resNum, tagReq);
	    PnPvendorID(out, devTab(), &tag[1]);
	    break;

	case 0x04:	/* IRQ format */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sIRQ:              ", devTab());
	    value = *(u_short *)&tag[1];
	    more = 0;
	    for (n = 0; n < 16; ++n)
	    {
		if (value & 0x01)
		{
		    if (more)
			fprintf(out, ", ");

		    fprintf(out, "%d", n);
		    more = 1;
		}

		value >>= 1;
	    }
	    if (!more)
		fprintf(out, "none");

	    fprintf(out, "\n%sTrigger:          ", devTab());
	    if (tagReq->tagLen >= 4)
	    {
		more = 0;

		if (tag[3] & 0x08U)
		{
		    fprintf(out, "Low true, Level sensitive\n");
		    more = 1;
		}
		if (tag[3] & 0x04U)
		{
		    if (more)
			fprintf(out, "%s                  ", devTab());
		    fprintf(out, "High true, Level sensitive\n");
		    more = 1;
		}
		if (tag[3] & 0x02U)
		{
		    if (more)
			fprintf(out, "%s                  ", devTab());
		    fprintf(out, "Low true, Edge sensitive\n");
		    more = 1;
		}
		if (tag[3] & 0x01U)
		{
		    if (more)
			fprintf(out, "%s                  ", devTab());
		    fprintf(out, "High true, Edge sensitive\n");
		}
	    }
	    else
		fprintf(out, "Default - High true, Edge sensitive\n");

	    break;

	case 0x05:	/* DMA format */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sDMA:              ", devTab());
	    value = tag[1];
	    more = 0;
	    for (n = 0; n < 8; ++n)
	    {
		if (value & 0x01)
		{
		    if (more)
			fprintf(out, ", ");

		    fprintf(out, "%d", n);
		    more = 1;
		}

		value >>= 1;
	    }
	    if (!more)
		fprintf(out, "none");

	    fprintf(out, "\n%sChannel speed:    ", devTab());
	    switch (tag[2] & 0x60U)
	    {
		case (0 << 5):
		    fprintf(out, "Compatibility mode");
		    break;

		case (1 << 5):
		    fprintf(out, "EISA DMA type A");
		    break;

		case (2 << 5):
		    fprintf(out, "Type B");
		    break;

		case (3 << 5):
		    fprintf(out, "Type F");
		    break;

	    }

	    fprintf(out, "\n%sWord mode:        %s\n",
						devTab(),
						(tag[2] & 0x10U) ? Yes : No);
	    fprintf(out, "%sByte mode:        %s\n",
						devTab(),
						(tag[2] & 0x08U) ? Yes : No);
	    fprintf(out, "%sBus master:       %s\n",
						devTab(),
						(tag[2] & 0x04U) ? Yes : No);
	    fprintf(out, "%sPreferred mode:   ", devTab());
	    switch (tag[2] & 0x03U)
	    {
		case 0:
		    fprintf(out, "8-bit only\n");
		    break;

		case 1:
		    fprintf(out, "8 and 16-bit\n");
		    break;

		case 2:
		    fprintf(out, "16-bit\n");
		    break;

		case 3:
		    fprintf(out, "reserved\n");
		    break;

	    }

	    break;

	case 0x06:	/* Start dependent Function */
	    fprintf(out, "\n\t    Function number:      %lu\n",
							    ++functionNumber);
	    ResCommon(out, resNum, tagReq);

	    fprintf(out, "%sPriority:         ", devTab());
	    if (tagReq->tagLen >= 2)
		switch (tag[1])
		{
		    case 0:
			fprintf(out, "Good\n");
			break;

		    case 1:
			fprintf(out, "Acceptable\n");
			break;

		    case 2:
			fprintf(out, "Sub-optimal\n");
			break;

		    default:
			fprintf(out, "undefined\n");
			break;

		}
	    else
		fprintf(out, "Default - acceptable\n");

	    break;

	case 0x07:	/* End dependent Function */
	    ResCommon(out, resNum, tagReq);
	    break;

	case 0x08:	/* I/O port descriptor */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%s16 bit address:   %s\n",
					    devTab(),
					    (tag[1] & 0x01U) ? Yes : No);

	    if (*(u_short *)&tag[2] == *(u_short *)&tag[4])
		fprintf(out, "%sAddress:          0x%.4hx\n",
						devTab(),
						*(u_short *)&tag[2]);
	    else
		fprintf(out, "%sAddress range:    0x%.4hx-0x%.4hx\n",
						devTab(),
						*(u_short *)&tag[2],
						*(u_short *)&tag[4]);

	    if ((*(u_short *)&tag[2] != *(u_short *)&tag[4]) ||
		(tag[6] != 0x01U))
		    fprintf(out, "%sBase alignment:   %u bytes\n",
						devTab(),
						tag[6]);

	    fprintf(out, "%sRange length:     %u bytes", devTab(), tag[7]);
	    if (*(u_short *)&tag[2] == *(u_short *)&tag[4])
		fprintf(out, "\t0x%.4hx-0x%.4hx",
					    *(u_short *)&tag[2],
					    *(u_short *)&tag[2] + tag[7] - 1);
	    fprintf(out, "\n");
	    break;

	case 0x09:	/* Fixed I/O port descriptor */
	    ResCommon(out, resNum, tagReq);
	    value = *(u_short *)&tag[1];
	    fprintf(out, "%sAddress:          0x%.4hx-0x%.4hx\n",
						devTab(),
						value,
						value + tag[3] - 1);
	    break;

	case 0x0e:	/* Vendor defined */
	    ResCommon(out, resNum, tagReq);
	    break;

	case 0x0f:	/* End tag */
	    ResCommon(out, resNum, tagReq);
	    if (verbose)
		fprintf(out, "%sChecksum:         0x%2.2x\n", devTab(), tag[1]);
	    endFound = 1;
	    break;
    }
}

static void
LargeRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq)
{
    const u_char	*tag = tagReq->tagPtr;
    u_short		value;
    int			n;

    /*
     * Show the known specifics
     */

    switch (tag[0] & 0x7fU)
    {
	case 0x01:	/* Memory range */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sExpansion ROM:    %s\n",
					    devTab(),
					    (tag[3] & 0x40) ? Yes : No);
	    fprintf(out, "%sShadowable:       %s\n",
					    devTab(),
					    (tag[3] & 0x20) ? Yes : No);
	    fprintf(out, "%sControl:          ", devTab());
	    switch (tag[3] & 0x18)
	    {
		case (0 << 3):
		    fprintf(out, "8-bit memory");
		    break;

		case (1 << 3):
		    fprintf(out, "16-bit memory");
		    break;

		case (2 << 3):
		    fprintf(out, "8 and 16-bit memory");
		    break;

		case (3 << 3):
		    fprintf(out, "reserved");
		    break;
	    }
	    fprintf(out, "\n");

	    fprintf(out, "%sSupport:          decode supports %s\n",
					    devTab(),
					    (tag[3] & 0x04)
						? "high address"
						: "range length");

	    fprintf(out, "%sCache:            %s\n",
					devTab(),
					(tag[3] & 0x02)
					    ? "read cacheable, write-through"
					    : "non-cacheable");

	    fprintf(out, "%sWrite:            %s\n",
					    devTab(),
					    (tag[3] & 0x01)
						? "writeable"
						: "non-writeable (ROM)");

	    fprintf(out, "%sMinimum base:     0x%.4hx00\n",
						devTab(),
						*(u_short *)&tag[4]);
	    fprintf(out, "%sMaximum base:     0x%.4hx00\n",
						devTab(),
						*(u_short *)&tag[6]);
	    fprintf(out, "%sBase allignment   0x%.6lx\n",
			    devTab(),
			    (u_long)(*(u_short *)&tag[8]) + (64 * 1024) - 1);
	    fprintf(out, "%sLength:           %hu\n",
						devTab(),
						*(u_short *)&tag[10]);
	    break;

	case 0x02:	/* ANSI ID string */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sANSI ID:          %*s\n",
						    devTab(),
						    tagReq->tagLen - 3,
						    &tag[3]);
	    break;

	case 0x03:	/* Unicode ID string */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sCountry:          0x%.4hx\n",
						devTab(),
						*(u_short *)&tag[3]);
	    /* The specification is incomplete */
	    break;

	case 0x04:	/* Vendor defined */
	    ResCommon(out, resNum, tagReq);
	    break;

	case 0x05:	/* 32-bit Memory range */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%sExpansion ROM:    %s\n",
					    devTab(),
					    (tag[3] & 0x40) ? Yes : No);
	    fprintf(out, "%sShadowable:       %s\n",
					    devTab(),
					    (tag[3] & 0x20) ? Yes : No);
	    fprintf(out, "%sControl:          ", devTab());
	    switch (tag[3] & 0x18)
	    {
		case (0 << 3):
		    fprintf(out, "8-bit memory");
		    break;

		case (1 << 3):
		    fprintf(out, "16-bit memory");
		    break;

		case (2 << 3):
		    fprintf(out, "8 and 16-bit memory");
		    break;

		case (3 << 3):
		    fprintf(out, "32-bit memory");
		    break;
	    }

	    fprintf(out, "%sSupport:          decode supports %s\n",
					    devTab(),
					    (tag[3] & 0x04)
						? "high address"
						: "range length");

	    fprintf(out, "%sCache:            %s\n",
					devTab(),
					(tag[3] & 0x02)
					    ? "read cacheable, write-through"
					    : "non-cacheable");

	    fprintf(out, "%sWrite:            %s\n",
					    devTab(),
					    (tag[3] & 0x01)
						? "writeable"
						: "non-writeable (ROM)");

	    fprintf(out, "%sMinimum base:     0x%.4hx00\n",
						devTab(),
						*(u_short *)&tag[4]);
	    fprintf(out, "%sMaximum base:     0x%.4hx00\n",
						devTab(),
						*(u_short *)&tag[6]);
	    fprintf(out, "%sBase allignment   0x%.6lx\n",
			    devTab(),
			    (u_long)(*(u_short *)&tag[8]) + (64 * 1024) - 1);
	    fprintf(out, "%sLength:           %hu\n",
						devTab(),
						*(u_short *)&tag[10]);
	    fprintf(out, "%s##\n", devTab());
	    /* ## */
	    break;

	case 0x06:	/* 32-bit Fixed Memory */
	    ResCommon(out, resNum, tagReq);
	    fprintf(out, "%s##\n", devTab());
	    /* ## */
	    break;
    }
}

static const char *
SmallResName(const u_char *tag)
{
    static char		errBuf[64];

    switch ((tag[0] >> 3) & 0x0fU)
    {
	case 0x01:
	    return "Plug-and-Play version";

	case 0x02:
	    return "Logical device ID";

	case 0x03:
	    return "Compatible device ID";

	case 0x04:
	    return "IRQ format";

	case 0x05:
	    return "DMA format";

	case 0x06:
	    return "Start dependent Function";

	case 0x07:
	    return "End dependent Function";

	case 0x08:
	    return "I/O port descriptor";

	case 0x09:
	    return "Fixed I/O port descriptor";

	case 0x0e:
	    return "Vendor defined";

	case 0x0f:
	    return "End tag";
    }

    sprintf(errBuf, "Reserved 0x%2.2x", (tag[0] >> 3) & 0x0fU);
    return errBuf;
}

static const char *
LargeResName(const u_char *tag)
{
    static char		errBuf[64];

    switch (tag[0] & 0x7fU)
    {
	case 0x01:
	    return "Memory range";

	case 0x02:
	    return "ANSI ID string";

	case 0x03:
	    return "Unicode ID string";

	case 0x04:
	    return "Vendor defined";

	case 0x05:
	    return "32-bit Memory range";

	case 0x06:
	    return "32-bit Fixed Memory";
    }

    sprintf(errBuf, "Reserved 0x%2.2x", tag[0] & 0x7fU);
    return errBuf;
}

static int
open_PnP()
{
    static const char	pnpDev[] = "/dev/pnp";

    if (pnpFd != -1)
	return 0;

    if ((pnpFd = open(pnpDev, O_RDONLY)) == -1)
    {
	debug_print("Cannot open %s: %s", pnpDev, strerror(errno));
	return errno;
    }

    return 0;
}

static void
PnPvendorID(FILE *out, const char *tab, const u_char *id)
{
    u_short	prod_id;
    const char	*key;
    const char	*vendor;
    const char	*product;

    if (verbose)
	fprintf(out,
	    "%sID:               0x%02x 0x%02x 0x%02x 0x%02x   0x%8.8lx\n",
							tab,
							id[0],
							id[1],
							id[2],
							id[3],
							*(u_long *)id);

    fprintf(out, "%sCfg file:         %s\n", tab, eisa_cfg_name(id));

    key = eisa_vendor_key(id);
    fprintf(out, "%sVendor:           %s\n",
			tab,
			(vendor = eisa_vendor_name(key)) ? vendor : key);

    fprintf(out, "%sProduct:          ", tab);
    prod_id = ((u_short)id[2] << 8) | id[3];
    if ((product = eisa_product_name(key, prod_id)) != NULL)
	fprintf(out, "%s\n", product);
    else
    {
	prod_id = ((u_short)id[2] << 4) |
		  ((u_short)(id[3] & 0xf0) >> 4);
	fprintf(out, "0x%03x\n", prod_id);
    }
}

