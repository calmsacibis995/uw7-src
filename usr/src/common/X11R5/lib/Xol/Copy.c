/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)flat:Copy.c	1.6"
#endif

/*
 *************************************************************************
 * Copyright 1987, 1988 by Digital Equipment Corporation, Maynard,
 * Massachusetts, and the Massachusetts Institute of Technology, Cambridge,
 * Massachusetts.
 *                         All Rights Reserved
 *************************************************************************
 */

/*
 *************************************************************************
 *
 * Description:
 *	This file contains two private externals used for setting/getting
 *	the state of a flat container's sub-object.
 *
 ******************************file*header********************************
 */
						/* #includes go here	*/
#include <stdio.h>
#include <memory.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>

#define bcopy(src, dst, len)	OlMemMove(char, dst, src, len)

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Public Procedures 
 *
 **************************forward*declarations***************************
 */

void	_OlConvertToXtArgVal OL_ARGS((char *, XtArgVal *, Cardinal));
void	_OlCopyFromXtArgVal OL_ARGS((XtArgVal, char *, Cardinal));
void	_OlCopyToXtArgVal OL_ARGS((char *, XtArgVal *, Cardinal));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlConvertToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.
 ****************************procedure*header*****************************
 */
void
_OlConvertToXtArgVal OLARGLIST((src, dst, size))
	OLARG( char *,			src)
	OLARG( XtArgVal *,		dst)
	OLGRA( register Cardinal,	size)
{
	if	(size == sizeof(long))     *dst = (XtArgVal)(*(long*)src);
	else if (size == sizeof(short))    *dst = (XtArgVal)(*(short*)src);
	else if (size == sizeof(char))	   *dst = (XtArgVal)(*(char*)src);
	else if (size == sizeof(char*))    *dst = (XtArgVal)(*(char**)src);
	else if (size == sizeof(XtPointer)) *dst = (XtArgVal)(*(XtPointer*)src);
	else if (size == sizeof(caddr_t))  *dst = (XtArgVal)(*(caddr_t*)src);
	else if (size == sizeof(XtArgVal)) *dst = *(XtArgVal*)src;
	else
	{
				/* Assume the XtArgVal currently contains
				 * the address of place to write	*/

		bcopy((char*)src, (char*)*dst, (int)size);
	}
} /* END OF _OlConvertToXtArgVal() */

#ifdef UNALIGNED

/*
 *************************************************************************
 * _OlCopyFromXtArgVal - this procedure copies an XtArgVal's value into
 * an arbitrary memory location.  The size parameter indicates the size
 * of the destination type.
 ****************************procedure*header*****************************
 */
void
_OlCopyFromXtArgVal OLARGLIST((src, dst, size))
	OLARG( XtArgVal,		src)
	OLARG( char *,			dst)
	OLGRA( register Cardinal,	size)
{
	if    (size == sizeof(long))		*(long *)dst = (long)src;
	else if (size == sizeof(short))		*(short *)dst = (short)src;
	else if (size == sizeof(char))		*(char *)dst = (char)src;
	else if (size == sizeof(char *))	*(char **)dst = (char *)src;
	else if (size == sizeof(XtPointer))	*(XtPointer *)dst =
								(XtPointer)src;
	else if (size == sizeof(caddr_t))	*(caddr_t *)dst = (caddr_t)src;
	else if (size == sizeof(XtArgVal))	*(XtArgVal *)dst = src;
	else if (size > sizeof(XtArgVal))
		bcopy((char *)  src, (char *) dst, (int) size);
	else
		bcopy((char *) &src, (char *) dst, (int) size);
} /* END OF _OlCopyFromXtArgVal() */

/*
 *************************************************************************
 * _OlCopyToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.  The 'dst' parameter is an XtArgVal that
 * holds the address where we are to write to.
 ****************************procedure*header*****************************
 */
void
_OlCopyToXtArgVal OLARGLIST((src, dst, size))
	OLARG( char *,			src)
	OLARG( XtArgVal *,		dst)
	OLGRA( register Cardinal,	size)
{
	if	(size == sizeof(long))     *((long*)*dst) = *(long*)src;
	else if (size == sizeof(short))    *((short*)*dst) = *(short*)src;
	else if (size == sizeof(char))	   *((char*)*dst) = *(char*)src;
	else if (size == sizeof(char*))    *((char**)*dst) = *(char**)src;
	else if (size == sizeof(XtPointer)) *((XtPointer*)*dst) =
							*(XtPointer*)src;
	else if (size == sizeof(caddr_t))  *((caddr_t*)*dst) = *(caddr_t*)src;
	else if (size == sizeof(XtArgVal)) *((XtArgVal*)*dst)= *(XtArgVal*)src;
	else bcopy((char*)src, (char*)*dst, (int)size);
} /* END OF _OlCopyToXtArgVal() */

#else /* UNALIGNED */

/*
 *************************************************************************
 * _OlCopyFromXtArgVal - this procedure copies an XtArgVal's value into
 * an arbitrary memory location.  The size parameter indicates the size
 * of the destination type.
 ****************************procedure*header*****************************
 */
void
_OlCopyFromXtArgVal OLARGLIST((src, dst, size))
	OLARG( XtArgVal,		src)
	OLARG( char *,			dst)
	OLGRA( register Cardinal,	size)
{
	if (size > sizeof(XtArgVal))
	{
		bcopy((char *) src, (char *) dst, (int) size);
	}
	else
	{
		union {
			long		longval;
			short		shortval;
			char		charval;
			char *		charptr;
			XtPointer	xtptr;
			caddr_t		ptr;
		} u;

		char * p = (char *)&u;

		if	(size == sizeof(long))	     u.longval = (long)src;
		else if (size == sizeof(short))	     u.shortval = (short)src;
		else if (size == sizeof(char))	     u.charval = (char)src;
		else if (size == sizeof(char*))	     u.charptr = (char*)src;
		else if (size == sizeof(XtPointer))  u.xtptr = (XtPointer)src;
		else if (size == sizeof(caddr_t))    u.ptr = (caddr_t)src;
		else				     p = (char *)&src;

		bcopy(p, (char *) dst, (int) size);
	}
} /* END OF _OlCopyFromXtArgVal */

/*
 *************************************************************************
 * _OlCopyToXtArgVal - this procedure fills in an XtArgVal with the
 * value from an arbitrary source type.  The size parameter indicates the
 * size of the source type.  The 'dst' parameter is an XtArgVal that
 * holds the address where we are to write to.
 ****************************procedure*header*****************************
 */
void
_OlCopyToXtArgVal OLARGLIST((src, dst, size))
	OLARG( char *,			src)
	OLARG( XtArgVal *,		dst)
	OLGRA( register Cardinal,	size)
{
	bcopy( (char*)src, (char*)*dst, (int)size );
} /* END OF _OlCopyToXtArgVal() */

#endif /* UNALIGNED */
