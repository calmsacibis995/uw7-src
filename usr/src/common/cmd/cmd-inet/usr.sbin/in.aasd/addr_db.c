#ident "@(#)addr_db.c	1.3"
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
 * This file contains code that manipulates the address database.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <malloc.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <db.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_db.h>
#include <aas/aas_util.h>
#include "aasd.h"
#include "atype.h"
#include "addr_db.h"
#include "pathnames.h"

/*
 * Doubly-linked list macros
 */

#define LIST_ADD_BEFORE(el, new_el) \
	((new_el)->next = (el), \
	 (new_el)->prev = (el)->prev, \
	 (el)->prev->next = (new_el), \
	 (el)->prev = (new_el))

#define LIST_ADD_AFTER(el, new_el) \
	((new_el)->next = (el)->next, \
	 (new_el)->prev = (el), \
	 (el)->next->prev = (new_el), \
	 (el)->next = (new_el))

#define LIST_DELETE(el) \
	((el)->prev->next = (el)->next, \
	 (el)->next->prev = (el)->prev)

#define LIST_INIT(head) \
	((head)->next = (head)->prev = (head))

#define LIST_FOR(head, var) \
	for ((var) = (head)->next; (var) != (head); (var) = (var)->next)


Pool *pools;				/* linked list of pools */
static Pool **pool_link;		/* used to build pool list */

static Password **password_link;

/*
 * The orphanage maintains a list of allocated addresses that aren't in any
 * pool.  These addresses are remembered internally and on disk until
 * they expire.
 */

AddrList orphanage[AAS_NUM_ATYPES];

#define ORPHANAGE_CHUNK		64

/*
 * Global address indexes
 * These are used for two things: to cross-check addresses to prevent
 * an addresses from existing in more than one pool; and to locate an
 * address when replaying the transaction file (no pool information is
 * contained in the transaction records).  There is a separate index
 * for each address type.
 */

typedef struct {
	Pool *pool;		/* pool in which the address is located */
	AddrInfo *ai;		/* the address info */
} AddrLoc;

typedef struct {
	AddrLoc *index;		/* the index itself */
	int num;		/* number of elements in the list */
	int size;		/* allocated size of the list */
} AddrIndex;

static AddrIndex addr_index[AAS_NUM_ATYPES];

#define ADDR_INDEX_CHUNK	1024

/*
 * Address type information table
 */

extern AddressType atype_inet;

AddressType *address_types[AAS_NUM_ATYPES] = {
	NULL,			/* 0 is AAS_ATYPE_ANY */
	&atype_inet
};

static int db_fd;		/* file descriptor for database file */

static int config_line;
static FILE *config_fp;

static AtAddrCompareFunc compare_func;

extern char *config_file;
extern char *db_dir;
extern char *cp_dir;
extern int cp_intvl;
extern int cp_num;
extern int db_max;
extern Password *passwords;
extern int config_ok;

static void free_checkpoint_list(char **list, int num);
static int get_checkpoint_list(char ***list_ret, int *num_ret);
static void compress_db(void);
static AddrLoc *global_index_search(AasAddr *addr, int addr_type, int *pos);
static void reset_pools(void);
static int init_db(void);

/*
 * write_transaction -- write a transaction record to the given file
 * Returns 1 on success or 0 on failure.  If a partial record is
 * written, it is removed by truncating the file back to its
 * size before writing the transaction.
 */

static int
write_transaction(int fd, char *buf, int len)
{
	int ret;
	off_t file_size;

	file_size = lseek(fd, 0, SEEK_CUR);
	if ((ret = write(fd, buf, len)) == -1) {
		report(LOG_ERR, "write_transaction: write failed: %m");
		ftruncate(fd, file_size);
		lseek(fd, 0, SEEK_END);
		return 0;
	}
	return 1;
}

/*
 * record_allocation -- write an allocation transaction
 * Returns 1 if ok, 0 if error.
 */

static int
record_allocation(int fd, AasAddr *addr, int addr_type, AasTime alloc_time,
	AasTime lease_time, char *service, AasClientId *client_id)
{
	char *buf, *p;
	AasTransAlloc *trans;
	ulong len;
	int service_len, ret;

	/*
	 * Calculate length of record && allocate buffer.
	 */
	
	service_len = strlen(service) + 1;
	len = sizeof(AasTransAlloc) + addr->len + service_len + client_id->len;
	
	if (!(buf = malloc(len))) {
		malloc_error("record_allocation");
		return 0;
	}

	/*
	 * Construct the transaction record.
	 */

	trans = (AasTransAlloc *) buf;
	trans->trans_type = htonl(AAS_TRANS_ALLOC);
	trans->trans_len = htonl(len);
	trans->alloc_time = htonl(alloc_time);
	trans->lease_time = htonl(lease_time);
	trans->addr_type = htons(addr_type);
	trans->addr_len = htons(addr->len);
	trans->service_len = htons(service_len);
	trans->client_id_len = htons(client_id->len);

	p = (char *) (trans + 1);

	memcpy(p, addr->addr, addr->len);
	p += addr->len;
	memcpy(p, service, service_len);
	p += service_len;
	memcpy(p, client_id->id, client_id->len);

	/*
	 * Write the transaction.
	 */
	
	ret = write_transaction(fd, buf, len);
	free(buf);
	return ret;
}

/*
 * record_free -- write a free transaction
 * Returns 1 if ok, 0 if error.
 */

static int
record_free(int fd, AasAddr *addr, int addr_type, AasTime free_time)
{
	char *buf;
	AasTransFree *trans;
	ulong len;
	int ret;

	/*
	 * Calculate length of record && allocate buffer.
	 */
	
	len = sizeof(AasTransFree) + addr->len;
	
	if (!(buf = malloc(len))) {
		malloc_error("record_free");
		return 0;
	}

	/*
	 * Construct the transaction record.
	 */

	trans = (AasTransFree *) buf;
	trans->trans_type = htonl(AAS_TRANS_FREE);
	trans->trans_len = htonl(len);
	trans->free_time = htonl(free_time);
	trans->addr_type = htons(addr_type);
	trans->addr_len = htons(addr->len);

	memcpy((char *) (trans + 1), addr->addr, addr->len);

	/*
	 * Write the transaction.
	 */
	
	ret = write_transaction(fd, buf, len);
	free(buf);
	return ret;
}

/*
 * record_disable -- write a free transaction
 * Returns 1 if ok, 0 if error.
 */

static int
record_disable(int fd, AasAddr *addr, int addr_type, int disable)
{
	char *buf;
	AasTransDisable *trans;
	ulong len;
	int ret;

	/*
	 * Calculate length of record && allocate buffer.
	 */
	
	len = sizeof(AasTransDisable) + addr->len;
	
	if (!(buf = malloc(len))) {
		malloc_error("record_disable");
		return 0;
	}

	/*
	 * Construct the transaction record.
	 */

	trans = (AasTransDisable *) buf;
	trans->trans_type = htonl(AAS_TRANS_DISABLE);
	trans->trans_len = htonl(len);
	trans->disable = htons((ushort) disable);
	trans->addr_type = htons(addr_type);
	trans->addr_len = htons(addr->len);

	memcpy((char *) (trans + 1), addr->addr, addr->len);

	/*
	 * Write the transaction.
	 */
	
	ret = write_transaction(fd, buf, len);
	free(buf);
	return ret;
}

/*
 * record_init -- write the init record to the file
 * Returns 1 if ok or 0 if error.
 */

static int
record_init(int fd, AasTime now)
{
	AasTransInit trans;

	/*
	 * Construct the transaction record.
	 */

	trans.trans_type = htonl(AAS_TRANS_INIT);
	trans.trans_len = htonl(sizeof(trans));
	trans.start_time = htonl(now);

	/*
	 * Write it out.
	 */
	
	return write_transaction(fd, (char *) &trans, sizeof(trans));
}

/*
 * write_address_info -- write transaction records to reflect the state of
 * an address.
 * If the address has been allocated, an allocation record is written.
 * If the address has been explicitly freed, a free record is also
 * written.
 * Returns 1 if ok or 0 if error.
 */

static int
write_address_info(int fd, AddrInfo *ai, int addr_type)
{
	/*
	 * Check for addresses that have never been allocated.
	 */

	if (!ai->service) {
		return 1;
	}

	if (!record_allocation(fd, &ai->addr, addr_type, ai->alloc_time,
	    ai->lease_time, ai->service, &ai->client_id)) {
		return 0;
	}

	if (ai->freed &&
	    !record_free(fd, &ai->addr, addr_type, ai->free_time)) {
		return 0;
	}
}

/*
 * write_state -- write a transaction file that reflects the state of the
 * world
 * The file referenced by fd must be empty.  The file is sync'ed
 * after everything is written.
 * Returns 1 if ok, or 0 if an error occurs.
 */

static int
write_state(int fd, AasTime now)
{
	int i, j;
	Pool *pool;
	AddrInfo *ai;

	/*
	 * Write the init record.
	 */

	if (!record_init(fd, now)) {
		return 0;
	}

	/*
	 * Write records for each address in each pool.
	 */

	for (pool = pools; pool; pool = pool->next) {
		for (i = 0; i < pool->num_addrs; i++) {
			if (!write_address_info(fd, pool->addrs[i],
			    pool->atype->addr_type)) {
				return 0;
			}
		}
	}

	/*
	 * Write information for addresses in the orphanage.
	 */
	
	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		for (j = 0; j < orphanage[i].num; j++) {
			ai = orphanage[i].list[j];
			if (EXPIRED(ai->alloc_time, ai->lease_time, now)) {
				ai->inuse = 0;
			}
			if (!ai->inuse) {
				continue;
			}
			if (!write_address_info(fd, ai, i)) {
				return 0;
			}
		}
	}

	/*
	 * Force the information to be written to disk.
	 */
	
	fsync(fd);

	return 1;
}

#define BUF_CHUNK	1024

/*
 * read_record -- read a record from a transaction file
 * read_record reads the next record from the given file into the
 * supplied buffer.  If the record is bigger than the buffer,
 * realloc() is called to increase the size of the buffer.
 * dir and file are used for error reporting only.
 * Returns 1 if ok, 0 on EOF, and -1 on failure (system call error
 * or invalid or incomplete record).  On return, the header fields are
 * in host order.
 */

static int
read_record(int fd, char *dir, char *file, char **buf, int *buf_size)
{
	AasTransHeader *header;
	int n, rem, nsize;
	char *nbuf;

	if ((n = read(fd, *buf, sizeof(AasTransHeader))) == 0) {
		return 0;
	}
	else if (n == -1) {
		report(LOG_ERR, "Error reading %s/%s: %m", dir, file);
		return -1;
	}
	else if (n != sizeof(AasTransHeader)) {
		report(LOG_ERR, "Incomplete record in file %s/%s.", dir, file);
		return -1;
	}

	header = (AasTransHeader *) *buf;
	header->trans_type = ntohl(header->trans_type);
	header->trans_len = ntohl(header->trans_len);

	if (header->trans_len < sizeof(AasTransHeader)) {
		report(LOG_ERR, "Bad record length in file %s/%s.", dir, file);
		return -1;
	}

	if ((rem = header->trans_len - sizeof(AasTransHeader)) == 0) {
		return 1;
	}

	/*
	 * Increase the size of the buffer if necessary.
	 */
	
	if (header->trans_len > *buf_size) {
		nsize = ((header->trans_len + BUF_CHUNK - 1) / BUF_CHUNK)
			* BUF_CHUNK;
		if (!(nbuf = realloc(*buf, nsize))) {
			malloc_error("read_record");
			return -1;
		}
		*buf = nbuf;
		*buf_size = nsize;
	}

	/*
	 * Read in the rest of it.
	 */
	
	if ((n = read(fd, &(*buf)[sizeof(AasTransHeader)], rem)) == -1) {
		report(LOG_ERR, "Error reading %s/%s: %m", dir, file);
		return -1;
	}
	else if (n != rem) {
		report(LOG_ERR, "Incomplete record in file %s/%s.", dir, file);
		return -1;
	}

	return 1;
}

