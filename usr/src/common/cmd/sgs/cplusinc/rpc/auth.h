#ident	"@(#)cplusinc:rpc/auth.h	1.3"

#ifndef _NET_RPC_AUTH_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/auth.h	1.21"
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
 *	auth.h, describes the authentication interface for rpc.
 *
 *	All the data structures described here are completely opaque to
 *	the client. The client is required to pass a AUTH * to routines
 *	that create rpc "sessions".
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/xti.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/xdr.h> 	/* REQUIRED */
#include <proc/cred.h>	 	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/xti.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */
#include <sys/cred.h>	 	/* REQUIRED */

#else

#include <netinet/in.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define AUTH_NONE		0	/* no authentication */
#define	AUTH_NULL		0	/* backward compatibility */
#define	AUTH_SYS		1	/* unix style (uid, gids) */
#define AUTH_UNIX		AUTH_SYS
#define	AUTH_SHORT		2	/* short hand unix style */
#define AUTH_DES		3	/* des style (encrypted timestamps) */
#define AUTH_ESV		200004	/* esv style */
#define MAX_AUTH_BYTES		400	/* maximum length of auth data */
#define MAXNETNAMELEN		255	/* max len of network user's name */
#define	MILLION			1000000L/* just for convenience */

/*
 * Status returned from authentication check
 */
enum auth_stat {
	/*
	 * success
	 */
	AUTH_OK=0,
	/*
	 * failed at remote end
	 */
	AUTH_BADCRED=1,			/* bogus credentials (seal broken) */
	AUTH_REJECTEDCRED=2,		/* client should begin new session */
	AUTH_BADVERF=3,			/* bogus verifier (seal broken) */
	AUTH_REJECTEDVERF=4,		/* verifier expired or was replayed */
	AUTH_TOOWEAK=5,			/* rejected due to security reasons */
	/*
	 * failed locally
	*/
	AUTH_INVALIDRESP=6,		/* bogus response verifier */
	AUTH_FAILED=7			/* some unknown reason */
};

/*
 * 32-bit unsigned integers
 */
typedef u_long u_int32;

/*
 * encrypted or decrypted des keys, may also be read as bytes (chars)
 */
union des_block {
	struct {
		u_int32 high;
		u_int32 low;
	} key;
	char c[8];
};
typedef union des_block des_block;

/*
 * authentication info. opaque to client
 */
struct opaque_auth {
	enum_t	oa_flavor;		/* flavor of auth */
	caddr_t	oa_base;		/* address of more auth stuff */
	u_int	oa_length;		/* not to exceed MAX_AUTH_BYTES */
};

/*
 * auth handle, used by the client side for authentication
 */
typedef struct __auth {
	struct	opaque_auth	ah_cred;	/* client credentials */
	struct	opaque_auth	ah_verf;	/* credential verifier */
	union	des_block	ah_key;		/* encrypt/decrypt key */
	struct auth_ops {
#ifdef __STDC__
		void	(*ah_nextverf)(struct __auth *);
#ifdef _KERNEL
		bool_t	(*ah_marshal)(struct __auth *, XDR *,
				struct cred *, struct netbuf *, u_int);
#else
		bool_t	(*ah_marshal)(struct __auth *, XDR *);
#endif
		int	(*ah_validate)(struct __auth *, struct opaque_auth *);
		int	(*ah_refresh)(struct __auth *);
		void	(*ah_destroy)(struct __auth *);
#else
		void	(*ah_nextverf)();	/* next verify */
		int	(*ah_marshal)();	/* serialize */
		int	(*ah_validate)();	/* validate verifier */
		int	(*ah_refresh)();	/* refresh credentials */
		void	(*ah_destroy)();	/* destroy this structure */
#endif
	} *ah_ops;
	caddr_t ah_private;			/* private data of auth style */
} AUTH;


/*
 * Authentication ops.
 * The ops and the auth handle provide the interface to the authenticators.
 *
 */

#ifdef _KERNEL

#define AUTH_MARSHALL(auth, xdrs, cred, haddr, pre4dot0)	\
	((*((auth)->ah_ops->ah_marshal))(auth, xdrs, cred, haddr, pre4dot0))
