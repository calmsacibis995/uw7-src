/*
 *	@(#) effGlCache.c 11.1 97/10/22
 *
 * routines for caching glyphs in off screen memory
 *
 * Modification History
 *
 * S014, 04-Dec-92, mikep
 *	remove S013.  Add effDrawFontText.
 * S013, 31-Oct-92, mikep
 *	rearrange the arguments for font routines to match the rest of nfb.
 *	completely revamp DownloadFont, and Text8 routines to not know
 *	anything about X Font structures.
 * S012, 23-Oct-92, staceyc
 * 	image text is always copy and shouldn't extract alu from nfb
 * S011, 13-Oct-92, staceyc
 * 	use the correct value for stride with fast 8 bit text download
 * S010, 04-Sep-92, staceyc
 * 	fast text added
 * S009, 03-Sep-92, hiramc
 *	Correctly declare argument to GlClearCache
 * S008, 14-Oct-91, staceyc
 * 	reworked some comments
 * S007, 01-Oct-91, staceyc
 * 	removed font id stuff, now done in nfb
 * S006, 29-Sep-91, mikep@sco.com
 *	change interface to match nfb ClearFont().
 * S005, 24-Sep-91, staceyc
 * 	added cache flushing routine for screen switching
 * S004, 20-Sep-91, staceyc
 * 	backed out 5 bit nugget stuff - 1280x1024 Matrox uses 4 bits
 * S003, 09-Sep-91, staceyc
 * 	initial 5 bit nugget support
 * S002, 20-Aug-91, staceyc
 * 	fixed memory leak identified during code walkthrough, good one Hiram!
 * S001, 16-Aug-91, staceyc
 * 	reimplemented free/busy list as linked list
 * S000, 14-Aug-91, staceyc
 * 	created
 */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "dixfontstr.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

extern WindowPtr *WindowTable;

typedef struct effGlPlaneRectInfo_t {
	int writeplane;
	int readplane;
	int w, h;
	} effGlPlaneRectInfo_t;

/*
 * IMPORTANT: the width for each glyph for each plane _must_ be a
 * multiple of 4 (the 8514 blits and draws from nibble boundaries
 * much faster than non-nibble boundaries)
 */
#define EFF_FONT_WRITE_PLANE(n) (EFF_FONT_START_WRITE << (n))
#define EFF_FONT_READ_PLANE(n) (EFF_CONVERT_WRITE(EFF_FONT_WRITE_PLANE(n)))
static effGlPlaneRectInfo_t effGlPlaneRects[EFF_MONO_PLANES] =
	{{EFF_FONT_WRITE_PLANE(0), EFF_FONT_READ_PLANE(0), 12, 15},
	 {EFF_FONT_WRITE_PLANE(1), EFF_FONT_READ_PLANE(1), 16, 24}};
#define EFF_FONT_PLANE_COUNT \
	(sizeof(effGlPlaneRects) / sizeof(effGlPlaneRects[0]))
/*
 * the hash macro, have tried a few different implementations (see
 * Numerical Recipies in C, page 211) and this gives about the best
 * performance.
 */
#define EFF_RND(a) (((a) * 129749) >> 14)
#define EFF_GL_HASH(a, b, nel) (((EFF_RND((a) << 8)) + EFF_RND(b)) % (nel))

static void effGlDeleteCacheEntry(ScreenPtr pScreen, effGlFontID_t font_id,
	unsigned int byteOffset);

/*
 * initialize glyph cache structs
 */
