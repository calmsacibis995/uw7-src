/* copyright "%c%" */
#ident	"@(#)hwmetric:pfmt.c	1.2"
#ident	"$Header$"


/* Copyright (c) 1996 HAL Computer Systems, Inc.  All Rights Reserved. */

/* pfmt.c - display diagnostic messages for 'hwmetric' command */

#include <sys/ddi.h>
#include <sys/types.h>
#include <sys/dl.h>
#include <sys/ksynch.h>

#ifdef HEADERS_INSTALLED
# include <sys/cpumtr.h>
# include <sys/cgmtr.h>
#else /* !HEADERS_INSTALLED */
# include "../cpumtr/cpumtr.h"
# include "../cgmtr/cgmtr.h"
#endif /* !HEADERS_INSTALLED */

#include "hwmetric.h"

#include <pfmt.h>
#include <stdarg.h>
#include <stdio.h>

/* Format and print an action message. */
void
pfmtact (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);

  (void)vpfmt (stderr, MM_STD|MM_NOGET|MM_ACTION, fmt, ap);

  va_end (ap);
}

/* Format and print an informative message. */
void
pfmtinf (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);

  (void)vpfmt (stderr, MM_STD|MM_NOGET|MM_INFO, fmt, ap);

  va_end (ap);
}

/* Format and print an error message. */
void
pfmterr (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);

  (void)vpfmt (stderr, MM_STD|MM_NOGET|MM_ERROR, fmt, ap);

  va_end (ap);
}
