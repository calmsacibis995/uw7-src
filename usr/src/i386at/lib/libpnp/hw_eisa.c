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

#define EISA_MB_ID	0xfffd9		/* f000:ffd9 "EISA" */
#define NO_EISA_ID	0xffffffff

#define	EISA_BASE	0x0c80		/* base address of EISA IDs */
#define	EISA_SLOTS	16		/* slots including MB */
#define EISA_NEXT	0x1000		/* Offset to next adapter */
#define	EISA_SLOT(x)	(EISA_BASE+(EISA_NEXT*slot))
#define MB_SLOT		0

eisa_vend_t	*eisa_vendors = NULL;

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

