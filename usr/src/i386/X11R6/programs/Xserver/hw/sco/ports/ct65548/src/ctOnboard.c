/*
 *	@(#)ctOnboard.c	11.1	10/22/97	12:34:06
 *	@(#) ctOnboard.c 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id: ctOnboard.c 58.1 96/10/09 "

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "ctDefs.h"
#include "ctMacros.h"

/*
 * Chunks can have various states.
 *	Free:	available for use.
 *	Busy:	in use by the server
 *	Screen:	it's the actual displayed screen
 *	Locked:	not to be moved (should an allocation fail)
 *	Static:	keep between X server resets (not implemented yet)
 */

#define	CT_CHUNK_FREE	0x00000001L	/* chunk is free */
#define	CT_CHUNK_BUSY	0x00000002L	/* chunk is inuse */
#define	CT_CHUNK_SCREEN	0x00000004L	/* chunk is screen memory */
#define	CT_CHUNK_LOCKED	0x00000008L	/* chunk can not be moved */
#define	CT_CHUNK_STATIC	0x00000010L	/* chunk is not freed on X reset */

/*
 * Each chunk has  a data structure assoicated with it.  The
 * data structure stores all data about the offscreen memory area.
 * A free area of offscreen memory has Free set in the flags.
 */
typedef struct _OnboardChunk {
	struct _OnboardChunk	*next;		/* next element */
	struct _OnboardChunk	*prev;		/* previous element */
	CT_PIXEL		*chunkPointer;	/* element virtual address */
	unsigned long		size;		/* in pixels */
	unsigned int		stride;		/* in bytes */
	unsigned long		offset;		/* in bytes */
	unsigned long		flags;		/* usage flags */
	char			*calling_file;	/* for debug */
	int			calling_line;	/* for debug */
} ctOnboardChunkRec, *ctOnboardChunkPtr;

/*
 * All chunks are referenced from here. Only one heap structure exists
 * for each screen.
 */
typedef struct _OnboardHeap {
	CT_PIXEL		*heapPointer;	/* heap virtual address */
	unsigned long		size;		/* in pixels */
	int			round_up;	/* in pixels */
	ctOnboardChunkPtr	head;		/* head element */
} ctOnboardHeapRec, *ctOnboardHeapPtr;

/*******************************************************************************

				Public Routines

*******************************************************************************/

/*
 * Called at init time. This allocates all display memory into one chunk
 * and then allocates the required amount of memory for the screen W x H x D.
 * Returns false if not enough memory exists.
 */
