#ident	"@(#)pool.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: pool.c,v 1.6 1995/04/11 23:42:13 neil Exp"
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

/*
 * This file contains all of the code that makes up the 
 * address pooling mechanism.
 */
#include <stdio.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

#include "pool.h"
#include "pool_proto.h"


/* 
 * Globals
 */

addr_pool_t *pool_list_head = NULL;
addr_pool_t *pool_list_tail = NULL;

/*
 * pool_addr_alloc()
 *
 * This routine allocates an address from a given address pool.
 *
 * Parameters:
 *   
 *    pool_tag: The pool from which to allocate the network addresses.  
 *              There may be any number of user defined pools from which 
 *              to allocate addresses.  A pool must contain only 
 *              addresses of the same type.  Addresses from different protocols
 *              can't be intermixed.
 *
 *    addr_type: Specifies the type of protocol address being requested (e.g. IP
 *               or IPX).  This parameter will be used to insure that the specified
 *               pool  (in 'pool_tag') contains the type of address being requested.
 *
 *               The possible values are: POOL_IP 
 *
 *    addr_buf:  A buffer to hold the returned allocated address.  The buffer 
 *               is allocated and freed by the caller of this function. The caller
 *               of this function must have a knowledge of the size of the buffer 
 *               needed to hold the address being requested.
 *               
 *    buf_sz:    The length of 'addr_buf' in bytes. 
n *
 * Return values:
 *   0 on success
 * 
 *   -1 on error with errno set as follows:
 *     
 *      EINVAL     The addresses contained in the address pool 
 *                 specified by pool_tag, do not contain the 
 *                 address type specified by the addr_type field.
 *                 Or a NULL pointer was passed in for pool_tag 
 *                 or addr_buf.
 *
 *      EAGAIN     There are currently no addresses available in 
 *                 pool specified by pool_tag.
 *
 *      EFAULT     A pool does not exist by the name of pool_tag.
 *
 *      ENOMEM     The buffer specified by addr_buf is not large 
 *                 enough to accommodate the requested address type. 
 *                 The size of addr_buf is determined by buf_sz.
 */

int
pool_addr_alloc(pool_tag, addr_type, addr_buf, buf_sz)
     char *pool_tag;
     uint  addr_type;
     void *addr_buf;
     int buf_sz;
{
  addr_pool_t *poolp;
  addr_list_el_t *addrp;
  int len;

  if ((pool_tag == NULL) || (addr_buf == NULL)) {
    errno = EINVAL;
    return(-1);
  }

  /* Find the pool associated with pool_tag */
  if ((poolp = find_pool(pool_tag)) == NULL) {
    errno = EFAULT;
    return(-1);
  }

  /* See if the buffer passed in is large enough to hold address */
  if ((len = addr_size(addr_type)) > buf_sz) {
    errno = ENOMEM;
    return(-1);
  }

  /* 
   * Insure that the address type being requested matches
   * the type in the pool.
   */

  if (addr_type != poolp->addr_type) {
    errno = EINVAL;
    return(-1);
  }

  /* allocate address */

  if ((addrp = alloc_addr(poolp)) == NULL) {
    errno = EAGAIN;
    return(-1);
  }

  memcpy(addr_buf, addrp->addr, len);

  return(0);

}

