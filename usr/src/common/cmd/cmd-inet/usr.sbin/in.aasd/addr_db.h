#ident "@(#)addr_db.h	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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
 * This file contains information used by the internal address database
 * manipulation code.
 */

/*
 * Address information structure
 */

typedef struct AddrInfo {
	struct AddrInfo *next;		/* next in list */
	struct AddrInfo *prev;		/* prev in list */
	char inuse;			/* 1=inuse, 0=avail */
	char freed;			/* 1 if freed by free request, else 0 */
	char temp;			/* 1 if temporary allocation, else 0 */
	char disabled;			/* 1 if disabled, else 0 */
	AasAddr addr;			/* the address */
	char *service;			/* service that allocated it */
	AasClientId client_id;		/* ID of client that allocated it */
	AasTime alloc_time;		/* system time when allocated */
	AasTime lease_time;		/* lease duration (s) */
	AasTime free_time;		/* time freed by free request */
} AddrInfo;

/*
 * Pool information structure
 */

typedef struct Pool {
	struct Pool *next;	/* next in list */
	char *name;		/* name of the pool */
	AddressType *atype;	/* address type information */
	int num_addrs;		/* number of addresses in the pool */
	int num_alloc;		/* number of addresses allocated */
	AddrInfo avail_head;	/* linked list of avail addrs in LRU order */
	AddrInfo inuse_head;	/* linked list of in-use addrs in exp order */
	AddrInfo **addrs;	/* sorted array of ptrs to AddrInfo */
	DB *id2addr;		/* service/client -> addr database */
	char match;		/* used in responding to pool queries */
} Pool;

/*
 * The AddrList structure maintains a sorted list of addresses.
 * AddrList is used to implement the orphanage.
 */

typedef struct {
	AddrInfo **list;
	int num;
	int size;
} AddrList;

extern Pool *pools;
extern int num_pools;
extern AddrList orphanage[];
extern AddressType *address_types[];

AddrInfo *addr_search(AasAddr *addr, AddrInfo **list, int num,
	AtAddrCompareFunc compare, int *pos);
void expire_addresses(Pool *pool);
int config(void);
int alloc_addr(char *pool_name, int addr_type, AasAddr *req_addr,
	AasAddr *min_addr, AasAddr *max_addr, int flags, AasTime lease_time,
	char *service, AasClientId *client_id, AasAddr **allocp);
int free_addr(char *pool_name, int addr_type, AasAddr *addr,
	char *service, AasClientId *client_id);
int id_search(Pool *pool, char *service, AasClientId *client_id,
	AddrInfo **ainfo_ret);
int free_all(char *pool_name, char *service);
int disable_addr(char *pool_name, int addr_type, AasAddr *addr, int disable);
void reconfigure(void);

/*
 * pool_search -- search a pool's address list
 */

#define pool_search(pool, addr, pos) \
	addr_search((addr), (pool)->addrs, (pool)->num_addrs, \
		(pool)->atype->compare, (pos))

/*
 * EXPIRED -- determine whether a lease has expired
 * Returns true if expired, false if not.
 */

#define EXPIRED(alloc_time, lease_time, now) \
	((lease_time) != AAS_FOREVER && ((now) - (alloc_time)) >= (lease_time))