/*
 * addr_search -- generic address search routine
 * addr_search searches for the given address in the given list, which
 * is a list of pointers to AddrInfo structures.  A pointer to the AddrInfo
 * is returned if the address is found, or NULL if not.  If pos is non-null,
 * the position at which the address was found or would go is returned
 * in *pos.
 */

AddrInfo *
addr_search(AasAddr *addr, AddrInfo **list, int num,
	AtAddrCompareFunc compare, int *pos)
{
	int l, m, h, c;

	l = 0;
	h = num - 1;

	while (l <= h) {
		m = (l + h) / 2;
		if ((c = (*compare)(addr, &list[m]->addr)) == 0) {
			if (pos) {
				*pos = m;
			}
			return list[m];
		}
		if (c < 0) {
			h = m - 1;
		}
		else {
			l = m + 1;
		}
	}

	if (pos) {
		*pos = l;
	}
	return NULL;
}

/*
 * orphanage_search -- search for the address in the orphanage
 */

#define orphanage_search(addr_type, addr, pos) \
	addr_search((addr), orphanage[addr_type].list, \
		orphanage[addr_type].num, address_types[addr_type]->compare, \
		(pos))

/*
 * orphanage_add -- add an address to the orphanage
 * This function is called after an unsuccessful orphanage search.
 * The address is added at position pos.
 * Returns a pointer to the AddrInfo structure if successful, or NULL
 * if not (memory allocation failure).
 */

static AddrInfo *
orphanage_add(int addr_type, AasAddr *addr, int pos)
{
	AddrList *orp = &orphanage[addr_type];
	int i, nsize;
	AddrInfo *ai, **list;

	/*
	 * Grow the list if necessary.
	 */

	if (orp->num == orp->size) {
		nsize = orp->size + ORPHANAGE_CHUNK;
		if (orp->size == 0) {
			if (!(list = tbl_alloc(AddrInfo *, nsize))) {
				malloc_error("orphanage_add(1)");
				return NULL;
			}
		}
		else {
			if (!(list = tbl_grow(orp->list, AddrInfo *, nsize))) {
				malloc_error("orphanage_add(2)");
				return NULL;
			}
		}
		orp->list = list;
		orp->size = nsize;
	}
	else {
		list = orp->list;
	}

	/*
	 * Allocate an AddrInfo for the address (along with space for
	 * the address itself).
	 */
	
	if (!(ai = str_alloc(AddrInfo))) {
		malloc_error("orphanage_add(3)");
		return NULL;
	}
	if (!(ai->addr.addr = malloc(addr->len))) {
		free(ai);
		malloc_error("orphanage_add(4)");
		return NULL;
	}

	/*
	 * Initialize the address information.
	 */

	ai->inuse = 0;
	ai->freed = 0;
	ai->temp = 0;
	ai->disabled = 0;
	memcpy(ai->addr.addr, addr->addr, addr->len);
	ai->addr.len = addr->len;
	ai->service = NULL;
	ai->client_id.id = NULL;
	ai->client_id.len = 0;
	ai->alloc_time = 0;
	ai->lease_time = 0;
	ai->free_time = 0;
	ai->next = NULL;
	ai->prev = NULL;

	/*
	 * Put it in the list.
	 */

	for (i = orp->num; i > pos; i--) {
		list[i] = list[i - 1];
	}
	list[i] = ai;
	orp->num++;

	return ai;
}

static AddrInfo *
orphanage_free(int addr_type)
{
	AddrList *orp = &orphanage[addr_type];
	AddrInfo **list;
	int i;

	list = orp->list;
	for (i = 0; i < orp->num; i++) {
		free(list[i]->addr.addr);
		free(list[i]);
		list[i] = NULL;
	}
	orp->num = 0;
}

/*
 * proc_alloc -- process an allocation record
 * If the address exists in a pool, its allocation information is
 * set based on the record.  Otherwise, if the allocation is still
 * valid, the address is added to the orphanage.
 * Returns 1 if everything is ok, or 0 if the record was invalid or
 * some other error occurred (malloc failure).
 */

static int
proc_alloc_trans(AasTransAlloc *trans, char *dir, char *file)
{
	ulong len;
	char *p, *service;
	AasAddr addr;
	AasClientId client_id;
	AddrInfo *ai;
	AasTime now;
	AddrLoc *loc;
	AddressType *at;
	int pos;

	/*
	 * Put fields in host order.
	 */
	
	trans->alloc_time = ntohl(trans->alloc_time);
	trans->lease_time = ntohl(trans->lease_time);
	trans->addr_type = ntohs(trans->addr_type);
	trans->addr_len = ntohs(trans->addr_len);
	trans->service_len = ntohs(trans->service_len);
	trans->client_id_len = ntohs(trans->client_id_len);

	/*
	 * Verify length and make sure fields are set.
	 */
	
	len = sizeof(AasTransAlloc) + trans->addr_len + trans->service_len
		+ trans->client_id_len;
	
	if (len != trans->trans_len
	    || trans->addr_type < AAS_FIRST_ATYPE
	    || trans->addr_type >= AAS_NUM_ATYPES
	    || trans->addr_len == 0
	    || trans->service_len == 0 || trans->client_id_len == 0) {
		report(LOG_ERR, "Invalid ALLOC record in file %s/%s",
			dir, file);
		return 0;
	}

	at = address_types[trans->addr_type];

	p = (char *) (trans + 1);

	/*
	 * Extract variable fields.
	 */

	GET_ADDR(trans, addr_len, &addr, p);
	if (!CHECK_STRING(trans, service_len, p)) {
		report(LOG_ERR, "Invalid ALLOC record in file %s/%s",
			dir, file);
		return 0;
	}
	GET_STRING(trans, service_len, service, p);
	GET_CLIENT_ID(trans, client_id_len, &client_id, p);

	/*
	 * Make sure the address is valid.
	 */
	
	if (!(*at->validate)(&addr)) {
		report(LOG_ERR, "Invalid type %d address in file %s/%s",
			trans->addr_type, dir, file);
		return 0;
	}

	/*
	 * Find the address.
	 */
	
	if (!(loc = global_index_search(&addr, trans->addr_type, NULL))) {
		/*
		 * The address is not in any pool's configuration.
		 * If this allocation hasn't expired, add it to the orphanage.
		 */
		now = (AasTime) time(NULL);
		if (EXPIRED(trans->alloc_time, trans->lease_time, now)) {
			/*
			 * It's expired.
			 */
			return 1;
		}
		/*
		 * Find it in or add it to the orphanage.
		 */
		if (!(ai = orphanage_search(trans->addr_type, &addr, &pos))) {
			if (!(ai = orphanage_add(trans->addr_type, &addr,
			    pos))) {
				return 0;
			}
		}
	}
	else {
		ai = loc->ai;
	}

	ai->alloc_time = trans->alloc_time;
	ai->lease_time = trans->lease_time;
	ai->inuse = 1;
	ai->freed = 0;
	if (ai->service) {
		free(ai->service);
		ai->service = NULL;
	}
	if (ai->client_id.id) {
		free(ai->client_id.id);
		ai->client_id.id = NULL;
	}
	if (!(ai->service = strdup(service))
	    || !(ai->client_id.id = malloc(client_id.len))) {
		malloc_error("proc_alloc");
		return 0;
	}
	memcpy(ai->client_id.id, client_id.id, client_id.len);
	ai->client_id.len = client_id.len;

	return 1;
}

/*
 * proc_free_trans -- proccess a free record in a transaction file
 * Returns 1 if ok or 0 if the record is invalid.
 */

static int
proc_free_trans(AasTransFree *trans, char *dir, char *file)
{
	ulong len;
	char *p;
	AasAddr addr;
	AddrInfo *ai;
	AddressType *at;
	AddrLoc *loc;

	/*
	 * Put fields in host order.
	 */
	
	trans->free_time = ntohl(trans->free_time);
	trans->addr_type = ntohs(trans->addr_type);
	trans->addr_len = ntohs(trans->addr_len);

	at = address_types[trans->addr_type];

	/*
	 * Verify length and make sure fields are set.
	 */
	
	len = sizeof(AasTransFree) + trans->addr_len;
	
	if (len != trans->trans_len || trans->addr_len == 0) {
		report(LOG_ERR, "Invalid FREE record in file %s/%s", dir, file);
		return 0;
	}

	p = (char *) (trans + 1);

	/*
	 * Extract variable fields.
	 */

	GET_ADDR(trans, addr_len, &addr, p);

	/*
	 * Make sure the address is valid.
	 */
	
	if (!(*at->validate)(&addr)) {
		report(LOG_ERR, "Invalid type %d address in file %s/%s",
			trans->addr_type, dir, file);
		return 0;
	}

	if (!(loc = global_index_search(&addr, trans->addr_type, NULL))) {
		/*
		 * The address isn't in a pool, but it could be in the
		 * orphanage.
		 */
		if (!(ai = orphanage_search(trans->addr_type, &addr, NULL))) {
			/*
			 * We don't have any info on the address, so
			 * ignore this transaction.
			 */
			return 1;
		}
	}
	else {
		ai = loc->ai;
	}

	ai->inuse = 0;
	ai->freed = 1;
	ai->free_time = trans->free_time;

	return 1;
}

/*
 * proc_disable_trans -- proccess a free record in a transaction file
 * Returns 1 if ok or 0 if the record is invalid.
 */

static int
proc_disable_trans(AasTransDisable *trans, char *dir, char *file)
{
	ulong len;
	char *p;
	AasAddr addr;
	AddrInfo *ai;
	AddressType *at;
	AddrLoc *loc;

	/*
	 * Put fields in host order.
	 */
	
	trans->disable = ntohs(trans->disable);
	trans->addr_type = ntohs(trans->addr_type);
	trans->addr_len = ntohs(trans->addr_len);

	at = address_types[trans->addr_type];

	/*
	 * Verify length and make sure fields are set.
	 */
	
	len = sizeof(AasTransDisable) + trans->addr_len;
	
	if (len != trans->trans_len || trans->addr_len == 0
	    || (trans->disable != 0 && trans->disable != 1)) {
		report(LOG_ERR, "Invalid DISABLE record in file %s/%s",
			dir, file);
		return 0;
	}

	p = (char *) (trans + 1);

	/*
	 * Extract variable fields.
	 */

	GET_ADDR(trans, addr_len, &addr, p);

	/*
	 * Make sure the address is valid.
	 */
	
	if (!(*at->validate)(&addr)) {
		report(LOG_ERR, "Invalid type %d address in file %s/%s",
			trans->addr_type, dir, file);
		return 0;
	}

	if (!(loc = global_index_search(&addr, trans->addr_type, NULL))) {
		/*
		 * The address isn't in a pool, but it could be in the
		 * orphanage.
		 */
		if (!(ai = orphanage_search(trans->addr_type, &addr, NULL))) {
			/*
			 * We don't have any info on the address, so
			 * ignore this transaction.
			 */
			return 1;
		}
	}
	else {
		ai = loc->ai;
	}

	ai->disabled = trans->disable;

	return 1;
}

