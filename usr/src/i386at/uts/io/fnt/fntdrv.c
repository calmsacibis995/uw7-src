/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ident	"@(#)fntdrv.c	1.6"
#ident	"$Header$"

/*
 * fntdrv.c, Memory Resident Font Module driver code.
 */

/*
 *	L000	7/3/97		rodneyh@sco.com
 *	- Remove Novell copyright, add SCO copyright.
 *	- Remove cmn_err sign on message.
 *	- Added fnt_getfont and fnt_setfont.
 *	  fnt_getfont not implemented so returns EINVAL, fnt_setfont handles
 *	  downloading of a new font for use with gsd.
 *	- Changed to allow fonts to be replaced, ie use KMA to hold fonts
 *	  rather than statically allocating them. Note that is not the correct
 *	  approach, the fonts are huge so the font server should run in user
 *	  space and serve the font data drirect from the BDF file.
 *	L001	24Jun97		rodneyh@sco.com
 *	- Fix for decode of input width 1 codesets other than the special case
 *	  of codeset 0. We were incorrectly trying to decode the row when it is
 *	  not actually specified.
 *	  Fix for MR ul97-17408
 *	L002	28Oct97		rodneyh@sco.com
 *	- Fix for panic during fnt_setfont() if the EUC width structure
 *	  pointer had not been initialised for the calling channel.
 *	- Fix for panic when cat'ing a binary file that contains illegal EUC
 *	  characters. This was caused by the decoding the illegal characters
 *	  which point outside of the font data matrix.
 *	  Fix for MR ul97-27909
 *	L003	17Oct97		rodneyh@sco.com
 *	- Change to fnt_getnitmap to correctly adjust the character index
 *	  into the alternate character set matrix.
 *	  Fix for ul97-23201.
 *
 */

/* L000 comment
 *
 * The basic unit we deal with here is a plane (which may be of height 1, ie
 * a line). Each plane is an array of pointers to pages (rows), each page is a
 * complete (ie no gaps) set of bitmaps for the font page (94 glyphs).
 * We take this approach because we are assuming that most pages are fully or
 * nearly fully populated, as are most planes (except for lines which we
 * special case) and that most cubes are sparsly populated with planes.
 * A side affect of this approach is that we have to take account of the glyph
 * output width (scrw) when we are calculating the offset from the row start to
 * the column, but that's not to scarey.
 * The only feasible memory based alternative is to have the planes contain
 * one pointer to each glyph or a pointer to the empty bitmap if the glyph is
 * not defined, disadvantage of this is there is a lot of wasted memory for
 * pointers when there are completely empty rows, plus since most not empty
 * rows are fully populated a pointer per glyph is a little heavyweight.
 */

#include <io/ldterm/euc.h>
#include <io/ws/chan.h>
#include <io/ws/ws.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>					/* L000 */
#include <io/fnt/fnt.h>
#include <io/gsd/gsd.h>
#include <svc/errno.h>					/* L000 */
#include <util/types.h>

#include <io/ddi.h>					/* Must be last */

/*
 * L000 begin
 *
 * Defines and variables to hold the planes of data
 */
#define MAX_PLANES ((0xFF - 0xA0) + 1)	/* Max number of planes possible */

/*
 * Note that this wastes a little memory because codeset 0 and the alt char set
 * never have more than one plane. In order to cut down on the waste we have
 * made the convention that the alt char set is plane 1 of codeset 0
 *
 * Each entry in the cset array is a pointer to a plane which me KME mem for.
 */
static unsigned char **cset[MAX_CODESET][MAX_PLANES];

/*
 * Array to keep the addresses and lengths of memory we KMA for fonts.
 * Note that we allow the statically allocated codesets to be overlayed, in
 * this case we KMA the memory as needed and make sure we set the pointers back
 * to the statics if we are asked to free the codeset. Effectively this means
 * we don't allow the statically allocated codesets to ever become empty.
 *
 * Allow +1 in this array to make room for the alternate character codeset.
 */
