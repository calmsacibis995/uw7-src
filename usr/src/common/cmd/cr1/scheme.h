/*		copyright	"%c%" 	*/

#ident	"@(#)scheme.h	1.2"
#ident  "$Header$"

/* cr1 scheme specific declarations */

#define FDIN 0		/* scheme's input file descriptor */
#define FDOUT 1		/* scheme's output file descriptor */

/* A nonce */

typedef struct {
	long time;	/* a timestamp */
	long pid;	/* the process ID */
} Nonce;

/* The protocol message type */

enum mtype {
	TYPE1=1,
	TYPE2=2,
	TYPE3=3
};

/* Logical protocol message */

typedef struct {
	enum mtype type;	/* The message type */
	Principal principal;	/* The identifier of the responder */
	Nonce nonce1;		/* The responder's nonce */
	Nonce nonce2;		/* The imposer's nonce */
	size_t size;		/* Number of data bytes */
	char data[256];		/* Arbitrary data */
} Pmessage;

/* Physical protocol message */

typedef struct {
	Key icv;
	unsigned int nbytes;
	char data[BUFSIZ];
	int sum_type;
	unsigned int sum;
} Emessage;

/* Declarations of XDR routines for cr1 protocol implementation */

extern bool_t
xdr_pmessage(XDR *xdrs, Pmessage *msg);