void
effGlCacheInit(
ScreenPtr pScreen)
{
	int free_planes, plane, width_count, height_count;
	int entries, i, y, x, j, off_screen_y, off_screen_x;
	int rect_width, rect_height;
	effPrivateData_t *effPriv;
	effFontData_t *effFontData;
	effGlPlaneListInfo_t *plane_lists;
	effGlHashHead_t *hash_head;
	effGlListEntry_t *old_list_ptr, *list_ptr;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effFontData = &effPriv->font_data;

	/*
	 * build free/busy lists for each plane
	 */
	free_planes = EFF_MONO_PLANES;
	effFontData->cache_plane_count = free_planes;
	/*
	 * create free/busy plane list info for all planes
	 */
	effFontData->plane_list_info =
	    (effGlPlaneListInfo_t **)Xalloc(effFontData->cache_plane_count *
	    sizeof(effGlPlaneListInfo_t *));
	/*
	 * initialize busy and free list for all planes
	 */
	off_screen_x = effPriv->off_screen_blit_area.x;
	off_screen_y = effPriv->off_screen_blit_area.y;
	for (plane = 0; plane < effFontData->cache_plane_count; ++plane)
	{
		rect_width = effGlPlaneRects[plane].w;
		rect_height = effGlPlaneRects[plane].h;
		width_count = effPriv->off_screen_blit_area.w / rect_width;
		height_count = effPriv->off_screen_blit_area.h / rect_height;
		/*
		 * number of glyphs to fit on this plane
		 */
		effFontData->plane_list_info[plane] = (effGlPlaneListInfo_t *)
		    Xalloc(sizeof(effGlPlaneListInfo_t));
		plane_lists = effFontData->plane_list_info[plane];
		plane_lists->free_list =
		    (effGlListEntry_t *)Xalloc(sizeof(effGlListEntry_t));
		old_list_ptr = list_ptr = plane_lists->free_list;
		plane_lists->busy_list = (effGlListEntry_t *)NULL;

		/*
		 * initialize free list with coords of glyphs for this plane
		 */
		for (y = 0; y < height_count; ++y)
			for (x = 0; x < width_count; ++x)
			{
				list_ptr->coords.y = off_screen_y +
				    y * rect_height;
				list_ptr->coords.x = off_screen_x +
				    x * rect_width;
				old_list_ptr = list_ptr;
				list_ptr->next = (effGlListEntry_t *)
				    Xalloc(sizeof(effGlListEntry_t));
				list_ptr = list_ptr->next;
			}
		Xfree(list_ptr);
		old_list_ptr->next = (effGlListEntry_t *)NULL;
	}
	/*
	 * initialize hash table
	 */
	effFontData->hash_table_size = 1024;       /* should be configurable */
	effFontData->initial_bucket_size = 3;      /* should be configurable */
	effFontData->hash_table =
	    (effGlHashHead_t **)Xalloc(effFontData->hash_table_size *
	    sizeof(effGlHashHead_t *));
	for (i = 0; i < effFontData->hash_table_size; ++i)
	{
		hash_head = (effGlHashHead_t *)Xalloc(sizeof(effGlHashHead_t));
		hash_head->bucket_size = effFontData->initial_bucket_size;
		hash_head->cache_positions = (effGlCachePosition_t *)
		    Xalloc(sizeof(effGlCachePosition_t) *
		    hash_head->bucket_size);
		for (j = 0; j < hash_head->bucket_size; ++j)
			hash_head->cache_positions[j].exists = 0;
		effFontData->hash_table[i] = hash_head;
	}
}

/*
 * free glyph cache structs
 */
void
effGlCacheClose(
ScreenPtr pScreen)
{
	int i, j, plane;
	effPrivateData_t *effPriv;
	effFontData_t *effFontData;
	effGlHashHead_t *hash_head;
	effGlListEntry_t *list_ptr, *new_list_ptr;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effFontData = &effPriv->font_data;

	/*
	 * free hash table
	 */
	for (i = 0; i < effFontData->hash_table_size; ++i)
	{
		hash_head = effFontData->hash_table[i];
		Xfree((char *)hash_head->cache_positions);
		Xfree((char *)hash_head);
	}
	Xfree((char *)effFontData->hash_table);

	/*
	 * free cache busy/free lists
	 */
	for (plane = 0; plane < effFontData->cache_plane_count; ++plane)
	{
		list_ptr = effFontData->plane_list_info[plane]->busy_list;
		while (list_ptr)
		{
			new_list_ptr = list_ptr->next;
			Xfree((char *)list_ptr);
			list_ptr = new_list_ptr;
		}
		list_ptr = effFontData->plane_list_info[plane]->free_list;
		while (list_ptr)
		{
			new_list_ptr = list_ptr->next;
			Xfree((char *)list_ptr);
			list_ptr = new_list_ptr;
		}
		Xfree((char *)effFontData->plane_list_info[plane]);
	}
	Xfree((char *)effFontData->plane_list_info);
}

