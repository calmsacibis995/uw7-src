/*
 * File hw_bus.c
 * Information handler for bus
 *
 * @(#) hw_bus.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_bus.h"
#include "hw_isa.h"
#include "hw_eisa.h"
#include "hw_mca.h"
#include "hw_pci.h"
#include "hw_pcmcia.h"
#include "hw_util.h"

const char	* const callme_bus[] =
{
    "bus",
    "all_bus",
    "all_buss",
    NULL
};

const char	short_help_bus[] = "Info on all CPU busses found";

#define HW_BUS(x)	\
    { have_ ## x, report_ ## x }

typedef struct
{
    int	(*have)(void);
    void	(*report)(FILE *out);
} bus_lst_t;
static bus_lst_t	bus_lst[] =
{
    HW_BUS(isa),
    HW_BUS(eisa),
    HW_BUS(mca),
    HW_BUS(pci),
    HW_BUS(pcmcia)
};

int
have_bus(void)
{
    int		n;

    for (n = 0; n < sizeof(bus_lst)/sizeof(*bus_lst); n++)
	if (bus_lst[n].have())
	    return 1;

    return 0;
}

void
report_bus(FILE *out)
{
    int		bus_found = 0;
    int		n;

    for (n = 0; n < sizeof(bus_lst)/sizeof(*bus_lst); n++)
	if (bus_lst[n].have())
	{
	    bus_lst[n].report(out);
	    ++bus_found;
	}

    if (!bus_found)
    {
	report_when(out, "bus");
	fprintf(out, "    No busses found!\n");
    }
}