Bool
CT(OnboardInit)(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctOnboardHeapPtr heap;
	ctOnboardChunkPtr chunk;
	unsigned char memory_bits;
	unsigned long requested_size;

	heap = (ctOnboardHeapPtr)xalloc(sizeof(ctOnboardHeapRec));
	if (!heap) {
		ctPriv->heap = (pointer)0;
		return (FALSE);
	}
	chunk = (ctOnboardChunkPtr)xalloc(sizeof(ctOnboardChunkRec));
	if (!chunk) {
		xfree(heap);
		return (FALSE);
	}
	ctPriv->heap = (pointer)heap;

	/*
	 * Read memory configuration from control register (XR04).
	 */
	CT_XRIN(0x0f, memory_bits);

	switch (memory_bits & 0x03) {
	default:
		ErrorF("ctOnboardInit(): hardware memory configuration not recognized\n");
		ErrorF("ctOnboardInit(): assuming 1/2Mbyte installed\n");
		/* FALL THRU */
	case 0x02:
		heap->size = 1024 * 1024;	/* 1MB */
		break;
	case 0x00:
		heap->size = 256 * 1024;	/* 256KB */
		break;
	case 0x01:
		heap->size = 512 * 1024;	/* 512KB */
		break;
	}

	/*
	 * Initialize onboard memory heap.
	 */

	switch (ctPriv->bltPixelSize) {
	case 1:
		heap->round_up = 64 / ctPriv->bltPixelSize;
		break;
	case 2:
		heap->round_up = 128 / ctPriv->bltPixelSize;
		break;
	case 3:
		heap->round_up = 4;			/* XXX: change? */
		break;
	}

	/*
	 * heap is always measured in pixels and not bytes.
	 */
	heap->size /= ctPriv->bltPixelSize;

	/*
 	 * heap is all of memory.
	 */
	heap->heapPointer = ctPriv->fbPointer;

	/*
	 * Allocate memory for the display.
	 *
	 * The allocated space MUST have an offset of zero to allow
	 * for the display setup code to show the correct bits from
	 * display memory.  If this code is changed to actually allocate
	 * a chunk, then force it's offset to zero.
	 */
	requested_size = ctPriv->width * ctPriv->height;

	if (heap->size < requested_size) {
		/* not enough memory */
		ErrorF("memory_bits register = 0x%x\n", memory_bits & 0x3);
		ErrorF("ctOnboardInit(): selected screen resolution requires more display memory\n");
		ErrorF("ctOnboardInit(): present display memory = %dKb\n",
			heap->size * ctPriv->bltPixelSize / 1024);
		ErrorF("ctOnboardInit(): required display memory = %dKb\n",
			requested_size * ctPriv->bltPixelSize / 1024);
		xfree(heap);
		ctPriv->heap = (pointer)0;
		return (FALSE);
	}
	heap->size -= requested_size;
	heap->heapPointer += requested_size;

	/*
	 * hand setup first free chunk.
	 */
	heap->head = chunk;

	chunk->next = (ctOnboardChunkPtr)0;
	chunk->prev = (ctOnboardChunkPtr)0;
	chunk->chunkPointer = heap->heapPointer;
	chunk->size = heap->size;
	chunk->stride = 0;
	chunk->offset = (chunk->chunkPointer - ctPriv->fbPointer)
				* ctPriv->bltPixelSize;
	chunk->flags = CT_CHUNK_FREE;
	chunk->calling_file = __FILE__;
	chunk->calling_line = __LINE__;

#ifdef DEBUG_PRINT
	ErrorF("OnboardInit(): first chunk 0x%08x, size=%d pixels, chunkPointer=0x%08x, offset=0x%08x\n",
		chunk, chunk->size, chunk->chunkPointer, chunk->offset);
#endif /* DEBUG_PRINT */

	return (TRUE);
}

/*
 * At shutdown time, all offscreen memory is free'ed.  This is more of
 * a sanity check than an actual requirement.
 */
void
CT(OnboardClose)(pScreen)
ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctOnboardHeapPtr heap = (ctOnboardHeapPtr)ctPriv->heap;
	ctOnboardChunkPtr chunk;

#ifdef DEBUG_PRINT
	ErrorF("OnboardClose()\n");
#endif /* DEBUG_PRINT */

	if (!heap)
		return;

	chunk = heap->head;
	while (chunk) {
		ctOnboardChunkPtr next;

		if (chunk->flags == CT_CHUNK_FREE) {
#ifdef DEBUG_PRINT
			ErrorF("OnboardClose(): 0x%08x chunk free: size=%d\n",
				chunk,
				chunk->size);
#endif /* DEBUG_PRINT */
		} else {
			ErrorF("ctOnboardClose(): 0x%08x chunk not free: %s:%d size=%d\n",
				chunk,
				chunk->calling_file, chunk->calling_line,
				chunk->size);
		}
		next = chunk->next;
		xfree(chunk);
		chunk = next;
	}
	xfree(heap);
	ctPriv->heap = (pointer)0;
}

/*
 * Allocate some area on the screen.  This returns an opaque pointer, always
 * use that routine to access the off-screen area later because offscreen
 * areas can be moved.  Use OnboardAddr() and OnboardVAddr() to convert
 * a chunk into an address.
 */
void *
CT(OnboardAlloc)(pScreen, width, height, stride, calling_file, calling_line)
ScreenPtr pScreen;
unsigned int width;
unsigned int height;
unsigned int stride;
char *calling_file;
int calling_line;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctOnboardHeapPtr heap = (ctOnboardHeapPtr)ctPriv->heap;
	ctOnboardChunkPtr chunk, new_chunk;
	unsigned long requested_size;

#ifdef DEBUG_PRINT
	ErrorF("OnboardAlloc(): %d x %d %s:%d\n",
		width, height, calling_file, calling_line);
