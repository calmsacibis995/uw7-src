/*
 *	@(#)s3cGC.c	6.1	3/20/96	10:23:11
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
 * SCO Modification History
 *
 * S004, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S003, 11-May-93, staceyc
 * 	include file cleanup
 * S002	Thu Oct 29 17:35:16 PST 1992	mikep@sco.com
 *	Add s3cValidateWindowGC16 for 16 color mode.  This
 *	means you can have a multiheaded server, one in 16 bit
 *	mode and the other in 8.
 * S001	Fri Aug 28 15:19:30 PDT 1992	hiramc@sco.COM
 *	remove all previous Modification history here.
 *	Remove the #include "cfb.h" - it isn't needed
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

extern nfbGCOps 	S3CNAME(SolidPrivOps);
extern nfbGCOps 	S3CNAME(TiledPrivOps);
extern nfbGCOps 	S3CNAME(StippledPrivOps);
extern nfbGCOps 	S3CNAME(OpStippledPrivOps);

extern nfbGCOps 	S3CNAME(SolidPrivOps16);
extern nfbGCOps 	S3CNAME(TiledPrivOps16);
extern nfbGCOps 	S3CNAME(StippledPrivOps16);
extern nfbGCOps 	S3CNAME(OpStippledPrivOps16);


/*
 *  s3cValidateWindowGC() -- Validate Window Graphics Context
 *
 *	This routine will set the nfb GC private ops, nfbHelpValidateGC() 
 *	will do most of the work.
 */

void
S3CNAME(ValidateWindowGC)(
	GCPtr 		pGC,
	Mask 		changes,
	DrawablePtr 	pDraw)
{
	nfbGCPrivPtr 	pPriv = NFB_GC_PRIV(pGC);

	/*
	 * set the private ops based on fill style 
	 */

	if ( changes & GCFillStyle )
	{
		switch ( pGC->fillStyle ) 
		{
		    	case FillSolid:
				pPriv->ops = &S3CNAME(SolidPrivOps);
				break;

		    	case FillTiled:
				pPriv->ops = &S3CNAME(TiledPrivOps);
				break;

		    	case FillStippled:
				pPriv->ops = &S3CNAME(StippledPrivOps);
				break;

		    	case FillOpaqueStippled:
				if (pGC->fgPixel == pGC->bgPixel)
			    		pPriv->ops = &S3CNAME(SolidPrivOps);
				else
			    		pPriv->ops = &S3CNAME(OpStippledPrivOps);
				break;

		    	default:
				FatalError("s3cValidateWindowGC(): %s\n",
					"illegal fillStyle");
		}
	}
	nfbHelpValidateGC(pGC, changes, pDraw);
}


void
S3CNAME(ValidateWindowGC16)(
	GCPtr 		pGC,
	Mask 		changes,
	DrawablePtr 	pDraw)
{
	nfbGCPrivPtr 	pPriv = NFB_GC_PRIV(pGC);

	/*
	 * set the private ops based on fill style 
	 */

	if ( changes & GCFillStyle )
	{
		switch ( pGC->fillStyle ) 
		{
		    	case FillSolid:
				pPriv->ops = &S3CNAME(SolidPrivOps16);
				break;

		    	case FillTiled:
				pPriv->ops = &S3CNAME(TiledPrivOps16);
				break;

		    	case FillStippled:
				pPriv->ops = &S3CNAME(StippledPrivOps16);
				break;

		    	case FillOpaqueStippled:
				if (pGC->fgPixel == pGC->bgPixel)
			    		pPriv->ops = &S3CNAME(SolidPrivOps16);
				else
			    		pPriv->ops = &S3CNAME(OpStippledPrivOps16);
				break;

		    	default:
				FatalError("s3cValidateWindowGC(): %s\n",
					"illegal fillStyle");
		}
	}
	nfbHelpValidateGC(pGC, changes, pDraw);
}
