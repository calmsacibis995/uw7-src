/*
 * File hw_cache.c
 * Information handler for cache
 *
 * @(#) hw_cache.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_cache.h"
#include "hw_util.h"

const char	* const callme_cache[] =
{
    "cache",
    "cpu_cache",
    NULL
};

const char	short_help_cache[] = "CPU cache statistics";

int
have_cache(void)
{
    return 0;	/* ## */
}

void
report_cache(FILE *out)
{
    report_when(out, "CPU cache");
    fprintf(out, "    I have much to learn about cache\n");
}

