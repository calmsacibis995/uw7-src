/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)setkey.c	1.2"
#ident	"$Header$"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * Do the real work of the keyserver.
 * Store secret keys. Compute common keys,
 * and use them to decrypt and encrypt DES keys.
 * Cache the common keys, so the expensive computation is avoided.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mp.h"
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <rpc/des_crypt.h>
#include <sys/errno.h>

extern char ROOTKEY[];

static MINT *MODULUS;
static char *fetchsecretkey();
static keystatus pk_crypt();
static int nodefaultkeys = 0;

/*
 * prohibit the nobody key on this machine k (the -d flag)
 */
pk_nodefaultkeys()
{
	nodefaultkeys = 1;
}

/*
 * Set the modulus for all our Diffie-Hellman operations
 */
setmodulus(modx)
	char *modx;
{
	MODULUS = xtom(modx);
}

/*
 * Set the secretkey key for this uid
 */
keystatus
pk_setkey(uid, skey)
	uid_t uid;
	keybuf skey;
{
	if (!storesecretkey(uid, skey)) {
		return (KEY_SYSTEMERR);
	}
	return (KEY_SUCCESS);
}

/*
 * Encrypt the key using the public key associated with remote_name and the
 * secret key associated with uid.
 */
keystatus
pk_encrypt(uid, remote_name, remote_key, key)
	uid_t uid;
	char *remote_name;
	netobj	*remote_key;
	des_block *key;
{
	return (pk_crypt(uid, remote_name, remote_key, key, DES_ENCRYPT));
}

/*
 * Decrypt the key using the public key associated with remote_name and the
 * secret key associated with uid.
 */
keystatus
pk_decrypt(uid, remote_name, remote_key, key)
	uid_t uid;
	char *remote_name;
	netobj *remote_key;
	des_block *key;
{
	return (pk_crypt(uid, remote_name, remote_key, key, DES_DECRYPT));
}

/*
 * Do the work of pk_encrypt && pk_decrypt
 */
static keystatus
pk_crypt(uid, remote_name, remote_key, key, mode)
	uid_t uid;
	char *remote_name;
	netobj *remote_key;
	des_block *key;
	int mode;
{
	char *xsecret;
	char xpublic[1024];
	char xsecret_hold[1024];
	des_block deskey;
	int err;
	MINT *public;
	MINT *secret;
	MINT *common;
	char zero[8];

	xsecret = fetchsecretkey(uid);
	if (xsecret == NULL || xsecret[0] == 0) {
		memset(zero, 0, sizeof (zero));
		xsecret = xsecret_hold;
		if (nodefaultkeys)
			return (KEY_NOSECRET);

		if (!getsecretkey("nobody", xsecret, zero) || xsecret[0] == 0) {
			return (KEY_NOSECRET);
		}
	}
	if (remote_key) {
		memcpy(xpublic, remote_key->n_bytes, remote_key->n_len);
	} else {
		if (!getpublickey(remote_name, xpublic)) {
			if (nodefaultkeys || !getpublickey("nobody", xpublic))
				return (KEY_UNKNOWN);
		}
	}

	if (!readcache(xpublic, xsecret, &deskey)) {
		public = xtom(xpublic);
		secret = xtom(xsecret);
		common = itom(0);
		pow(public, secret, MODULUS, common);
		extractdeskey(common, &deskey);
		writecache(xpublic, xsecret, &deskey);
		mfree(secret);
		mfree(public);
		mfree(common);
	}
	err = ecb_crypt((char *)&deskey, (char *)key, sizeof (des_block),
		DES_HW | mode);
	if (DES_FAILED(err)) {
		return (KEY_SYSTEMERR);
	}
	return (KEY_SUCCESS);
}

/*
 * Choose middle 64 bits of the common key to use as our des key, possibly
 * overwriting the lower order bits by setting parity.
 */
static
extractdeskey(ck, deskey)
	MINT *ck;
	des_block *deskey;
{
	MINT *a;
	short r;
	int i;
	short base = (1 << 8);
	char *k;

	a = itom(0);
	_mp_move(ck, a);
	for (i = 0; i < ((KEYSIZE - 64) / 2) / 8; i++) {
		sdiv(a, base, a, &r);
	}
	k = deskey->c;
	for (i = 0; i < 8; i++) {
		sdiv(a, base, a, &r);
		*k++ = r;
	}
	mfree(a);
	des_setparity((char *)deskey);
}