#define auth_marshall(auth, xdrs, cred, haddr, pre4dot0)	\
	((*((auth)->ah_ops->ah_marshal))(auth, xdrs, cred, haddr, pre4dot0))

#else

#define AUTH_MARSHALL(auth, xdrs)	\
		((*((auth)->ah_ops->ah_marshal))(auth, xdrs))
#define auth_marshall(auth, xdrs)	\
		((*((auth)->ah_ops->ah_marshal))(auth, xdrs))

#endif

#define AUTH_NEXTVERF(auth)		\
		((*((auth)->ah_ops->ah_nextverf))(auth))
#define auth_nextverf(auth)		\
		((*((auth)->ah_ops->ah_nextverf))(auth))
#define AUTH_VALIDATE(auth, verfp)	\
		((*((auth)->ah_ops->ah_validate))((auth), verfp))
#define auth_validate(auth, verfp)	\
		((*((auth)->ah_ops->ah_validate))((auth), verfp))
#define AUTH_REFRESH(auth)		\
		((*((auth)->ah_ops->ah_refresh))(auth))
#define auth_refresh(auth)		\
		((*((auth)->ah_ops->ah_refresh))(auth))
#define AUTH_DESTROY(auth)		\
		((*((auth)->ah_ops->ah_destroy))(auth))
#define auth_destroy(auth)		\
		((*((auth)->ah_ops->ah_destroy))(auth))

/*
 * no auth, verifier for auth_kern and auth_esv
 */
extern	struct opaque_auth	_null_auth;

/*
 * various styles of authentications, routines to manipulate netname,
 * routines to interface with the keyserv daemon, and serializer for
 * des_block.
 */

#ifndef	_KERNEL

#ifdef	__STDC__
struct authdes_cred;	/* Fully defined in rpc/auth_des.h */

extern	int		authdes_getucred (const struct authdes_cred *, uid_t *,
					  gid_t *, short *, gid_t *);
extern	AUTH		*authsys_create (const char *, const uid_t, gid_t,
					 const int, const gid_t *);
extern	AUTH		*authsys_create_default (void);
extern	AUTH		*authnone_create (void);
extern	AUTH		*authdes_create(char *, u_int,
				struct sockaddr_in *, des_block *);
extern	AUTH		*authdes_seccreate (const char *, const u_int, char *,
				const des_block *);
extern	int		getnetname(char *);
extern	int		host2netname(char *, const char *, const char *);
extern	int		user2netname(char *, const uid_t, const char *);
extern	int		netname2user(const char *, uid_t *, gid_t *, int *, 
				     gid_t *);
extern	int		netname2host(const char *, char *, const int);
extern	int		key_decryptsession(const char *, des_block *);
extern	int		key_encryptsession(const char *, des_block *);
extern	int		key_gendes(des_block *);
extern	int		key_setsecret(const char *);
extern	bool_t		xdr_des_block (XDR *, des_block *);

#else

extern	int		authdes_getucred ();
extern	AUTH		*authsys_create ();
extern	AUTH		*authsys_create_default ();
extern	AUTH		*authnone_create ();
extern	AUTH		*authdes_create();
extern	AUTH		*authdes_seccreate ();
extern	int		getnetname();
extern	int		host2netname();
extern	int		user2netname();
extern	int		netname2user();
extern	int		netname2host();
extern	int		key_decryptsession();
extern	int		key_encryptsession();
extern	int		key_gendes();
extern	int		key_setsecret();
extern	bool_t		xdr_des_block ();

#endif

/*
 * these will get obsolete in near future
 */
#define	authunix_create(machname, uid, gid, len, aup_gids)	\
			authsys_create(machname, uid, gid, len, aup_gids)
#define	authunix_create_default()				\
			authsys_create_default()

#else	/* !_KERNEL */

extern	AUTH		*authesv_create(void);
extern	AUTH		*authesv_create_default(void);
extern	AUTH		*authkern_create(void);
extern	int		authdes_create(char *, u_int, struct netbuf *,
				dev_t, des_block *, int, AUTH **);
extern	enum clnt_stat	netname2user(char *, uid_t *, gid_t *, int *, int *);
extern	bool_t		xdr_des_block (XDR *, des_block *);

#endif	/* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_AUTH_H */
