/* $XConsortium: FSlibint.h,v 1.6 91/05/13 15:12:01 gildea Exp $ */

/* @(#)FSlibint.h	4.1	91/05/02
 * Copyright 1990 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this protoype software
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * FSlib internal decls
 */

#ifdef USG
#ifndef __TYPES__
#include <sys/types.h>			/* forgot to protect it... */
#define __TYPES__
#endif /* __TYPES__ */
#else
#if defined(_POSIX_SOURCE) && defined(MOTOROLA)
#undef _POSIX_SOURCE
#include <sys/types.h>
#define _POSIX_SOURCE
#else
#include <sys/types.h>
#endif
#endif /* USG */

#include	"FSlib.h"
#include	"FSproto.h"
#include	"FSlibos.h"
#include	<errno.h>

#ifndef NULL
#define NULL 0
#endif

extern int  errno;		/* Internal system error number. */

extern      (*_FSIOErrorFunction) ();
extern      (*_FSErrorFunction) ();

extern int  _FGetHostname();

extern void	_FSEatData();

#ifndef BUFSIZE
#define BUFSIZE 2048		/* FS output buffer size. */
#endif

/*
 * server flags 
 */
#define	FSlibServerIOError	(1L << 0)
#define	FSlibServerClosing	(1L << 1)

/*   Need to start requests on 64 bit word boundries
 *   on a CRAY computer so add a NoOp (127) if needed.
 *   A character pointer on a CRAY computer will be non-zero
 *   after shifting right 61 bits of it is not pointing to
 *   a word boundary.
 */

#ifdef WORD64
#define WORD64ALIGN if ((long)svr->bufptr >> 61) {\
           svr->last_req = svr->bufptr;\
           *(svr->bufptr)   = FS_Noop;\
           *(svr->bufptr+1) =  0;\
           *(svr->bufptr+2) =  0;\
           *(svr->bufptr+3) =  1;\
             svr->request += 1;\
             svr->bufptr += 4;\
         }
#else				/* else does not require alignment on 64-bit
				 * boundaries */
#define WORD64ALIGN
#endif				/* WORD64 */

/*
 * GetReq - Get the next avilable FS request packet in the buffer and
 * return it.
 *
 * "name" is the name of the request, e.g. InternAtom, OpenFont, etc.
 * "req" is the name of the request pointer.
 *
 */