/*
 * pool_addr_free()
 *
 * This routine frees an address allocated using pool_addr_alloc().
 *
 * Parameters:
 *
 *    pool_tag: A character string specifying the pool from which 
 *              the address was allocated. 
 *
 *    addr_type: Specifies the type of Protocol address being requested 
 *               (e.g. IP or IPX). This will be used to insure that the 
 *               specified pool contains the type of addresses being freed.
 *
 *    addr_buf:  A pointer to a buffer containing the address being freed. 
 *               The buffer is allocated and freed by the caller of this 
 *               function.
 *
 *    buf_sz:    The length of 'addr_buf' in bytes. This can will be used 
 *               to verify that 'addr_buf' is big enough to accommodate 
 *               the type of address being freed.
 *
 * Return values:
 *    0 on success
 *
 *    -1 on error, with errno set as follows:
 *       EINVAL      The addresses contained in the address pool specified 
 *                   by pool_tag, do not contain the address type specified 
 *                   by the addr_type field. Or a NULL pointer was passed
 *                   in for pool_tag or addr_buf.
 *
 *       EFAULT      A pool does not exist by the name of pool_tag.
 *
 *       ENOMEM      The buffer specified by 'addr_buf' is not large enough to 
 *                   accommodate the address type being freed.  The size of 
 *                   'addr_buf' is determined by 'buf_sz'.
 *
 */

int
pool_addr_free(pool_tag, addr_type, addr_buf, buf_sz)
     char *pool_tag;
     uint addr_type;
     void *addr_buf;
     int  buf_sz;
{
  addr_pool_t *poolp;
  int len;

  if ((pool_tag == NULL) || (addr_buf == NULL)) {
    errno = EINVAL;
    return(-1);
  }

  /* Find the pool associated with pool_tag */
  if ((poolp = find_pool(pool_tag)) == NULL) {
    errno = EFAULT;
    return(-1);
  }

  /* See if the buffer passed in is large enough to hold address */
  if ((len = addr_size(addr_type)) > buf_sz) {
    errno = ENOMEM;
    return(-1);
  }

  /* 
   * Insure that the address type being requested matches
   * the type in the pool.
   */

  if (addr_type != poolp->addr_type) {
    errno = EINVAL;
    return(-1);
  }

  free_addr(poolp, addr_buf);

  return(0);
}

/*
 * pool_init()
 *
 * This function initializes the pool allocation mechanism.  
 *
 * Parameters:
 *   None.
 *
 * Returns:
 *   0 if address pooling is successfully initialized
 *   -1 if there was an error initializing the address pool.
 */