/*
 * replay_trans -- read a database file and "replay" the transactions
 * dir and file are used for error reporting only.  Returns 1 if ok, or 0 if an
 * error occurs, or the file is corrupted or invalid.
 */

static int
replay_trans(int fd, char *dir, char *file, AasTime *time_ret)
{
	char *buf;
	int buf_size, ret;
	AasTransHeader *header;
	AasTransInit init;

	/*
	 * Read and verify the INIT record.
	 */
	
	if ((ret = read(fd, &init, sizeof(init))) == -1) {
		report(LOG_ERR, "Read failed for %s/%s: %m", dir, file);
		return 0;
	}
	init.trans_type = ntohl(init.trans_type);
	init.trans_len = ntohl(init.trans_len);
	if (ret != sizeof(init)
	    || init.trans_type != AAS_TRANS_INIT
	    || init.trans_len != sizeof(init)) {
		report(LOG_ERR, "%s/%s is not an address database file.",
			dir, file);
		return 0;
	}

	if (time_ret) {
		*time_ret = ntohl(init.start_time);
	}

	/*
	 * Allocate a buffer to hold transactions.
	 */

	if (!(buf = malloc(BUF_CHUNK))) {
		malloc_error("set_state");
		return 0;
	}
	buf_size = BUF_CHUNK;

	/*
	 * Read and process the remaining records.
	 */
	
	while ((ret = read_record(fd, dir, file, &buf, &buf_size)) == 1) {
		header = (AasTransHeader *) buf;
		switch (header->trans_type) {
		case AAS_TRANS_ALLOC:
			if (!proc_alloc_trans((AasTransAlloc *) buf,
			    dir, file)) {
				reset_pools();
				free(buf);
				return 0;
			}
			break;
		case AAS_TRANS_FREE:
			if (!proc_free_trans((AasTransFree *) buf, dir, file)) {
				reset_pools();
				free(buf);
				return 0;
			}
			break;
		case AAS_TRANS_DISABLE:
			if (!proc_disable_trans((AasTransDisable *) buf,
			    dir, file)) {
				reset_pools();
				free(buf);
				return 0;
			}
			break;
		default:
			report(LOG_ERR, "Unexpected transaction type %d in file %s/%s",
				header->trans_type, dir, file);
			reset_pools();
			free(buf);
			return 0;
		}
	}

	free(buf);

	if (ret == 0) {
		return 1;
	}
	else if (ret == -1) {
		reset_pools();
		return 0;
	}
}

static AasTime compare_time;

/*
 * inuse_compare -- compare to addresses for the inuse list, based on time
 * to expiration.
 */

static int
inuse_compare(const void *v1, const void *v2)
{
	AddrInfo *a1 = *((AddrInfo **) v1);
	AddrInfo *a2 = *((AddrInfo **) v2);
	AasTime r1, r2;

	/*
	 * Do special checks for infinite leases.
	 */
	
	if (a1->lease_time == AAS_FOREVER) {
		if (a2->lease_time == AAS_FOREVER) {
			return 0;
		}
		else {
			return 1;
		}
	}
	else if (a2->lease_time == AAS_FOREVER) {
		return -1;
	}

	r1 = a1->lease_time - (compare_time - a1->alloc_time);
	r2 = a2->lease_time - (compare_time - a2->alloc_time);
	if (r1 < r2) {
		return -1;
	}
	else if (r1 > r2) {
		return 1;
	}
	else {
		return 0;
	}
}

/*
 * avail_compare -- compare to addresses for the avail list, based on time
 * since expiration.  We want the list to end up in LRU order, so
 * a greater time since expiration is considered a "lower" value.
 * So that addresses are handed out in order, if all other things are
 * equal, we compare the addresses themselves.
 */

static int
avail_compare(const void *v1, const void *v2)
{
	AddrInfo *a1 = *((AddrInfo **) v1);
	AddrInfo *a2 = *((AddrInfo **) v2);
	AasTime e1, e2;

	/*
	 * Addresses that have never been allocated are put at the beginning.
	 */
	
	if (!a1->service) {
		if (!a2->service) {
			return (*compare_func)(&a1->addr, &a2->addr);
		}
		else {
			return -1;
		}
	}
	else if (!a2->service) {
		return 1;
	}

	if (a1->freed) {
		e1 = compare_time - a1->free_time;
	}
	else {
		e1 = (compare_time - a1->alloc_time) - a1->lease_time;
	}
	if (a2->freed) {
		e2 = compare_time - a2->free_time;
	}
	else {
		e2 = (compare_time - a2->alloc_time) - a2->lease_time;
	}

	if (e1 > e2) {
		return -1;
	}
	else if (e1 < e2) {
		return 1;
	}
	else {
		return (*compare_func)(&a1->addr, &a2->addr);
	}
}

/*
 * setup_address_lists -- construct the inuse, and avail
 * lists and id2addr database for the pool.
 * Returns 1 if ok, or 0 if an error occurs.
 */

static int
setup_address_lists(Pool *pool)
{
	int i, num;
	AasTime now;
	AddrInfo *ai, *ai2, **list;
	int keybufsize, slen, ret;
	DBT key, data;
	DB *db;
	char *nbuf;
	AasAddr addr;
	int (*putfunc)(const DB *, DBT *, const DBT *, u_int);
	int (*getfunc)(const DB *, const DBT *, DBT *, u_int);

	/*
	 * Construct in-use list, available list, and id2addr database.  The
	 * in-use and available lists are constructed by creating a list of
	 * pointers and sorting it based on the appropriate properties
	 * (expiration time for the in-use list, and expiration or time freed
	 * for the available list).  The sorted array is then used to
	 * construct the linked list.
	 */
	
	if (!(list = tbl_alloc(AddrInfo *, pool->num_addrs))) {
		malloc_error("setup_address_list");
		return 0;
	}

	/*
	 * Find addresses that are in use.  Note that addresses marked as
	 * in use may have expired, so we have to check that.
	 */

	num = 0;
	now = (AasTime) time(NULL);
	for (i = 0; i < pool->num_addrs; i++) {
		ai = pool->addrs[i];
		if (!ai->inuse) {
			continue;
		}
		if (EXPIRED(ai->alloc_time, ai->lease_time, now)) {
			ai->inuse = 0;
			continue;
		}
		list[num++] = ai;
	}

	/*
	 * Sort the list.
	 */
	
	compare_time = now;
	qsort(list, num, sizeof(AddrInfo *), inuse_compare);

	/*
	 * Construct the in-use list.
	 */
	
	for (i = 0; i < num; i++) {
		LIST_ADD_BEFORE(&pool->inuse_head, list[i]);
	}

	pool->num_alloc = num;

	/*
	 * Do the same thing with available addresses.
	 */

	num = 0;
	for (i = 0; i < pool->num_addrs; i++) {
		ai = pool->addrs[i];
		if (!ai->inuse) {
			list[num++] = ai;
		}
	}

	/*
	 * Sort the list.
	 */
	
	compare_func = (*pool->atype->compare);
	qsort(list, num, sizeof(AddrInfo *), avail_compare);

	/*
	 * Construct the available list.
	 */
	
	for (i = 0; i < num; i++) {
		LIST_ADD_BEFORE(&pool->avail_head, list[i]);
	}

	free(list);

	/*
	 * Now do the id2addr database.  Any address that has ever been
	 * allocated has en entry in this database.
	 */
	
	if (!(pool->id2addr = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL))) {
		malloc_error("setup_address_lists: dopen");
		return 0;
	}
	keybufsize = 64;
	if (!(key.data = malloc(keybufsize))) {
		malloc_error("setup_address_lists: key");
		return 0;
	}
	db = pool->id2addr;
	putfunc = db->put;
	getfunc = db->get;
	for (i = 0; i < pool->num_addrs; i++) {
		ai = pool->addrs[i];
		if (!ai->service) {
			continue;
		}
		slen = strlen(ai->service) + 1;
		key.size = slen + ai->client_id.len;
		if (key.size > keybufsize) {
			keybufsize = key.size;
			if (!(nbuf = realloc(key.data, keybufsize))) {
				free(key.data);
				malloc_error("setup_address_lists: key(2)");
				return 0;
			}
			key.data = nbuf;
		}
		memcpy((char *) key.data, ai->service, slen);
		memcpy(((char *) key.data) + slen,
			ai->client_id.id, ai->client_id.len);
		data.data = ai->addr.addr;
		data.size = ai->addr.len;
		/*
		 * Put the info in the database but don't overwrite
		 * if there's already an entry for this client.  If there
		 * is an entry, look up the address and keep the one
		 * that was allocated most recently.
		 */
		if ((ret = (*putfunc)(db, &key, &data, R_NOOVERWRITE)) == -1) {
			free(key.data);
			malloc_error("setup_address_lists: put");
			return 0;
		}
		else if (ret == 1) {
			/*
			 * Get the entry for the address and see which one
			 * is more recent.
			 */
			if ((ret = (*getfunc)(db, &key, &data, 0)) == -1) {
				report(LOG_ERR,
					"setup_address_lists: get failed");
				free(key.data);
				return 0;
			}
			else if (ret == 0) {
				addr.addr = data.data;
				addr.len = data.size;
				now = (AasTime) time(NULL);
				if (!(ai2 = pool_search(pool, &addr, NULL))
				  || (now - ai2->alloc_time)
				  > (now - ai->alloc_time)) {
					data.data = ai->addr.addr;
					data.size = ai->addr.len;
					if ((*putfunc)(db, &key, &data, 0)
					    == -1) {
						malloc_error(
						  "setup_address_lists: put");
						free(key.data);
						return 0;
					}
				}
			}
		}
	}
	free(key.data);

	return 1;
}

/*
 * id_compare -- comparison function based on service/client ID
 */

static int
id_compare(char *svc1, AasClientId *id1, char *svc2, AasClientId *id2)
{
	int c, len1, len2, len;

	if ((c = strcmp(svc1, svc2)) == 0) {
		len1 = id1->len;
		len2 = id2->len;
		if (len1 > len2) {
			len = len2;
		}
		else {
			len = len1;
		}
		if ((c = memcmp(id1->id, id2->id, len))
		    == 0) {
			return len1 - len2;
		}
	}

	return c;
}

/*
 * id_search -- search for allocation info based on id
 * id_search looks up the given service/client in the id2addr database.
 * A pointer to the AddrInfo structure is returned in *ainfo_ret.
 * *ainfo_ret is set to NULL if no matching entry is found.
 * id_search allocates a buffer to build a key to access the
 * entry in the database.  If this allocation fails, or the database get
 * call fails, 0 is returned.  Otherwise, 1 is returned.
 */

int
id_search(Pool *pool, char *service, AasClientId *client_id,
	AddrInfo **ainfo_ret)
{
	DBT key, data;
	int slen, ret;
	AasAddr addr;

	*ainfo_ret = NULL;
	slen = strlen(service) + 1;
	key.size = slen + client_id->len;
	if (!(key.data = malloc(key.size))) {
		malloc_error("id_search");
		return 0;
	}
	memcpy((char *) key.data, service, slen);
	memcpy(((char *) key.data) + slen, client_id->id, client_id->len);
	if ((ret = (*pool->id2addr->get)(pool->id2addr, &key, &data, 0)) == 1) {
		free(key.data);
		return 1;
	}
	else if (ret == -1) {
		report(LOG_ERR, "id_search: get failed");
		free(key.data);
		return 0;
	}
	free(key.data);
	addr.addr = data.data;
	addr.len = data.size;
	*ainfo_ret = pool_search(pool, &addr, NULL);
	return 1;
}

