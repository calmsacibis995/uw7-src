#ident "@(#)database.c	1.2"
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "dhcpd.h"
#include "proto.h"
#include "hash.h"

/*
 * The database consists of four tables of entries (client, user class,
 * vendor class, and subnet), and the global options.  These are stored
 * as hash tables using the routines in hash.c.  The subnet table is also
 * stored as a linked list to allow searching by matching addresses (rather
 * than looking up the subnet number itself).
 */

static hash_tbl *client_hash_table;
static hash_tbl *user_class_hash_table;
static hash_tbl *vendor_class_hash_table;
static hash_tbl *subnet_hash_table;
static Subnet *subnet_list;

#define HASHTABLESIZE		257

/*
 * Global options
 */

OptionSetting *global_options;

/*
 * Server parameters
 */

ServerParams server_params;

/*
 * The user-defined option table is an array of OptionDesc structures
 * which are indexed by option code.  If a given option is not defined,
 * its entry is a NULL pointer.
 */

OptionDesc *user_options[MAX_USER_OPTION + 1];

/*
 * *_compare -- hash table comparison functions for the various tables.
 * These return 1 if the elements are equal, or 0 if not.
 */

static int
client_compare(hash_datum *h1, hash_datum *h2)
{
#define client1 ((Client *) h1)
#define client2 ((Client *) h2)

	return (client1->id_type == client2->id_type
		&& client1->id_len == client2->id_len
		&& memcmp(client1->id, client2->id, client1->id_len) == 0);

#undef client1
#undef client2
}

static int
user_class_compare(hash_datum *h1, hash_datum *h2)
{
#define uclass1 ((UserClass *) h1)
#define uclass2 ((UserClass *) h2)

	return (uclass1->class_len == uclass2->class_len
	    && memcmp(uclass1->class, uclass2->class, uclass1->class_len) == 0);

#undef uclass1
#undef uclass2
}

static int
vendor_class_compare(hash_datum *h1, hash_datum *h2)
{
#define vclass1 ((VendorClass *) h1)
#define vclass2 ((VendorClass *) h2)

	return (vclass1->class_len == vclass2->class_len
	    && memcmp(vclass1->class, vclass2->class, vclass1->class_len) == 0);

#undef vclass1
#undef vclass2
}

static int
subnet_compare(hash_datum *h1, hash_datum *h2)
{
#define subnet1 ((Subnet *) h1)
#define subnet2 ((Subnet *) h2)

	return (subnet1->subnet.s_addr == subnet2->subnet.s_addr);

#undef subnet1
#undef subnet2
}

/*
 * add_* -- functions to add entries to the various tables
 * The caller must allocate storage for the structure and fill it in
 * before calling the add function.  These functions return 0 if ok,
 * -1 if the entry already exists, and -2 if unable to allocate memory.
 */

int
add_client(Client *client)
{
	unsigned hashcode;
	int ret;

	hashcode = hash_HashFunction(client->id, client->id_len);
	if ((ret = hash_Insert(client_hash_table, hashcode,
	    client_compare, client, client)) == -2) {
		malloc_error("add_client");
	}

	return ret;
}

int
add_user_class(UserClass *uclass)
{
	unsigned hashcode;
	int ret;

	hashcode = hash_HashFunction((u_char *) uclass->class,
		uclass->class_len);
	if ((ret = hash_Insert(user_class_hash_table, hashcode,
	    user_class_compare, uclass, uclass)) == -2) {
		malloc_error("add_user_class");
	}

	return ret;
}

int
add_vendor_class(VendorClass *vclass)
{
	unsigned hashcode;
	int ret;

	hashcode = hash_HashFunction(vclass->class, vclass->class_len);
	if ((ret = hash_Insert(vendor_class_hash_table, hashcode,
	    vendor_class_compare, vclass, vclass)) == -2) {
		malloc_error("add_vendor_class");
	}

	return ret;
}

int
add_subnet(Subnet *subnet)
{
	unsigned hashcode;
	int ret;

	hashcode = hash_HashFunction((u_char *) &subnet->subnet,
		sizeof(struct in_addr));
	if ((ret = hash_Insert(subnet_hash_table, hashcode,
	    subnet_compare, subnet, subnet)) == -2) {
		malloc_error("add_subnet");
	}
	if (ret != 0) {
		return ret;
	}

	subnet->next = subnet_list;
	subnet_list = subnet;

	return 0;
}

/*
 * lookup_* -- look up entries in the various tables
 * Functions return a pointer to the entry if found, or NULL if not.
 */

Client *
lookup_client(u_char id_type, u_char *id, int id_len)
{
	unsigned hashcode;
	Client key;

	key.id_type = id_type;
	key.id = id;
	key.id_len = id_len;
	hashcode = hash_HashFunction(id, id_len);
	return (Client *) hash_Lookup(client_hash_table, hashcode,
		client_compare, &key);
}

