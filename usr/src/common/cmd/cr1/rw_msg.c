#ident	"@(#)rw_msg.c	1.2"
#ident  "$Header$"

/* Routines for reading/writing encrypting/decrypting protocol messages */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stropts.h>
#include <rpc/rpc.h>
#include <unistd.h>

#include <cr1.h>
#include <crypt.h>
#include "cr1.h"
#include "keymaster.h"
#include "scheme.h"

#define MSG_WAIT_TIME 180	/*  Seconds to wait for a message  */

extern int x_type;
extern long mypid;
extern int role;
extern FILE *logfp;

/*
 *	Signal handler for alarm()
 */

static void
no_msg(int sig)
{
	LOG("Timeout failure (signal %d) in protocol\n", sig);
}

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;
extern int t_errno;

extern Principal Local, Expected, Actual;

static XDR mxdrs;	/*  XDR stream for protocol messages  */
static XDR cxdrs;	/*  XDR stream for encrypted messages  */
static XDR sxdrs;	/*  XDR stream for encrypted message size  */

static int xdr_int_size = 0;

static char mbuf[MLEN];	/* buffer for complete protocol message */
static char sbuf[MLEN];	/* buffer for complete protocol message size */

extern FILE *logfp;

static int rw_flag = -1;	/* do read/write or t_snd/t_rcv */

static Key cr1_key;

/*
 * checksum algorithm used by cr1 scheme to validate protocol messages
 * borrowed from sum command (alternate i.e. -r algorithm)
 */

static unsigned int
cksum(unsigned char *bytes, size_t nbytes)
{
	register unsigned sum = 0;

	while(nbytes-- > 0) {
		if(sum & 01)
			sum = (sum >> 1) + 0x8000;
		else
			sum >>= 1;
		sum += *bytes++;
		sum &= 0xFFFF;
	}

	return(sum);
}

/*
 *	"read" from a connection, read(2)/write(2) are
 *	used unless rw_flag == 0, then we use t_snd()/t_rcv()
 */

int
Read(fd, buffer, bufsize)
int fd;
char *buffer;
int bufsize;
{
	int nchar = 0;		/* # characters read from connection */
	int tchar = 0;		/* total # characters read from connection */

	sigset(SIGALRM, no_msg);
	alarm(MSG_WAIT_TIME);
	
	while (bufsize) {
		if (rw_flag) {
			if ((nchar = read(fd, buffer, bufsize)) == -1) {
				alarm(0);
				LOG("message read failure %s\n", sys_errlist[errno]);
				failure(CR_MSGIN, "read");
			} else if (nchar == 0) {
				alarm(0);
				LOG("message read %d length\n", nchar);
				break;
			}
		} else {
			int tflags = 0;

			if ((nchar = t_rcv(fd, buffer, bufsize, &tflags)) == -1) {
				alarm(0);
				LOG("message t_rcv failed, t_errno=%d\n", t_errno);
				if (t_errno == TSYSERR)
					LOG("message failure %s\n", sys_errlist[errno]);
				failure(CR_MSGIN, "t_rcv");
			} else if (nchar == 0) {
				alarm(0);
				LOG("message t_rcv %d length\n", nchar);
				break;
			}
		}

		bufsize -= nchar;
		buffer += nchar;
		tchar += nchar;

	}

	alarm(0);

	return(tchar);
}

/*
 *	"read" a protocol message from a connection. This "read"s
 *	and XDR encoded size and then calls Read() to read the
 *	actual message.
 */

int
ReadMsg(fd, buffer, bufsize)
int fd;
char *buffer;
int bufsize;
{
	int nchar;		/* # of characters read from connection */
	int msg_size;		/* size of this message */

	/* associate sbuf with the message size stream */
	xdrmem_create(&sxdrs, sbuf, sizeof(sbuf), XDR_DECODE);

	if ((nchar = Read(fd, sbuf, xdr_int_size)) == -1) {
		LOG("message size read failure %s\n", sys_errlist[errno]);
		failure(CR_MSGIN, "Read");
	}

	/* get the actual message size */

	if (!xdr_int(&sxdrs, &msg_size)) {
		LOG("xdr_int failed.\n%s", "");
		failure(CR_XDRIN, "xdr_int");
	}

	xdr_destroy(&sxdrs);

	if ((nchar = Read(fd, mbuf, msg_size)) == -1) {
		LOG("message read failure %s\n", sys_errlist[errno]);
		failure(CR_MSGIN, "Read");
	}

	return(nchar);
}