static fnt_codeset_t codeset_mem[MAX_CODESET][MAX_PLANES];	/* L000 end */

/*
 * int fntinit(void) - called from fnt_load()
 *	Initializes fnt.
 *
 * Calling/Exit status:
 *
 */
int
fntinit(void)
{
int i,j;							/* L000 */

	Gs.fnt_getbitmap = fnt_getbitmap;
	Gs.fnt_unlockbitmap = fnt_unlockbitmap;
	Gs.fnt_setfont = fnt_setfont;			/* L000 begin */
	Gs.fnt_getfont = fnt_getfont;
	/*
	 * We are going to make sure there are always enough codesets to
	 * display any error messages, so initialise all but 0 and alt to 
	 * NULL
	 */
	for(i = 0; i < MAX_CODESET; i++)
		for(j = 0; j < MAX_PLANES; j++){
			cset[i][j] = (unsigned char **)NULL;
			codeset_mem[i][j].addr = (void *)NULL;
			codeset_mem[i][j].length = 0;
		}

	cset[0][0] = codeset0data;
	cset[0][1] = altcharsetdata;		/* L000 end */

	fnt_init_flg = 1;
}


/* L000 comment
 *
 * Note that the size of this bitmap is the only place that we make an
 * assumption about the size of scrw, ie. Max of 4, this is kinda OK because
 * gsd is rife with the assumption that scrw is 2.
 */
unsigned char EmptyBitMap[] =
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* L000 begin
 *
 * unsigned char *
 * fnt_getbitmap(ws_channel_t *chp, wchar_t ch)
 *
 * Calling/Exit status:
 *	Called from gsd driver to get a pointer to a bitmap representing ch.
 *
 * Description:
 *	Returns a pointer to a bitmap, where a bitmap is an array of unsigned
 *	characters.
 *
 * New version of fnt_getbitmap to properly deal with packed EUC, note the
 * following assumptions.
 *
 * i)	We can only handle upto input width 3 characters, ie no more than
 *	a cube.
 *
 * ii)	We assume that any cubes will be relatively sparse but the planes
 *	within the cube will be well populated. Ie. There will be a few fullish
 *	planes within each cube.
 *
 * iii)	Each character bitmap is 8x16 bits, ie 16 bytes, but we make no 
 *	assumption here about how many bitmaps are needed to display a
 *	character, we get that information from scrw.
 *	
 * Note:
 *	We are treating a line as a height one plane, ie, row = 0.
 *	I have no idea why but all the EUC bytes we get here have already had
 *	the top bit stripped except for the alt char set codeset 0 bytes. A
 *	quick search reveals gsd/gsdtcl.c:gcl_norm is the culprit, this may
 *	need to change for unicode support.
 */