/*
 * allocate -- allocate the given address from the given pool
 * to the given service/client for the given period.
 * This function does the necessary database manipulation to allocate
 * an address.
 * This function may fail if unable to allocate memory
 * or if the recording of the transaction fails for some reason.
 * Return value is 1 if ok or 0 if a problem occurred.
 */

static int
allocate(Pool *pool, AddrInfo *ainfo, AasTime alloc_time,
	AasTime lease_time, char *service,
	AasClientId *client_id, int temp)
{
	char *service_save;
	void *id_save;
	AddrInfo *ai;
	AasTime age;
	int slen;
	DBT key, data;

	/*
	 * Allocate the memory we'll need before we do anything else.
	 * Before allocating space for the service & client ID, we check
	 * to see if they're the same as those already associated with this
	 * address.
	 */

	service_save = NULL;
	id_save = NULL;
	key.data = NULL;

	if ((!ainfo->service || strcmp(service, ainfo->service) != 0)
	    && !(service_save = strdup(service))) {
		goto nomem;
	}

	if (!ainfo->client_id.id || client_id->len != ainfo->client_id.len ||
	    memcmp(client_id->id, ainfo->client_id.id, client_id->len) != 0) {
		if (!(id_save = malloc(client_id->len))) {
			goto nomem;
		}
		memcpy(id_save, client_id->id, client_id->len);
	}

	/*
	 * Allocate a buffer to construct a key for the id2addr database.
	 */
	
	slen = strlen(service) + 1;
	key.size = slen + client_id->len;
	if (!(key.data = malloc(key.size))) {
		goto nomem;
	}

	/*
	 * Add the ID/address pair to the id2addr database.
	 * This is done before writing the transaction to disk because
	 * this operation could fail (if no memory is available).
	 * If writing the transaction fails, it won't hurt to leave this
	 * entry in the database.
	 */
	
	memcpy((char *) key.data, service, slen);
	memcpy(((char *) key.data) + slen, client_id->id, client_id->len);
	data.data = ainfo->addr.addr;
	data.size = ainfo->addr.len;
	if ((*pool->id2addr->put)(pool->id2addr, &key, &data, 0) == -1) {
		goto nomem;
	}
	free(key.data);
	key.data = NULL;

	/*
	 * Record the transaction in the file unless this is a temporary
	 * allocation.
	 */
	
	if (!temp
	    && !record_allocation(db_fd, &ainfo->addr, pool->atype->addr_type,
	    alloc_time, lease_time, service, client_id)) {
		goto error;
	}

	/*
	 * Set up the address info and put it on the in use list.
	 * Note: this code has to work whether the address is currently
	 * in use or available.
	 */
	
	if (!ainfo->inuse) {
		ainfo->inuse = 1;
		ainfo->freed = 0;
		pool->num_alloc++;
	}

	if (temp) {
		ainfo->temp = 1;
	}
	else {
		ainfo->temp = 0;
	}
	
	if (service_save) {
		if (ainfo->service) {
			free(ainfo->service);
		}
		ainfo->service = service_save;
	}

	if (id_save) {
		if (ainfo->client_id.id) {
			free(ainfo->client_id.id);
		}
		ainfo->client_id.id = id_save;
		ainfo->client_id.len = client_id->len;
	}

	ainfo->alloc_time = alloc_time;
	ainfo->lease_time = lease_time;
	
	LIST_DELETE(ainfo);

	/*
	 * Put the address on the in use list, which is kept in order of
	 * expiration time.
	 */

	if (lease_time == AAS_FOREVER) {
		LIST_ADD_BEFORE(&pool->inuse_head, ainfo);
	}
	else {
		for (ai = pool->inuse_head.prev; ai != &pool->inuse_head;
		     ai = ai->prev) {
			if (ai->lease_time == AAS_FOREVER) {
				continue;
			}
			age = alloc_time - ai->alloc_time;
			/*
			 * If we have reached the expired addresses,
			 * stop and add this one after.
			 */
			if (age >= ai->lease_time) {
				break;
			}
			/*
			 * If the address we're adding expires no earlier
			 * than the current one, stop.
			 */
			if (lease_time >= ai->lease_time - age) {
				break;
			}
		}
		LIST_ADD_AFTER(ai, ainfo);
	}

	/*
	 * Compress the pool's database file if necessary.
	 */

	compress_db();

	return 1;

nomem:
	malloc_error("allocate");

error:
	if (service_save) {
		free(service_save);
	}
	if (id_save) {
		free(id_save);
	}
	if (key.data) {
		free(key.data);
	}

	return 0;
}

/*
 * free_it -- free an address
 */

static void
free_it(Pool *pool, AddrInfo *ainfo, AasTime free_time)
{
	/*
	 * Remove it from the in use list and add it to the end of the
	 * available list.  Set fields appropriately.  Also decrement
	 * count of allocated addresses in the pool.
	 */

	LIST_DELETE(ainfo);
	ainfo->inuse = 0;
	ainfo->free_time = free_time;
	ainfo->freed = 1;
	LIST_ADD_BEFORE(&pool->avail_head, ainfo);
	pool->num_alloc--;
}

/*
 * expire_addresses -- make expired addresses available.
 */

void
expire_addresses(Pool *pool)
{
	AasTime now;
	AddrInfo *ai;

	now = (AasTime) time(NULL);
	for (ai = pool->inuse_head.next; ai != &pool->inuse_head;
	     ai = pool->inuse_head.next) {
		if (EXPIRED(ai->alloc_time, ai->lease_time, now)) {
			LIST_DELETE(ai);
			ai->inuse = 0;
			LIST_ADD_BEFORE(&pool->avail_head, ai);
			pool->num_alloc--;
		}
		else {
			break;
		}
	}
}

/*
 * free_pool -- free all malloc'ed memory associated with a pool
 * This can be called while the pool is under construction -- pointers
 * to malloc'ed areas are checked for nullness before freeing.
 */

static void
free_pool(Pool *pool)
{
	AddrInfo *ai;
	int i;

	if (pool->name) {
		free(pool->name);
	}

	if (pool->addrs) {
		for (i = 0; i < pool->num_addrs; i++) {
			ai = pool->addrs[i];
			if (ai->addr.addr) {
				free(ai->addr.addr);
			}
			if (ai->service) {
				free(ai->service);
			}
			if (ai->client_id.id) {
				free(ai->client_id.id);
			}
			free(ai);
		}
		free(pool->addrs);
	}

	if (pool->id2addr) {
		(*pool->id2addr->close)(pool->id2addr);
	}

	free(pool);
}

/*
 * init_from_checkpoint -- read allocation data from the most
 * recent checkpoint file.
 * Returns 1 if a valid file was found, or 0 if not.
 */

static int
init_from_checkpoint(void)
{
	char **list;
	int fd, num, i;
	AasTime cp_time;
	char timebuf[64];
	struct tm *tm;
	time_t t;

	/*
	 * Go to the checkpoint directory and get the list of
	 * available checkpoint files.
	 */

	if (chdir(cp_dir) == -1) {
		report(LOG_ERR,
			"Unable to change to checkpoint diretory %s: %m",
			cp_dir);
		return 0;
	}

	if (!get_checkpoint_list(&list, &num)) {
		return 0;
	}

	/*
	 * Go through them from newest to oldest until we read one
	 * successfully.
	 */
	
	for (i = 0; i < num; i++) {
		if ((fd = open(list[i], O_RDONLY)) == -1) {
			report(LOG_ERR,
				"Unable to open checkpoint file %s: %m",
				cp_dir, list[i]);
			continue;
		}
		if (replay_trans(fd, cp_dir, list[i], &cp_time)) {
			close(fd);
			t = (time_t) cp_time;
			tm = localtime(&t);
			strftime(timebuf, sizeof(timebuf), "%x %X", tm);
			report(LOG_NOTICE,
				"Using data from checkpoint of %s.",
					timebuf);
			free_checkpoint_list(list, num);
			return 1;
		}
	}

	free_checkpoint_list(list, num);
	return 0;
}

/*
 * init_db -- initialize the address database from
 * disk, and start a new disk file if necessary.
 * Returns 1 on success or 0 on failure.
 */

static int
init_db(void)
{
	int fd, flags;
	AasTime now;

	/*
	 * Go to the database directory.
	 */
	
	if (chdir(db_dir) == -1) {
		report(LOG_ERR, "Unable to change to directory %s: %m",
			db_dir);
		return 0;
	}

	/*
	 * Try to open the live database file.
	 */
	
	if ((fd = open(AAS_TRANS_FILE, O_RDWR|O_SYNC)) == -1) {
		/*
		 * If we get any error other than the file not
		 * being there, fail.  We don't know what's
		 * going on if there is a transaction file there and
		 * we can't open it.
		 */
		
		if (errno != ENOENT) {
			report(LOG_ERR,
				"Unable to open database file %s/%s: %m",
				db_dir, AAS_TRANS_FILE);
			return 0;
		}

		/*
		 * See if old_trans is there.  This means that
		 * something bad happen when making a compressed
		 * database between unlinking the current trans
		 * and linking trans to the newly-created file.
		 * In this case, old_trans is the live database.
		 */
		if ((fd = open(AAS_OLD_TRANS_FILE, O_RDWR|O_SYNC)) != -1) {
			/*
			 * Link trans to old_trans & remove
			 * old_trans.
			 */
			if (link(AAS_OLD_TRANS_FILE, AAS_TRANS_FILE) == -1) {
				report(LOG_ERR,
					"Unable to create link %s/%s: %m",
					db_dir, AAS_TRANS_FILE);
				close(fd);
				return 0;
			}
			else {
				(void) unlink(AAS_OLD_TRANS_FILE);
			}
		}
		else if (errno != ENOENT) {
			/*
			 * Same as above -- this is equivalent to the live
			 * transaction file.  If it's there and we can't
			 * open it, consider it an error.
			 */
			report(LOG_ERR,
				"Unable to open database file %s/%s: %m",
				db_dir, AAS_OLD_TRANS_FILE);
			return 0;
		}
	}

	if (fd == -1 || !replay_trans(fd, db_dir, AAS_TRANS_FILE, NULL)) {
		if (fd != -1) {
			close(fd);
		}

		/*
		 * Try to read state from a checkpoint file.
		 */
		
		if (!init_from_checkpoint()) {
			report(LOG_WARNING,
			  "No address allocation data available on disk.");
		}

		/*
		 * init_from_checkpoint changed directories on us,
		 * so we need to change back.
		 */

		if (chdir(db_dir) == -1) {
			report(LOG_ERR, "Unable to change to directory %s: %m",
				db_dir);
			return 0;
		}

		/*
		 * Create a new live database file.  We write into
		 * a different file and the move it to the real name,
		 * which guarantees that the real file is complete.
		 */

		(void) unlink(AAS_NEW_TRANS_FILE);
		if ((fd = open(AAS_NEW_TRANS_FILE,
		    O_RDWR|O_CREAT|O_EXCL, 0600)) == -1) {
			report(LOG_ERR, "Unable to create transaction file %s/%s: %m",
				db_dir, AAS_NEW_TRANS_FILE);
			return 0;
		}
		now = (AasTime) time(NULL);
		if (!write_state(fd, now)) {
			close(fd);
			(void) unlink(AAS_NEW_TRANS_FILE);
			return 0;
		}

		(void) unlink(AAS_TRANS_FILE);
		if (link(AAS_NEW_TRANS_FILE, AAS_TRANS_FILE) == -1) {
			report(LOG_ERR, "Unable to link transaction file %s/%s: %m",
				db_dir, AAS_TRANS_FILE);
			return 0;
		}
		(void) unlink(AAS_NEW_TRANS_FILE);

		/*
		 * Set synchronous write mode on the file.
		 */
		
		if (fcntl(fd, F_GETFL, &flags) == -1
		    || fcntl(fd, F_SETFL, flags | O_SYNC) == -1) {
			report(LOG_ERR, "fcntl failed for %s: %m",
				db_dir, AAS_TRANS_FILE);
			close(fd);
			return 0;
		}
	}

	db_fd = fd;

	return 1;
}