int
pool_init(reinit)
int reinit;
{
  char *p;
  char line[POOLBUFSIZ];
  FILE *fp;
  char *pool_tag, *addr_type_s;
  addr_pool_t *list;
  addr_list_el_t *list_el, *list_el_last;
  char *addr;
  addr_pool_t *old_pool;

  /*
   * If this is a reinit call then we need to re-read
   * the pool file.  We will temporarily tuck away the address
   * of the current pool, and create a new pool. Then the new pool
   * will be scanned, marking addresses that are currently allocated
   * as used. The old pool will be used to determine which addresses 
   * are currently being used.
   */

  if (reinit) {
    syslog(LOG_INFO, gettxt(":63", "Re-initializing address pool"));
    old_pool = pool_list_head;
    pool_list_head = NULL;
  }

  if ((fp = fopen(POOL_FILE, "r")) == NULL) {
    syslog(LOG_WARNING, gettxt(":64", "pool_init: can't open %s:%m"), POOL_FILE);
    return(-1);
  }

  /* 
   * Build the address pool list.
   */

  while (1) { 

    if ((list = (addr_pool_t *) malloc(sizeof(addr_pool_t))) == NULL) {
      free_pool_list(pool_list_head);
      return(-1);
    }

    list->next = NULL;

    if ((p = poolfgets(line, POOLBUFSIZ, fp)) == NULL) {
      fclose(fp);
      break;
    }

    if (*p == '\0') {
      free(list);
      continue;
    }

    /* Get the pool tag and address type field */
    pool_tag = strtok(p, " \t");
    
    /* Remove the address type */
    if ((addr_type_s = strchr(pool_tag, ':')) == NULL) {
      syslog(LOG_WARNING, gettxt(":65", "pool_init: No address type specified for %s tag"),
	     pool_tag);
      syslog(LOG_WARNING, gettxt(":66", "pool_init: %s pool not being processed ... continuing"),
	     pool_tag);
      free(list);
      continue;
    }

    *addr_type_s = '\0'; /* NULL Terminate pool_tag (YES pool_tag !!)*/
    addr_type_s++;

    if ((list->addr_type = check_addr_type(addr_type_s)) == POOL_INVAL) {
      syslog(LOG_WARNING, gettxt(":67", "pool_init: Invalid address type specified for %s tag"),
	     pool_tag);
      syslog(LOG_WARNING, gettxt(":66", "pool_init: %s pool not being processed ... continuing"),
	     pool_tag);
      free(list);
      continue;
    }

    if ((list->pool_tag = (char *) malloc(strlen(pool_tag)+1)) == NULL) {
      syslog(LOG_WARNING, gettxt(":68", "pool_init: memory allocation failure ... Aborting"));
      free_pool_list(pool_list_head);
      return(-1);
    }

    strcpy(list->pool_tag, pool_tag);

    list->count = 0;

    /* Fill in the address list */
    
    list_el_last = NULL;
    while (1) {
      if ((addr = strtok(NULL, " \t")) == NULL)
	break;

      if ((list_el = (addr_list_el_t *) malloc(sizeof(addr_list_el_t)))
	  == NULL) {
	syslog(LOG_WARNING, gettxt(":68", "pool_init: memory allocation failure ... Aborting"));
	free_pool_list(pool_list_head);
	return(-1);
      }

      list_el->used = 0;
      list_el->next = NULL;

      if (list_el_last != NULL)
	list_el_last->next = list_el;
      else
	list->list = list_el;

      list_el_last = list_el;
      if ((list_el->addr = pool_get_addr(list->addr_type, addr)) == NULL) {
	syslog(LOG_WARNING, 
	       gettxt(":69", "pool_init: invalid address %s in pool %s ... continuing"),
	       addr, pool_tag);
	free(list_el);
	continue;
      }

      list->count++;
    } /* while */
	
    /* Put address pool in list of pools */

    if (pool_list_head == NULL)
      pool_list_head = pool_list_tail = list;
    else {
      pool_list_tail->next = list;
      pool_list_tail = list;
    }
  } /* while */

  if (reinit) {
    /*
     * Mark any addresses that were allocated in the old
     * address pool as allocated in the new one.
     */
    alloc_from_old(old_pool, pool_list_head);
    free_pool_list(old_pool);
  }
    

  return(0);
}

/*
 * free_pool_list()
 * 
 * Free all address pools in a list.
 */

int
free_pool_list(pool_list)
addr_pool_t *pool_list;
{
  addr_list_el_t *addr_list, *temp;
  addr_pool_t *ptemp;

  while(pool_list != NULL) {

    /* Free the elements of the list */
    addr_list = pool_list->list;

    while (addr_list) {
      temp = addr_list;
      addr_list = addr_list->next;
      free(temp->addr);
      free(temp);
    }

    ptemp = pool_list;
    pool_list = pool_list->next;
    free(ptemp);
  }
  
  return(0);
}
    
/*
 * check_addr_type()
 *
 * Verify that an address type is valid, and return the numeric pool type
 * if the address type is valid.
 *
 */

uint
check_addr_type(type)
     char *type;
{
  if (strcmp(type, "IP") == 0) 
    return(POOL_IP);
  else
    return(POOL_INVAL); /* Invalid address type */
}
  
/*
 * pool_get_addr()
 *
 * Convert a character string representation of an address into a pointer
 * to a structure that is specific to an address type.
 */

void *
pool_get_addr(type, addr)
     uint type;
     char *addr;
{
  switch (type) {
  case POOL_IP:
    return((void *)ip_get_addr(addr));
  default:
    return(NULL);
  }
}
    
/*
 * pool_ip_get_addr()
 * Convert a string to a sockaddr_in structure.
 *
 */
