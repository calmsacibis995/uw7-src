#ident	"@(#)pool.h	1.2"
#ident	"$Header$"
#ident "@(#) pool.h,v 1.4 1995/08/21 14:48:34 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1995 Legent Corporation.
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
 *
 */

/* Misc Defines */

#define POOLBUFSIZ  8192
#define POOL_FILE   "/etc/addrpool"

/* structure to hold elements of a address list within a pool */

struct addr_list_el_s {
	struct addr_list_el_s	*next;	/* Pointer to next addr in list */
	void	*addr;          /* pointer to address */
	u_char	 used;		/* 0 if unused, 1 if used */
};

typedef struct addr_list_el_s addr_list_el_t;

/* Per Pool structure */

typedef struct addr_pool_s addr_pool_t;

struct addr_pool_s {
	addr_pool_t	*next;	/* Pointer to Next Pool */
	char 	*pool_tag;	/* Tag to identify this pool */
	uint   	 addr_type;	/* Type of addresses in the Pool */
	int	 count;	        /* Number of addresses in Pool */
	addr_list_el_t	*list;		/* Linked List of addresses */
};

/* Address type defines */

#define   POOL_IP    0x00000001   /* IP addresses */
#define   POOL_IPX   0x00000002   /* IPX Addresses 
				     -- NOT Currently Supported */
#define   POOL_INVAL 0xffffffff   /* Invalid address type */