/*
 * global_index_search -- search for an address in the global index
 * If the address is found, a pointer to an AddrLoc structure is returned.
 * If pos is non-null, the index at which the address was found (or the
 * location where it would go) is returned in *pos.
 */

static AddrLoc *
global_index_search(AasAddr *addr, int addr_type, int *pos)
{
	int l, m, h, c;
	AtAddrCompareFunc compare = address_types[addr_type]->compare;
	AddrLoc *index = addr_index[addr_type].index;

	l = 0;
	h = addr_index[addr_type].num - 1;

	while (l <= h) {
		m = (l + h) / 2;
		if ((c = (*compare)(addr, &index[m].ai->addr)) == 0) {
			if (pos) {
				*pos = m;
			}
			return &index[m];
		}
		if (c < 0) {
			h = m - 1;
		}
		else {
			l = m + 1;
		}
	}

	if (pos) {
		*pos = l;
	}
	return NULL;
}

/*
 * global_index_add -- add an address to the global index
 * The given address is added at position pos.
 * Returns 1 if ok, or 0 if unable to allocate more memory.
 */

static int
global_index_add(Pool *pool, AddrInfo *ainfo, int pos)
{
	int i, nsize;
	AddrIndex *aindex;
	AddrLoc *index;

	aindex = &addr_index[pool->atype->addr_type];

	/*
	 * Grow the list if necesary.
	 */

	if (aindex->num == aindex->size) {
		nsize = aindex->size + ADDR_INDEX_CHUNK;
		if (!(index = tbl_grow(aindex->index, AddrLoc, nsize))) {
			return 0;
		}
		aindex->index = index;
		aindex->size = nsize;
	}
	else {
		index = aindex->index;
	}

	for (i = aindex->num; i > pos; i--) {
		index[i] = index[i - 1];
	}
	index[pos].pool = pool;
	index[pos].ai = ainfo;
	aindex->num++;

	return 1;
}

/*
 * addr_compare -- qsort compare function for sorting addrs list
 */

static int
addr_compare(const void *v1, const void *v2)
{
	AddrInfo *a1 = *((AddrInfo **) v1);
	AddrInfo *a2 = *((AddrInfo **) v2);

	return (*compare_func)(&a1->addr, &a2->addr);
}

/*
 * setup_pool -- set up data structure for a pool
 * The pool structure is set up and added to the list of pools.  The
 * addresses are then read from the configuration file.  A pointer to
 * the pool structure is returned in *pool_ret.  The function return value
 * is 1 if ok or 0 if an error occurs.
 */

static int
setup_pool(char *name, AddressType *at, FILE *cf, Pool **pool_ret)
{
	Pool *pool;
	AasAddr addr;
	AddrInfo *ainfo, *alist, **alink;
	int i, ret, pos;
	AddrLoc *loc;
	int parse_init;

	pool = NULL;
	alist = NULL;

	if (!(pool = str_alloc(Pool))) {
		goto nomem;
	}

	/*
	 * Initialize fields.
	 */

	pool->atype = at;
	pool->num_addrs = 0;
	pool->num_alloc = 0;
	LIST_INIT(&pool->avail_head);
	LIST_INIT(&pool->inuse_head);
	pool->addrs = NULL;
	pool->id2addr = NULL;

	if (!(pool->name = strdup(name))) {
		goto nomem;
	}

	/*
	 * Read addresses and create AddrInfo structures.
	 */
	
	alist = NULL;
	alink = &alist;

	parse_init = 1;

	while ((ret = (*at->parse)(cf, &addr, &config_line, &parse_init))
	    == 1) {
		/*
		 * See if the address is already in another pool.
		 */
		if (loc = global_index_search(&addr,
		    at->addr_type, &pos)) {
			report(LOG_WARNING,
			  "Pool %s: Skipping address %s (already exists in pool %s)",
				pool->name, (*at->show)(&addr),
				loc->pool->name);
			continue;
		}
		if (!(ainfo = str_alloc(AddrInfo))) {
			goto nomem;
		}
		if (!(ainfo->addr.addr = malloc(addr.len))) {
			free(ainfo);
			goto nomem;
		}
		ainfo->inuse = 0;
		ainfo->freed = 0;
		ainfo->temp = 0;
		ainfo->disabled = 0;
		memcpy(ainfo->addr.addr, addr.addr, addr.len);
		ainfo->addr.len = addr.len;
		ainfo->service = NULL;
		ainfo->client_id.id = NULL;
		ainfo->client_id.len = 0;
		ainfo->alloc_time = 0;
		ainfo->lease_time = 0;
		ainfo->free_time = 0;
		ainfo->next = NULL;
		ainfo->prev = NULL;
		*alink = ainfo;
		alink = &ainfo->next;
		pool->num_addrs++;

		/*
		 * Add it into the global index.
		 */
		
		if (!global_index_add(pool, ainfo, pos)) {
			goto nomem;
		}
	}

	/*
	 * Check for error during address parsing.
	 */
	
	if (ret == -1) {
		goto error;
	}

	/*
	 * Now create the sorted list.
	 */

	if (!(pool->addrs = tbl_alloc(AddrInfo *, pool->num_addrs))) {
		goto nomem;
	}

	i = 0;
	for (ainfo = alist; ainfo; ainfo = ainfo->next) {
		pool->addrs[i++] = ainfo;
	}
	alist = NULL;

	compare_func = pool->atype->compare;
	qsort(pool->addrs, pool->num_addrs, sizeof(AddrInfo *), addr_compare);

	*pool_ret = pool;
	return 1;

nomem:
	malloc_error("setup_pool");

error:
	/*
	 * Free whatever we did manage to allocate.
	 */

	if (pool) {
		free_pool(pool);
	}

	for (ainfo = alist; ainfo; ainfo = ainfo->next) {
		free(ainfo->addr.addr);
		free(ainfo);
	}
		
	return 0;
}

/*
 * reset_pool -- reset a pool to its initial state
 * The given pool is reset to its initial state, i.e., all addresses
 * available.
 */

static void
reset_pool(Pool *pool)
{
	AddrInfo *ai;
	int i;

	pool->num_alloc = 0;
	LIST_INIT(&pool->inuse_head);
	LIST_INIT(&pool->avail_head);

	for (i = 0; i < pool->num_addrs; i++) {
		ai = pool->addrs[i];
		ai->inuse = 0;
		ai->freed = 0;
		ai->temp = 0;
		ai->disabled = 0;
		if (ai->service) {
			free(ai->service);
			ai->service = NULL;
		}
		if (ai->client_id.id) {
			free(ai->client_id.id);
			ai->client_id.id = NULL;
		}
		ai->client_id.len = 0;
		ai->alloc_time = 0;
		ai->lease_time = 0;
		ai->free_time = 0;
	}
}

/*
 * reset_pools -- reset all pools to their initial states
 */

static void
reset_pools(void)
{
	Pool *pool;

	for (pool = pools; pool; pool = pool->next) {
		reset_pool(pool);
	}
}

#define AAS_CONFIG_MAX_ARGS	64

/*
 * parse_line -- parse a line from the configuration file
 * The next non-comment line in the config file is parsed into space- or
 * tab-separated tokens.  The first token must start at the beginning of the
 * line.  A list of tokens is returned in *argvp, and the number of tokens is
 * returned in *argcp.  If a token begins with a '#', the rest of the line
 * from that point is ignored.
 * The return value is 1 if ok, 0 on EOF, or -1 if an error occurred.
 */

static int
parse_line(int *argcp, char ***argvp)
{
	int len, c, blen, argc;
	char *arg, *ret;
	static char linebuf[1024];
	static char *argv[AAS_CONFIG_MAX_ARGS];

	/*
	 * Find the next non-comment & non-blank line.
	 */
	
	while (ret = fgets(linebuf, sizeof(linebuf), config_fp)) {
		config_line++;
		len = strlen(linebuf);
		if (linebuf[len - 1] != '\n') {
			report(LOG_ERR, "%s line %d: line too long.",
				config_file, config_line);
			/*
			 * Skip the rest of the line.
			 */
			while ((c = getc(config_fp)) != '\n' && c != EOF)
				;
			return -1;
		}
		/*
		 * Remove the newline.
		 */
		linebuf[len - 1] = '\0';
		len--;
		/*
		 * Check white space at the beginning.  If the entire
		 * line is blank or the first non-white character is '#',
		 * skip the line.  Otherwise, there must be no blanks at
		 * the beginning of the line.
		 */
		blen = strspn(linebuf, " \t");
		if ((c = linebuf[blen]) == '\0' || c == '#') {
			continue;
		}
		else if (blen > 0) {
			report(LOG_ERR, "%s line %d: line starts with spaces.",
				config_file, config_line);
			return -1;
		}
		/*
		 * Found a line with something on it.
		 */
		break;
	}

	/*
	 * Check for EOF.
	 */

	if (!ret) {
		return 0;
	}

	/*
	 * Separate the line into tokens.
	 */
	argv[0] = strtok(linebuf, " \t");
	argc = 1;
	while (arg = strtok(NULL, " \t")) {
		if (arg[0] == '#') {
			break;
		}
		if (argc == AAS_CONFIG_MAX_ARGS) {
			report(LOG_ERR, "%s line %d: too many tokens.",
				config_file, config_line);
			return -1;
		}
		argv[argc++] = arg;
	}

	*argcp = argc;
	*argvp = argv;
	return 1;
}

/*
 * The following functions handle the various configuration file entries.
 * They return 1 if ok or 0 if error.
 */

static int
config_path(int argc, char *argv[], void *param)
{
	char *p;

	if (argc != 2) {
		report(LOG_ERR, "%s line %d: %s requires 1 argument.",
			config_file, config_line, argv[0]);
		return 0;
	}

	/*
	 * Make sure it's an absolute path.
	 */
	
	if (argv[1][0] != '/') {
		report(LOG_ERR, "%s line %d: an absolute path is required.",
			config_file, config_line);
		return 0;
	}

	if (!(p = strdup(argv[1]))) {
		report(LOG_ERR, "config_path: unable to allocate memory: %m");
		return 0;
	}

	*((char **) param) = p;

	return 1;
}

/*
 * A special function is required to set the database directory because
 * the database directory cannot be changed once the server is running.
 */

static int
config_db_dir(int argc, char *argv[], void *param)
{
	char *ndir;

	if (!config_path(argc, argv, &ndir)) {
		return 0;
	}

	if (!db_dir) {
		db_dir = ndir;
		return 1;
	}

	if (strcmp(db_dir, ndir) == 0) {
		free(ndir);
		return 1;
	}

	report(LOG_ERR, "The database directory cannot be changed by a reconfigure operation.");
	return 0;
}

