/*
 *	@(#)s3cInit.c	6.1	3/20/96	10:23:20
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
 * Modification History
 *
 * S033, 13-Aug-93, staceyc
 * 	don't fail if memory range not specified in grafinfo file
 * S032, 30-Jun-93, staceyc
 * 	fix problems with screen private allocations for multiheaded cards
 * S031, 27-May-93, staceyc
 * 	enable direct memory access to framebuffer
 * S030, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S029, 11-May-93, staceyc
 * 	include file cleanup
 * S028	Thu Apr 08 15:56:47 PDT 1993	hiramc@sco.COM
 *	Missing break statements in a switch statement.
 *	More info on a FatalError exit, although I can't
 *	always get it to be in Text mode properly when it exits ?
 * S027, 05-Apr-93, staceyc
 * 	Merged in Xware changes with our original source, development in
 *	this file was occurring in two seperate source trees at the same
 *	time, for old SCO comments in this file check sccs ids from 1.10.
 *	For old Xware comments see rickk for source.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

unsigned char	S3CNAME(RasterOps)[16];
int 		S3CNAME(ScreenPrivateIndex) = -1;
int		S3CNAME(ServerGeneration) = -1;
#if S3C_MULTIHEAD_SUPPORT == 1
int		S3CNAME(LastScreen) = -1;
#endif

static int
s3cRightMostOne(unsigned long	word);			/*	X027	*/

static char	*s3cVisualNames[] =
{
	"StaticGray",
	"GrayScale",
	"StaticColor",
	"PseudoColor",
	"TrueColor",
	"DirectColor"
};


/*
 *  s3cProbe() -- Probe for graphics adapter.
 *
 *	This routine tests the graphics board if the 86c911 based board
 *	is installed, and returns true if its present, false otherwise.
 *
 */

Bool
S3CNAME(Probe)(
	ddxDOVersionID		version,
	ddxScreenRequest	*pReq)
{
	int 			present;
	int			crx;
	int			crd;

	/*
	 * determine base I/O address
	 */

	if ( S3C_INB(S3C_MISCO_RD) & S3C_MISCO_IOASEL_COLOR )
	{
		crx = S3C_CRX_COLOR;
		crd = S3C_CRD_COLOR;
	}
	else
	{
		crx = S3C_CRX_MONO;
		crd = S3C_CRD_MONO;
	}

	/*
	 * unlock the S3 registers and enable
	 * the S3 enhanced (8514) registers
	 */

	S3C_OUTB(crx, S3C_S3R8);
	S3C_OUTB(crd, S3C_S3R8_KEY1);

	S3C_OUTB(crx, S3C_S3R9);
	S3C_OUTB(crd, S3C_S3R9_KEY2);

	S3C_OUTB(crx, S3C_SYS_CNFG);
	S3C_OUTB(crd, S3C_INB(crd) | S3C_SYS_CNFG_8514);

	S3C_OUT(S3C_ERROR_ACC, 0x0123);
	present = S3C_INW(S3C_ERROR_ACC) == 0x0123;

	/*
	 * lock the S3 registers and disable
	 * the S3 enhanced (8514) registers
	 */

	S3C_OUTB(crx, S3C_S3R8);
	S3C_OUTB(crd, ~S3C_S3R8_KEY1);

	S3C_OUTB(crx, S3C_S3R9);
	S3C_OUTB(crd, ~S3C_S3R9_KEY2);

	S3C_OUTB(crx, S3C_SYS_CNFG);
	S3C_OUTB(crd, S3C_INB(crd) & ~S3C_SYS_CNFG_8514);

	/*
	 * if it is not present return false, otherwise setup ddx
	 */

	if ( !present )
	{
		return(FALSE);
	}
	return(ddxAddPixmapFormat(pReq->dfltDepth,pReq->dfltBpp,pReq->dfltPad));
}


/*
 *  s3cInit() -- Initialize screen info and graphics adapter.
 *
 *	This routine will parse the DATA section of the grafinfo file
 *	initialize both public and private data structures, and initialize
 *	the graphics adapter.
 *
 */

