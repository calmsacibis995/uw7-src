#ident "@(#)aasd.h	1.2"
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
 * This file contains definitions used by aasd.
 */


/*
 * The AddressFamily structure contains information about address families
 * the server may listen on.  Below are definitions for address family
 * functions.
 */

/*
 * AfStringToAddrFunc -- string to address function ptr definition
 * These functions take an address string and return a pointer to
 * a sockaddr (in a static area) in *fromp and the length in *lenp.  Returns 1
 * if ok or 0 if invalid.
 */

typedef int (*AfStringToAddrFunc)(char *addr, struct sockaddr **addrp,
	int *lenp);

/*
 * AfAddrToStringFunc -- converts an address to a string, returning
 * a pointer to the string (this points to a static area).
 */

typedef char *(*AfAddrToStringFunc)(struct sockaddr *addr, int len);

/*
 * AfBindFunc -- handles binding the socket.  This can just be bind
 * if no special processing is necessary.
 * Returns 0 if ok, or -1 if error.
 */

typedef int (*AfBindFunc)(int s, const struct sockaddr *addr, size_t len);

typedef struct {
	char *name;			/* address family name */
	int family;			/* family code (AF_*) */
	int protocol;			/* protocol (arg 3 for socket) */
	size_t sockaddr_len;		/* length of sockaddr for family */
	AfStringToAddrFunc str2addr;	/* string to address conversion func */
	AfAddrToStringFunc addr2str;	/* address to string conversion func */
	AfBindFunc bind;		/* post-bind processing */
	/*
	 * The password field specifies a fixed password for this address
	 * family.  If there is one, this password is always used.  This
	 * was added to effectively remove password protection from UNIX
	 * domain connections without the need for extensive changes.
	 */
	char *password;
} AddressFamily;

/*
 * The Endpoint structure contains information about a socket the server
 * is listening on.
 */

typedef struct Endpoint {
	struct Endpoint *next;		/* next in the list */
	AddressFamily *af;		/* address family info */
	int fd;				/* socket fd */
	struct sockaddr *addr;		/* the address to listen on */
	int addr_len;			/* length of the address */
} Endpoint;

/*
 * The Connection structure contains information about a client connection.
 */

typedef struct Connection {
	struct Connection *next;	/* next in list */
	AddressFamily *af;		/* address family info */
	int fd;				/* socket fd */
	struct sockaddr *from;		/* client's address */
	int fromlen;			/* length of from address */
	AasRcvMsgInfo msg_info;		/* incoming message */
	/*
	 * The following fields are used when an entire response message
	 * couldn't be sent right away.  If send_buf is non-NULL, a mesage
	 * is being sent, and the loop in receive will select on write
	 * for this connection.
	 */
	char *send_buf;			/* message being sent */
	char *send_ptr;			/* position in message */
	int send_rem;			/* bytes remaining */
	/*
	 * Authentication information
	 */
	char *password;			/* password this client is using */
	int password_len;		/* length of the password */
	/*
	 * last_digest is the digest from the last message sent or received
	 * on the connection.
	 */
	char last_digest[AAS_MSG_DIGEST_SIZE];
} Connection;

/*
 * Password -- entry on the password list
 */

typedef struct Password {
	struct Password *next;
	char *password;
	int len;
} Password;

/*
 * WorkFunc -- function called by receive at specified intervals
 */

typedef void (*WorkFunc)(void);

/*
 * Variables
 */

extern int debug;

/*
 * Functions
 */

void send_nack(Connection *conn, int code);
void malloc_error(char *where);
void report(int pri, char *fmt, ...);
char *binary2string(u_char *buf, int len);
void process_request(Connection *conn);
int init_auth(Connection *conn);
void receive(Endpoint *endpoints, WorkFunc work_func, time_t intvl);

/*
 * Useful macros
 */

/*
 * str_alloc -- allocate memory to hold the given structure type and
 * return a pointer.
 */

#define str_alloc(t)		((t *) malloc(sizeof(t)))
#define tbl_alloc(t, n)		((t *) malloc((n) * sizeof(t)))
#define tbl_grow(p, t, n)	((t *) realloc((p), (n) * sizeof(t)))

/*
 * Default configuration parameter values
 * (path name parameters are in pathnames.h)
 */

#define AAS_DFLT_CP_INTVL	1800	/* .5 hours */
#define AAS_DFLT_CP_NUM		48
#define AAS_DFLT_DB_MAX_SIZE	(1024 * 1024)
