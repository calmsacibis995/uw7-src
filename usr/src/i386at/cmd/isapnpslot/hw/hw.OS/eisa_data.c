/*
 * File eisa_data.c
 *
 * @(#) eisa_data.c 65.1 97/06/02 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 *
 * This data is used by EISA and ISA PnP
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
#include "eisa_data.h"

eisa_vend_t	*eisa_vendors = NULL;

int
load_eisa_data()
{
    static const char	cfg_file[] = "eisa.cfg";
    const char		*filename;
    FILE		*fd;
    char		buf[256];
    u_long		line;
    int			err = 0;

    if (eisa_vendors)
	return 0;	/* Load only once */

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
	    eisa_vend_t		*vp;
	    eisa_prod_t		*pp;
	    eisa_prod_t		**ppp;
	    u_long		id;
	    char		*name;

	    if (!eisa_vendors)
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

	    name = tp;

	    /*
	     * Find a place to put this new product.
	     * We will attach it to the end of the list of
	     * products for the last vendor.
	     */

	    if (!(pp = (eisa_prod_t *)malloc(sizeof(eisa_prod_t))))
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
	    pp->next = NULL;

	    for (vp = eisa_vendors; vp->next; vp = vp->next)
		;	/* Find the last vendor */
	    for (ppp = &vp->prod; *ppp; ppp = &(*ppp)->next)
		;	/* Find the last product */

	    *ppp = pp;
	}
	else
	{
	    eisa_vend_t		*vp;
	    eisa_vend_t		**vpp;
	    char		*key;
	    char		*name;

	    key = tp;

	    while (*tp && !isspace(*tp))
		++tp;

	    if (*tp)
	    {
		*tp++ = '\0';	/* End of key */

		while (isspace(*tp))
		    ++tp;
	    }

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

	    if (!(vp = (eisa_vend_t *)malloc(sizeof(eisa_vend_t))))
	    {
		err = ENOMEM;
		break;
	    }

	    if (!(vp->key = strdup(key)))
	    {
		free(vp);
		err = ENOMEM;
		break;
	    }

	    if (!(vp->name = strdup(name)))
	    {
		free((void *)vp->key);
		free(vp);
		err = ENOMEM;
		break;
	    }

	    vp->prod = NULL;
	    vp->next = NULL;

	    for (vpp = &eisa_vendors; *vpp; vpp = &(*vpp)->next)
		;

	    *vpp = vp;
	}
    }

    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