#endif /* DEBUG_PRINT */

	if (!heap)
		return ((void *)0);

	if (width > ctPriv->fbStride) {
		/* XXX: can not handle this case */
		return ((void *)0);
	}

	requested_size = width * height + (heap->round_up - 1);
	requested_size = (requested_size / heap->round_up) * heap->round_up;

#ifdef DEBUG_PRINT
	ErrorF("\treq_size=%d\n", requested_size);
#endif /* DEBUG_PRINT */

	chunk = heap->head;
	while (chunk) {
		if ((chunk->flags == CT_CHUNK_FREE) && (chunk->size >= requested_size)) {
			/* will fit */
			break;
		}
		chunk = chunk->next;
	}
	if (!chunk) {
		/*
		 * out of space!!!. We could move all offscreen memory
		 * downwards and join all the free space together.
		 * We don't for now.
		 */
		ErrorF("ctOnboardAlloc(): out of space: %d x %d from %s:%d\n",
			width, height,
			calling_file, calling_line);

		return ((void *)0);
	}
	if (chunk->size == requested_size) {
		/* special case */
		new_chunk = chunk;
		new_chunk->stride = stride;
		new_chunk->flags = CT_CHUNK_BUSY;
		new_chunk->calling_file = calling_file;
		new_chunk->calling_line = calling_line;
#ifdef DEBUG_PRINT
		ErrorF("\t0x%08x offset=0x%08x\n",
			new_chunk, new_chunk->offset);
#endif /* DEBUG_PRINT */

		return ((void *)new_chunk);
	}

	new_chunk = (ctOnboardChunkPtr)xalloc(sizeof(ctOnboardChunkRec));
	if (!new_chunk) {
		return ((void *)0);
	}

	/*
	 * add new chunk into chain
	 */
	new_chunk->prev = chunk->prev;
	new_chunk->next = chunk;
	chunk->prev = new_chunk;
	if (new_chunk->prev) {
		new_chunk->prev->next = new_chunk;
	} else {
		/*
		 * Special case: no previous chunk.
		 */
		heap->head = new_chunk;
	}

	/*
	 * setup new chunk
	 */
	new_chunk->chunkPointer = chunk->chunkPointer;
	new_chunk->size = requested_size;
	new_chunk->stride = stride;
	new_chunk->offset = (new_chunk->chunkPointer - ctPriv->fbPointer)
				* ctPriv->bltPixelSize;
	new_chunk->flags = CT_CHUNK_BUSY;
	new_chunk->calling_file = calling_file;
	new_chunk->calling_line = calling_line;

	/*
	 * reduce size of original chunk
	 */
	chunk->chunkPointer += requested_size;
	chunk->size -= requested_size;
	chunk->offset = (chunk->chunkPointer - ctPriv->fbPointer)
				* ctPriv->bltPixelSize;

#ifdef DEBUG_PRINT
	ErrorF("\t0x%08x offset=0x%08x\n",
		new_chunk,
		new_chunk->offset);
#endif /* DEBUG_PRINT */

	return ((void *)new_chunk);
}

/*
 * Free an already allocated area.  This will join the free'ed area with
 * the areas above and this area.
 */
void
CT(OnboardFree)(pScreen, ptr, calling_file, calling_line)
ScreenPtr pScreen;
void *ptr;
char *calling_file;
int calling_line;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	ctOnboardHeapPtr heap = (ctOnboardHeapPtr)ctPriv->heap;
	ctOnboardChunkPtr chunk = (ctOnboardChunkPtr)ptr;

#ifdef DEBUG_PRINT
	ErrorF("OnboardFree(): 0x%08x: offset=0x%08x size=%d %s:%d\n",
		chunk, chunk->offset, chunk->size,
		calling_file, calling_line);
#endif /* DEBUG_PRINT */

	if (!heap) {
		ErrorF("ctOnboardFree(): free of chunk without heap\n");
		return;
	}

	/*
	 * first free this chunk.  Leave it in the list of chunks.
	 */
	chunk->flags = CT_CHUNK_FREE;
	chunk->calling_file = "";
	chunk->calling_line = 0;

