#ident "@(#)a_md512crypt.c	1.2"

/*
 *  md5crypt - MD5 based authentication routines
 */

#include "ntp_types.h"
#include "ntp_string.h"
#include "md5.h"
#include "ntp_stdlib.h"

extern u_long cache_keyid;
extern char *cache_key;
extern int cache_keylen;

/*
 * Stat counters, imported from data base module
 */
extern u_int32 authencryptions;
extern u_int32 authdecryptions;
extern u_int32 authkeyuncached;
extern u_int32 authnokey;

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of encrypted data, multiple of 8 bytes, followed by:
 *	NOCRYPT_OCTETS worth of unencrypted data, followed by:
 *	BLOCK_OCTETS worth of ciphered checksum.
 */ 
#define	NOCRYPT_OCTETS	4
#define	BLOCK_OCTETS	16

#define	NOCRYPT_int32S	((NOCRYPT_OCTETS)/sizeof(u_int32))
#define	BLOCK_int32S	((BLOCK_OCTETS)/sizeof(u_int32))

static MD5_CTX ctx;

/*
 *  Do first stage of a two stage authenticator generation.
 */

void
MD5auth1crypt(keyno, pkt, length)
    u_long keyno;
    u_int32 *pkt;
    int length;	/* length of all encrypted data */
{

    authencryptions++;

    if (keyno != cache_keyid) {
	authkeyuncached++;
	if (!authhavekey(keyno)) {
	    authnokey++;
	    return;
	}
    }

    MD5Init(&ctx);
    MD5Update(&ctx, cache_key, cache_keylen);
    MD5Update(&ctx, (char *)pkt, length - 8);
    /* just leave the partially computed value in the static MD5_CTX */
}

/*
 *  Do second state of a two stage authenticator generation.
 */
int
MD5auth2crypt(keyno, pkt, length)
    u_long keyno;
    u_int32 *pkt;
    int length;	/* total length of encrypted area */
{
    /*
     *  Don't bother checking the keys.  The first stage would have
     *  handled that.  Finish up the generation by also including the
     *  last 8 bytes of the data area.
     */

    MD5Update(&ctx, (char *)(pkt) + length - 8, 8);
    MD5Final(&ctx);

    memmove((char *) &pkt[NOCRYPT_int32S + length/sizeof(u_int32)],
	    (char *) ctx.digest,	    
	    BLOCK_OCTETS);
    return (4 + BLOCK_OCTETS);
}