unsigned char *
fnt_getbitmap(ws_channel_t *chp, wchar_t ch)
{
int eucw, scrw;			/* eucw is input width, scrw is output width */
unsigned int row, column, plane;
unsigned int codeset;
unsigned char **base;		/* Pointer to the plane */
unsigned char *page;		/* Pointer to the page (row) */


	switch(ch & EUCMASK){

	case P00:
		/*
		 * Codeset 0
		 */
		codeset = 0;
		break;

	case P11:
		/*
		 * Codeset 1
		 */
		codeset = 1;
		break;

	case P01:
		/*
		 * Codeset 2
		 */
		codeset = 2;
		break;

	case P10:
		/*
		 * Codeset 3
		 */
		codeset = 3;
		break;

	default:
		/*
		 * Looks like one of our assumptions has not held true so just
		 * return the empty bit map. Note that we stay silent because
		 * it seems that calling cmn_err here can cause some problems.
		 */
		return EmptyBitMap;

	}	/* End switch */

	eucw = EUC_INFO(chp)->eucw[codeset];		/* Input width */
	scrw = EUC_INFO(chp)->scrw[codeset];		/* Output width */

	row = 0;					/* L001 */

	if(codeset == 0){
		/*
		 * Special case this because we know that there is only
		 * going to be one input character and that if the top bit is
		 * set we go to the alternate character set.
		 * We also know that we have arranged the cset[0] pointers wont
		 * be NULL.
		 */
		if(eucw != 1 || ch < 0x20 || ch > 0xFF)
			return EmptyBitMap;

		if(ch < 0x80){
			base = cset[0][0];	/* cset 0, plane 0 */
			column = ch - 0x20;
		}
		else if(ch >= 0xA0) {
			base = cset[0][1];	/* cset 0, plane 1 == alt */
			column = ch - 0xA0;	/* L003 */
		}
		else
			return EmptyBitMap;

	}	/* End if codeset == 0 */
	else{
		/*
		 * Figure out the correct plane and set row and column
		 */
		if(eucw > 2){
			/*
			 * Things can get a little hairy here, we are dealing
			 * with at least a cube. All we do here is arrange for
			 * base to point to the appropriate plane which we can
			 * deal with below.
			 */
			if(eucw > 3){
				/*
				 * This is our first big assumption, we can't
				 * deal with this now so just return the empty
				 * bitmap
				 */
				return EmptyBitMap;
			}

			plane = ((ch >> 16) & 0xFF) - 0x20;

		}	/* End if eucw > 2 */
		else{
			plane = 0;

			if(eucw != 1)				  /* L001 */
				row = ((ch >> 8) & 0xFF) - 0x20;  /* L001 */

		}	/* End else eucw < 3 */

		column = (ch & 0xFF) - 0x20;

		/*
		 * L002 begin.
		 *
		 * Range check the everything to make sure we don't return
		 * a bogus pointer. Note that we received the character top
		 * bit stripped so the max we get is 0x7F, then we sub 0x20
		 * so we range check here with 0x5E
		 */
		if(plane >= MAX_PLANES || row > 0x5E || column > 0x5E)
			return EmptyBitMap;			/* L002 end */

		base = cset[codeset][plane];

	}	/* End else codeset != 0 */

	/*
	 * We know we are dealing with a plane here, we already calculated the
	 * row and the column so all we need to do is index into the plane to
	 * find the character bitmap.
	 */
	if(base == (unsigned char **)NULL)
		return EmptyBitMap;			/* No plane here */

	/*
	 * Base is a pointer to an array of pointers to rows (pages)
	 * Note:
	 * 	There may be an empty row but any row that is not empty is
	 * guaranteed to be full.
	 */
	page = *(base + row);

	if(page == (unsigned char *)NULL)
		return EmptyBitMap;			/* Empty row */

	return (page + column*scrw*BITMAP_SIZE);


}	/* End function fnt_getbitmap, L000 end */

/*
 * void
 * fnt_unlockbitmap(ws_channel_t *chp, wchar_t)
 *
 * Calling/Exit status:
 *
 * Description:
 *	This routine is called to unlock the font cache line.
 *	This implementation uses in core fonts and therefore
 *	this routine is a dummy.
 */
void
fnt_unlockbitmap(ws_channel_t *chp, wchar_t ch)
{
	/* do nothing */
}

/* L000 begin 
 *
 * int
 * fnt_setfont(ws_channel_t *chp, caddr_t)
 *
 * Calling/Exit status:
 *	Called from kdvmstr_doioctl with valid user context.
 *
 * Description:
 *	user_addr is the user virtual address of a gsd_font_arg structure. We
 *	have user context here so we can use copyin to read the font.
 *	copyin the font length and codeset, if all ok malloc some memory and
 *	copyin the font itself. bitmap addr is the user virtual address of the
 *	active row bitmap, ie. A one in this bitmap indicates the row (page)
 *	for that bit position has some defind glyphs in the font, a zero
 *	indicates the while row (page) is empty and we set the row pointer to
 *	NULL. Note that since the bitmap is so small (96 bits) we actually use
 * 	byte per bit to make parsing easier here and initialisation easier for
 *	the utility.
 * Note:
 *	A font length of zero indicates that the specified codeset should no
 *	longer be used. Individual planes can't be freed but they can be
 *	overloaded with a new font provided it is the same shap as the rest
 *	of the fonts in the codeset.
 *
 * Since cset is the pointer to the active codeset and codeset_mem contains
 * pointers to memory we have allocated it is possible that there will be valid
 * pointers in cset at the same plane as NULLs in codeset_mem if that plane is
 * one of the statically allocated ones.
 */
