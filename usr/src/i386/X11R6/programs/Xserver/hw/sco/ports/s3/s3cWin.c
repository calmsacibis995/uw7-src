/*
 *	@(#)s3cWin.c	6.1	3/20/96	10:23:40
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History:
 *
 * S007, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S006, 11-May-93, staceyc
 * 	include file cleanup
 * S005 30-Oct-92 mikep@sco.com
 *	don't bail out if bitperPixels isn't what we expect
 * S004 29-Oct-92 mikep@sco.com
 *	add 16 bit support here
 * X003 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X002 31-Dec-91 kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X001 06-Dec-91 kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
 *  s3cValidateWindowPriv() -- Validate Window Private Ops
 *
 *	This routine will sets the window ops pointer in the nfb window
 *	priv. If you are supporting multiple depths/visuals you have to
 * 	test here and set ops to the correct version.
 *
 *	InputOnly windows will come in here with a depth of 0.  Be careful.
 */

extern nfbWinOps 	S3CNAME(WinOps);
extern nfbWinOps 	S3CNAME(WinOps16);

void
S3CNAME(ValidateWindowPriv)(
	WindowPtr 	pWin)
{

	switch (pWin->drawable.bitsPerPixel)
	{
	    case 8:
		(NFB_WINDOW_PRIV(pWin))->ops = &S3CNAME(WinOps);
		break;
	    case 16:
		(NFB_WINDOW_PRIV(pWin))->ops = &S3CNAME(WinOps16);
		break;
	}
}
