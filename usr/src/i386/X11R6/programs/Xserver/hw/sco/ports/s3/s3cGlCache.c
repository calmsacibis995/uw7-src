/*
 *	@(#)s3cGlCache.c	6.1	3/20/96	10:23:13
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
 *
 * Modification History
 *
 * S007, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S006, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * X005 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X004 31-Dec-91 kevin@xware.com
 *	merged s3cGlyphInfo_t and s3cPlaneRectInfo_t readplane and writeplane 
 *	into plane, changed s3cGlCachePosition_t readplane to plane, and
 *	changed s3cGlPlaneRects sizes to support limited off screen memory.
 * X003 08-Dec-91 kevin@xware.com
 *	embedded s3cReadCacheEntry() function into s3cDrawMonoGlyphs(),
 *	removed s3cReadCacheEntry() function.
 * X002 06-Dec-91 kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X001 29-Nov-91 kevin@xware.com
 *	removed support for 8514a.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

typedef struct 	s3cGlPlaneRectInfo_t 
{
	int 	plane;
	int 	w;
	int	h;
} s3cGlPlaneRectInfo_t;

/*
 * list of glyph size for each off screen plane (7 max) - make this
 * configurable?
 *
 * IMPORTANT: the width for each glyph for each plane _must_ be a
 * multiple of 4 (the 86C911 blits and draws from nibble boundaries
 * much faster than non-nibble boundaries)
 */

#define S3C_FONT_PLANE(n)	(S3C_FONT_START_PLANE << (n))

static s3cGlPlaneRectInfo_t s3cGlPlaneRects[] =
{
	{ S3C_FONT_PLANE(0),  8, 13},
	{ S3C_FONT_PLANE(1), 12, 13},
	{ S3C_FONT_PLANE(2), 12, 15},
	{ S3C_FONT_PLANE(3), 16, 20},
	{ S3C_FONT_PLANE(4), 16, 24},
	{ S3C_FONT_PLANE(5), 20, 30},
	{ S3C_FONT_PLANE(6), 20, 30}
};

#define S3C_FONT_PLANE_COUNT \
	(sizeof(s3cGlPlaneRects) / sizeof(s3cGlPlaneRects[0]))

static void
s3cGlDeleteCacheEntry(
	ScreenPtr 		pScreen,
	s3cGlFontID_t 		font_id,
	unsigned int 		byteOffset);

/*
 *  s3cGlCacheInit() -- Initialize Glyph Cache
 *
 *	This routime will initialize the local glyph cache structures.
 *
 */
 
void
S3CNAME(GlCacheInit)(
	ScreenPtr 		pScreen)
{
	int 			free_planes; 
	int			plane;
	int			width_count;
	int			height_count;
	int			entries;
	int			i;
	int			y;
	int			x;
	int			j;
	int			off_screen_y;
	int			off_screen_x;
	int			rect_width;
	int			rect_height;
	s3cPrivateData_t 	*s3cPriv;
	s3cFontData_t 		*s3cFontData;
	s3cGlPlaneListInfo_t 	*plane_lists;
	s3cGlHashHead_t 	*hash_head;
	s3cGlListEntry_t 	*old_list_ptr;
	s3cGlListEntry_t 	*list_ptr;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cFontData = &s3cPriv->font_data;

	/*
	 * build free/busy lists for each plane
	 * calculate number of planes available -
	 * one plane used for stipples, mono images etc, rest are free
	 * for cached glyphs
	 */

	free_planes = pScreen->rootDepth - 1;  /* should query card here */
	if ( free_planes < S3C_FONT_PLANE_COUNT )
		s3cFontData->cache_plane_count = free_planes;
	else
		s3cFontData->cache_plane_count = S3C_FONT_PLANE_COUNT;

	/*
	 * create list array for all planes
	 */

	s3cFontData->plane_list_info =
		(s3cGlPlaneListInfo_t **)Xalloc(s3cFontData->cache_plane_count *
	    	sizeof(s3cGlPlaneListInfo_t *));

	/*
	 * initialize busy and free list for all planes
	 */

	off_screen_x = s3cPriv->off_screen_blit_area.x;
	off_screen_y = s3cPriv->off_screen_blit_area.y;

	for ( plane = 0; plane < s3cFontData->cache_plane_count; ++plane )
	{
		rect_width = s3cGlPlaneRects[plane].w;
		rect_height = s3cGlPlaneRects[plane].h;
		width_count = s3cPriv->off_screen_blit_area.w / rect_width;
		height_count = s3cPriv->off_screen_blit_area.h / rect_height;

		/*
		 * number of glyphs to fit on this plane
		 */

		s3cFontData->plane_list_info[plane] = (s3cGlPlaneListInfo_t *)
		    	Xalloc(sizeof(s3cGlPlaneListInfo_t));
		plane_lists = s3cFontData->plane_list_info[plane];
		plane_lists->free_list =
		    	(s3cGlListEntry_t *)Xalloc(sizeof(s3cGlListEntry_t));
		old_list_ptr = list_ptr = plane_lists->free_list;
		plane_lists->busy_list = (s3cGlListEntry_t *)NULL;

		/*
		 * initialize free list with coords of glyphs for this plane
		 */

		for ( y = 0; y < height_count; ++y )
			for ( x = 0; x < width_count; ++x )
			{
				list_ptr->coords.y = off_screen_y +
				    	y * rect_height;
				list_ptr->coords.x = off_screen_x +
				    	x * rect_width;
				old_list_ptr = list_ptr;
				list_ptr->next = (s3cGlListEntry_t *)
				    	Xalloc(sizeof(s3cGlListEntry_t));
				list_ptr = list_ptr->next;
			}
		Xfree(list_ptr);
		old_list_ptr->next = (s3cGlListEntry_t *)NULL;
	}

	/*
	 * initialize hash table
	 */

	s3cFontData->hash_table_size = 
			      S3C_HASH_TABLE_SIZE; /* should be configurable */
	s3cFontData->initial_bucket_size = 3;      /* should be configurable */
	s3cFontData->hash_table =
	    	(s3cGlHashHead_t **)Xalloc(s3cFontData->hash_table_size *
	    	sizeof(s3cGlHashHead_t *));

	for ( i = 0; i < s3cFontData->hash_table_size; ++i )
	{
		hash_head = (s3cGlHashHead_t *)Xalloc(sizeof(s3cGlHashHead_t));
		hash_head->bucket_size = s3cFontData->initial_bucket_size;
		hash_head->cache_positions = (s3cGlCachePosition_t *)
			Xalloc(sizeof(s3cGlCachePosition_t) *
			hash_head->bucket_size);
		for (j = 0; j < hash_head->bucket_size; ++j)
			hash_head->cache_positions[j].exists = 0;
		s3cFontData->hash_table[i] = hash_head;
	}
}


