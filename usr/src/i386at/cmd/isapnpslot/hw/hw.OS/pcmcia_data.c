/*
 * File pcmcia_data.c
 *
 * @(#) pcmcia_data.c 58.1 96/10/16 
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
#include "pcmcia_data.h"

pcmcia_vend_t	*pcmcia_vendors = NULL;

int
load_pcmcia_data()
{
    static const char	cfg_file[] = "pcmcia.cfg";
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
	    pcmcia_vend_t	*vp;
	    pcmcia_prod_t	*pp;
	    pcmcia_prod_t	**ppp;
	    u_long		id;
	    char		*name;

	    if (!pcmcia_vendors)
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

	    if (!(pp = (pcmcia_prod_t *)malloc(sizeof(pcmcia_prod_t))))
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

	    for (vp = pcmcia_vendors; vp->next; vp = vp->next)
		;	/* Find the last vendor */
	    for (ppp = &vp->prod; *ppp; ppp = &(*ppp)->next)
		;	/* Find the last product */

	    *ppp = pp;
	}
	else
	{
	    pcmcia_vend_t	*vp;
	    pcmcia_vend_t	**vpp;
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

	    if (!(vp = (pcmcia_vend_t *)malloc(sizeof(pcmcia_vend_t))))
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

	    for (vpp = &pcmcia_vendors; *vpp; vpp = &(*vpp)->next)
		;

	    *vpp = vp;
	}
    }

    fclose(fd);
    debug_print("%s: lines read %lu", filename, line);
    return errno = err;
}