static int
config_nonneg_int(int argc, char *argv[], void *param)
{
	int val;
	char *ptr;

	if (argc != 2) {
		report(LOG_ERR, "%s line %d: %s requires 1 argument.",
			config_file, config_line, argv[0]);
		return 0;
	}

	/*
	 * Get the value.  Make sure there is no additional crud in
	 * the string by checking *ptr (if it doesn't point to the end
	 * of the string, there was extra stuff).
	 */

	val = strtol(argv[1], &ptr, 10);
	if (*ptr) {
		report(LOG_ERR, "%s line %d: %s invalid value.",
			config_file, config_line, argv[0]);
		return 0;
	}

	*((int *) param) = val;
	return 1;
}

static int
config_size(int argc, char *argv[], void *param)
{
	int val;
	char *ptr;

	if (argc != 2) {
		report(LOG_ERR, "%s line %d: %s requires 1 argument.",
			config_file, config_line, argv[0]);
		return 0;
	}

	/*
	 * Get the value.  The number can be terminated by 'k', 'K',
	 * 'm', or 'M' to convert to kbytes or Mbytes.
	 */

	val = strtol(argv[1], &ptr, 10);
	switch (*ptr) {
	case 'k':
	case 'K':
		if (*(ptr + 1)) {
			report(LOG_ERR, "%s line %d: invalid value.",
				config_file, config_line, argv[0]);
			return 0;
		}
		val *= 1024;
		break;
	case 'm':
	case 'M':
		if (*(ptr + 1)) {
			report(LOG_ERR, "%s line %d: invalid value.",
				config_file, config_line, argv[0]);
			return 0;
		}
		val *= (1024 * 1024);
		break;
	case '\0':
		break;
	default:
		report(LOG_ERR, "%s line %d: %s invalid value.",
			config_file, config_line, argv[0]);
		return 0;
	}

	*((int *) param) = val;
	return 1;
}

static int
config_password(int argc, char *argv[], void *param)
{
	Password *pwd;

	if (argc != 2) {
		report(LOG_ERR, "%s line %d: %s requires 1 argument.",
			config_file, config_line, argv[0]);
		return 0;
	}

	if (!(pwd = str_alloc(Password))) {
		malloc_error("config_password(1)");
		return 0;
	}
	if (!(pwd->password = strdup(argv[1]))) {
		free(pwd);
		malloc_error("config_password(2)");
		return 0;
	}
	pwd->len = strlen(argv[1]);
	pwd->next = NULL;
	*password_link = pwd;
	password_link = &pwd->next;

	return 1;
}

static int
config_pool(int argc, char *argv[], void *param)
{

	Pool *pool;
	AddressType *at;
	int i;
	char *p;

	if (argc != 3 || strcmp(argv[2], "{") != 0) {
		report(LOG_ERR, "%s line %d: %s requires 1 argument followed by a brace.",
			config_file, config_line, argv[0]);
		return 0;
	}

	if (!(p = strchr(argv[1], ':'))) {
		report(LOG_ERR, "Invalid pool specification \"%s\".",
			argv[1]);
		return 0;
	}
	*p++ = '\0';
	if (debug > 1) {
		report(LOG_INFO, "Pool %s, type %s", argv[1], p);
	}
	/*
	 * Look up address type.
	 */
	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		at = address_types[i];
		if (strcmp(at->name, p) == 0) {
			break;
		}
	}
	if (i == AAS_NUM_ATYPES) {
		report(LOG_ERR, "Unknown address type \"%s\".", p);
		return 0;
	}

	/*
	 * Set up the pool data structures (includes reading in
	 * the pool's config info).
	 */

	if (!setup_pool(argv[1], at, config_fp, &pool)) {
		return 0;
	}

	/*
	 * Add the pool to the list.
	 */
	pool->next = NULL;
	*pool_link = pool;
	pool_link = &pool->next;
}

/*
 * The following table maps configuration file keywords to the
 * functions that process them.
 */

static struct {
	char *keyword;
	int (*func)(int, char **, void *);
	void *param;
} keywords[] = {
	"database_dir", config_db_dir, NULL,
	"checkpoint_dir", config_path, &cp_dir,
	"checkpoint_intvl", config_nonneg_int, &cp_intvl,
	"num_checkpoints", config_nonneg_int, &cp_num,
	"max_db_size", config_size, &db_max,
	"password", config_password, NULL,
	"pool", config_pool, NULL,
};
#define NUM_KEYWORDS	(sizeof(keywords) / sizeof(keywords[0]))

/*
 * config -- read configuration file and set up configuration
 * Returns 1 if ok or 0 if something went wrong.
 */

int
config(void)
{
	int i, ret;
	Pool *pool, *npool;
	struct stat st;
	int argc;
	char **argv;
	int no_db_dir = !db_dir;
	Password *pwd, *npwd, *opasswords;

	/* 
	 * Initialize configuration params.
	 */

	cp_dir = NULL;
	cp_intvl = AAS_DFLT_CP_INTVL;
	cp_num = AAS_DFLT_CP_NUM;
	db_max = AAS_DFLT_DB_MAX_SIZE;

	/*
	 * Save the old passwords in case configuration fails.
	 */

	opasswords = passwords;
	passwords = NULL;
	password_link = &passwords;

	pools = NULL;
	pool_link = &pools;

	db_fd = -1;
	config_fp = NULL;

	/*
	 * Allocate space for a global index of addresses.  There is a
	 * separate index for each address type.
	 */
	
	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		addr_index[i].index = NULL;
	}

	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		if (!(addr_index[i].index =
		    tbl_alloc(AddrLoc, ADDR_INDEX_CHUNK))) {
			goto nomem;
		}
		addr_index[i].size = ADDR_INDEX_CHUNK;
		addr_index[i].num = 0;
	}

	/*
	 * Open and process config file.
	 */

	if (!(config_fp = fopen(config_file, "r"))) {
		report(LOG_ERR, "Unable to open configuration file %s",
			config_file);
		goto error;
	}

	config_line = 0;

	while ((ret = parse_line(&argc, &argv)) == 1) {
		for (i = 0; i < NUM_KEYWORDS; i++) {
			if (strcmp(argv[0], keywords[i].keyword) == 0) {
				break;
			}
		}
		if (i == NUM_KEYWORDS) {
			report(LOG_ERR, "%s line %d: unknown keyword \"%s\".",
				config_file, config_line, argv[0]);
			goto error;
		}
		if (!(*keywords[i].func)(argc, argv, keywords[i].param)) {
			goto error;
		}
	}

	if (ret == -1) {
		goto error;
	}

	fclose(config_fp);
	config_fp = NULL;

	/*
	 * Since db_dir is not allowed to change, reconfig() won't try
	 * to free it, so we don't have to put it in malloc'ed space.
	 * Since cp_dir can change, we put it in malloc space so that
	 * reconfig() can free it (when directories come out of the config
	 * file, they end up in malloc'ed space).
	 */

	if (!db_dir) {
		db_dir = _PATH_DFLT_DB_DIR;
	}

	if (!cp_dir) {
		if (!(cp_dir = strdup(_PATH_DFLT_CP_DIR))) {
			goto nomem;
		}
	}

	/*
	 * Create database and checkpoint directories if they don't
	 * already exist.
	 */
	
	if (stat(db_dir, &st) == -1) {
		if (mkdir(db_dir, 0700) == -1) {
			report(LOG_ERR,
			  "Unable to create database directory %s: %m", db_dir);
			goto error;
		}
	}
	else if (!S_ISDIR(st.st_mode)) {
		report(LOG_ERR, "Database directory %s is not a directory.",
			db_dir);
		goto error;
	}

	if (stat(cp_dir, &st) == -1) {
		if (mkdir(cp_dir, 0700) == -1) {
			report(LOG_ERR, "Unable to create checkpoint directory %s: %m",
				cp_dir);
			goto error;
		}
	}
	else if (!S_ISDIR(st.st_mode)) {
		report(LOG_ERR, "Database checkpoint %s is not a directory.",
			cp_dir);
		goto error;
	}

	/*
	 * Initialize orphanage.
	 */
	
	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		orphanage[i].list = NULL;
		orphanage[i].num = 0;
		orphanage[i].size = 0;
	}

	/*
	 * Initialize allocation data from disk.
	 */
	
	if (!init_db()) {
		goto error;
	}
	
	/*
	 * Set up each pool's address lists.
	 */
	
	for (pool = pools; pool; pool = pool->next) {
		if (!setup_address_lists(pool)) {
			goto error;
		}
	}

	/*
	 * We don't need the address indexes anymore.
	 */

	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		if (addr_index[i].index) {
			free(addr_index[i].index);
		}
	}

	/*
	 * Can get rid of the old passwords now.
	 */
	
	for (pwd = opasswords; pwd; pwd = npwd) {
		npwd = pwd->next;
		free(pwd->password);
		free(pwd);
	}

	report(LOG_INFO, "Configuration complete; ready.");

	return 1;

nomem:
	malloc_error("config");

error:
	/*
	 * Clean up.  Close the config file & database file and free storage.
	 */

	if (config_fp) {
		fclose(config_fp);
		config_fp = NULL;
	}

	if (db_fd != -1) {
		close(db_fd);
		db_fd = -1;
	}

	if (no_db_dir) {
		db_dir = NULL;
	}

	if (cp_dir) {
		free(cp_dir);
		cp_dir = NULL;
	}

	for (pool = pools; pool; pool = npool) {
		npool = pool->next;
		/*
		 * Free the pool's storage.
		 */
		free_pool(pool);
	}
	pools = NULL;

	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		if (addr_index[i].index) {
			free(addr_index[i].index);
		}
		if (orphanage[i].list) {
			free(orphanage[i].list);
		}
	}

	/*
	 * Free the new list of passwords and go back to the old list.
	 */

	for (pwd = passwords; pwd; pwd = npwd) {
		npwd = pwd->next;
		free(pwd->password);
		free(pwd);
	}

	passwords = opasswords;

	return 0;
}

/*
 * alloc_addr -- allocate an address based on request parameters
 * This funciton is called from the protocol code to allocate an
 * address.  Parameters are extracted from the request message and
 * passed to this routine.  A pointer to the allocated address is
 * returned in *allocp.  The return value is one of the defined
 * AAS error codes (AAS_OK on success).
 */

