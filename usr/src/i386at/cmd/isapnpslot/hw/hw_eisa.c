/*
 * File hw_eisa.c
 * Information handler for eisa
 *
 * @(#) hw_eisa.c 65.2 97/06/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#include "hw_eisa.h"
#include "hw_util.h"
#include "eisa_data.h"

#define EISA_MB_ID	0xfffd9		/* f000:ffd9 "EISA" */
#define NO_EISA_ID	0xffffffff

#define	EISA_BASE	0x0c80		/* base address of EISA IDs */
#define	EISA_SLOTS	16		/* slots including MB */
#define EISA_NEXT	0x1000		/* Offset to next adapter */
#define	EISA_SLOT(x)	(EISA_BASE+(EISA_NEXT*slot))
#define MB_SLOT		0

const char	* const callme_eisa[] =
{
    "eisa",
    "eisa_bus",
    "eisa_buss",
    NULL
};

const char	short_help_eisa[] = "Info on EISA bus devices";

static u_long		eisa_mb_id = 0;
static const size_t	MbIdLen = sizeof(eisa_mb_id);

int
have_eisa(void)
{
    static const u_long		mb_sig = 0x41534945;	/* "EISA" */

    if (!eisa_mb_id)
    {
	/*
	 * eisa_mb_id = *(u_long *)ptok(EISA_MB_ID);
	 */

	if (read_mem(EISA_MB_ID, &eisa_mb_id, MbIdLen) != MbIdLen)
	{
	    debug_print("EISA signature read fail");
	    eisa_mb_id = NO_EISA_ID;
	}
	else
	    debug_print("EISA signature: 0x%8.8lx", eisa_mb_id);
    }

    return (eisa_mb_id == mb_sig) ? 1 : 0;
}

#ifndef GEMINI
void
report_eisa(FILE *out)
{
    int		slot;


    report_when(out, "EISA");

    if (!have_eisa())
    {
	fprintf(out, "    No EISA bus found!\n");
	return;
    }

    load_eisa_data();

    for (slot = 0; slot < EISA_SLOTS; slot++)
    {
	union
	{
	    u_long	value;
	    u_char	byte[sizeof(u_long)];
	} eisa_id;
	const char	*key;
	u_long		rev;
	const char	*vendor;
	const char	*product;
	const char	*cfg_name;
	u_short		prod_id;

	eisa_id.value = io_ind(EISA_SLOT(slot));
	if (eisa_id.value == 0xffffffff)
	    continue;	/* No device or not ready */

	if (slot == MB_SLOT)
	{
	    fprintf(out, "    Motherboard\n");

	    if (verbose)
	    {
		const u_char	*tp = (u_char *)&eisa_mb_id;
		int		n;

		for (n = 0; n < MbIdLen; ++n)
		    if (!isprint(tp[n]))
			break;

		fprintf(out, "        Signature: ");
		if (n < MbIdLen)
		    fprintf(out, "0x%8.8lx\n", eisa_mb_id);
		else
		    fprintf(out, "\"%c%c%c%c\"\n", tp[0], tp[1], tp[2], tp[3]);
	    }
	}
	else
	    fprintf(out, "    Slot: %d\n", slot);

	if (verbose)
	    fprintf(out, "        ID:        0x%02x 0x%02x 0x%02x 0x%02x\n",
							    eisa_id.byte[0],
							    eisa_id.byte[1],
							    eisa_id.byte[2],
							    eisa_id.byte[3]);

	cfg_name = eisa_cfg_name(eisa_id.byte);
	fprintf(out, "        Cfg file:  %s\n", cfg_name);

	key = eisa_vendor_key(eisa_id.byte);
	fprintf(out, "        Vendor:    %s\n",
			    (vendor = eisa_vendor_name(key)) ? vendor : key);

	fprintf(out, "        Product:   ");
	prod_id = ((u_short)eisa_id.byte[2] << 8) | eisa_id.byte[3];
	if ((product = eisa_product_name(key, prod_id)) != NULL)
	    fprintf(out, "%s\n", product);
	else if (slot == MB_SLOT)
	{
	    prod_id = eisa_id.byte[2];
	    fprintf(out, "0x%02x\n", prod_id);
	}
	else
	{
	    prod_id = ((u_short)eisa_id.byte[2] << 4) |
			((u_short)(eisa_id.byte[3] & 0xf0) >> 4);
	    fprintf(out, "0x%03x\n", prod_id);
	}

	if (slot == MB_SLOT)
	    rev = (u_long)(eisa_id.byte[3] & 0xf8) >> 3;
	else
	    rev = eisa_id.byte[3] & 0x0f;
	fprintf(out, "        Revision:  %d\n", rev);

	if (slot == MB_SLOT)
	    fprintf(out, "        EISA Ver:  %d\n", eisa_id.byte[3] & 0x7);

	parse_eisa_cfg(cfg_name);
    }
}
#endif

const char *
eisa_vendor_name(const char *vendor_key)
{
    eisa_vend_t	*vp;

    if (eisa_vendors)
	for (vp = eisa_vendors; vp->next; vp = vp->next)
	    if (strcmp(vendor_key, vp->key) == 0)
		return vp->name;

    return NULL;	/* Not known */
}

const char *
eisa_product_name(const char *vendor_key, u_short prod_id)
{
    eisa_vend_t	*vp;

    if (eisa_vendors)
	for (vp = eisa_vendors; vp->next; vp = vp->next)
	    if (strcmp(vendor_key, vp->key) == 0)
	    {
		eisa_prod_t	*pp;

		for (pp = vp->prod; pp; pp = pp->next)
		    if (pp->id == prod_id)
			return pp->name;

		break;
	    }

    return NULL;	/* Not known */
}

const char *
eisa_vendor_key(const u_char *eisa_id)
{
    static char		key[] = "ABC";

    key[0] = 0x40 | ((u_long)(eisa_id[0] & 0x7c) >> 2);
    key[1] = 0x40 | ((u_long)(eisa_id[0] & 0x03) << 3) |
		    ((u_long)(eisa_id[1] & 0xe0) >> 5);
    key[2] = 0x40 |  (eisa_id[1] & 0x1f);
    key[3] = '\0';

    return key;
}

const char *
eisa_cfg_name(const u_char *eisa_id)
{
    static char		name[] = "!ISA0000.cfg";

    sprintf(name, "!%s%02X%02X.CFG",
			    eisa_vendor_key(eisa_id), eisa_id[2], eisa_id[3]);
    return name;
}

void
parse_eisa_cfg(const char *cfg_name)
{
    /* ## not yet */
    /* Should find the file cfg_name and talk about what is in it */
}