#if defined(__STDC__) && !defined(UNIXCPP)
#define GetReq(name, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fs##name##Req)) > svr->bufmax)\
		_FSFlush(svr);\
	req = (fs##name##Req *)(svr->last_req = svr->bufptr);\
	req->reqType = FS_##name;\
	req->length = (SIZEOF(fs##name##Req))>>2;\
	svr->bufptr += SIZEOF(fs##name##Req);\
	svr->request++

#else				/* non-ANSI C uses empty comment instead of
				 * "##" for token concatenation */
#define GetReq(name, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fs/**/name/**/Req)) > svr->bufmax)\
		_FSFlush(svr);\
	req = (fs/**/name/**/Req *)(svr->last_req = svr->bufptr);\
	req->reqType = FS_/**/name;\
	req->length = (SIZEOF(fs/**/name/**/Req))>>2;\
	svr->bufptr += SIZEOF(fs/**/name/**/Req);\
	svr->request++
#endif

/* GetReqExtra is the same as GetReq, but allocates "n" additional
   bytes after the request. "n" must be a multiple of 4!  */

#if defined(__STDC__) && !defined(UNIXCPP)
#define GetReqExtra(name, n, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fs##name##Req) + n) > svr->bufmax)\
		_FSFlush(svr);\
	req = (fs##name##Req *)(svr->last_req = svr->bufptr);\
	req->reqType = FS_##name;\
	req->length = (SIZEOF(fs##name##Req) + n)>>2;\
	svr->bufptr += SIZEOF(fs##name##Req) + n;\
	svr->request++
#else
#define GetReqExtra(name, n, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fs/**/name/**/Req) + n) > svr->bufmax)\
		_FSFlush(svr);\
	req = (fs/**/name/**/Req *)(svr->last_req = svr->bufptr);\
	req->reqType = FS_/**/name;\
	req->length = (SIZEOF(fs/**/name/**/Req) + n)>>2;\
	svr->bufptr += SIZEOF(fs/**/name/**/Req) + n;\
	svr->request++
#endif


/*
 * GetResReq is for those requests that have a resource ID
 * (Window, Pixmap, GContext, etc.) as their single argument.
 * "rid" is the name of the resource.
 */

#if defined(__STDC__) && !defined(UNIXCPP)
#define GetResReq(name, rid, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fsResourceReq)) > svr->bufmax)\
	    _FSFlush(svr);\
	req = (fsResourceReq *) (svr->last_req = svr->bufptr);\
	req->reqType = FS_##name;\
	req->length = 2;\
	req->id = (rid);\
	svr->bufptr += SIZEOF(fsResourceReq);\
	svr->request++
#else
#define GetResReq(name, rid, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fsResourceReq)) > svr->bufmax)\
	    _FSFlush(svr);\
	req = (fsResourceReq *) (svr->last_req = svr->bufptr);\
	req->reqType = FS_/**/name;\
	req->length = 2;\
	req->id = (rid);\
	svr->bufptr += SIZEOF(fsResourceReq);\
	svr->request++
#endif

/*
 * GetEmptyReq is for those requests that have no arguments
 * at all.
 */

#if defined(__STDC__) && !defined(UNIXCPP)
#define GetEmptyReq(name, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fsReq)) > svr->bufmax)\
	    _FSFlush(svr);\
	req = (fsReq *) (svr->last_req = svr->bufptr);\
	req->reqType = FS_##name;\
	req->length = 1;\
	svr->bufptr += SIZEOF(fsReq);\
	svr->request++
#else
#define GetEmptyReq(name, req) \
        WORD64ALIGN\
	if ((svr->bufptr + SIZEOF(fsReq)) > svr->bufmax)\
	    _FSFlush(svr);\
	req = (fsReq *) (svr->last_req = svr->bufptr);\
	req->reqType = FS_/**/name;\
	req->length = 1;\
	svr->bufptr += SIZEOF(fsReq);\
	svr->request++
#endif

#define	SyncHandle()	\
	if (svr->synchandler) (*svr->synchandler)(svr)

/*
 * Data - Place data in the buffer and pad the end to provide
 * 32 bit word alignment.  Transmit if the buffer fills.
 *
 * "svr" is a pointer to a Display.
 * "data" is a pinter to a data buffer.
 * "len" is the length of the data buffer.
 * we can presume buffer less than 2^16 bytes, so bcopy can be used safely.
 */

#ifdef DataRoutineIsProcedure
extern void Data();

#else
#define Data(svr, data, len) \
	if (svr->bufptr + (len) <= svr->bufmax) {\
		bcopy(data, svr->bufptr, (int)len);\
		svr->bufptr += ((len) + 3) & ~3;\
	} else\
		_FSSend(svr, data, len)
#endif				/* DataRoutineIsProcedure */


/* Allocate bytes from the buffer.  No padding is done, so if
 * the length is not a multiple of 4, the caller must be
 * careful to leave the buffer aligned after sending the
 * current request.
 *
 * "type" is the type of the pointer being assigned to.
 * "ptr" is the pointer being assigned to.
 * "n" is the number of bytes to allocate.
 *
 * Example:
 *    xTextElt *elt;
 *    BufAlloc (xTextElt *, elt, nbytes)
 */

#define BufAlloc(type, ptr, n) \
    if (svr->bufptr + (n) > svr->bufmax) \
        _FSFlush (svr); \
    ptr = (type) svr->bufptr; \
    svr->bufptr += (n);

/*
 * provide emulation routines for smaller architectures
 */
#ifndef WORD64
#define Data16(dpy, data, len) Data((dpy), (char *)(data), (len))
#define Data32(dpy, data, len) Data((dpy), (char *)(data), (len))
#define _FSRead16Pad(dpy, data, len) _FSReadPad((dpy), (char *)(data), (len))
#define _FSRead16(dpy, data, len) _FSRead((dpy), (char *)(data), (len))
#define _FSRead32(dpy, data, len) _FSRead((dpy), (char *)(data), (len))
#endif /* not WORD64 */

#define PackData16(dpy,data,len) Data16 (dpy, data, len)
#define PackData32(dpy,data,len) Data32 (dpy, data, len)

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef MUSTCOPY
/* a little bit of magic */
#define OneDataCard32(svr,dstaddr,srcvar) \
  { svr->bufptr -= 4; Data32 (svr, (char *) &(srcvar), 4); }

#define STARTITERATE(tpvar,type,start,endcond,decr) \
  { register char *cpvar; \
  for (cpvar = (char *) start; endcond; cpvar = NEXTPTR(cpvar,type), decr) { \
    type dummy; bcopy (cpvar, (char *) &dummy, SIZEOF(type)); \
    tpvar = (type *) cpvar;
#define ENDITERATE }}

#else

/* srcvar must be a variable for large architecture version */
#define OneDataCard32(svr,dstaddr,srcvar) \
  { *(unsigned long *)(dstaddr) = (srcvar); }

#define STARTITERATE(tpvar,type,start,endcond,decr) \
  for (tpvar = (type *) start; endcond; tpvar++, decr) {
#define ENDITERATE }
#endif				/* MUSTCOPY - used machines whose C structs
#ident	"@(#)r5fonts:lib/fs/FSlibint.h	1.2"
				 * don't line up with proto */
