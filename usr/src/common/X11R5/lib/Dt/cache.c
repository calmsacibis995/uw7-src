
#ifndef NOIDENT
#ident	"@(#)Dt:cache.c	1.2"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <Dt/DesktopI.h>

#define SCREEN_STEP	4
#define TYPE_STEP	4
#define ID_STEP		32

typedef struct {
	void		*id;
	int		id_len;
	void		*data;
} IdCache, *IdCachePtr;

typedef struct {
	long		type;
	IdCachePtr	i_ptr;
	int		used;
	int		alloc;
} TypeCache, *TypeCachePtr;

typedef struct {
	Screen		*screen;
	TypeCachePtr	t_ptr;
	int		used;
	int		alloc;
} ScreenCache, *ScreenCachePtr;

static ScreenCachePtr cache = NULL;
static int cache_used = 0;
static int cache_alloc = 0;

static ScreenCachePtr
Dt__FindScreen(screen)
Screen *screen;
{
	register ScreenCachePtr scrn;
	register int i;

	for (i=cache_used, scrn=cache; i; i--, scrn++)
		if (scrn->screen == screen)
			return(scrn);
	return(NULL);
}

static TypeCachePtr
Dt__FindType(scrn, type)
ScreenCachePtr scrn;
long type;
{
	register TypeCachePtr ty;
	register int i;

	if (scrn) {
		for (i=scrn->used, ty=scrn->t_ptr; i; i--, ty++)
			if (ty->type == type)
				return(ty);
	}
	return(NULL);
}

static IdCachePtr
Dt__FindId(ty, id, id_len)
TypeCachePtr ty;
void *id;
int id_len;
{
	register IdCache *ic;
	register int i;

	if (ty) {
		for (i=ty->used, ic=ty->i_ptr; i; i--, ic++) {
			int len, ret;

			if (id == NULL) {
				if (ic->id == NULL)
					/* If both are NULLs, it is a match */
					return(ic);
			}
			else {
				/*
				 * Since entries are stored in sorted order,
				 * stop when the next item is greater.
				 */
				if (ic->id == NULL)
					continue;
				len = (ic->id_len < id_len)?ic->id_len:id_len;
				ret = memcmp(ic->id, id, len);
				if (ret < 0)
					continue;
				if ((ret == 0) && (ic->id_len == id_len))
					return(ic);
				if ((ret > 0) || (ic->id_len > id_len))
					return(NULL);
			}
		}
	}
	return(NULL);
}

int
DtPutData(screen, type, id, id_len, data)
Screen *screen;
long type;
void *id;
int id_len;
void *data;
{
	ScreenCachePtr scrn;
	TypeCachePtr ty;
	IdCachePtr ic;

	if ((scrn = Dt__FindScreen(screen)) == NULL) {
		/* new screen entry */
		if (cache_used == cache_alloc) {
			ScreenCachePtr new;

			/* expand list */
			cache_alloc += SCREEN_STEP;
			if ((new = (ScreenCachePtr)realloc(cache, cache_alloc *
				sizeof(ScreenCache))) == NULL) {
				cache_alloc -= SCREEN_STEP;
				return(-1);
			}
			cache = new;
		}
		scrn = cache + cache_used++;
		scrn->screen = screen;
		scrn->t_ptr = NULL;
		scrn->used = scrn->alloc = 0;
	}

	if ((ty = Dt__FindType(scrn, type)) == NULL) {
		/* new type entry */
		if (scrn->used == scrn->alloc) {
			TypeCachePtr new;

			/* expand list */
			scrn->alloc += TYPE_STEP;
			if ((new = (TypeCachePtr)realloc(scrn->t_ptr,
				scrn->alloc * sizeof(TypeCache))) == NULL) {
				scrn->alloc -= TYPE_STEP;
				return(-1);
			}
			scrn->t_ptr = new;
		}
		ty = scrn->t_ptr + scrn->used++;
		ty->type = type;
		ty->i_ptr = NULL;
		ty->used = ty->alloc = 0;
	}

	if ((ic = Dt__FindId(ty, id, id_len)) == NULL) {
		int i, len, ret;

		/* new id entry */
		if (ty->used == ty->alloc) {
			IdCachePtr new;

			/* expand list */
			ty->alloc += ID_STEP;
			if ((new = (IdCachePtr)realloc(ty->i_ptr,
				ty->alloc * sizeof(IdCache))) == NULL) {
				ty->alloc -= ID_STEP;
				return(-1);
			}
			ty->i_ptr = new;
		}

		/* insert new entry and keep the list sorted */
		for (i=ty->used, ic=ty->i_ptr; i; i--, ic++) {
			len = (ic->id_len < id_len) ? ic->id_len : id_len;
			ret = memcmp(ic->id, id, len);
			if ((ret > 0) || ((ret == 0) && (ic->id_len > id_len)))
				break;
		}

		if (i)
			/* shift all entries from ic to EOL by one */
			memmove((void *)(ic+1), (void *)ic, i*sizeof(IdCache));

		ic->id = id;
		ic->id_len = id_len;
		ty->used++;
	}

	ic->data = data;

	/* done */
	return(0);
}

void *
DtGetData(screen, type, id, id_len)
Screen *screen;
long type;
void *id;
int id_len;
{
	IdCache *ic;

	ic = Dt__FindId(Dt__FindType(Dt__FindScreen(screen), type), id, id_len);
	return(ic ? ic->data : NULL);
}

/*
 * DtDelData:
 * This function deletes an entry from the cache.
 * Note that only the data entry is really deleted. None of the space is
 * reclaimed, except decrementing the "used" count.
 */
int
DtDelData(screen, type, id, id_len)
Screen *screen;
long type;
void *id;
int id_len;
{
	register IdCachePtr ic;
	register TypeCachePtr ty;
	register int count;

	if ((ty = Dt__FindType(Dt__FindScreen(screen), type)) == NULL)
		return(-1);
	if ((ic = Dt__FindId(ty, id, id_len)) == NULL)
		return(-1);

	/* remove id entry */
	count = --(ty->used) - (int)(ic - ty->i_ptr);
	if (count)
		/* fill the hole */
		memmove((void *)ic, (void *)(ic+1), count * sizeof(IdCache));
	return(0);
}

