#ident	"@(#)cg:common/arena.c	1.11"
#include "mfile2.h"
#include <malloc.h>
#include <unistd.h>
#include "arena.h"
#ifdef NODBG
#ifndef NDEBUG
#define NDEBUG /* for assert */
#endif
#endif
#include <assert.h>

#ifdef NODBG
#define MALLOC(cnt,type,p) \
	{ \
	p = (type *)malloc(cnt *  sizeof(type)); \
	if (!p) \
		cerror(gettxt(":593","storage failure")); \
	}
#else
#define MALLOC(cnt,type,p) \
	{ \
	cur_mem_used += cnt * sizeof(type); \
	if (cur_mem_used > max_mem_used) \
		max_mem_used = cur_mem_used;	\
	p = (type *)malloc(cnt *  sizeof(type)); \
	if (!p) \
		cerror(gettxt(":593","storage failure")); \
	}
#endif

#ifndef NODBG
static void add_check_space(), remove_check_space();
#define MAGIC_BYTE 123
#define CHECK_SPACE 16
#endif


#define DEFAULT_ALLOC 8192
#define DEFAULT_THRESH DEFAULT_ALLOC/4
#define EXTENTS 500
#define MAGIC -987654321

struct Extent {
	char *extent; /* block of storage */
	struct Extent *next_extent;
};

struct Arena_struct {
	char * curr_byte;
	char * last_byte;
	struct Extent *extent_list;	
	int magic;
#ifndef NODBG
	char * arena_name;
	unsigned int arena_size;
	unsigned int last_report;
#endif
};

#ifndef NODBG

static max_mem_used, cur_mem_used;

void
arena_init_stats()
{
	max_mem_used = cur_mem_used = 0;
}

int
arena_max_mem()
{
	return max_mem_used;
}

#endif

struct Arena_struct *
arena_init()
{
	struct Arena_struct *a;
	MALLOC(1,struct Arena_struct, a)
	a->extent_list = 0;
	a->magic = MAGIC;
	a->curr_byte = a->last_byte=0;
#ifndef NODBG
	a->arena_name = 0;
	a->arena_size = 0;
	a->last_report = 0;
#endif
	return a;
}

#ifndef NODBG
void
arena_init_debug(a, name)
struct Arena_struct * a;
char *name;
{
	a->arena_name = name;
	if (a->arena_size)
		fprintf(stderr, "start debugging for %s: %d bytes already allocated\n", name, a->arena_size);
	else
		fprintf(stderr, "start debugging for %s\n", name);
	a->last_report = a->arena_size;
}

void
arena_debug(a, s)
struct Arena_struct * a;
char *s;
{
	if (!a->arena_name)
		return;
	fprintf(stderr, "arena %s: %s: %d bytes\n",a->arena_name,s,a->arena_size);
}
#endif

void
arena_term(a)
struct Arena_struct * a;
{
	assert(a->magic == MAGIC);
#ifndef NODBG
	arena_check();
	if (a->arena_name)
		fprintf(stderr, "Free %d bytes for arena %s\n", 
			a->arena_size, a->arena_name);
	cur_mem_used -= a->arena_size;
#endif
	while(a->extent_list) {
		struct Extent *e = a->extent_list;
#ifndef NODBG
		remove_check_space(e->extent);
#endif
		free((myVOID *)e->extent);
		a->extent_list = e->next_extent;
		free((myVOID *)e);
	}
	free((myVOID *)a);
}

static struct  Align_s {
	char x;

	union Align_u  {
		short s;
		long l;		

#ifdef __STDC__
		long double ld;
		long double *ldp;
#endif
		double d;
		double *dp;
		float f;
	

		/* assume every pointer type has alignment no stricter
		   then one of the following.. May not be the case in
		   general, but is the case in practice
		*/
		char *cp;
		short *sp;
		long *lp;
		float *fp;
		int (*fup)();
		char **cpp;
	} u;
};
#define ALLOC_ALIGN (sizeof(struct Align_s) - sizeof (union Align_u) )
		

myVOID *
arena_alloc(a,size)
struct Arena_struct *a;
int size;
{
	char *p;
	int alloc_size;
	struct Extent *new_extent;
	size = ((size+ALLOC_ALIGN - 1 ) / ALLOC_ALIGN ) * ALLOC_ALIGN;
	assert(a->magic == MAGIC);
	if (a->curr_byte + size < a->last_byte) {
		p = a->curr_byte;
		a->curr_byte += size;
		return (myVOID *)p;
	}
		/* Need new extent */
	if (size < DEFAULT_THRESH) 
		alloc_size = DEFAULT_ALLOC;
	else
		alloc_size = size;
#ifndef NODBG
	alloc_size += 2*CHECK_SPACE;
	a->arena_size += alloc_size + sizeof(struct Extent);
#define ONE_MEG	1000000
	if (a->arena_name && (a->arena_size > a->last_report + ONE_MEG)) {
		fprintf(stderr, "%s: %d Meg\n", a->arena_name, a->arena_size/ONE_MEG);
		a->last_report = a->arena_size;
	}
#endif
	MALLOC(alloc_size, char, p)
	MALLOC(1, struct Extent, new_extent);
	new_extent->extent = p;
	new_extent->next_extent = a->extent_list;
	a->extent_list = new_extent;
#ifndef NODBG
	add_check_space(p, p + alloc_size - CHECK_SPACE);
	p += CHECK_SPACE;
	alloc_size -= 2*CHECK_SPACE;
#endif
	a->curr_byte = p+size;
	a->last_byte = p+alloc_size;
	return (myVOID *)p;
}

#ifndef NODBG

typedef struct check_space_list {
	char *space1;
	char *space2;
	struct check_space_list *next;
} CHECKLIST;

static CHECKLIST *checklist;

static void
add_check_space(p1, p2)
char *p1, *p2;
{
	CHECKLIST *check;
	int i;

	MALLOC(sizeof(CHECKLIST), CHECKLIST, check);
	check->space1 = p1;
	check->space2 = p2;
	check->next = checklist;
	checklist = check;
	for (i=0; i<CHECK_SPACE; i++)
		p1[i] = p2[i] = MAGIC_BYTE;
}

static void
remove_check_space(p)
char *p;
{
	CHECKLIST *curr, *prev;
	curr = checklist;
	prev = 0;
	while (curr && curr->space1 != p) {
		prev = curr;
		curr = curr->next;
	}
	if (!curr) cerror("Cannot find check space");
	if (prev)
		prev->next = curr->next;
	else
		checklist = curr->next;
}

void
arena_check()
{
	CHECKLIST *check = checklist;
	while (check) {
		int i;
		for (i=0; i<CHECK_SPACE; i++)
			if (check->space1[i] != MAGIC_BYTE
			 || check->space2[i] != MAGIC_BYTE)
				cerror("Arena Trashed");
		check = check->next;
	}
}

#endif