join_again:

	/*
	 * See if this chunk and the next chunk are both free.  If so, then
	 * join them by moving the space in this chunk into the next chunk
	 * and freeing this chunk.
 	 */
	if (chunk->next && (chunk->next->flags == CT_CHUNK_FREE)) {
		/* next item on chain is free, hence join */
#ifdef DEBUG_PRINT
		ErrorF("OnboardFree(): join 0x%08x size=%d & 0x%08x size=%d\n",
			chunk, chunk->size,
			chunk->next, chunk->next->size);
#endif /* DEBUG_PRINT */
		if ((chunk->chunkPointer + chunk->size) != chunk->next->chunkPointer) {
			ErrorF("ctOnboardFree(): failed join of chunk - bad addresses\n");
			return;
		}
		/* move space into next chunk */
		chunk->next->chunkPointer -= chunk->size;
		chunk->next->size += chunk->size;
		chunk->next->stride = 0;
		chunk->next->offset = (chunk->next->chunkPointer - ctPriv->fbPointer)
					* ctPriv->bltPixelSize;
		chunk->next->calling_file = "";
		chunk->next->calling_line = 0;
		chunk->next->prev = chunk->prev;
		if (chunk->prev) {
			chunk->prev->next = chunk->next;
		} else {
			/*
			 * Special case: no previous chunk.
			 */
			heap->head = chunk->next;
		}
		xfree(chunk);
	}

	/*
	 * look at the previous chunk.  If it is free, then repeat the
	 * join code.
 	 */
	if (chunk->prev && (chunk->prev->flags == CT_CHUNK_FREE)) {
		chunk = chunk->prev;
		goto join_again;
	}
}

/*
 * Lock a chunk. Dont allow it to be moved.
 */
void
CT(OnboardLock)(ptr)
void *ptr;
{
	ctOnboardChunkPtr chunk = (ctOnboardChunkPtr)ptr;

#ifdef DEBUG_PRINT
	ErrorF("OnboardLock(): 0x%08x: flags=0x%08x\n",
		chunk,
		chunk->flags);
#endif /* DEBUG_PRINT */
	chunk->flags |= CT_CHUNK_LOCKED;
}

/*
 * Unlock a chunk. Allow it to be moved.
 */
void
CT(OnboardUnlock)(ptr)
void *ptr;
{
	ctOnboardChunkPtr chunk = (ctOnboardChunkPtr)ptr;

#ifdef DEBUG_PRINT
	ErrorF("OnboardUnlock(): 0x%08x: flags=0x%08x\n",
		chunk,
		chunk->flags);
#endif /* DEBUG_PRINT */
	chunk->flags &= ~CT_CHUNK_LOCKED;
}

/*
 * Return a chunks byte offset into display memory 
 */
unsigned long
CT(OnboardOffset)(ptr, x, y)
void *ptr;
unsigned int x, y;
{
	ctOnboardChunkPtr chunk = (ctOnboardChunkPtr)ptr;
	unsigned long offset;

	offset =  chunk->offset + (x * sizeof(CT_PIXEL)) + (y * chunk->stride);

#ifdef DEBUG_PRINT
	ErrorF("OnboardAddr(): 0x%08x: x,y=%d,%d offset=0x%08x\n",
		chunk,
		x, y,
		offset);
#endif /* DEBUG_PRINT */

	return (offset);
}

/*
 * Return a chunks virtual address.
 */
CT_PIXEL *
CT(OnboardVAddr)(ptr)
void *ptr;
{
	ctOnboardChunkPtr chunk = (ctOnboardChunkPtr)ptr;

#ifdef DEBUG_PRINT
	ErrorF("OnboardVAddr(): 0x%08x: chunkPointer=0x%08x\n",
		chunk,
		chunk->chunkPointer);
#endif /* DEBUG_PRINT */
	return (chunk->chunkPointer);
}

int
CT(OnboardAvailable)(pScreen)
ScreenPtr pScreen;
{
	/*
	 * Return the amount of currently available memory.
	 */
	return (((ctOnboardHeapPtr)CT_PRIVATE_DATA(pScreen)->heap)->size);
}
