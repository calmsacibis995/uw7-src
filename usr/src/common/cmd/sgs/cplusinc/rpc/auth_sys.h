#ident	"@(#)cplusinc:rpc/auth_sys.h	1.3"

#ifndef _NET_RPC_AUTH_SYS_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_SYS_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/auth_sys.h	1.13"
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
 *	auth_sys.h, Protocol for UNIX (or sys or kern) style authentication
 *	parameters for rpc. It is very weak. The client uses no
 *	encryption for its credentials and only sends null verifiers. The
 *	server sends backs null verifiers or optionally a verifier that
 *	suggests a new short hand for the credentials.
 *
 *	The server does no validation of the client's credentials and vice
 *	versa. Authentication fails only when there is an error during
 *	serialization and deserialization of authentication information.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h> 		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The machine name is part of a credential.
 * It may not exceed 255 bytes.
 */
#define MAX_MACHINE_NAME	255

/*
 * groups compose part of a credential.
 * There may not be more than 16 of them.
 */
#define	NGRPS			16

/*
 * base (smallest) credentials size
 */
#define BASE_CREDSZ		5

/*
 * "Unix" (sys) style credentials.
 */
struct authsys_parms {
	u_long	 aup_time;	/* arbitrary timestamp */
	char	*aup_machname;	/* machine name */
	uid_t	 aup_uid;	/* user id */
	gid_t	 aup_gid;	/* group id */
	u_int	 aup_len;	/* total length of groups */
	gid_t	*aup_gids;	/* groups */
};

/*
 * For backward compatibility
 */
#define	authunix_parms	authsys_parms

/*
 * serializer for auth sys credentials
 */
#ifdef __STDC__

extern	bool_t		xdr_authsys_parms (XDR *,
					   const struct authsys_parms *);

#else

extern	bool_t		xdr_authsys_parms ();

#endif

/*
 * For backward compatibility. Will get obsolete
 */
#define	xdr_authunix_parms(xdrs, p)	\
			xdr_authsys_parms(xdrs, p)

/* 
 * If a response verifier has flavor AUTH_SHORT, then the body
 * of the response verifier encapsulates the following structure;
 * again it is serialized in the obvious fashion.
 */
struct short_hand_verf {
	struct opaque_auth new_cred;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_AUTH_SYS_H */
