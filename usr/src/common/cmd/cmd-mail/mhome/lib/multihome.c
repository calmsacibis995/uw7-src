#ident "@(#)multihome.c	11.1"

/*
 * our mhome API
 *
 * entrypoints: mhome_user_map mhome_user_map_close
 */

#include <fcntl.h>
#include <stdio.h>
#include <ndbm.h>
#include <db.h>
#include "multihome.h"

static DB *mhdb;		/* multihome users db */
static DB *mddb;		/* multihome domains db */

#define MAXNAME	256

static char buf[MAXNAME+1];

/*
 * map a multimome user name to a local system user name.
 * pass in FQDN of virtual domain and virtual user name.
 * returned is system user name or null if no mapping available
 */
char *
mhome_user_map(char *username, char *fqdn)
{
	int ch;
	DBT key;
	DBT data;
	char *cp;
	int len;
	int len1;
	int ret;

	len = strlen(username);
	len1 = len + strlen(fqdn) + 1;
	if (len1 > MAXNAME)
		return(0);
	memcpy(buf, username, len);
	buf[len++] = '@';
	strcpy(buf + len, fqdn);
	if (mhdb == 0) {
		mhdb = dbopen(MHOMEDB, O_RDONLY, 0444, DB_HASH, 0);
	}
	if (mhdb) {
		key.data = buf;
		key.size = strlen(buf);
		data.data = 0;
		data.size = 0;
		ret = mhdb->get(mhdb, &key, &data, 0);
		if (ret != RET_SUCCESS)
			return(0);
		if (data.size > MAXNAME)
			return(0);
		memcpy(buf, data.data, data.size);
		buf[data.size] = 0;
		return(buf);
	}
	return(0);
}

void
mhome_user_map_close()
{
	if (mhdb) {
		mhdb->close(mhdb);
		mhdb = 0;
	}
}

/*
 * test if a virtual domain exists, returns ip addr if found.
 */
char *
mhome_virtual_domain_ip(char *domain)
{
	int ch;
	DBT key;
	DBT data;
	DB db;
	char *cp;
	int len;
	int len1;
	int ret;

	if (mddb == 0) {
		mddb = dbopen(MDOMAINDB, O_RDONLY, 0444, DB_HASH, 0);
	}
	if (mddb) {
		key.data = domain;
		key.size = strlen(domain);
		data.data = 0;
		data.size = 0;
		ret = mddb->get(mddb, &key, &data, 0);
		if (ret != RET_SUCCESS)
			return(0);
		if (data.size > MAXNAME)
			return(0);
		memcpy(buf, data.data, data.size);
		buf[data.size] = 0;
		return(buf);
	}
	return(0);
}

void
mhome_virtual_domain_close()
{
	if (mddb) {
		mddb->close(mddb);
		mddb = 0;
	}
}

/*
 * given the ip, return the virtual domain name
 */
char *
mhome_ip_virtual_name(char *ip)
{
	int ch;
	DBT key;
	DBT data;
	char *cp;
	int len;
	int len1;
	int ret;
	DB *db;

	db = dbopen(MDOMAINDB, O_RDONLY, 0444, DB_HASH, 0);
	buf[0] = 0;
	if (db) {
		len = strlen(ip);
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
		while (!db->seq(db, &key, &data, R_NEXT)) {
			if (len != data.size)
				continue;
			if (memcmp(data.data, ip, len))
				continue;
			if (data.size > MAXNAME)
				continue;
			/* found a valid one */
			memcpy(buf, key.data, key.size);
			buf[key.size] = 0;
			break;
		}
		db->close(db);
	}
	return(buf[0] ? buf : 0);
}
