/*
 * File pci_data.c
 *
 * @(#) pci_data.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "hw_util.h"
#include "pci_data.h"

pci_vend_t	*pci_vendors = NULL;

int
load_pci_data()
{
    static const char	cfg_file[] = "pci.cfg";
    const char		*filename;
    FILE		*fd;
    char		buf[256];
    u_long		line;
    int			err = 0;

    if (!(filename = get_lib_file_name(cfg_file)))
	return errno;

    if (!(fd = fopen(filename, "r")))
	return 0;

    line = 0;
    while (fgets(buf, sizeof(buf), fd))
    {
	int	n;
	char	*tp;

	++line;
	n = strlen(buf);
	while ((n > 0) && isspace(buf[n-1]))
	    buf[--n] = '\0';

	for (tp = buf; isspace(*tp); ++tp)
	    ;

	if (!*tp || (*tp == '#'))
	    continue;			/* Ignore blank and comment lines */

	if (*tp == '+')
	{
	    pci_vend_t		*vp;
	    pci_prod_t		*pp;
	    pci_prod_t		**ppp;
	    u_long		id;
	    u_long		rev;
	    char		*name;

	    if (!pci_vendors)
	    {
		debug_print("%s: line %lu: Product without vendor",
							    filename, line);
		continue;
	    }

	    do
		++tp;
	    while (isspace(*tp));

	    if (!isxdigit(*tp))
	    {
		debug_print("%s: line %lu: HEX product ID expected",
							    filename, line);
		continue;
	    }

	    id = strtoul(tp, &tp, 16);
	    if ((id > USHRT_MAX) || !tp || !isspace(*tp))
	    {
		debug_print("%s: line %lu: Invalid product ID", filename, line);
		continue;
	    }

	    while (isspace(*tp))
		++tp;

	    rev = strtoul(tp, &tp, 16);
	    if ((rev > UCHAR_MAX) || !tp || !isspace(*tp))
	    {
		debug_print("%s: line %lu: Invalid product ID", filename, line);
		continue;
	    }

	    while (isspace(*tp))
		++tp;

	    name = tp;

	    /*
	     * Find a place to put this new product.
	     * We will attach it to the end of the list of
	     * products for the last vendor.
	     */

	    if (!(pp = (pci_prod_t *)malloc(sizeof(pci_prod_t))))
	    {
		err = ENOMEM;
		break;
	    }

	    if (!(pp->name = strdup(name)))
	    {
		free(pp);
		err = ENOMEM;
		break;
	    }

	    pp->id = (u_short)id;
	    pp->rev = (u_char)rev;
	    pp->next = NULL;

	    for (vp = pci_vendors; vp->next; vp = vp->next)
		;	/* Find the last vendor */
	    for (ppp = &vp->prod; *ppp; ppp = &(*ppp)->next)
		;	/* Find the last product */

	    *ppp = pp;
	}
	else
	{
	    pci_vend_t		*vp;
	    pci_vend_t		**vpp;
	    u_long		id;
	    char		*name;

	    id = strtoul(tp, &tp, 16);
	    if ((id > USHRT_MAX) || !tp || !isspace(*tp))
	    {
		debug_print("%s: line %lu: Invalid vendor ID", filename, line);
		continue;
	    }

	    while (*tp && !isspace(*tp))
		++tp;

	    if (*tp)
		while (isspace(*tp))
		    ++tp;

	    if (!*tp)
	    {
		debug_print("%s: line %lu: Vendor name expected",
							    filename, line);
		name = "<Unknown>";
	    }
	    else
		name = tp;

	    /*
	     * Find a place to put this new vendor.
	     * We will attach it to the end of the list.
	     */

	    if (!(vp = (pci_vend_t *)malloc(sizeof(pci_vend_t))))
	    {
		err = ENOMEM;
		break;
	    }

	    if (!(vp->name = strdup(name)))
	    {
		free(vp);
		err = ENOMEM;
		break;
	    }

	    vp->id = (u_short)id;
	    vp->prod = NULL;
	    vp->next = NULL;

	    for (vpp = &pci_vendors; *vpp; vpp = &(*vpp)->next)
		;

	    *vpp = vp;
	}
    }

    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