int
alloc_addr(char *pool_name, int addr_type, AasAddr *req_addr, AasAddr *min_addr,
	AasAddr *max_addr, int flags, AasTime lease_time, char *service,
	AasClientId *client_id, AasAddr **allocp)
{
	Pool *pool;
	AddrInfo *cur, *req, *ai, *it;
	AasTime now;

	/*
	 * Find the pool.
	 */
	
	for (pool = pools; pool; pool = pool->next) {
		if (strcmp(pool->name, pool_name) == 0) {
			break;
		}
	}
	if (!pool) {
		return AAS_UNKNOWN_POOL;
	}

	/*
	 * Verify address type.
	 */
	
	if (pool->atype->addr_type != addr_type) {
		return AAS_WRONG_ADDR_TYPE;
	}

	/*
	 * Make sure addresses are valid.
	 */
	
	if ((req_addr->len > 0 && !(*pool->atype->validate)(req_addr))
	     || (min_addr->len > 0 && !(*pool->atype->validate)(min_addr))
	     || (max_addr->len > 0 && !(*pool->atype->validate)(max_addr))) {
		return AAS_BAD_ADDRESS;
	}

	if (debug) {
		if (req_addr->len > 0) {
			report(LOG_INFO, "    req addr %s",
			    (*pool->atype->show)(req_addr));
		}
		if (min_addr->len > 0) {
			report(LOG_INFO, "    min addr %s",
			    (*pool->atype->show)(min_addr));
		}
		if (max_addr->len > 0) {
			report(LOG_INFO, "    max addr %s",
			    (*pool->atype->show)(max_addr));
		}
	}

	if (lease_time == 0) {
		return AAS_BAD_LEASE_TIME;
	}

	/*
	 * If a specific address was requested,
	 * make sure an address was actually given.
	 */
	
	if ((flags & AAS_ALLOC_SPECIFIC)
	    && req_addr->len == 0) {
		return AAS_NO_ADDRESS;
	}

	/*
	 * Make expired addresses available.
	 */

	expire_addresses(pool);

	/*
	 * See if we have a record of a previously allocated address.
	 */
	
	if (!id_search(pool, service, client_id, &cur)) {
		return AAS_SERVER_ERROR;
	}

	/*
	 * Don't consider the last allocated address if it's currently
	 * allocated to another client or disabled.
	 */
	
	if (cur) {
		if ((cur->inuse && (strcmp(service, cur->service) != 0
		    || client_id->len != cur->client_id.len
		    || memcmp(client_id->id, cur->client_id.id,
		    client_id->len) != 0)) || cur->disabled) {
			cur = NULL;
		}
	}
	
	/*
	 * Make sure current address is within range.
	 */
	
	if (cur && ((min_addr->len > 0
	     && (*pool->atype->compare)(&cur->addr, min_addr) < 0)
	    || (max_addr->len > 0
	     && (*pool->atype->compare)(&cur->addr, max_addr) > 0))) {
		/*
		 * cur is outside the range, so ignore it.
		 */
		cur = NULL;
	}

	/*
	 * If the request says we should take the last address allocated
	 * to this guy, do it.
	 */

	if (cur && ((flags & AAS_ALLOC_PREFER_PREV) || req_addr->len == 0)) {
		it = cur;
	}
	else {
		/*
		 * See if the requested address is ok.
		 */
		if (req_addr->len > 0) {
			req = pool_search(pool, req_addr, NULL);
			if (req) {
				/*
				 * It must be available or allocated to
				 * the same client, and not disabled.
				 */
				if ((req->inuse
				    && (strcmp(service, req->service) != 0
				     || client_id->len != req->client_id.len
				     || memcmp(client_id->id, req->client_id.id,
				      client_id->len) != 0)) || req->disabled) {
					/*
					 * If the client only wants this
					 * one and it's not available, fail.
					 */
					if (flags & AAS_ALLOC_SPECIFIC) {
						return AAS_NO_ADDRS_AVAIL;
					}
					req = NULL;
				}
			}
			else if (flags & AAS_ALLOC_SPECIFIC) {
				return AAS_NO_SUCH_ADDR;
			}
		}
		else {
			req = NULL;
		}

		/*
		 * Use the requested address, or the last address,
		 * or pick an address from pool.
		 */
		
		if (req) {
			it = req;
		}
		else if (cur) {
			it = cur;
		}
		else {
			it = NULL;
			LIST_FOR(&pool->avail_head, ai) {
				if (ai->disabled) {
					continue;
				}
				if ((min_addr->len == 0
				     || (*pool->atype->compare)(&ai->addr,
				      min_addr) >= 0)
				    && (max_addr->len == 0
				     || (*pool->atype->compare)(&ai->addr,
				      max_addr) <= 0)) {
					it = ai;
					break;
				}
			}
			if (!it) {
				return AAS_NO_ADDRS_AVAIL;
			}
		}
	}
	
	if (debug) {
		report(LOG_INFO, "Allocating %s",
			(*pool->atype->show)(&it->addr));
	}

	/*
	 * Allocate it.
	 */
	
	now = (AasTime) time(NULL);
	if (!allocate(pool, it, now, lease_time, service, client_id,
	    (flags & AAS_ALLOC_TEMP))) {
		return AAS_SERVER_ERROR;
	}

	*allocp = &it->addr;
	return AAS_OK;
}

/*
 * free_deleted_addr -- check the orphanage and free the address if it's there
 * If the address is found in the orphanage, 1 is returned; otherwise 0
 * is returned.  If 1 is returned, *error_code is set to AAS_OK or an
 * error code if there was a problem.
 */

static int
free_deleted_addr(int addr_type, AasAddr *addr, char *service,
	AasClientId *client_id, int *error_code)
{
	AddrInfo *ai;
	AasTime free_time;

	if (!(ai = orphanage_search(addr_type, addr, NULL))) {
		return 0;
	}

	/*
	 * Verify that the service and client are the same as those the
	 * address is allocated to.
	 */
	
	if (strcmp(service, ai->service) != 0
	    || client_id->len != ai->client_id.len
	    || memcmp(client_id->id, ai->client_id.id, client_id->len) != 0)
	{
		*error_code = AAS_INCORRECT_ID;
		return 1;
	}

	free_time = (AasTime) time(NULL);

	/*
	 * Record the free transaction unless the address was allocated
	 * on a temporary basis.
	 */

	if (!ai->temp
	    && !record_free(db_fd, addr, addr_type, free_time)) {
		*error_code = AAS_SERVER_ERROR;
		return 1;
	}

	ai->inuse = 0;
	ai->free_time = free_time;
	ai->freed = 1;

	*error_code = AAS_OK;
	return 1;
}

/*
 * free_addr -- free an address based on request parameters
 * This funciton is called from the protocol code to free an address.
 * Parameters are extracted from the request message and passed to this
 * routine.  This routine checks the orphanage for the address being freed
 * if any of the following are true: no such pool; addr_type doesn't match
 * pool's address type; or address doesn't exist in pool.
 * The return value is one of the defined AAS error codes (AAS_OK
 * on success).
 */

int
free_addr(char *pool_name, int addr_type, AasAddr *addr, char *service,
	AasClientId *client_id)
{
	Pool *pool;
	AddrInfo *ainfo;
	AasTime free_time;
	int error_code;

	/*
	 * Find the pool.
	 */
	
	for (pool = pools; pool; pool = pool->next) {
		if (strcmp(pool->name, pool_name) == 0) {
			break;
		}
	}
	if (!pool) {
		if (free_deleted_addr(addr_type, addr, service, client_id,
		    &error_code)) {
			return error_code;
		}
		else {
			return AAS_UNKNOWN_POOL;
		}
	}

	/*
	 * Verify the address type.
	 */
	
	if (pool->atype->addr_type != addr_type) {
		if (free_deleted_addr(addr_type, addr, service, client_id,
		    &error_code)) {
			return error_code;
		}
		else {
			return AAS_WRONG_ADDR_TYPE;
		}
	}

	/*
	 * Make sure the address is valid.
	 */
	
	if (!(*pool->atype->validate)(addr)) {
		return AAS_BAD_ADDRESS;
	}

	if (debug) {
		report(LOG_INFO, "FREE_REQ addr %s",
			(*pool->atype->show)(addr));
	}

	/*
	 * Find the address.
	 */
	
	if (!(ainfo = pool_search(pool, addr, NULL))) {
		if (free_deleted_addr(addr_type, addr, service, client_id,
		    &error_code)) {
			return error_code;
		}
		else {
			return AAS_NO_SUCH_ADDR;
		}
	}

	/*
	 * Before freeing this one, expire addresses (to preserve
	 * LRU order).
	 */
	
	expire_addresses(pool);

	/*
	 * Return an error if it's already free.
	 */
	
	if (!ainfo->inuse) {
		return AAS_NOT_ALLOCATED;
	}

	/*
	 * Verify that the service and client are the same as those the
	 * address is allocated to.
	 */
	
	if (strcmp(service, ainfo->service) != 0
	    || client_id->len != ainfo->client_id.len
	    || memcmp(client_id->id, ainfo->client_id.id, client_id->len) != 0)
	{
		return AAS_INCORRECT_ID;
	}

	/*
	 * Record the transaction
	 */
	
	free_time = (AasTime) time(NULL);

	if (!record_free(db_fd, addr, pool->atype->addr_type, free_time)) {
		return AAS_SERVER_ERROR;
	}

	/*
	 * Free the address.
	 */
	
	free_it(pool, ainfo, free_time);

	/*
	 * Compress the pool's database file if necessary.
	 */

	compress_db();

	return AAS_OK;
}

/*
 * free_all -- free all addresses in a pool that are allocated to the
 * given service. If the pool_name is NULL, the addresses will be freed
 * in all pools.
 * This funciton is called from the protocol code to free addresses.
 * Parameters are extracted from the request message and passed to this
 * routine. The return value is one of the defined AAS error codes (AAS_OK
 * on success).
 */

int
free_all(char *pool_name, char *service)
{
	Pool *pool;
	int ret;

	report(LOG_INFO, "free_all enter %s ", pool_name);

	ret = AAS_OK;
	if (pool_name) {
		report(LOG_INFO, "free_all for pool %s ", pool_name);
		/*
		 * Find the pool.
		 */
		for (pool = pools; pool; pool = pool->next) {
			if (strcmp(pool->name, pool_name) == 0) {
				report(LOG_INFO, "free_all pool %s found ", pool_name);
				break;
			}
		}
		ret = free_all_addrs(pool, service);
	} else {
		/*
		 * Get each pool 
		 */
		for (pool = pools; pool; pool = pool->next) {

			report(LOG_INFO, "free_all do pool %s ", pool->name);

			if ((ret = free_all_addrs(pool, service)) != AAS_OK) {		
				break;
			}
		}	
	}
	return ret;
}

/*
 * free_all_addrs -- free all addresses in a pool that are allocated 
 * to the given service. 
 */
int 
free_all_addrs(Pool *pool, char *service)
{
	AddrInfo *ainfo, *nainfo;
	AasTime free_time;

	if (!pool) {
		return AAS_UNKNOWN_POOL;
	}

	/*
	 * Before freeing, expire addresses (to preserve
	 * LRU order).
	 */
	
	expire_addresses(pool);

	free_time = (AasTime) time(NULL);

	/*
	 * Go through all in-use addresses in the pool and free those
	 * that have a matching service.
	 */
	
	for (ainfo = pool->inuse_head.next; ainfo != &pool->inuse_head;
	     ainfo = nainfo) {
		nainfo = ainfo->next;
		if (strcmp(ainfo->service, service) != 0) {
			continue;
		}
		/*
		 * Record the transaction
		 */
		if (!record_free(db_fd, &ainfo->addr, pool->atype->addr_type,
		    free_time)) {
			return AAS_SERVER_ERROR;
		}
		/*
		 * Free the address.
		 */
		free_it(pool, ainfo, free_time);
	}

	/*
	 * Compress the pool's database file if necessary.
	 */

	compress_db();

	return AAS_OK;
}

/*
 * disable_addr -- set or clear the disabled flag for the given address.
 * If disable is nonzero, the disabled flag is set to 1.  Otherwise,
 * the disabled flag is set to 0.
 */

