#ident	"@(#)cplusinc:rpc/key_prot.h	1.3"

#ifndef _NET_RPC_KEY_PROT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_KEY_PROT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/key_prot.h	1.12"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* 
 *	key_prot.h, interface to the keyserver daemon
 *	Compiled from key_prot.x using rpcgen.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */

#endif

#define	KEY_PROG		100029
#define KEY_VERS		1
#define KEY_SET			1
#define KEY_ENCRYPT		2
#define KEY_DECRYPT		3
#define KEY_GEN			4
#define KEY_GETCRED		5
#define KEY_VERS2		2
#define KEY_ENCRYPT_PK		6
#define KEY_DECRYPT_PK		7
#define PROOT			3
#define HEXKEYBYTES		48
#define KEYSIZE			192
#define KEYBYTES		24
#define KEYCHECKSUMSIZE		16
#define MAXGIDS			16
#define HEXMODULUS	"d4a0ba0250b6fd2ec626e7efd637df76c716e22d0944b88b"

enum keystatus {
	KEY_SUCCESS = 0,
	KEY_NOSECRET = 1,
	KEY_UNKNOWN = 2,
	KEY_SYSTEMERR = 3
};
typedef	enum keystatus	keystatus;
#ifdef __STDC__
extern	bool_t		xdr_keystatus(XDR *, keystatus *);
#else
extern	bool_t		xdr_keystatus();
#endif

#ifndef _KERNEL

typedef	char		keybuf[HEXKEYBYTES];
extern	bool_t		xdr_keybuf(XDR *, keybuf);

#endif

typedef char *netnamestr;
#ifdef __STDC__
extern	bool_t xdr_netnamestr(XDR *, netnamestr *);
#else
extern	bool_t xdr_netnamestr();
#endif


struct cryptkeyarg {
	netnamestr remotename;
	des_block deskey;
};
typedef struct cryptkeyarg cryptkeyarg;
#ifdef __STDC__
extern	bool_t xdr_cryptkeyarg(XDR *, cryptkeyarg *);
#else
extern	bool_t xdr_cryptkeyarg();
#endif

struct cryptkeyarg2 {
	netnamestr remotename;
	netobj remotekey;
	des_block deskey;
};
typedef struct cryptkeyarg2 cryptkeyarg2;
extern	bool_t xdr_cryptkeyarg2();

struct cryptkeyres {
	keystatus status;
	union {
		des_block deskey;
	} cryptkeyres_u;
};
typedef struct cryptkeyres cryptkeyres;
#ifdef __STDC__
extern	bool_t xdr_cryptkeyres(XDR *, cryptkeyres *);
#else
extern	bool_t xdr_cryptkeyres();
#endif

struct unixcred {
	uid_t uid;
	uid_t gid;
	struct {
		u_int gids_len;
		u_int *gids_val;
	} gids;
};
typedef struct unixcred unixcred;
#ifdef __STDC__
extern	bool_t xdr_unixcred(XDR *, unixcred *);
#else
extern	bool_t xdr_unixcred();
#endif


struct getcredres {
	keystatus status;
	union {
		unixcred cred;
	} getcredres_u;
};
typedef struct getcredres getcredres;
#ifdef __STDC__
extern	bool_t xdr_getcredres(XDR *, getcredres *);
#else
extern	bool_t xdr_getcredres();
#endif

#ifdef __STDC__

int	getpublickey(const char[MAXNETNAMELEN], keybuf);
int	getsecretkey(const char[MAXNETNAMELEN], keybuf, const char*);

#else /* !__STDC__ */

int	getpublickey();
int	getsecretkey();

#endif


#ifndef opaque
#define opaque char
#endif

extern keystatus *key_set_1();
extern cryptkeyres *key_encrypt_1();
extern cryptkeyres *key_decrypt_1();
extern des_block *key_gen_1();
extern getcredres *key_getcred_1();
extern keystatus *key_set_2();
extern cryptkeyres *key_encrypt_2();
extern cryptkeyres *key_decrypt_2();
extern des_block *key_gen_2();
extern getcredres *key_getcred_2();
extern cryptkeyres *key_encrypt_pk_2();
extern cryptkeyres *key_decrypt_pk_2();

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_KEY_PROT_H */
