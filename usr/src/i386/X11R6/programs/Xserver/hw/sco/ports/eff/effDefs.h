/*
 *	@(#) effDefs.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Modification History
 *
 * S025, 22-Nov-93, staceyc
 * 	save state of screen blanker
 * S024, 27-Aug-93, buckm
 * 	in screen privates, change card_clip, add current_clip.
 * S023, 01-Feb-93, staceyc
 * 	added flag for buggy C&T 481 8514/A clone chip
 * S022, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S021, 23-Oct-92, staceyc
 * 	new base address item for pixelworks boards
 * S020, 04-Sep-92, staceyc
 * 	fast text struct mods
 * S019, 21-Nov-91, staceyc
 * 	use global window table instead of dummy pixmap
 * S018, 14-Nov-91, staceyc
 * 	512K card support
 * S017, 01-Oct-91, staceyc
 * 	removed font id stuff - now done in nfb
 * S016, 26-Sep-91, staceyc
 * 	screen private stuff to save and restore VGA 8514 DAC state
 * S015, 24-Sep-91, staceyc
 * 	flush the cache instead of keeping in memory during scr switch
 * S014, 20-Sep-91, staceyc
 * 	backked out nugget size stuff - added non-rep mono image drawing
 * S013, 18-Sep-91, staceyc
 * 	new include file for draw mono glyphs
 * S012, 09-Sep-91, staceyc
 * 	add nugget size to screen private
 * S011, 03-Sep-91, staceyc
 * 	more grafinfo stuff - save and restore off-screen area during scr switch
 * S010, 28-Aug-91, staceyc
 * 	new grafinfo api
 * S009, 28-Aug-91, staceyc
 * 	glyph list support
 * S008, 21-Aug-91, staceyc
 * 	add tile blit area to screen private
 * S007, 16-Aug-91, staceyc
 * 	reimplement busy/free lists for glyph cache
 * S006, 15-Aug-91, staceyc
 * 	glyph caching structs
 * S005, 13-Aug-91, staceyc
 * 	added include files to satisfy procs.h parameters
 * S004, 06-Aug-91, staceyc
 * 	added mono image structs
 * S003, 05-Aug-91, staceyc
 * 	keep track of current cursor
 * S002, 01-Aug-91, staceyc
 * 	added cursor related screen private stuff
 * S001, 28-Jun-91, staceyc
 * 	added screen private structure
 * S000, 21-Jun-91, staceyc
 * 	initial changes
 */

#include "Xproto.h"
#include "grafinfo.h"
#include "miscstruct.h"
#include "cursorstr.h"
#include "windowstr.h"
#include "fontstruct.h"
#include "font.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "dixfont.h"
#include "colormapst.h"
#include "ddxScreen.h"
#include "nfbGlyph.h"

#define EFF_NORMAL_MODE -1

extern struct _nfbGCOps effSolidPrivOps;

typedef unsigned int effGlFontID_t;

typedef struct effGlGlyphInfo_t {
	int readplane;
	int writeplane;
	DDXPointRec coords;
	} effGlGlyphInfo_t;

typedef struct effGlCachePosition_t {
	Bool exists;
	int readplane;
	effGlFontID_t font_id;
	unsigned int byteOffset;
	DDXPointRec coords;
	} effGlCachePosition_t;

typedef struct effGlListEntry_t {
	effGlFontID_t font_id;
	unsigned int byteOffset;
	DDXPointRec coords;
	struct effGlListEntry_t *next;
	} effGlListEntry_t;

typedef struct effGlPlaneListInfo_t {
	effGlListEntry_t *busy_list;
	effGlListEntry_t *free_list;
	} effGlPlaneListInfo_t;

typedef struct effGlHashHead_t {
	effGlCachePosition_t *cache_positions;
	int bucket_size;
	} effGlHashHead_t;

typedef struct effFontData_t {
	int cache_plane_count;
	effGlPlaneListInfo_t **plane_list_info;
	int hash_table_size;
	int initial_bucket_size;
	effGlHashHead_t **hash_table;
	} effFontData_t;

typedef struct effCursorSave_t {
	int x, y;
	int w, h;
	} effCursorSave_t;

typedef effCursorSave_t effOffScreenBlitArea_t;

typedef struct effCursorData_t {
	short cursor_max_size;
	short save_max_size;
	DDXPointRec save_under;
	effCursorSave_t save_coords;
	DDXPointRec mask_bitmap;
	DDXPointRec cursor_bitmap;
	CursorPtr current_cursor;
	} effCursorData_t;

typedef struct effGlyphList_t {
	DDXPointRec off_screen_point;
	BoxRec screen_box;
	unsigned long readplane;
	} effGlyphList_t;
	

typedef struct effGlyphListInfo_t {
	unsigned long fg;
	unsigned char alu;
	unsigned long writeplane;
	int list_size;
	int list_index;
	effGlyphList_t *list;
	} effGlyphListInfo_t;

typedef struct effDACState_t {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	} effDACState_t;

typedef struct effText8Data_t {
	unsigned long readplane;
	unsigned long writeplane;
	DDXPointRec coords[NFB_TEXT8_SIZE];
	} effText8Data_t;

typedef struct effPalette_t {
	unsigned short write_addr;
	unsigned short read_addr;
	unsigned short data;
	unsigned short mask;
	} effPalette_t;

typedef struct effPrivateData_t {
	int width, height, depth;
	unsigned int dac_shift;
	effDACState_t dac_state[EFF_DAC_SIZE];
	void (*APBlockOutW)();
	effCursorData_t cursor;
	effOffScreenBlitArea_t off_screen_blit_area;
	effOffScreenBlitArea_t tile_blit_area;
	effOffScreenBlitArea_t all_off_screen;
	effFontData_t font_data;
	effGlyphListInfo_t glyph_list;
	grafData *graf_data;
	BoxRec card_clip;
	BoxRec current_clip;
	int text8_font_count;
	effText8Data_t *text8_data;
	effPalette_t eff_pal;
	int f82c481;
	int screen_blanked;
	} effPrivateData_t;

extern int effScreenPrivateIndex;