int
Write(fd, buffer, bufsize)
int fd;
char *buffer;
int bufsize;
{
	int nchar;		/* # of characters written to connection */

	if (rw_flag) {
		if ((nchar = write(fd, buffer, bufsize)) == -1) {
			LOG("message could not be written: %s\n",
				sys_errlist[errno]);
			failure(CR_MSGOUT, "write");
		}
	} else {
		if ((nchar = t_snd(fd, buffer, bufsize, 0)) == -1) {
			LOG("message could not be sent: %d\n", t_errno);
			if (t_errno == TSYSERR)
				LOG("system error: %s\n", sys_errlist[errno]);
			failure(CR_MSGOUT, "t_snd");
		}
	}

	return(nchar);
}
int
WriteMsg(fd, buffer, bufsize)
int fd;
char *buffer;
int bufsize;
{
	int nchar;		/* # of characters written to connection */

	/* associate sbuf with the message size XDR stream */
	xdrmem_create(&sxdrs, sbuf, sizeof(sbuf), XDR_ENCODE);

	/* put the actual message size in the stream */

	if (!xdr_int(&sxdrs, &bufsize)) {
		failure(CR_XDROUT, "xdr_int");
	}

	if ((nchar = Write(fd, sbuf, xdr_int_size)) == -1) {
			LOG("message size could not be written: %s\n", sys_errlist[errno]);
			failure(CR_MSGOUT, "Write");
	}

	xdr_destroy(&sxdrs);

	/* now Write out the actual message */

	if ((nchar = Write(fd, buffer, bufsize)) == -1) {
		LOG("message could not be sent: %s\n",
				sys_errlist[errno]);
		failure(CR_MSGOUT, "write");
	}

	return(nchar);
}

void
set_flag(role)
int role;
{
#define STARTUP_CHAR '\001'

	char startup = STARTUP_CHAR;

	/* assume read()/write() unless tirdwr is not found && timod is */

	if (rw_flag == -1) {
		rw_flag = 1;
		if (isastream(FDIN)
			&& (ioctl(FDIN, I_FIND, "tirdwr") != 1)
			&& (ioctl(FDIN, I_FIND, "timod") == 1))
			rw_flag = 0;
	}

	/*
	 * synchronize the protocol start: the IMPOSER sends an \001
	 * to the RESPONDER to indicate that the line is in the
	 * right state. Only then can the RESPONDER send the first
	 * real message. This is intended to handle the problem of
	 * the message being mangled by the line discipline on the
	 * IMPOSER's side.
	 */

	if (role == 'i' ) {
		Write(FDOUT, &startup, sizeof(startup));
	} else {
		sigset(SIGALRM, no_msg);
		alarm(MSG_WAIT_TIME);

		startup = '\0';

		while (startup != STARTUP_CHAR) {
			if (Read(FDIN, &startup, sizeof(startup)) != 
						sizeof(startup) ) {
				alarm(0);
				failure(CR_MSGIN, NULL);
			}
		}

	}

	/* determine how much space XDR requires for an endoded int */

	xdrmem_create(&sxdrs, sbuf, sizeof(sbuf), XDR_ENCODE);

	if (!xdr_int(&sxdrs, &xdr_int_size)) {
		failure(CR_XDROUT, "xdr_int");
	}

	xdr_int_size = xdr_getpos(&sxdrs);

	xdr_destroy(&sxdrs);

	return;
}

/*
 *	"read" a message from a connection, read(2)/write(2) are
 *	used unless rw_flag == 0, then we use t_snd()/t_rcv()
 */

