/*
 * File hw_mca.c
 * Information handler for mca
 *
 * @(#) hw_mca.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_mca.h"
#include "hw_util.h"
#include "mca_data.h"

const char	* const callme_mca[] =
{
    "mca",
    "mca_bus",
    "mca_buss",
    NULL
};

const char	short_help_mca[] = "Info on MicroChannel bus devices";

int
have_mca(void)
{
    return 0;	/* ## */
}

void
report_mca(FILE *out)
{
    report_when(out, "MCA");
    fprintf(out, "    I have much to learn about mca\n");

    load_mca_data();

    /* ## */
}