int
disable_addr(char *pool_name, int addr_type, AasAddr *addr, int disable)
{
	Pool *pool;
	AddrInfo *ainfo;

	/*
	 * Find the pool.
	 */
	
	for (pool = pools; pool; pool = pool->next) {
		if (strcmp(pool->name, pool_name) == 0) {
			break;
		}
	}
	if (!pool) {
		return AAS_UNKNOWN_POOL;
	}

	/*
	 * Verify the address type.
	 */
	
	if (pool->atype->addr_type != addr_type) {
		return AAS_WRONG_ADDR_TYPE;
	}

	/*
	 * Make sure the address is valid.
	 */
	
	if (!(*pool->atype->validate)(addr)) {
		return AAS_BAD_ADDRESS;
	}

	if (debug) {
		report(LOG_INFO, "DISABLE_REQ addr %s",
			(*pool->atype->show)(addr));
	}

	/*
	 * Find the address.
	 */
	
	if (!(ainfo = pool_search(pool, addr, NULL))) {
		return AAS_NO_SUCH_ADDR;
	}

	/*
	 * We accept any nonzero value for disabling, but we only want
	 * to record 0 or 1.
	 */

	if (disable) {
		disable = 1;
	}

	/*
	 * Record the transaction
	 */
	
	if (!record_disable(db_fd, addr, pool->atype->addr_type, disable)) {
		return AAS_SERVER_ERROR;
	}

	/*
	 * Set the flag.
	 */
	
	ainfo->disabled = disable;

	/*
	 * Compress the pool's database file if necessary.
	 */

	compress_db();

	return AAS_OK;
}

/*
 * free_checkpoint_list -- free storage associated with a list returned
 * by get_checkpoint_list
 */

static void
free_checkpoint_list(char **list, int num)
{
	int i;

	for (i = 0; i < num; i++) {
		free(list[i]);
	}
	free(list);
}

/*
 * file_name_compare -- used with qsort to sort the list of files
 * compiled by get_checkpoint_list in descending order.
 */

static int
file_name_compare(const void *v1, const void *v2)
{
	/*
	 * Compare reversed so we sort in descending order
	 * (newest to oldest).
	 */

	return strcmp(*((char **) v2), *((char **) v1));
}

/*
 * get_checkpoint_list -- return a sorted list of checkpoint files
 * get_checkpoint_list reads the current directory and makes a list of
 * checkpoint files, which are named with their creation time as
 * yyyymmddhhmmss (time in GMT).  A pointer to an array of strings
 * (malloc'ed) is returned in *list_ret.  The number of items is returned in
 * *num_ret.  The function return value is 1 if ok or 0 if error.
 */

static int
get_checkpoint_list(char ***list_ret, int *num_ret)
{
	char **list, **nlist, *name;
	int num, list_size;
	DIR *dir;
	struct dirent *ent;

	/*
	 * Allocate space for the list.  Start with 2*cp_num and
	 * grow if necessary.
	 */
	
	list_size = 2 * cp_num;
	if (!(list = tbl_alloc(char *, list_size))) {
		malloc_error("get_checkpoint_list(1)");
		return 0;
	}

	/*
	 * Read the directory and save the names of the files.
	 * Only files that look like checkpoint files (14 char names, all
	 * digits) are considered.
	 */

	if (!(dir = opendir("."))) {
		report(LOG_ERR, "Unable to open checkpoint directory %s: %m",
			cp_dir);
		free(list);
		return 0;
	}

	num = 0;
	while (ent = readdir(dir)) {
		if (!AAS_IS_CP_FILE(ent->d_name)) {
			continue;
		}
		if (num == list_size) {
			list_size += cp_num;
			if (!(nlist = tbl_grow(list, char *, list_size))) {
				malloc_error("get_checkpoint_list(2)");
				free_checkpoint_list(list, num);
				closedir(dir);
				return 0;
			}
			list = nlist;
		}
		if (!(name = strdup(ent->d_name))) {
			malloc_error("get_checkpoint_list(3)");
			free_checkpoint_list(list, num);
			closedir(dir);
			return 0;
		}
		list[num++] = name;
	}

	/*
	 * Sort the list in descending order.
	 */
	
	qsort(list, num, sizeof(char *), file_name_compare);

	*list_ret = list;
	*num_ret = num;
	closedir(dir);
	return 1;
}

/*
 * create_checkpoint_file -- create a checkpoint file
 * A checkpoint file is created in the current directory.
 * The file is named according to the time of creation, i.e.
 * yyyymmddhhmmss.
 */

static void
create_checkpoint_file(void)
{
	int fd;
	char fname[AAS_CP_FILE_NAME_LEN + 1];
	time_t now;
	struct tm *tm;

	/*
	 * Create & write checkpoint file.  This is done in a file called
	 * "new_cp".  After the file is written, it is renamed to the
	 * real name.
	 */
	
	(void) unlink(AAS_NEW_CP_FILE);
	if ((fd = open(AAS_NEW_CP_FILE, O_WRONLY|O_CREAT|O_EXCL, 0600)) == -1) {
		report(LOG_ERR, "Unable to create checkpoint file %s/%s: %m",
			cp_dir, AAS_NEW_CP_FILE);
		return;
	}

	now = time(NULL);

	if (!write_state(fd, (AasTime) now)) {
		close(fd);
		(void) unlink(AAS_NEW_CP_FILE);
		return;
	}

	close(fd);

	/*
	 * Rename the file base on the current time.  We use GMT because
	 * local time repeats when going from daylight to standard.
	 */
	
	tm = gmtime(&now);
	strftime(fname, sizeof(fname), AAS_CP_FILE_NAME_FMT, tm);
	if (link(AAS_NEW_CP_FILE, fname) == -1) {
		report(LOG_ERR, "Unable to link checkpoint file %s/%s: %m",
			cp_dir, fname);
		return;
	}
	(void) unlink(AAS_NEW_CP_FILE);
}

/* 
 * remove_old_checkpoints -- remove old checkpoint files in the
 * current directory.
 * All but the cp_num most recent checkpoints are removed.
 */

static void
remove_old_checkpoints(void)
{
	int i, num;
	char **list;

	/*
	 * Get list of checkpoint files.  The list is sorted newest to
	 * oldest.
	 */

	if (!get_checkpoint_list(&list, &num)) {
		return;
	}

	for (i = cp_num; i < num; i++) {
		if (unlink(list[i]) == -1) {
			report(LOG_ERR, "Unable to remove checkpoint file %s/%s: %m",
				cp_dir, list[i]);
		}
	}

	free_checkpoint_list(list, num);
}

/*
 * checkpoint -- start a new process to create a checkpoint file and
 * remove old checkpoints.
 */

void
checkpoint(void)
{
	int pid;

	/*
	 * If we don't have a valid configuration, don't checkpoint.
	 */
	
	if (!config_ok) {
		return;
	}

	report(LOG_INFO, "Checkpointing.");

	/*
	 * Start a new process to do the checkpoint.
	 */

	if ((pid = fork()) == -1) {
		report(LOG_ERR, "checkpoint: fork failed: %m");
		return;
	}
	else if (pid != 0) {
		return;
	}

	if (chdir(cp_dir) == -1) {
		report(LOG_ERR, "Unable to change to checkpoint diretory %s: %m",
			cp_dir);
		return;
	}

	create_checkpoint_file();

	/*
	 * Remove all but the cp_num most recent checkpoint files.
	 * If cp_num is 0, we don't do this at all.
	 */

	if (cp_num > 0) {
		remove_old_checkpoints();
	}

	/*
	 * We're done with this process.
	 */

	exit(0);
}

/*
 * compress_db -- if the database file is too big, write a compressed one
 * and switch over.
 */

static void
compress_db(void)
{
	int fd, flags;
	off_t size;

	/*
	 * db_max == 0 means we don't do compression.
	 */

	if (db_max == 0) {
		return;
	}

	/*
	 * Check the size.
	 */

	size = lseek(db_fd, 0, SEEK_CUR);
	if (size <= db_max) {
		return;
	}

	report(LOG_INFO, "Compressing database.");

	/*
	 * Go to the database directory.
	 */
	
	if (chdir(db_dir) == -1) {
		report(LOG_ERR, "Unable to change to directory %s: %m",
			db_dir);
		return;
	}

	/*
	 * Open a new transaction file and write the current state to it.
	 */
	
	(void) unlink(AAS_NEW_TRANS_FILE);
	if ((fd = open(AAS_NEW_TRANS_FILE,
	    O_RDWR|O_CREAT|O_EXCL, 0600)) == -1) {
		report(LOG_ERR, "Unable to create compressed transaction file %s/%s: %m",
			db_dir, AAS_NEW_TRANS_FILE);
		return;
	}

	if (!write_state(fd, (AasTime) time(NULL))) {
		close(fd);
		return;
	}

	/*
	 * Set synchronous write mode on the file.
	 */
	
	if (fcntl(fd, F_GETFL, &flags) == -1
	    || fcntl(fd, F_SETFL, flags | O_SYNC) == -1) {
		report(LOG_ERR, "fcntl failed for %s: %m",
			db_dir, AAS_NEW_TRANS_FILE);
		close(fd);
		return;
	}

	/*
	 * Now move them around so trans becomes old_trans
	 * and new_trans becomes trans.
	 */
	
	(void) unlink(AAS_OLD_TRANS_FILE);
	if (link(AAS_TRANS_FILE, AAS_OLD_TRANS_FILE) == -1) {
		report(LOG_ERR, "Unable to create link %s/%s: %m",
			db_dir, AAS_OLD_TRANS_FILE);
		close(fd);
		return;
	}
	(void) unlink(AAS_TRANS_FILE);
	if (link(AAS_NEW_TRANS_FILE, AAS_TRANS_FILE) == -1) {
		report(LOG_ERR, "Unable to create link %s/%s: %m",
			db_dir, AAS_TRANS_FILE);
		close(fd);
		return;
	}
	(void) unlink(AAS_NEW_TRANS_FILE);

	if (unlink(AAS_OLD_TRANS_FILE) == -1) {
		report(LOG_ERR, "Unable to remove old transaction file %s/%s: %m",
			db_dir, AAS_OLD_TRANS_FILE);
	}

	/*
	 * Set the new file as the database file.
	 */

	close(db_fd);
	db_fd = fd;
}

/*
 * reconfigure -- re-read configuration file
 * reconfigure causes configuration file changes to take effect.  This
 * is accomplished by basically getting rid of everything we have in
 * memory and starting over.  The configuration file is parsed and
 * each pool is initialized from its database file.
 */

void
reconfigure(void)
{
	Pool *pool, *npool;
	int i;
	extern int clear_passwords;

	report(LOG_INFO, "Reconfiguring.");

	/*
	 * Free pool and orphanage storage.
	 */

	for (pool = pools; pool; pool = npool) {
		npool = pool->next;
		/*
		 * Free the pool's storage.
		 */
		free_pool(pool);
	}

	for (i = AAS_FIRST_ATYPE; i < AAS_NUM_ATYPES; i++) {
		if (orphanage[i].list) {
			orphanage_free(i);
			free(orphanage[i].list);
			orphanage[i].list = NULL;
			orphanage[i].num = 0;
			orphanage[i].size = 0;
		}
	}

	/*
	 * Free other storage.
	 */
	
	if (cp_dir) {
		free(cp_dir);
		cp_dir = NULL;
	}
	
	/*
	 * Close the database file.
	 */
	
	if (db_fd != -1) {
		close(db_fd);
		db_fd = -1;
	}

	/*
	 * Read configuration.  If something fails, we go into a state
	 * where we don't honor any requests except reconfiguration.
	 */
	
	if (config()) {
		config_ok = 1;
		checkpoint();
		/*
		 * Tell receive to clear the password set on each connection.
		 */
		clear_passwords = 1;
	}
	else {
		config_ok = 0;
		report(LOG_ERR, "Configuration error.  Requests other than RECONFIG will be denied.");
	}
}