/*
 *  S3CNAME(GlCacheClose)() -- Close Glyph Cache
 *
 *	This routime will free the local glyph cache structures.
 *
 */
 
void
S3CNAME(GlCacheClose)(
	ScreenPtr 		pScreen)
{
	int			i;
	int			j;
	int			plane;
	s3cPrivateData_t 	*s3cPriv;
	s3cFontData_t 		*s3cFontData;
	s3cGlHashHead_t		*hash_head;
	s3cGlListEntry_t	*list_ptr;
	s3cGlListEntry_t	*new_list_ptr;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cFontData = &s3cPriv->font_data;

	/*
	 * free hash table
	 */

	for ( i = 0; i < s3cFontData->hash_table_size; ++i )
	{
		hash_head = s3cFontData->hash_table[i];
		Xfree((char *)hash_head->cache_positions);
		Xfree((char *)hash_head);
	}
	Xfree((char *)s3cFontData->hash_table);

	/*
	 * free cache busy/free lists
	 */

	for ( plane = 0; plane < s3cFontData->cache_plane_count; ++plane )
	{
		list_ptr = s3cFontData->plane_list_info[plane]->busy_list;

		while ( list_ptr )
		{
			new_list_ptr = list_ptr->next;
			Xfree((char *)list_ptr);
			list_ptr = new_list_ptr;
		}

		list_ptr = s3cFontData->plane_list_info[plane]->free_list;

		while ( list_ptr )
		{
			new_list_ptr = list_ptr->next;
			Xfree((char *)list_ptr);
			list_ptr = new_list_ptr;
		}
		Xfree((char *)s3cFontData->plane_list_info[plane]);
	}
	Xfree((char *)s3cFontData->plane_list_info);
}


/*
 *  s3cGlClearCache() -- Clear Cache of Font
 *
 *	This routime will clear the entire glyph cache of all the glyphs
 *	belonging to the specified font ID.
 *
 */
 