void
effGlClearCache(
int fontID,						/* S006 S009 */
ScreenPtr pScreen)
{
	int plane;
	effGlFontID_t dead_font_id;
	effPrivateData_t *effPriv;
	effFontData_t *effFontData;
	effGlPlaneListInfo_t *plane_list_info;
	effGlListEntry_t *new_busy_ptr, *busy_ptr, *old_free_ptr, *old_busy_ptr;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effFontData = &effPriv->font_data;

	/*
	 * all cache entries with dead_font_id will be removed
	 */
	dead_font_id = (effGlFontID_t)fontID;			/* S006 */
	for (plane = 0; plane < effFontData->cache_plane_count; ++plane)
	{
		plane_list_info = effFontData->plane_list_info[plane];
		old_busy_ptr = busy_ptr = plane_list_info->busy_list;
		while (busy_ptr)
		{
			if (busy_ptr->font_id == dead_font_id)
			{
				effGlDeleteCacheEntry(pScreen, dead_font_id,
				    busy_ptr->byteOffset);
				/*
				 * delete entry from busy list
				 */
				if (busy_ptr == plane_list_info->busy_list)
				{
					/*
					 * need to modify list head
					 */
					plane_list_info->busy_list =
					    busy_ptr->next;
					new_busy_ptr =
					    plane_list_info->busy_list;
				}
				else
				{
					old_busy_ptr->next = busy_ptr->next;
					new_busy_ptr = old_busy_ptr;
				}
				/*
				 * move freed list entry from busy to free list
				 */
				old_free_ptr = plane_list_info->free_list;
				plane_list_info->free_list = busy_ptr;
				plane_list_info->free_list->next = old_free_ptr;
				/*
				 * set busy ptr to one before the next
				 */
				busy_ptr = new_busy_ptr;
			}
			else
			{
				/*
			 	* move to next busy_ptr
			 	*/
				old_busy_ptr = busy_ptr;
				busy_ptr = busy_ptr->next;
			}
		}
	}
}

Bool
effGlReadCacheEntry(
ScreenPtr pScreen,
effGlFontID_t font_id,
unsigned int byteOffset,
effGlGlyphInfo_t *glyph_info)
{
	int index, i, bucket_size;
	effGlHashHead_t *bucket;
	effFontData_t *effFontData;
	effGlCachePosition_t *cache_positions;

	effFontData = &EFF_PRIVATE_DATA(pScreen)->font_data;
	index = EFF_GL_HASH(font_id, byteOffset, effFontData->hash_table_size);
	bucket = effFontData->hash_table[index];
	cache_positions = bucket->cache_positions;
	bucket_size = bucket->bucket_size;

	/*
	 * hopefully short linear search through bucket for glyph
	 */
	i = 0;
	while (i < bucket_size &&
	    !(cache_positions[i].exists &&
	      cache_positions[i].byteOffset == byteOffset &&
	      cache_positions[i].font_id == font_id))
		++i;
	if (i >= bucket_size)
		return FALSE;                            /* glyph not cached */

	/*
	 * glyph is cached, copy off-screen glyph info and return true
	 * note that writeplane is not set, as a read operation will not
	 * need it, so it's one less thing to store in the bucket entry
	 */
	glyph_info->coords = cache_positions[i].coords;
	glyph_info->readplane = cache_positions[i].readplane;
	return TRUE;
}

static
void
effGlDeleteCacheEntry(
ScreenPtr pScreen,
effGlFontID_t font_id,
unsigned int byteOffset)
{
	int index, i, bucket_size;
	effGlHashHead_t *bucket;
	effFontData_t *effFontData;
	effGlCachePosition_t *cache_positions;

	effFontData = &EFF_PRIVATE_DATA(pScreen)->font_data;
	index = EFF_GL_HASH(font_id, byteOffset, effFontData->hash_table_size);
	bucket = effFontData->hash_table[index];
	cache_positions = bucket->cache_positions;
	bucket_size = bucket->bucket_size;

	/*
	 * hopefully short linear search through bucket for glyph
	 */
	i = 0;
	while (i < bucket_size &&
	    !(cache_positions[i].exists &&
	      cache_positions[i].byteOffset == byteOffset &&
	      cache_positions[i].font_id == font_id))
		++i;
	if (i >= bucket_size)
		return;                                 /* glyph not cached! */

	/*
	 * glyph is cached, mark bucket entry as free,
	 * free and busy lists for plane must be updated by the caller!
	 */
	cache_positions[i].exists = 0;
}