int
fnt_setfont(ws_channel_t *chp, caddr_t addr)
{
uint_t eucw, scrw, cur_eucw, flen, blen, plane, codeset, row;
unsigned char **plane_ptr, *next_bm;
unsigned char *bitmap;
gsd_font_t args;

	/*
	 * copyin the length and user address of the font
	 */
	if(copyin(addr, &args, (size_t)(sizeof(gsd_font_t))) < 0)
		return EFAULT;


	if(((codeset = args.codeset) >= MAX_CODESET) && codeset !=ALT_CHAR_SET){

		cmn_err(CE_WARN, "fnt: Invalid codeset requested %d", codeset);
		return EINVAL;
	}

	if((plane = args.plane) >= MAX_PLANES){

		cmn_err(CE_WARN, "fnt: Invalid plane requested %d", plane);
		return EINVAL;
	}

	if((eucw = args.eucw) > 3){

		cmn_err(CE_WARN, "fnt: Invalid euc width %d", eucw);
		return EINVAL;
	}

	if((blen = args.bitmap_len) != BITMAP_LEN){

		cmn_err(CE_WARN, "fnt: Invalid bitmap length %d", blen);
		return EINVAL;
	}

	
	flen = args.font_len;

	if(flen != 0){				/* L002 begin */
		/* 
		 * If we are not doing a codeset unload we must have a valid
		 * euc width
		 */
		if(EUC_INFO(chp) == NULL){

			cmn_err(CE_WARN, "fnt: No euc width for font");
			return EINVAL;

		}				/* L002 end */

		cur_eucw = EUC_INFO(chp)->eucw[codeset];
		scrw = EUC_INFO(chp)->scrw[codeset];

		if(cur_eucw != eucw){		/* L002 begin */
			/*
			 * This font is a different shape to that specified
			 * for this codeset and we are not doing a generel
			 * clear of the whole codeset so we have to leave the
			 * status quo unchanged and bounce this request.
			 */
			cmn_err(CE_WARN, "fnt: New and old eucw differ");
			return EINVAL;
		}

	}	/* End if, L002 end */

	/*
	 * If we are here then it is either OK to load this font onto the
	 * requested plane or we are about to do a full free of the codeset.
	 */
	if(flen == 0){

		int pl;

		/*
		 * Clear codeset request.
		 *
		 * Make sure that we always leave the ASCII and ALT char sets
		 * in place in codeset zero so that we can display error
		 * messages.
		 */
		for(pl = 0; pl < MAX_PLANES; pl++){
			/*
			 * Go down all the plane pointers in this codeset,
			 * freeing the memory and clearing the cset array
			 * pointers.
			 */
			if(codeset_mem[codeset][pl].addr != (void *)NULL){

				kmem_free((void *)codeset_mem[codeset][pl].addr,
					    codeset_mem[codeset][pl].length);

				kmem_free((void *)cset[codeset][pl],
					  NUM_COLS*sizeof(unsigned char *));

				codeset_mem[codeset][pl].addr = (void *)NULL;
				codeset_mem[codeset][pl].length = 0;
				cset[codeset][pl] = (unsigned char **)NULL;
			}
		}	/* End for all planes */

		/*
		 * All planes in this codeset have now had thier memory freed
		 * and thier pointers NULLed. If this is codeset0 or the alt
		 * char set we have to put back the statically allocated planes.
		 */
		if(codeset == 0 || codeset == ALT_CHAR_SET){

			cset[0][0] = codeset0data;
			cset[0][1] = altcharsetdata;
		}

		return 0;
	}
	else if(codeset_mem[codeset][plane].addr != (void *)NULL){
		/*
		 * If we already have a codeset at this position then free it. 
		 */
		kmem_free(codeset_mem[codeset][plane].addr,
					codeset_mem[codeset][plane].length);

		kmem_free((void *)cset[codeset][plane],
				NUM_COLS*sizeof(unsigned char *));

		codeset_mem[codeset][plane].addr = (void *)NULL;
		codeset_mem[codeset][plane].length = 0;
		cset[codeset][plane] = (unsigned char **)NULL;
	}

	/*
	 * OK, this is the real deal, see if we can get some memory for the
	 * codeset, the plane ptrs and the bitmap then copy them in.
	 */
	if((codeset_mem[codeset][plane].addr =
				kmem_alloc(flen, KM_NOSLEEP)) == (void *)NULL){
		
		cmn_err(CE_WARN, "fnt: Can't allocate %d bytes for "
				 "codeset %d plane %d", flen, codeset, plane);
		return ENOMEM;
	}

	if((bitmap = kmem_alloc(BITMAP_LEN, KM_NOSLEEP)) == (void *)NULL){

		kmem_free((void *)codeset_mem[codeset][plane].addr, flen);

		cmn_err(CE_WARN, "fnt: Can't allocate %d bytes "
				  "for active row bitmap", BITMAP_LEN);
		return ENOMEM;

	}	/* End KMA failed */

	if((plane_ptr =
		kmem_alloc(NUM_COLS*sizeof(unsigned char *), KM_NOSLEEP)) ==
								(void *)NULL){

		cmn_err(CE_WARN, "fnt: Can't allocate %d bytes for plane "
				 "pointers", NUM_COLS*sizeof(unsigned char *));

		kmem_free((void *)codeset_mem[codeset][plane].addr, flen);
		kmem_free((void *)bitmap, BITMAP_LEN);

		return ENOMEM;
	}

	/*
	 * Got the memory, now copyin the codeset.
	 */
	if(copyin(args.font_addr,
			(caddr_t)(codeset_mem[codeset][plane].addr), flen) < 0){

		kmem_free((void *)codeset_mem[codeset][plane].addr, flen);
		kmem_free((void *)bitmap, BITMAP_LEN);
		kmem_free((void *)plane_ptr, NUM_COLS*sizeof(unsigned char *));

		return EFAULT;
	}

	/*
	 * Got the codeset, copy in the active row bitmap.
	 */
	if(copyin(args.bitmap_addr, (caddr_t)bitmap, BITMAP_LEN) < 0){

		kmem_free((void *)codeset_mem[codeset][plane].addr, flen);
		kmem_free((void *)bitmap, BITMAP_LEN);
		kmem_free((void *)plane_ptr, NUM_COLS*sizeof(unsigned char *));

		return EFAULT;
	}

	/*
	 * We have all we need now parse the bitmap and set up the row pointers
	 * in the plane.
	 */
	next_bm = codeset_mem[codeset][plane].addr;	/* First glyph bitmap */

	for(row = 0; row < NUM_COLS; row++){
		/*
		 * If the bitmap says this row is active, point this row ptr at
		 * the next bit map and bump the next bit map ptr to start of
		 * the next row. If this row is empty simply set this row
		 * pointer to NULL.
		 */
		if(bitmap[row]){

			plane_ptr[row] = next_bm;
			next_bm += NUM_COLS*BITMAP_SIZE*scrw;
		}
		else{
			plane_ptr[row] = (unsigned char *)NULL;
		}

	}	/* End for row */

	codeset_mem[codeset][plane].length = flen;
	cset[codeset][plane] = plane_ptr;

	kmem_free((caddr_t)bitmap, BITMAP_LEN);

	return 0;

}	/* End function fnt_setfont */


/* 
 *
 * int
 * fnt_getfont(ws_channel_t *chp, caddr_t)
 *
 * Calling/Exit status:
 *	Called from kdvmstr_doioctl with valid user context.
 *
 * Description:
 *	Not implemented yet so just return EINVAL.
 */
int
fnt_getfont(ws_channel_t *chp, caddr_t addr)
{
	return EINVAL;				/* Not implemented */

}	/* End funtion fnt_getfont, L000 end */