/*
 * Key storage management
 */
struct secretkey_list {
	uid_t uid;
	char secretkey[HEXKEYBYTES+1];
	struct secretkey_list *next;
};

static struct secretkey_list *g_secretkeys;

/*
 * Fetch the secret key for this uid
 */
static char *
fetchsecretkey(uid)
	uid_t uid;
{
	struct secretkey_list *l;

	for (l = g_secretkeys; l != NULL; l = l->next) {
		if (l->uid == uid) {
			return (l->secretkey);
		}
	}
	return (NULL);
}

/*
 * Store the secretkey for this uid
 */
storesecretkey(uid, key)
	uid_t uid;
	keybuf key;
{
	struct secretkey_list *new;
	struct secretkey_list **l;
	int nitems;

	nitems = 0;
	for (l = &g_secretkeys; *l != NULL && (*l)->uid != uid;
			l = &(*l)->next) {
		nitems++;
	}
	if (*l == NULL) {
		new = (struct secretkey_list *)malloc(sizeof (*new));
		if (new == NULL) {
			return (0);
		}
		new->uid = uid;
		new->next = NULL;
		*l = new;
	} else {
		new = *l;
	}
	memcpy(new->secretkey, key, HEXKEYBYTES);
	new->secretkey[HEXKEYBYTES] = 0;
	return (1);
}

static
hexdigit(val)
	int val;
{
	return ("0123456789abcdef"[val]);
}

bin2hex(bin, hex, size)
	unsigned char *bin;
	unsigned char *hex;
	int size;
{
	int i;

	for (i = 0; i < size; i++) {
		*hex++ = hexdigit(*bin >> 4);
		*hex++ = hexdigit(*bin++ & 0xf);
	}
}

static
hexval(dig)
	char dig;
{
	if ('0' <= dig && dig <= '9') {
		return (dig - '0');
	} else if ('a' <= dig && dig <= 'f') {
		return (dig - 'a' + 10);
	} else if ('A' <= dig && dig <= 'F') {
		return (dig - 'A' + 10);
	} else {
		return (-1);
	}
}

hex2bin(hex, bin, size)
	unsigned char *hex;
	unsigned char *bin;
	int size;
{
	int i;

	for (i = 0; i < size; i++) {
		*bin = hexval(*hex++) << 4;
		*bin++ |= hexval(*hex++);
	}
}

/*
 * Exponential caching management
 */
struct cachekey_list {
	keybuf secret;
	keybuf public;
	des_block deskey;
	struct cachekey_list *next;
};
static struct cachekey_list *g_cachedkeys;

/*
 * cache result of expensive multiple precision exponential operation
 */
static
writecache(pub, sec, deskey)
	char *pub;
	char *sec;
	des_block *deskey;
{
	struct cachekey_list *new;

	new = (struct cachekey_list *) malloc(sizeof (struct cachekey_list));
	if (new == NULL) {
		return;
	}
	memcpy(new->public, pub, sizeof (keybuf));
	memcpy(new->secret, sec, sizeof (keybuf));
	new->deskey = *deskey;
	new->next = g_cachedkeys;
	g_cachedkeys = new;
}

/*
 * Try to find the common key in the cache
 */
static
readcache(pub, sec, deskey)
	char *pub;
	char *sec;
	des_block *deskey;
{
	struct cachekey_list *found;
	register struct cachekey_list **l;

#define	cachehit(pub, sec, list)	\
		(memcmp(pub, (list)->public, sizeof (keybuf)) == 0 && \
		 memcmp(sec, (list)->secret, sizeof (keybuf)) == 0)

	for (l = &g_cachedkeys; (*l) != NULL && !cachehit(pub, sec, *l);
		l = &(*l)->next)
		;
	if ((*l) == NULL) {
		return (0);
	}
	found = *l;
	(*l) = (*l)->next;
	found->next = g_cachedkeys;
	g_cachedkeys = found;
	*deskey = found->deskey;
	return (1);
}
