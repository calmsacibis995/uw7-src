#ident	"@(#)list.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: list.c,v 1.8 1994/12/16 15:14:06 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */

#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <errno.h>
#include <syslog.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <termios.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
/* STREAMS includes */
#include <setjmp.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>

#ifndef _KMEMUSER
#define _KMEMUSER
#endif

#include <sys/sockio.h>
#include <stdlib.h>
#include <pfmt.h>
#include <locale.h>
#include <unistd.h>


#include <poll.h>
#include <string.h>
#include <dirent.h>
/* inet includes */
#include <netdb.h>
#ifdef _DLPI
#include <sys/dlpi.h>
#endif
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h> 
#include <netinet/ppp.h> 
#include <netinet/asyhdlc.h>
#undef DIAL
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <sys/filio.h>

#include <net/bpf.h>

#include "md5.h"


#include "pathnames.h"

#include "pppd.h"
#include "pppu_proto.h"

/* Address Pooling API */
#include "pool.h"
#include "pool_proto.h"

/*
 * Allocate and add an element to a given list
 *
 * Returns:
 *  Success: Pointer to allocated buffer
 *  Failure: NULL
 *
 */

void *
element_add(list,size)
     list_header_t **list;
     int size;
{
  list_header_t *el;
  
  /* Allocate a the list element */

  if ((el = (list_header_t *) malloc(size)) == NULL) {
    syslog(LOG_WARNING, gettxt(":60", "element_add: malloc failed: %m"));
    return(NULL);
  }

  memset(el, 0, size);

  /* Add it to the head of the list list */
  
  if (*list == NULL) {
    /* This is the first element in the list */
    *list = el;
    (*list)->next = (*list)->prev = NULL;
    return(el);
  }

  el->next = *list;
  el->prev = NULL;
  (*list)->prev = el;
  *list = el;

  return(el);

}
  
/*
 * Remove an element from a list and free it.
 *
 * Returns:
 *   -1 : Invalid address specified to remove/free
 *    0 : Success
 */

int
element_free(list, el)
     list_header_t **list;
     list_header_t *el;
{
  list_header_t *tmp;
  int found = 0;

  if ((*list) == NULL) {
    /* This shouldn't happen */
    syslog(LOG_WARNING, 
	       gettxt(":2", "element_free: called with list empty !"));
    return(-1);
  }
    
  tmp = (*list);

  while (tmp) {
    if (tmp == el) {
      found++;
      
      if (tmp->prev) 
	tmp->prev->next = tmp->next;
      else
	(*list) = tmp->next; /* first element of the list */

      if (tmp->next)
	tmp->next->prev = tmp->prev;

      break;
    }

    tmp = tmp->next;

  } 

  if (found) {
    free(el);
    return(0);
  }
  else {
    syslog(LOG_WARNING, gettxt(":3", "element_free: Element not in list"));
    return(-1);
  }
}
