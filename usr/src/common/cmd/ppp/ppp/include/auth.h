#ifndef _AUTH_H
#define _AUTH_H

#ident	"@(#)auth.h	1.4"

/*
 * Authentication protocol values
 */
#define PROTO_PAP		0xc023
#define PROTO_CHAP		0xc223
#define PROTO_SPAP		0xc027	/* Shiva PAP */
#define PROTO_OSPAP		0xc123	/* Old Shiva PAP */
/*
 * Maximum length of a chap/pap secret
 */
#define MAXSECRET 255

/*
 * Type of password lookup
 */
#define LOCALSEC 0
#define PEERSEC 1

#endif /* _AUTH_H */