UserClass *
lookup_user_class(char *class, int class_len)
{
	unsigned hashcode;
	UserClass key;

	key.class = class;
	key.class_len = class_len;
	hashcode = hash_HashFunction((u_char *) class, class_len);
	return (UserClass *) hash_Lookup(user_class_hash_table, hashcode,
		user_class_compare, &key);
}

VendorClass *
lookup_vendor_class(u_char *class, int class_len)
{
	unsigned hashcode;
	VendorClass key;

	key.class = class;
	key.class_len = class_len;
	hashcode = hash_HashFunction(class, class_len);
	return (VendorClass *) hash_Lookup(vendor_class_hash_table, hashcode,
		vendor_class_compare, &key);
}

/*
 * lookup_subnet is different than the other lookup routines.
 * Subnets are looked up by finding the subnet corresponding to
 * a full address, not by searching for the subnet number.
 * This routine finds the longest matching subnet entry (i.e., most
 * number of bits).
 */

Subnet *
lookup_subnet(struct in_addr *addrp)
{
	Subnet *subnet, *match;
	u_long addr, best;

	addr = ntohl(addrp->s_addr);

	/*
	 * In finding the best match, we look for the greatest mask
	 * value.  We are therefore assuming that masks are composed
	 * of 1's on the left followed by 0's on the right, with no
	 * mixing.
	 */
	
	best = 0;
	match = NULL;

	for (subnet = subnet_list; subnet; subnet = subnet->next) {
		if ((addr & subnet->mask.s_addr) == subnet->subnet.s_addr) {
			if (subnet->mask.s_addr > best) {
				match = subnet;
				best = subnet->mask.s_addr;
			}
		}
	}

	return match;
}

/*
 * free_options -- free storage for an option list
 */

void
free_options(OptionSetting *options)
{
	OptionSetting *osp, *nosp;

	for (osp = options; osp; osp = nosp) {
		nosp = osp->next;
		if (osp->val) {
			free(osp->val);
		}
		free(osp);
	}
}

/*
 * free_* -- free functions for the various types of table entries
 * Pointers to these functions are passed to hash_Reset, which frees
 * the items.
 */

static void
free_client(hash_datum *h)
{
#define client ((Client *) h)

	free(client->id);
	free_options(client->options);

#undef client
}

static void
free_user_class(hash_datum *h)
{
#define user_class ((UserClass *) h)

	free(user_class->class);
	free_options(user_class->options);

#undef user_class
}

static void
free_vendor_class(hash_datum *h)
{
#define vendor_class ((VendorClass *) h)

	free(vendor_class->class);
	free_options(vendor_class->options);

#undef vendor_class
}

static void
free_subnet(hash_datum *h)
{
#define subnet ((Subnet *) h)

	if (subnet->pool) {
		free(subnet->pool);
	}
	free_options(subnet->options);

#undef subnet
}

/*
 * init_database -- initialize the database.  This must be called before
 * the database is built for the first time.  Returns 1 if ok, or 0 if
 * unable to allocate memory for hash tables.
 */

int
init_database(void)
{
	int i;

	if (!(client_hash_table = hash_Init(HASHTABLESIZE))
	    || !(user_class_hash_table = hash_Init(HASHTABLESIZE))
	    || !(vendor_class_hash_table = hash_Init(HASHTABLESIZE))
	    || !(subnet_hash_table = hash_Init(HASHTABLESIZE))) {
		malloc_error("init_database");
		return 0;
	}

	subnet_list = NULL;

	global_options = NULL;

	server_params.aas_password = NULL;

	for (i = MIN_USER_OPTION; i < MAX_USER_OPTION; i++) {
		user_options[i] = NULL;
	}

	return 1;
}

/*
 * reset_database -- reset everything in preparation for (re-)reading the
 * config file.
 */

void
reset_database(void)
{
	OptionDesc *opt;
	int i;

	hash_Reset(client_hash_table, free_client);
	hash_Reset(user_class_hash_table, free_user_class);
	hash_Reset(vendor_class_hash_table, free_vendor_class);
	hash_Reset(subnet_hash_table, free_subnet);

	subnet_list = NULL;

	free_options(global_options);
	global_options = NULL;

	/*
	 * Initialize server parameters.
	 */
	
	if (server_params.aas_password) {
		free(server_params.aas_password);
	}
	server_params.aas_remote = 0;
	server_params.aas_password = NULL;
	server_params.option_overload = DFLT_OPTION_OVERLOAD;
	server_params.lease_res = DFLT_LEASE_RES;
	server_params.lease_pad = ((double) DFLT_LEASE_PAD + 1000.0) / 1000.0;
	server_params.address_probe = DFLT_ADDRESS_PROBE;
	server_params.set = 0;

	for (i = MIN_USER_OPTION; i < MAX_USER_OPTION; i++) {
		if (!(opt = user_options[i])) {
			continue;
		}
		free(opt->name);
		if (opt->min_val) {
			free(opt->min_val);
		}
		if (opt->max_val) {
			free(opt->max_val);
		}
		free(opt);
		user_options[i] = NULL;
	}
}