struct sockaddr_in *
ip_get_addr(s)
        char    *s;
{
  struct hostent  *hp;
  int val;
  int r = 0;
  unsigned long   l;
  struct sockaddr_in      *sin;
  
  if ((sin = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in))) == NULL) {
    syslog(LOG_WARNING, gettxt(":70", "ip_get_addr: memory allocation failure"));
    return(NULL);
  }

  memset(sin, 0, sizeof(struct sockaddr_in));
  
  l = inet_addr(s);
  
  if (l == INADDR_NONE) {  /* not dot format */
    hp = gethostbyname(s);
    if (hp) {
      sin->sin_family = hp->h_addrtype;
      memcpy((char *)&sin->sin_addr, hp->h_addr, hp->h_length);
    } else {
      syslog(LOG_WARNING, gettxt(":58", "gethostbyname(%s) fail: %m"),s);
      return(NULL);
    }
  } else {        /* dot format */
    sin->sin_family = AF_INET;
    r = inet_aton(s, &val);
    if (r != -1) {
      sin->sin_addr.s_addr = val;
    } else {
      syslog(LOG_WARNING,gettxt(":59", "inet_aton(%s) fail: %m"),s);
      free(sin);
      return(NULL);
    }
  }
  return(sin);
}



/*
 * Get an entry from a the pool file.
 * Entries may continue onto multiple lines by giving a '\' 
 * as the last character of a line.
 * Comments start with '#'
 * return NULL: end of file or buffer too small
 */

char *
poolfgets(line, n, fd)
        char *line;
        int n;
        FILE *fd;
{
        char tmp[POOLBUFSIZ];
        int i, j, len = 0;

        if (fgets(tmp, POOLBUFSIZ, fd) == NULL)
                return(NULL);

        while (len < n) {
                /* skip space, '\t' and '\n' */
                j = 0;
                while (tmp[j] == ' ' || tmp[j] == '\t' || tmp[j] == '\n')
                        j++;

                /* look for '\0', '\n' and POUND key */
                for (i = j; tmp[i] != '\n' && tmp[i] != '\0' && tmp[i] != '#'; i++);
                if (tmp[i] == '#' || tmp[i] == '\n')
                        tmp[i] = '\0';

                /*  not exceed line buffer size */
                if (len + i - j + 1 < n)
                        memcpy(&line[len], &tmp[j], i - j + 1);
                else {
                        syslog(LOG_ERR, gettxt(":71", "Config file entry too large"));
                        return(NULL);
                }

                len += i - j + 1;

                /* If continue on next line, read again */
                if (len < 2 || line[len-2] != '\\') {
                        return(line);
                }
                else {
                        line[len-2] = ' ';
                        /* reset line array pointer -- delete '\0' */
                        len --;
                }
                if (fgets(tmp, POOLBUFSIZ, fd) == NULL)
                        return(line);
        }
        return(NULL);
}

/*
 * This function prints all of the pools in the address pool to
 * 'fp'.
 */
int
print_pool(fp)
FILE *fp;
{
  addr_list_el_t *addr;
  addr_pool_t *pool;
  int loops = 0;

  pool = pool_list_head;

  while(pool != NULL) {
    loops++;
    /* Print Info about the pool */
    fprintf(fp, gettxt(":72", "Pool Tag: %s\n"), pool->pool_tag);
    fprintf(fp, gettxt(":73", "Pool Type: %s\n"), type_to_string(pool->addr_type));
    fprintf(fp, gettxt(":74", "Free addresses in List: %d\n"), pool->count);
    fprintf(fp, gettxt(":75", "Addresses in pool:\n"));
    addr = pool->list;

    while (addr) {
      fprintf(fp, "\t%s: %s\n", addr_to_string(addr->addr, pool->addr_type), 
	      addr->used ? gettxt(":76", "USED") : gettxt(":77", "FREE"));
      addr = addr->next;
    }

    pool = pool->next;
  }

  if (loops == 0) 
    fprintf(fp, gettxt(":78", "There are no address pools defined\n"));
  return(0);
}

