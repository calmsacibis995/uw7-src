/*		copyright	"%c%" 	*/


#ifndef	ERROR_MGMT_H
#define	ERROR_MGMT_H
/*==================================================================*/
/*
*/
#ident	"@(#)errorMgmt.h	1.2"
#ident	"$Header$"

#include	<errno.h>

/*----------------------------------------------------------*/
/*
*/
typedef	enum
{

	Fatal,
	NonFatal

}  errorClass;

typedef	enum
{

	Unix,
	TLI,
	XdrEncode,
	XdrDecode,
	Internal

}  errorType;
	

/*----------------------------------------------------------*/
/*
**	Interface definition.
*/
#ifdef	__STDC__

extern	void	TrapError (errorClass, errorType, char *, char *);

#else

extern	void	TrapError ();

#endif


/*----------------------------------------------------------*/
/*
*/
extern	int	errno;

/*==================================================================*/
#endif