int
rd_msg(Pmessage *msg)
{
	Key icv;

	Emessage E;		/* to hold an encrypted message */

	enum mtype type = msg->type;

	int nchar;		/* # of characters read from connection */

	/* associate mbuf with total message stream */
	xdrmem_create(&mxdrs, mbuf, sizeof(mbuf), XDR_DECODE);

	/* associate E.data with encrypted message stream */
	xdrmem_create(&cxdrs, E.data, sizeof(E.data), XDR_DECODE);

	/*  Read message into message buffer  */

	if ((nchar = ReadMsg(FDIN, mbuf, sizeof(mbuf))) == -1) {
		LOG("message read failure %s\n", sys_errlist[errno]);
		failure(CR_MSGIN, "ReadMsg");
	}

	if (nchar == 0)
		failure(CR_MSGIN, NULL);

	DUMP(mbuf, nchar, "buffer received");

	if (type == TYPE1) {
		char *k;
		int x_peer;

		/* get the responder's cleartext identity from message */

		if (!xdr_principal(&mxdrs, &Actual)) {
			LOG("xdr_principal failed.\n%s", "");
			failure(CR_XDRIN, "xdr_principal");
		}

		/* get the responder's encryption algorithm */

		if (!xdr_int(&mxdrs, &x_peer)) {
			LOG("xdr_int failed.\n%s", "");
			failure(CR_XDRIN, "xdr_int");
		}

		if (x_peer != x_type) {
			LOG("Encryption type (%0o) mismatch.\n", x_peer);
			failure(CR_CRYPT_TYPE, x_peer);
		}

		DLOG("Responder's claimed identity '%s'.\n", Actual);

		/* get the key for use in this transaction :
		 * - if the responder sent the machine key,
		 *   use our machine key
		 * - if the responder sent a user key, use
		 *   our user key
		 */

		if (((k=strchr(Actual, '@')) == NULL) || (k == Actual)) {
			char *loc_node;
			loc_node = strchr(Local, '@');
			k = getkey(DEF_SCHEME, loc_node, Actual);
		} else
			k = getkey(DEF_SCHEME, Local, Actual);
		if (k == NULL)
			failure(CR_NOKEY, Actual);
		memset(cr1_key, 0, KEYLEN);
		(void) strncpy(cr1_key, k, KEYLEN);
	}

	/*  Get encrypted message from message buffer and decrypt it  */

	DLOG("Local principal '%s'\n", Local);
	DLOG("Purported remote identity '%s'\n", Actual);
		
	/* get the initial chaining value from the stream */

	if (!xdr_key(&mxdrs, &icv))
        	failure(CR_XDRIN, "xdr_key");

	/* decrypt the initial chaining value */

	cryptbuf(icv, KEYLEN, cr1_key, NULL, x_type | X_ECB | X_DECRYPT);

	/* get the encrypted message from the stream */

	if (!xdr_emessage(&mxdrs, &E))
        	failure(CR_XDRIN, "xdr_emessage");

	DUMP(E.data, E.nbytes, "encrypted msg");

	/* decrypt the message in place */

	cryptbuf(E.data, E.nbytes, cr1_key, icv, x_type | X_CBC | X_DECRYPT);

	DUMP(E.data, E.nbytes, "decrypted msg");

	/* extract the real message from the encryption buffer */

	if (!xdr_pmessage(&cxdrs, msg))
         	failure(CR_XDRIN, "xdr_pmessage");

	/* determine the number of bytes in the encoded message */

	E.nbytes = xdr_getpos(&cxdrs);

	/* extract the checksum type from the message */
	/* for now, we only handle the default type 0 */

	if (!xdr_int(&cxdrs, &E.sum_type))
		failure(CR_XDRIN, "xdr_int");

	if (E.sum_type != 0)
		failure(CR_CKSUM_TYPE, E.sum_type);

	/* extract the checksum from the encryption buffer	*/
	/* if new checksum types are different sizes, then	*/
	/* the following code must change.			*/

	if (!xdr_u_int(&cxdrs, &E.sum))
         	failure(CR_XDRIN, "xdr_u_int");

	/* make sure the checksum agrees with the received data */

	if (E.sum != cksum((unsigned char *) E.data, E.nbytes))
         	failure(CR_PROTOCOL, "ckecksum");

	/*  Verify the type of the message  */

	if (msg->type != type) {
		DLOG("message is wrong type: type=%d\n", msg->type);
		failure(CR_PROTOCOL, gettxt(":55", "type"));
	}
	
	DLOG("Responder's decrypted identity '%s'\n", msg->principal);
	DLOG("Responder's nonce.timestamp %ld \n", msg->nonce1.time);
	DLOG("Responder's nonce.pid       %ld \n", msg->nonce1.pid);
	DLOG("Imposer's nonce.timestamp %ld \n", msg->nonce2.time);
	DLOG("Imposer's nonce.pid       %ld \n", msg->nonce2.pid);
	DLOG("Remote's data		 %s \n", msg->data);

	/*  Verify the identity of the responder  */

	if ((type == TYPE1) && (strcmp(msg->principal, Actual) != 0)) {
		DLOG("Cleartext and encrypted identities do not match.\n", "");
		failure(CR_PROTOCOL, gettxt(":56", "principal mismatch"));
	}

	/* destroy the XDR streams */

	xdr_destroy(&mxdrs);
	xdr_destroy(&cxdrs);

	return(0);

}

