/*
 * File hw_net.c
 * Information handler for net
 *
 * @(#) hw_net.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_net.h"
#include "hw_util.h"

const char	* const callme_net[] =
{
    "net",
    "network",
    NULL
};

const char	short_help_net[] = "Network related statistics";

int
have_net(void)
{
    return 0;	/* ## */
}

void
report_net(FILE *out)
{
    report_when(out, "networking");
    fprintf(out, "    I have much to learn about networking\n");
}

#ifdef NOT_YET	/* ## */

    /var/`llipathmap`/sysdb/\*

    netstat
    llistat
    ifconfig
    hardware config

#endif