Bool
effGlAddCacheEntry(
ScreenPtr pScreen,
effGlFontID_t font_id,
unsigned int byteOffset,
int glyph_width,
int glyph_height,
effGlGlyphInfo_t *glyph_info)
{
	int index, i, bucket_size, plane_index;
	effGlHashHead_t *bucket;
	effFontData_t *effFontData;
	effGlCachePosition_t *cache_positions, *new_bucket_entry;
	effGlPlaneListInfo_t *plane_list;
	effGlListEntry_t *list_ptr;

	effFontData = &EFF_PRIVATE_DATA(pScreen)->font_data;

	/*
	 * determine plane for glyph to be drawn on
	 * this involves finding a plane where the glyph can fit into
	 * the fixed glyph size for that plane _and_ where there is
	 * room available on that plane (i.e. free list is not null)
	 */
	plane_index = 0;
	while (plane_index < effFontData->cache_plane_count &&
	    (effGlPlaneRects[plane_index].w < glyph_width ||
	    effGlPlaneRects[plane_index].h < glyph_height ||
	    effFontData->plane_list_info[plane_index]->free_list ==
	    (effGlListEntry_t *)NULL))
		++plane_index;

	if (plane_index >= effFontData->cache_plane_count)
		/*
		 * cache is full and/or glyph is too big
		 */
		return FALSE;

	index = EFF_GL_HASH(font_id, byteOffset, effFontData->hash_table_size);
	bucket = effFontData->hash_table[index];
	cache_positions = bucket->cache_positions;
	bucket_size = bucket->bucket_size;

	/*
	 * hopefully short linear search through bucket for free entry
	 * we assume that the glyph isn't already stashed in cache
	 */
	i = 0;
	while (i < bucket_size && cache_positions[i].exists)
		++i;
	if (i >= bucket_size)
	{
		/*
		 * bucket is full, need to reallocate a larger bucket
		 * hope this doesn't happen too often!
		 * look at improving hash function or increasing size of
		 * hash table if this slows things down
		 */
		bucket->bucket_size += effFontData->initial_bucket_size;
		bucket->cache_positions =
		    (effGlCachePosition_t *)Xrealloc(bucket->cache_positions,
		    bucket->bucket_size * sizeof(effGlCachePosition_t));
		/*
		 * update local ptr in case realloc has changed its value
		 */
		cache_positions = bucket->cache_positions;
		/*
		 * mark new bucket entries as empty
		 */
		for (i = bucket_size; i < bucket->bucket_size; ++i)
			cache_positions[i].exists = 0;

		/*
		 * set i to new free bucket entry
		 */
		i = bucket_size;
		bucket_size = bucket->bucket_size;
	}

	/*
	 * establish entry in hash table for new entry
	 */
	new_bucket_entry = &cache_positions[i];

	/*
	 * grab a rect on the selected plane from the free list for that
	 * plane and put it on the busy list
	 */
	plane_list = effFontData->plane_list_info[plane_index];
	list_ptr = plane_list->free_list;
	plane_list->free_list = plane_list->free_list->next;
	list_ptr->next = plane_list->busy_list;
	plane_list->busy_list = list_ptr;

	/*
	 * identify the glyph with the off-screen rect it will be
	 * stored in
	 */
	list_ptr->font_id = font_id;
	list_ptr->byteOffset = byteOffset;

	/*
	 * fill in new bucket entry
	 */
	new_bucket_entry->exists = 1;
	new_bucket_entry->readplane = effGlPlaneRects[plane_index].readplane;
	new_bucket_entry->font_id = font_id;
	new_bucket_entry->byteOffset = byteOffset;
	new_bucket_entry->coords = list_ptr->coords;

	/*
	 * update glyph info for calling function
	 */
	glyph_info->coords = list_ptr->coords;
	glyph_info->readplane = effGlPlaneRects[plane_index].readplane;
	glyph_info->writeplane = effGlPlaneRects[plane_index].writeplane;

	return TRUE;
}

void
effGlFlushCache(
ScreenPtr pScreen)
{
	int plane;
	effPrivateData_t *effPriv;
	effFontData_t *effFontData;
	effGlPlaneListInfo_t *plane_list_info;
	effGlListEntry_t *busy_ptr, *old_free_ptr, *next_busy_ptr;

	effPriv = EFF_PRIVATE_DATA(pScreen);
	effFontData = &effPriv->font_data;

	for (plane = 0; plane < effFontData->cache_plane_count; ++plane)
	{
		plane_list_info = effFontData->plane_list_info[plane];
		busy_ptr = plane_list_info->busy_list;
		while (busy_ptr)
		{
			effGlDeleteCacheEntry(pScreen, busy_ptr->font_id,
			    busy_ptr->byteOffset);
			next_busy_ptr = busy_ptr->next;
			/*
			 * move freed list entry from busy to free list
			 */
			old_free_ptr = plane_list_info->free_list;
			plane_list_info->free_list = busy_ptr;
			plane_list_info->free_list->next = old_free_ptr;
			/*
		 	 * move to next busy_ptr
		 	 */
			busy_ptr = next_busy_ptr;
		}
		plane_list_info->busy_list = (effGlListEntry_t *)NULL;
	}
}