Bool
S3CNAME(Init)(
	int 			index,
	ScreenPtr 		pScreen,
	int 			argc,
	char 			**argv)
{
	unsigned int 		width;
	unsigned int 		height;
	unsigned int 		nplanes;
	unsigned int		width_max;
	unsigned int		height_max;

	int 			rgb_bits;
	grafData 		*grafinfo;
	int 			colors;
	nfbScrnPrivPtr 		pNfb;
	int 			mmx;
	int 			mmy;
	int			console_type;
	s3cPrivateData_t 	*s3cPriv;
	s3cHWCursorData_t	*s3cHWCurs;
	s3cSWCursorData_t	*s3cSWCurs;
	
	extern VisualRec 	S3CNAME(Visual);
	extern nfbGCOps 	S3CNAME(SolidPrivOps);
	extern scoScreenInfo 	S3CNAME(SysInfo);


	if (S3CNAME(ServerGeneration) != serverGeneration)
	{
		S3CNAME(ScreenPrivateIndex) = AllocateScreenPrivateIndex();
		S3CNAME(ServerGeneration) = serverGeneration;
		if (S3CNAME(ScreenPrivateIndex) < 0)
			return FALSE;
	}
	s3cPriv	= (s3cPrivateData_t *)xalloc(sizeof(s3cPrivateData_t));
	S3C_PRIVATE_DATA(pScreen) = s3cPriv;
	s3cHWCurs = &s3cPriv->hw_cursor;
	s3cSWCurs = &s3cPriv->sw_cursor;
	
	/*
	 * read resolution parameters from grafinfo file
	 */

	grafinfo = DDX_GRAFINFO(pScreen);
	s3cPriv->graf_data = grafinfo;

	S3CNAME(InitializeMultihead)(pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	if ( grafGetInt(grafinfo, "PIXWIDTH", &s3cPriv->width) == FAILURE 
		|| s3cPriv->width <= 0 )
	{
		FatalError("s3cInit(): %s.\n",
			"missing or invalid grafinfo value for PIXWIDTH");
	}

	if ( grafGetInt(grafinfo, "PIXHEIGHT", &s3cPriv->height) == FAILURE 
		|| s3cPriv->height <= 0 )
	{
		FatalError("s3cInit(): %s.\n",
			"missing or invalid grafinfo value for PIXHEIGHT");
	}

	if ( grafGetInt(grafinfo, "DEPTH", &s3cPriv->depth) == FAILURE )
	{
		FatalError("s3cInit(): %s.\n",
			"missing grafinfo value for DEPTH");
	}
	else
	{
		switch( s3cPriv->depth )
		{
			case 4:
			case 8:
			case 16:
				break;

			default:
				FatalError("s3cInit(): %s.\n",
					"invalid grafinfo value for DEPTH");
		}
	}	

	grafGetMemInfo(grafinfo, NULL, NULL, &s3cPriv->fb_size,
	    &s3cPriv->fb_base);

	/*
	 * initialize visual parameters to the default values for given
	 * depth and allow grafinfo parameters to override the defaults
	 */

	if ( ( S3CNAME(Visual).nplanes = s3cPriv->depth ) != 16 )
	{
		grafGetInt(grafinfo, "RGBBITS",
		    &S3CNAME(Visual).bitsPerRGBValue);

		s3cPriv->dac_shift 
			= sizeof(short) * 8 - S3CNAME(Visual).bitsPerRGBValue;

		S3CNAME(Visual).ColormapEntries = 1 << S3CNAME(Visual).nplanes;

		colors = S3CNAME(Visual).ColormapEntries;
	}
	else
	{
		S3CNAME(Visual).class 		= TrueColor;
		S3CNAME(Visual).bitsPerRGBValue 	= 5;
		S3CNAME(Visual).redMask 		= 0x7C00;
		S3CNAME(Visual).greenMask 		= 0x03E0;
		S3CNAME(Visual).blueMask 		= 0x001F;

		grafGetInt(grafinfo, "RGBBITS",
		    &S3CNAME(Visual).bitsPerRGBValue);
		grafGetInt(grafinfo, "REDMASK", &S3CNAME(Visual).redMask);
		grafGetInt(grafinfo, "GRNMASK", &S3CNAME(Visual).greenMask);
		grafGetInt(grafinfo, "BLUMASK", &S3CNAME(Visual).blueMask);

		S3CNAME(Visual).offsetRed 
			= s3cRightMostOne(S3CNAME(Visual).redMask);
		S3CNAME(Visual).offsetGreen 
			= s3cRightMostOne(S3CNAME(Visual).greenMask);
		S3CNAME(Visual).offsetBlue 
			= s3cRightMostOne(S3CNAME(Visual).blueMask);
		S3CNAME(Visual).ColormapEntries 
			= 1 << S3CNAME(Visual).bitsPerRGBValue;

		colors = 1 
			+ S3CNAME(Visual).redMask 
			+ S3CNAME(Visual).greenMask 
			+ S3CNAME(Visual).blueMask;
	}
	
	/*
	 * get adjust CR13 flag if present, otherwise use default
	 */

	if ( grafGetInt(grafinfo, "ADJCR13", &s3cPriv->adjust_cr13) == FAILURE)
	{
		s3cPriv->adjust_cr13 = TRUE;
	}

	/* S034
	 * get mode assist flag if present, otherwise use default
	 */

	if ( grafGetInt(grafinfo,
			"MODEASSIST", &s3cPriv->mode_assist) == FAILURE)
	{
		s3cPriv->mode_assist = TRUE;
	}

	/*
	 * get monitor info if present, otherwise default to 13" diag
	 */

	if ( grafGetInt(grafinfo, "MON_WIDTH",  &mmx) == FAILURE 
		|| grafGetInt(grafinfo, "MON_HEIGHT", &mmy) == FAILURE ) 
	{
		mmx = 250;
		mmy = 180;
	}

	/*
	 * determine base I/O address
	 */

	if ( S3C_INB(S3C_MISCO_RD) & S3C_MISCO_IOASEL_COLOR )
	{
		s3cPriv->stat = S3C_STAT1_RD_COLOR;
		s3cPriv->crx = S3C_CRX_COLOR;
		s3cPriv->crd = S3C_CRD_COLOR;
	}
	else
	{
		s3cPriv->stat = S3C_STAT1_RD_MONO;
		s3cPriv->crx = S3C_CRX_MONO;
		s3cPriv->crd = S3C_CRD_MONO;
	}

	/*
	 * unlock the S3 registers and determine if we are running 
	 * single or dual monitor, and save the mode of the console 
	 * if we are in single monitor mode.
	 */

	S3C_OUTB(s3cPriv->crx, S3C_S3R8);
	S3C_OUTB(s3cPriv->crd, S3C_S3R8_KEY1);

	S3C_OUTB(s3cPriv->crx, S3C_S3R2);
	
	/*console_type = ioctl(1, CONS_CURRENT, 0);*/
	console_type = VGA;

	if ( (( S3C_INB(s3cPriv->crd) & S3C_S3R2_EMUMODE ) == S3C_S3R2_EMULVGA
			&& console_type == VGA )
	    || (( S3C_INB(s3cPriv->crd) & S3C_S3R2_EMUMODE ) != S3C_S3R2_EMULVGA
			&& console_type != VGA ) )
	{
		s3cPriv->console_single = TRUE;
	}
	else
	{
		s3cPriv->console_single = FALSE;
	}

	s3cPriv->console_changed = FALSE;

	/*
	 * read the display memory size from the board
	 */

	S3C_OUTB(s3cPriv->crx, S3C_S3R6);
	
	switch ( S3C_INB(s3cPriv->crd) & S3C_S3R6_DISPMEMSZ )
	{
		case S3C_S3R6_DISPMEMSZ_512K:
			s3cPriv->dispmem_size = 512;
			break;

		case S3C_S3R6_DISPMEMSZ_1M:
			s3cPriv->dispmem_size = 1024;
			break;

                case S3C_S3R6_DISPMEMSZ_2M:
                        s3cPriv->dispmem_size = 2048;
                        break;

		default: /* X025 some cards do not report mem size properly */
			s3cPriv->dispmem_size = 1024;	/*	X025	*/
			ErrorF("s3cInit(): %s\n",
		"card not reporting memory size correctly, assuming 1 Meg." );
			break;	/*	X025	*/
	}

	S3C_OUTB(s3cPriv->crx, S3C_S3R8);
	S3C_OUTB(s3cPriv->crd, ~S3C_S3R8_KEY1);

	/*
	 * set hardware clipping limits and allocate off screen memory
	 */

	s3cPriv->cursor_type = S3C_CURSOR_TYPE_HW;

	switch (s3cPriv->dispmem_size)
	{
	    case 512 :
		if ( s3cPriv->depth == 4 )
		{
			width_max = 1024;
			height_max = 768;

			s3cPriv->card_clip.x 		= 1023;
			s3cPriv->card_clip.y 		= 1023;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 768;
			s3cPriv->off_screen_blit_area.w = 1024;
			s3cPriv->off_screen_blit_area.h = 128;
		
			s3cPriv->tile_blit_area.x 	= 0;
			s3cPriv->tile_blit_area.y 	= 896;
			s3cPriv->tile_blit_area.w 	= 1024;
			s3cPriv->tile_blit_area.h 	= 126;

			s3cHWCurs->source_y             = 511;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 1022;
			s3cHWCurs->off_screen.w		= 1024;
			s3cHWCurs->off_screen.h		= 2;
		}
		else if ( s3cPriv->depth == 8 )
		{
			width_max = 640;
			height_max = 480;

			s3cPriv->card_clip.x 		= 1023;
			s3cPriv->card_clip.y 		= 511;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 480;
			s3cPriv->off_screen_blit_area.w = 1024;
			s3cPriv->off_screen_blit_area.h = 30;
	
			s3cPriv->tile_blit_area.x 	= 640;
			s3cPriv->tile_blit_area.y 	= 0;
			s3cPriv->tile_blit_area.w 	= 384;
			s3cPriv->tile_blit_area.h 	= 480;

			s3cHWCurs->source_y             = 511;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 511;
			s3cHWCurs->off_screen.w		= 1024;
			s3cHWCurs->off_screen.h		= 1;
		}
		break;					/*	S028	*/
	    case 1024 :
		if ( s3cPriv->depth == 4 )
		{
			width_max = 1280;
			height_max = 
				( s3cPriv->height <= 1022 ? 1022 : 1024 );
			
			s3cPriv->card_clip.x 		= 2047;
			s3cPriv->card_clip.y 		= 1023;
	
			s3cPriv->off_screen_blit_area.x	= 1280;
			s3cPriv->off_screen_blit_area.y	= 0;
			s3cPriv->off_screen_blit_area.w = 768;
			s3cPriv->off_screen_blit_area.h = 512;

			s3cPriv->tile_blit_area.x 	= 1280;
			s3cPriv->tile_blit_area.y 	= 512;
			s3cPriv->tile_blit_area.w 	= 768;
			s3cPriv->tile_blit_area.h 	= 
				( height_max != 1024  ? 
					510 : 512 - S3C_MAX_CURSOR_SAVE );
	
			if  ( height_max != 1024 )
			{
				s3cHWCurs->source_y     = 1023;
				s3cHWCurs->off_screen.x	= 0;
				s3cHWCurs->off_screen.y	= 1023;
				s3cHWCurs->off_screen.w	= 2048;
				s3cHWCurs->off_screen.h	= 1;
			}
			else
			{
				s3cPriv->cursor_type = S3C_CURSOR_TYPE_SW;

				s3cSWCurs->save.x = 1280;
				s3cSWCurs->save.y = 1023 - S3C_MAX_CURSOR_SAVE;

				s3cSWCurs->source.x = s3cSWCurs->save.x 
					+ S3C_MAX_CURSOR_SAVE;
				s3cSWCurs->source.y = s3cSWCurs->save.y;

				s3cSWCurs->mask.x = s3cSWCurs->source.x 
					+ S3C_MAX_CURSOR_SAVE;
				s3cSWCurs->mask.y = s3cSWCurs->save.y;
			}
		}		
		else if ( s3cPriv->depth == 8 )
		{
			width_max = 1024;
			height_max = 768;

			s3cPriv->card_clip.x 		= 1023;
			s3cPriv->card_clip.y 		= 1023;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 768;
			s3cPriv->off_screen_blit_area.w = 1024;
			s3cPriv->off_screen_blit_area.h = 128;

			s3cPriv->tile_blit_area.x 	= 0;
			s3cPriv->tile_blit_area.y 	= 896;
			s3cPriv->tile_blit_area.w 	= 1024;
			s3cPriv->tile_blit_area.h 	= 126;

			s3cHWCurs->source_y             = 1023;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 1023;
			s3cHWCurs->off_screen.w		= 1024;
			s3cHWCurs->off_screen.h		= 1;
		}
		else if ( s3cPriv->depth == 16 )
		{
			width_max = 640;
			height_max = 480;

			s3cPriv->card_clip.x 		= 2047;
			s3cPriv->card_clip.y 		= 511;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 480;
			s3cPriv->off_screen_blit_area.w = 2048;
			s3cPriv->off_screen_blit_area.h = 30;
	
			s3cPriv->tile_blit_area.x 	= 640;
			s3cPriv->tile_blit_area.y 	= 0;
			s3cPriv->tile_blit_area.w 	= 384;
			s3cPriv->tile_blit_area.h 	= 480;

			s3cHWCurs->source_y             = 1022;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 511;
			s3cHWCurs->off_screen.w		= 2048;
			s3cHWCurs->off_screen.h		= 1;

		}
		break;					/*	S028	*/
	    case 2048 :
		/*
		 * 2MB memory support is limited to 1280x1024x256,
		 * the rest is cloned from above for now
		 */

		if ( s3cPriv->depth == 4 )
		{
			width_max = 1280;
			height_max = 
				( s3cPriv->height <= 1022 ? 1022 : 1024 );
			
			s3cPriv->card_clip.x 		= 2047;
			s3cPriv->card_clip.y 		= 1023;
	
			s3cPriv->off_screen_blit_area.x	= 1280;
			s3cPriv->off_screen_blit_area.y	= 0;
			s3cPriv->off_screen_blit_area.w = 768;
			s3cPriv->off_screen_blit_area.h = 512;

			s3cPriv->tile_blit_area.x 	= 1280;
			s3cPriv->tile_blit_area.y 	= 512;
			s3cPriv->tile_blit_area.w 	= 768;
			s3cPriv->tile_blit_area.h 	= 
				( height_max != 1024  ? 
					510 : 512 - S3C_MAX_CURSOR_SAVE );
	
			if  ( height_max != 1024 )
			{
				s3cHWCurs->source_y	= 1023;
				s3cHWCurs->off_screen.x	= 0;
				s3cHWCurs->off_screen.y	= 1023;
				s3cHWCurs->off_screen.w	= 2048;
				s3cHWCurs->off_screen.h	= 1;
			}
			else
			{
				s3cPriv->cursor_type = S3C_CURSOR_TYPE_SW;

				s3cSWCurs->save.x = 1280;
				s3cSWCurs->save.y = 1023 - S3C_MAX_CURSOR_SAVE;

				s3cSWCurs->source.x = s3cSWCurs->save.x 
					+ S3C_MAX_CURSOR_SAVE;
				s3cSWCurs->source.y = s3cSWCurs->save.y;

				s3cSWCurs->mask.x = s3cSWCurs->source.x 
					+ S3C_MAX_CURSOR_SAVE;
				s3cSWCurs->mask.y = s3cSWCurs->save.y;
			}
		}		
		else if ( s3cPriv->depth == 8 && s3cPriv->width <= 1024 )
		{
			width_max = 1024;
			height_max = 768;

			s3cPriv->card_clip.x 		= 1023;
			s3cPriv->card_clip.y 		= 1023;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 768;
			s3cPriv->off_screen_blit_area.w = 1024;
			s3cPriv->off_screen_blit_area.h = 128;

			s3cPriv->tile_blit_area.x 	= 0;
			s3cPriv->tile_blit_area.y 	= 896;
			s3cPriv->tile_blit_area.w 	= 1024;
			s3cPriv->tile_blit_area.h 	= 126;

			s3cHWCurs->source_y		= 1023;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 1023;
			s3cHWCurs->off_screen.w		= 1024;
			s3cHWCurs->off_screen.h		= 1;
		}
		else if ( s3cPriv->depth == 8 && s3cPriv->width > 1024 )
		{
			width_max = 1280;
			height_max = 1024;

			s3cPriv->card_clip.x 		= 1279;
			s3cPriv->card_clip.y 		= 1637;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 1024;
			s3cPriv->off_screen_blit_area.w = 1280;
			s3cPriv->off_screen_blit_area.h = 512;

			s3cPriv->tile_blit_area.x 	= 0;
			s3cPriv->tile_blit_area.y 	= 1330;
			s3cPriv->tile_blit_area.w 	= 1280;
			s3cPriv->tile_blit_area.h 	= 100;

			s3cHWCurs->source_y		= 2045;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 1636;
			s3cHWCurs->off_screen.w		= 1024;
			s3cHWCurs->off_screen.h		= 1;
		}
		else if ( s3cPriv->depth == 16 )
		{
			width_max = 640;
			height_max = 480;

			s3cPriv->card_clip.x 		= 2047;
			s3cPriv->card_clip.y 		= 511;

			s3cPriv->off_screen_blit_area.x	= 0;
			s3cPriv->off_screen_blit_area.y	= 480;
			s3cPriv->off_screen_blit_area.w = 2048;
			s3cPriv->off_screen_blit_area.h = 30;
	
			s3cPriv->tile_blit_area.x 	= 640;
			s3cPriv->tile_blit_area.y 	= 0;
			s3cPriv->tile_blit_area.w 	= 384;
			s3cPriv->tile_blit_area.h 	= 480;

			s3cHWCurs->source_y		= 1022;
			s3cHWCurs->off_screen.x		= 0;
			s3cHWCurs->off_screen.y		= 511;
			s3cHWCurs->off_screen.w		= 2048;
			s3cHWCurs->off_screen.h		= 1;
		}
	}	/*	switch (s3cPriv->dispmem_size)	*/	/*	 S028 */

	/*
	 * check if display memory can support grafinfo mode parameters
	 */

	if ( ( s3cPriv->width > width_max )
	    || ( s3cPriv->height > height_max ) )
	{
		S3CNAME(SetText)( pScreen );/* S028, doesn't really work all the way*/
		ErrorF("s3cInit(): requested width,height (%d, %d)\n",
			s3cPriv->width, s3cPriv->height ); /*	S028	*/
		ErrorF("s3cInit(): is greater than allowed width,height (%d, %d)\n",
			width_max, height_max );		/*	S028 */
		FatalError("s3cInit(): %s.\n",
		    "invalid grafinfo value for PIXWIDTH, PIXHEIGHT, or DEPTH");
	}

#ifdef NOT						/*	vvv S035 */
	/*
	 * display the server display configuration to the console
	 */

	ErrorF("s3cInit(): %dx%d %d%s %ss, %d%s of display memory found\n", 
		s3cPriv->width, 
		s3cPriv->height,
		( colors >= 1048576 ? colors / 1048576 : 
			( colors >= 1024 ? colors / 1024 : colors ) ),
		( colors >= 1048576 ? "M" : ( colors >= 1024 ? "K" : "" ) ),
		s3cVisualNames[S3CNAME(Visual).class],
		( s3cPriv->dispmem_size >= 1024 ? 
			s3cPriv->dispmem_size / 1024 : s3cPriv->dispmem_size),
		( s3cPriv->dispmem_size >= 1024 ? "M" : "K" ));
#endif							/*	^^^ S035 */

	/*
	 * initialize the nfb layer
	 */

	if (!nfbScreenInit(pScreen, s3cPriv->width, s3cPriv->height, mmx, mmy))
		return FALSE;

	if (! nfbAddVisual(pScreen, &S3CNAME(Visual)))
		return FALSE;

	/*	X026	Have to wrap these functions */
	/*	S030	CloseScreen wrap removed	*/

	s3cPriv->QueryBestSize = pScreen->QueryBestSize; /* maybe not this ? */

	pScreen->QueryBestSize 		= S3CNAME(QueryBestSize);

	pNfb = NFB_SCREEN_PRIV(pScreen);

	pNfb->protoGCPriv->ops	 	= &S3CNAME(SolidPrivOps);
	pNfb->SetColor		 	= S3CNAME(SetColor);
	pNfb->LoadColormap	 	= S3CNAME(LoadColormap);
	pNfb->BlankScreen	 	= S3CNAME(BlankScreen);
	pNfb->ClearFont 	 	= S3CNAME(GlClearCache);
	pNfb->ValidateWindowPriv 	= S3CNAME(ValidateWindowPriv);

	/*
 	 * initialize the table used to convert GC alu functions 
	 * to the 86c911 raster operation function.
	 */

	S3CNAME(RasterOps)[GXclear]	 	= 0x01;
	S3CNAME(RasterOps)[GXand] 		= 0x0C;
	S3CNAME(RasterOps)[GXandReverse] 	= 0x0D;
	S3CNAME(RasterOps)[GXcopy] 		= 0x07;
	S3CNAME(RasterOps)[GXandInverted] 	= 0x0E;
	S3CNAME(RasterOps)[GXnoop] 		= 0x03;
	S3CNAME(RasterOps)[GXxor] 		= 0x05;
	S3CNAME(RasterOps)[GXor] 		= 0x0B;
	S3CNAME(RasterOps)[GXnor] 		= 0x0F;
	S3CNAME(RasterOps)[GXequiv] 		= 0x06;
	S3CNAME(RasterOps)[GXinvert] 		= 0x00;
	S3CNAME(RasterOps)[GXorReverse] 	= 0x0A;
	S3CNAME(RasterOps)[GXcopyInverted] 	= 0x04;
	S3CNAME(RasterOps)[GXorInverted] 	= 0x09;
	S3CNAME(RasterOps)[GXnand] 		= 0x08;
	S3CNAME(RasterOps)[GXset]	 	= 0x02;
					
	/*
	 * initialize the display
	 */

	S3CNAME(SetGraphics)(pScreen);
						/*	S031	*/
	S3CNAME(GlCacheInit)(pScreen);

	/*
	 * initialize the cursor
	 */

	if ( s3cPriv->cursor_type == S3C_CURSOR_TYPE_HW )
		S3CNAME(HWCursorInitialize)(pScreen);
	else
		S3CNAME(SWCursorInitialize)(pScreen);

	/*
	 * create the default colormap
	 */

	if ( cfbCreateDefColormap(pScreen) == 0 )
		return(FALSE);

	/*
	 * give sco layer screen switch functions
	 */

	scoSysInfoInit(pScreen, &S3CNAME(SysInfo));

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);	/* S029, S033 */

	return(TRUE);
}


/*
 *  s3cRightMostOne() -- Right Most One
 *
 *	This routine will return the bit position of the right most one
 *	of the word passed.
 *
 */

static int
s3cRightMostOne( unsigned long	word)			/*	X027	*/
{
	int		b;

	if( ! word )		/*	protect against all zero word */
		return( 0 );	/*	something is wrong	X027	*/
	for ( b = 0 ; !(word & 1) ; word >>= 1, b++ )
		;
	
	return(b);
}