/*
 * Convert addrress type to string
 */

char *
type_to_string(type)
uint type;
{
  switch(type) {
  case POOL_IP:
    return(gettxt(":79", "IP"));
  default:
    return(gettxt(":80", "Invalid Type"));
  }
}

/*
 * Print a format address string based on address type.
 */

char *
addr_to_string(addr, type)
void *addr;
uint type;
{
  switch(type) {
  case POOL_IP:
    return(inet_ntoa(((struct sockaddr_in *) addr)->sin_addr));
  default:
    return(gettxt(":81", "Invalid Address"));
  }
}
 
/* 
 * Find the pool with name 'tag'
 */

addr_pool_t *
find_pool(tag)
char *tag;
{
  addr_pool_t *p;

  p = pool_list_head;

  while (p) {
    if (strcmp(tag, p->pool_tag) == 0)
      break;
    p = p->next;
  }

  return(p);
}

/*
 * Find the size of an address of type 'type'
 * 
 * NOTE: It is assumed that the type field has been verified to 
 *       be a valid address type.
 */

int 
addr_size(type)
uint type;
{
  switch(type) {
  case POOL_IP:
    return(sizeof(struct sockaddr_in));
  default:
    return(0x7fffffff); /* Something big */
  }
}

/*
 * Allocate an address from a pool.
 */
addr_list_el_t *
alloc_addr(poolp)
addr_pool_t *poolp;
{
  addr_list_el_t *addrp;
  
  if (poolp->count == 0)
    return(NULL);

  addrp = poolp->list;
  
  while (addrp) {
    if (addrp->used == 0) {
      addrp->used = 1;
      poolp->count--;
      break;
    }
    addrp = addrp->next;
  }
    
  return(addrp);
} 

/*
 * Free an allocated address.
 *
 * If the address id not contained in the pool, then silently
 * drop it.  This is to handle the case in which the address list
 * was updated between the time we allocated the address and we
 * are freeing it.
 */

int
free_addr(poolp, addr_buf)
     addr_pool_t *poolp;
     addr_list_el_t *addr_buf;
{
  addr_list_el_t *addrp;
  int len;

  addrp = poolp->list;
  len  = addr_size(poolp->addr_type);
  
  while (addrp) {
    if (memcmp(addr_buf, addrp->addr, len) == 0) {
      addrp->used = 0;
      poolp->count++;
      return(0);
    }
    addrp = addrp->next;
  }
  
  return(-1);
}


  
/*
 * The following function scans 'list_a', and marks
 * any allocated address as allocated in 'list_b'.
 */
int
alloc_from_old(list_a, list_b)
addr_pool_t *list_a, *list_b;
{
  addr_pool_t *tmp;
  addr_list_el_t *addrp_a, *addrp_b;
  uint type_a;
  int addr_sz, found;

  while (list_a) {
    addrp_a = list_a->list;
    type_a = list_a->addr_type;
    addr_sz = addr_size(list_a->addr_type);

    while (addrp_a) {
      if (! addrp_a->used) {
	/*
	 * We only care about used (allocated) ones,
	 * so forget this one.
	 */
	addrp_a = addrp_a->next;
	continue;
      }
      
      /*
       * See if the address exists in 
       * a pool in list_a.
       */
      tmp = list_b;
      found = 0;
      while (tmp) {
	if (tmp->addr_type == type_a) {
	  addrp_b = tmp->list;

	  while (addrp_b) {
	    if (memcmp(addrp_b->addr, addrp_a->addr, addr_sz) == 0) {
	      addrp_b->used = 1;
	      tmp->count--;
	      found++;
	      break;
	    }
	      
	    addrp_b = addrp_b->next;
	  }
	}
	if (found)
	  break;
	tmp = tmp->next;
      } /* while */
      addrp_a = addrp_a->next;
    } /* while */
    list_a = list_a->next;
  } /* while */
  return(0);
}