void
effInitializeText8(
ScreenRec *pScreen)
{
	int planes, fonts_per_plane, i, j, x, y, font_count;
	int glyphs_per_row, rows_per_font, glyph;
	DDXPointRec *coords;
	unsigned short y_coord, font_base_x, font_base_y;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

	planes = pScreen->rootDepth - EFF_SPARE - EFF_MONO_PLANES;
	glyphs_per_row = effPriv->off_screen_blit_area.w / EFF_TEXT8_WIDTH;
	rows_per_font = (NFB_TEXT8_SIZE + glyphs_per_row - 1) / glyphs_per_row;
	fonts_per_plane = effPriv->off_screen_blit_area.h /
	    (rows_per_font * EFF_TEXT8_HEIGHT);
	effPriv->text8_font_count = planes * fonts_per_plane;
	EFF_ASSERT(effPriv->text8_font_count > 0);
	effPriv->text8_data = (effText8Data_t *)xalloc(effPriv->text8_font_count
	    * sizeof(effText8Data_t));
	font_count = 0;
	i = 0;
	font_base_x = effPriv->off_screen_blit_area.x;
	while (i < planes)
	{
		j = 0;
		while (j < fonts_per_plane)
		{
			font_base_y = effPriv->off_screen_blit_area.y +
			    (j * EFF_TEXT8_HEIGHT * rows_per_font);
			effPriv->text8_data[font_count].writeplane =
			    EFF_TEXT8_START_WRITE << i;
			effPriv->text8_data[font_count].readplane =
			    EFF_CONVERT_WRITE(
			    effPriv->text8_data[font_count].writeplane);
			glyph = 0;
			coords = effPriv->text8_data[font_count].coords;
			for (y = 0; y < rows_per_font; ++y)
			{
				y_coord = font_base_y + y * EFF_TEXT8_HEIGHT;
				x = 0;
				while (x < glyphs_per_row &&
				    glyph < NFB_TEXT8_SIZE)
				{
					coords[glyph].x = font_base_x + x *
					    EFF_TEXT8_WIDTH;
					coords[glyph].y = y_coord;
					++glyph;
					++x;
				}
			}
			++j;
			++font_count;
		}
		++i;
	}
}

void
effCloseText8(
ScreenPtr pScreen)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

	xfree((char *)effPriv->text8_data);
}

#define GLYPHS_IN_FONT(pFI) ((pFI)->lastCol - (pFI)->firstCol + 1)

void
effDownloadFont8(
unsigned char **bits, 
int count, 
int width, 
int height, 
int stride,
int index,
ScreenRec *pScreen)
{
	int i;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	DDXPointRec *coords;
	BoxRec box;
	unsigned long writeplane;

	coords = effPriv->text8_data[index].coords;
	writeplane = effPriv->text8_data[index].writeplane;

	for (i = 0; i < count; ++i)
	{
		box.x1 = coords[i].x;
		box.y1 = coords[i].y;
		box.x2 = box.x1 + width;
		box.y2 = box.y1 + height;
		effDrawOpaqueMonoImage(&box, bits[i], 0, stride, ~0, 0,
		    GXcopy, writeplane, &WindowTable[pScreen->myNum]->drawable);
	}

}

void								/* S014 vvv */
effDrawFontText(
BoxPtr pbox,
unsigned char *chars,
unsigned int count,
unsigned short glyph_width,
int index,
unsigned long fg,
unsigned long bg,
unsigned char alu,
unsigned long planemask,
unsigned char transparent,
DrawablePtr pDraw)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);
	unsigned long readplane;
	DDXPointRec *coords;
	BoxRec tbox = *pbox;
	tbox.x2 = tbox.x1 + glyph_width;

	readplane = effPriv->text8_data[index].readplane;
	coords = effPriv->text8_data[index].coords;

	if (transparent)
	    effSetupCardForBlit(fg, alu, readplane, planemask);
	else
	    effSetupCardForOpaqueBlit(fg, bg, GXcopy, readplane, planemask);
	
	while (count--)
	{
		effStretchBlit(pDraw, &coords[*chars++], &tbox);
		tbox.x1 += glyph_width;
		tbox.x2 += glyph_width;
	}
	effResetCardFromBlit();
}								/* S014 ^^^ */
