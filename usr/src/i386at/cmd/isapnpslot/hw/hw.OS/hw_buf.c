/*
 * File hw_buf.c
 * Information handler for buf
 *
 * @(#) hw_buf.c 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#include "hw_buf.h"
#include "hw_util.h"

const char	* const callme_buf[] =
{
    "buf",
    "buffer_cache",
    NULL
};

const char	short_help_buf[] = "Buffer cache statistics";

int
have_buf(void)
{
    return 0;	/* ## */
}

void
report_buf(FILE *out)
{
    report_when(out, "Buffer cache");
    fprintf(out, "    I have much to learn about Buffer cache\n");
}

