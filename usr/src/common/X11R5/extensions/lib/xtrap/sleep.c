#ident	"@(#)r5extensions:lib/xtrap/sleep.c	1.1.1.1"

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991, 1992 by 

	Digital Equipment Corp., Maynard, MA

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*****************************************************************************/
/*
 *
 *  CONTRIBUTORS:
 *
 *      Marc Evans
 *      Ken Miller
 *
 */

# ifndef vms
#  include "Xos.h"
# endif

#if !(defined sun || defined OSF1)
# ifdef __STDC__
int usleep(unsigned long usecs)
# else
void usleep(usecs)
    unsigned long usecs; /* The number of desired micro seconds to sleep */
# endif
# ifndef vms
{
    struct timeval itm;
    itm.tv_sec  = usecs / 1000000L;
    itm.tv_usec = 1000000L * (usecs % 1000000L);
    select(0L,0L,0L,0L,&itm);
    return;
}
# else
{
    float delay = (float)usecs / 1000000.0;
    lib$wait(&delay);
    return;
}
# endif /* vms */
#endif /* sun */
