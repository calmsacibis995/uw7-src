/*		copyright	"%c%" 	*/

#ident	"@(#)xdr.c	1.2"
#ident  "$Header$"

/*  These XDR routines support the keymaster message formatting */

#include <stdio.h>
#include "cr1.h"
#include "scheme.h"

bool_t
xdr_principal(XDR *xdrs, Principal *A)
{
	if (xdr_string(xdrs, (char **)&A, (u_int) PRINLEN))
		return (TRUE);
	return (FALSE);
}

bool_t
xdr_key(XDR *xdrs, Key *key)
{
	u_int isz = KEYLEN;

	if (xdr_bytes(xdrs, (char **)&key, &isz, KEYLEN))
		return (TRUE);
	return (FALSE);
}

bool_t
xdr_kmessage(XDR *xdrs, Kmessage *kmsg)
{

	/* all messages have a type field */

	if (!xdr_enum(xdrs, (enum_t *)&kmsg->type))
		return(FALSE);

	/* get the encryption algorithm type */

	if (!xdr_enum(xdrs, (enum_t *)&kmsg->xtype))
		return(FALSE);

	/* take care of messages which have two principal fields */

	switch(kmsg->type) {

	case ADD_KEY:
	case DELETE_KEY:
	case CHANGE_KEY:
	case SEND_KEY:
	case GET_KEY:
		if (!xdr_principal(xdrs, &kmsg->principal1) ||
		    !xdr_principal(xdrs, &kmsg->principal2) )
			return(FALSE);
		break;

	default:
		break;

	}

	/* take care of messages which require an OLD key field */

	switch(kmsg->type) {

	case MASTER_KEY:
	case CHANGE_KEY:
	case DELETE_KEY:
	case SEND_KEY:
		if (!xdr_key(xdrs, &kmsg->key1))
			return(FALSE);
		break;

	default:
		break;

	}

	/* take care of messages which require a NEW key field */

	switch(kmsg->type) {

	case ADD_KEY:
	case MASTER_KEY:
	case CHANGE_KEY:
		if (!xdr_key(xdrs, &kmsg->key2))
			return(FALSE);
		break;

	default:
		break;

	}

	/* if we got here without failure, return success */

	return(TRUE);

}

/*  These XDR routines support the protocol message formatting  */

static bool_t
xdr_nonce(XDR *xdrs, Nonce *nonce)
{
	if (xdr_long(xdrs, &nonce->time) &&
	    xdr_long(xdrs, &nonce->pid))
		return (TRUE);
	return (FALSE);
}

bool_t
xdr_pmessage(XDR *xdrs, Pmessage *msg)
{
	char *dp;
	dp = msg->data;

	if (xdr_enum(xdrs, (enum_t *)&msg->type) &&
	    xdr_principal(xdrs, &msg->principal) &&
	    xdr_nonce(xdrs, &msg->nonce1) &&
	    xdr_nonce(xdrs, &msg->nonce2) &&
	    xdr_bytes(xdrs, &dp, &msg->size, BUFSIZ))
		return(TRUE);
	return(FALSE);
}

/*  These XDR routines support the encrypted message formatting  */

bool_t
xdr_emessage(XDR *xdrs, Emessage *msg)
{
	char *dp = msg->data;

	if (xdr_bytes(xdrs, &dp, &msg->nbytes, CLEN))
		return(TRUE);
	return(FALSE);
}