int
wr_msg(Pmessage *msg)
{
	Key icv, icv_crypt;

	Emessage E;		/* space for an encrypted message */

	int nchar;		/* # of characters written to connection */

	enum mtype type = msg->type;
	
	/* associate mbuf with total message XDR stream */
	xdrmem_create(&mxdrs, mbuf, sizeof(mbuf), XDR_ENCODE);

	/* associate E.data with encrypted message XDR stream */
	xdrmem_create(&cxdrs, E.data, sizeof(E.data), XDR_ENCODE);

	if (type == TYPE1) {
		char *k;

		/* get the appropriate key:
		 * - try the local principal in msg->principal
		 * - it this fails and it was machine key,
		 *   try the local principal in Local.
		 */

		if ((k = getkey(DEF_SCHEME, msg->principal, Actual)) == NULL)
			if (strncmp(msg->principal, Local, PRINLEN) != 0) {
				k = getkey(DEF_SCHEME, Local, Expected);
				if (k == NULL)
					failure(CR_NOKEY, Expected);
				(void) strncpy(msg->principal, Local, PRINLEN);
				(void) strncpy(Actual, Expected, PRINLEN);
			}
		memset(cr1_key, 0, KEYLEN);
		(void) strncpy(cr1_key, k, KEYLEN);

		/*  Put responder's identity into message buffer  */

		if (!xdr_principal(&mxdrs, &msg->principal)) {
			failure(CR_XDROUT, "xdr_principal");
		}

		/* tell the imposer the encryption type to use */

		if (!xdr_int(&mxdrs, &x_type)) {
			failure(CR_XDROUT, "xdr_int");
		}

	}

	/*  Pick an initial chaining value, and place into message buffer */

	{
	/* for now, pick a constant chaining value to facilitate debugging */
	int i;
	for (i=0; i<8; i++)
		icv[i] = icv_crypt[i] = 17;
	}

	/* encrypt the initial chaining value */

	cryptbuf(icv_crypt, KEYLEN, cr1_key, NULL, x_type | X_ECB | X_ENCRYPT);

	/* encode the initial chaining value into message buffer */

	if (!xdr_key(&mxdrs, &icv_crypt))
		failure(CR_XDROUT, "xdr_bytes");

	/* encode the message into encryption buffer */

	if (!xdr_pmessage(&cxdrs, msg))
		failure(CR_XDROUT, "xdr_pmessage");

	/* determine the number of bytes in the encoded message */

	E.nbytes = xdr_getpos(&cxdrs);	

	/* calculate the checksum of the cleartext message */

	E.sum = cksum((unsigned char *) E.data, E.nbytes);

	/* insert the checksum type from the message	*/
	/* for now, we only handle the default type (0)	*/

	E.sum_type = 0;
	if (!xdr_int(&cxdrs, &E.sum_type))
		failure(CR_XDROUT, "xdr_int");

	/* include the checksum in the cleartext message	*/
	/* if new checksum types are different sizes,		*/
	/* this will have to change.				*/

	if (!xdr_u_int(&cxdrs, &E.sum))
		failure(CR_XDROUT, "xdr_u_int");

	/* update the byte count to include the checksum */

	E.nbytes = xdr_getpos(&cxdrs);

	DUMP(E.data, E.nbytes, "encoded msg");

	/* encrypt the message in place */

	cryptbuf(E.data, E.nbytes, cr1_key, icv, x_type | X_CBC | X_ENCRYPT);

	DUMP(E.data, E.nbytes, "encrypted msg");

	/* encode the encrypted message into the message stream */

	if (!xdr_emessage(&mxdrs, &E))
		failure(CR_XDROUT, "xdr_emessage");

	DUMP(mbuf, xdr_getpos(&mxdrs), "re-encoded msg");

	/* send the completed message */

	if ((nchar = WriteMsg(FDOUT, mbuf, xdr_getpos(&mxdrs))) == -1) {
		LOG("message could not be written: %s\n", sys_errlist[errno]);
		failure(CR_MSGOUT, "write");
	}

	LOG("message %d was transmitted.\n", type);

	/* destroy the XDR message streams */

	xdr_destroy(&mxdrs);
	xdr_destroy(&cxdrs);

	return(0);
}