void
S3CNAME(GlClearCache)(
	int 		fontID,				/* S006 */
	ScreenPtr 		pScreen)
{
	int 			plane;
	s3cGlFontID_t 		dead_font_id;
	s3cPrivateData_t 	*s3cPriv;
	s3cFontData_t 		*s3cFontData;
	s3cGlPlaneListInfo_t 	*plane_list_info;
	s3cGlListEntry_t 	*new_busy_ptr;
	s3cGlListEntry_t 	*busy_ptr;
	s3cGlListEntry_t 	*old_free_ptr;
	s3cGlListEntry_t 	*old_busy_ptr;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cFontData = &s3cPriv->font_data;

	/*
	 * all cache entries with dead_font_id will be removed
	 */

	dead_font_id = (s3cGlFontID_t)fontID;			/* S006 */

	for ( plane = 0; plane < s3cFontData->cache_plane_count; ++plane )
	{
		plane_list_info = s3cFontData->plane_list_info[plane];
		old_busy_ptr = busy_ptr = plane_list_info->busy_list;

		while ( busy_ptr )
		{
			if ( busy_ptr->font_id == dead_font_id )
			{
				s3cGlDeleteCacheEntry(pScreen, dead_font_id,
				    	busy_ptr->byteOffset);
				/*
				 * delete entry from busy list
				 */

				if ( busy_ptr == plane_list_info->busy_list )
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


/*
 *  s3cGlDeleteCacheEntry() -- Delete Glyph Cache Entry
 *
 *	This routime will remove the specified glyph from the cache.
 *
 */
 
static void
s3cGlDeleteCacheEntry(
	ScreenPtr 		pScreen,
	s3cGlFontID_t 		font_id,
	unsigned int 		byteOffset)
{
	int 			index;
	int			i;
	int			bucket_size;
	s3cGlHashHead_t		*bucket;
	s3cFontData_t 		*s3cFontData;
	s3cGlCachePosition_t	*cache_positions;

	s3cFontData = &S3C_PRIVATE_DATA(pScreen)->font_data;
	index = S3C_GL_HASH(font_id, byteOffset, s3cFontData->hash_table_size);
	bucket = s3cFontData->hash_table[index];
	cache_positions = bucket->cache_positions;
	bucket_size = bucket->bucket_size;

	/*
	 * hopefully short linear search through bucket for glyph
	 */

	i = 0;
	while ( i < bucket_size &&
	    	!(cache_positions[i].exists &&
	      	cache_positions[i].byteOffset == byteOffset &&
	      	cache_positions[i].font_id == font_id) )
		++i;

	if ( i >= bucket_size )
		return; 

	/*
	 * glyph is cached, mark bucket entry as free,
	 * free and busy lists for plane must be updated by the caller!
	 */

	cache_positions[i].exists = 0;
}


/*
 *  s3cGlAddCacheEntry() -- Add Glyph Cache Entry
 *
 *	This routime will add the specified glyph to the cache.
 *
 */
 
Bool
S3CNAME(GlAddCacheEntry)(
	ScreenPtr 		pScreen,
	s3cGlFontID_t 		font_id,
	unsigned int 		byteOffset,
	int 			glyph_width,
	int 			glyph_height,
	s3cGlGlyphInfo_t	*glyph_info)
{
	int 			index;
	int			i;
	int			bucket_size;
	int			plane_index;
	s3cGlHashHead_t 	*bucket;
	s3cFontData_t 		*s3cFontData;
	s3cGlCachePosition_t 	*cache_positions;
	s3cGlCachePosition_t 	*new_bucket_entry;
	s3cGlPlaneListInfo_t 	*plane_list;
	s3cGlListEntry_t 	*list_ptr;

	s3cFontData = &S3C_PRIVATE_DATA(pScreen)->font_data;

	/*
	 * determine plane for glyph to be drawn on
	 * this involves finding a plane where the glyph can fit into
	 * the fixed glyph size for that plane _and_ where there is
	 * room available on that plane (i.e. free list is not null)
	 */

	plane_index = 0;
	while ( plane_index < s3cFontData->cache_plane_count &&
	    	(s3cGlPlaneRects[plane_index].w < glyph_width ||
	    	s3cGlPlaneRects[plane_index].h < glyph_height ||
	    	s3cFontData->plane_list_info[plane_index]->free_list ==
	    	(s3cGlListEntry_t *)NULL))
		++plane_index;

	if (plane_index >= s3cFontData->cache_plane_count)
		/*
		 * cache is full and/or glyph is too big
		 */
		return FALSE;

	index = S3C_GL_HASH(font_id, byteOffset, s3cFontData->hash_table_size);
	bucket = s3cFontData->hash_table[index];
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
		bucket->bucket_size += s3cFontData->initial_bucket_size;
		bucket->cache_positions =
		    (s3cGlCachePosition_t *)Xrealloc(bucket->cache_positions,
		    bucket->bucket_size * sizeof(s3cGlCachePosition_t));
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
	plane_list = s3cFontData->plane_list_info[plane_index];
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
	new_bucket_entry->plane = s3cGlPlaneRects[plane_index].plane;
	new_bucket_entry->font_id = font_id;
	new_bucket_entry->byteOffset = byteOffset;
	new_bucket_entry->coords = list_ptr->coords;

	/*
	 * update glyph info for calling function
	 */
	glyph_info->coords = list_ptr->coords;
	glyph_info->plane = s3cGlPlaneRects[plane_index].plane;

	return TRUE;
}


/*
 *  S3CNAME(GlFlushCache)() -- Flush Glyph Cache
 *
 *	This routine will flush entire glyph cache.
 *
 */
 
void
S3CNAME(GlFlushCache)(
	ScreenPtr 		pScreen)
{
	int 			plane;
	s3cPrivateData_t 	*s3cPriv;
	s3cFontData_t 		*s3cFontData;
	s3cGlPlaneListInfo_t 	*plane_list_info;
	s3cGlListEntry_t 	*busy_ptr;
	s3cGlListEntry_t 	*old_free_ptr;
	s3cGlListEntry_t 	*next_busy_ptr;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cFontData = &s3cPriv->font_data;

	for ( plane = 0; plane < s3cFontData->cache_plane_count; ++plane )
	{
		plane_list_info = s3cFontData->plane_list_info[plane];
		busy_ptr = plane_list_info->busy_list;

		while ( busy_ptr )
		{
			s3cGlDeleteCacheEntry(pScreen, busy_ptr->font_id,
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
		plane_list_info->busy_list = (s3cGlListEntry_t *)NULL;
	}
}
