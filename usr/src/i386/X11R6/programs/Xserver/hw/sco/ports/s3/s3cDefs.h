/*
 *	@(#)s3cDefs.h	6.1	3/20/96	10:23:09
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
 * S023, 30-Jun-93, staceyc
 * 	move multichip/headed card include file into here
 * S022, 02-Jun-93, staceyc
 * 	additions to hw cursor struct to correctly deal with cursor color
 * S021, 27-May-93, staceyc
 * 	support for direct access to framebuffer
 * S020, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S019, 11-May-93, staceyc
 * 	include file cleanup
 * S018	Mon Apr 05 10:00:51 PDT 1993	hiramc@sco.COM
 *	Add Kevin's changes for 86C80[15], 86C928
 * S017	Tue Nov 10 18:02:06 PST 1992	buckm@sco.COM
 *	Add mode_assist to the screen privates.
 * S016	Thu Sep 24 08:46:54 PDT 1992	hiramc@sco.COM
 *	Change the bit swap stuff to reference the array
 *	in ddxMisc.c
 * X015	Tue Sep 08 11:30:06 PDT 1992	hiramc@sco.COM
 *	Add bit swap stuff, remove all mod history comments
 *	prior to 1992
 * X014	Sun Jun 14 23:17:52 PDT 1992	buckm@sco.COM
 *	No need to store colors for software cursor.
 * X013	Tue Jun 02 14:08:56 PDT 1992	hiramc@sco.COM
 *	Provide wrapper info in the private structure.
 * X012 11-Jan-92 kevin@xware.com
 *      added intl_fix to s3cHWCursorData_t to workaround the problem dealing
 *	with the cursor y offset in the interlaced display modes.
 * X011 11-Jan-92 kevin@xware.com
 *      added support for software cursor for 1280x1024 display modes.
 * X010 02-Jan-92 kevin@xware.com
 *      added support for mode selectable hardware or software cursor.
 * X009 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 */

#ifndef	_S3CDEFS_H_
#define	_S3CDEFS_H_

#include "s3cScreen.h"

#define S3C_NORMAL_MODE -1

extern struct _nfbGCOps	s3cSolidPrivOps;

typedef unsigned int	s3cGlFontID_t;

typedef struct s3cGlGlyphInfo_t 
{
	int 			plane;
	DDXPointRec 		coords;
} s3cGlGlyphInfo_t;

typedef struct s3cGlCachePosition_t 
{
	Bool 			exists;
	int 			plane;
	s3cGlFontID_t 		font_id;
	unsigned int 		byteOffset;
	DDXPointRec 		coords;
} s3cGlCachePosition_t;

typedef struct s3cGlListEntry_t 
{
	s3cGlFontID_t 		font_id;
	unsigned int 		byteOffset;
	DDXPointRec 		coords;
	struct s3cGlListEntry_t *next;
} s3cGlListEntry_t;

typedef struct s3cGlPlaneListInfo_t 
{
	s3cGlListEntry_t 	*busy_list;
	s3cGlListEntry_t 	*free_list;
} s3cGlPlaneListInfo_t;

typedef struct s3cGlHashHead_t 
{
	s3cGlCachePosition_t 	*cache_positions;
	int 			bucket_size;
} s3cGlHashHead_t;

typedef struct s3cFontData_t 
{
	int 			cache_plane_count;
	s3cGlPlaneListInfo_t 	**plane_list_info;
	int 			hash_table_size;
	int 			initial_bucket_size;
	s3cGlHashHead_t 	**hash_table;
} s3cFontData_t;


typedef struct s3cOffScreenBlitArea_t 
{
	int 			x;
	int			y;
	int			w;
	int			h;
} s3cOffScreenBlitArea_t;

typedef s3cOffScreenBlitArea_t	s3cSWCursorSave_t;

typedef struct s3cHWCursorData_t 
{
	int			intl_fix;
	short 			xhot;
	short 			yhot;
	short			source_y;		/*	S018	*/
	s3cOffScreenBlitArea_t	off_screen;
	xColorItem		fore;
	xColorItem		mask;
	ColormapPtr		current_colormap;
	CursorPtr		current_cursor;
} s3cHWCursorData_t;

typedef struct s3cSWCursorData_t 
{
	CursorPtr 		current_cursor;
	int			width;
	int			height;
	DDXPointRec 		save;
	DDXPointRec 		source;
	DDXPointRec 		mask;
	unsigned int 		source_color;		/*	S018	*/
	unsigned int		mask_color;		/*	S018	*/
} s3cSWCursorData_t;


typedef struct s3cPrivateData_t 
{
	int 			width;
	int			height;
	int			depth;
	unsigned int 		dac_shift;
	int 			stat;
	int 			crx;
	int 			crd;
	int			mode_assist;			/* S017 */
	int			adjust_cr13;
	int 			dispmem_size;
	int			console_single;
	int			console_changed;
	int			cursor_type;
	s3cHWCursorData_t 	hw_cursor;
	s3cSWCursorData_t 	sw_cursor;
	s3cOffScreenBlitArea_t	off_screen_blit_area;
	s3cOffScreenBlitArea_t	tile_blit_area;
	s3cOffScreenBlitArea_t	all_off_screen;
	s3cFontData_t		font_data;
	grafData		*graf_data;
	DDXPointRec		card_clip;
	Bool		(*CloseScreen)();
	Bool		(*QueryBestSize)();
#if S3C_MULTIHEAD_SUPPORT == 1
	s3cMultihead_t		multihead;
#endif
	unsigned char		*fb_base;
	int			fb_size;
} s3cPrivateData_t;

#if	BITMAP_BIT_ORDER == LSBFirst		/*	X015	S016 vvv*/
#define	DDXBITSWAP(u)	ddxBitSwap[u]
#else
#define	DDXBITSWAP(u)	u
#endif						/*	X015	S016 ^^^*/

extern unsigned char S3CNAME(RasterOps)[];
extern int	S3CNAME(ScreenPrivateIndex);

#endif	/*	_S3CDEFS_H_	*/
