#ident	"@(#)ksh93:src/lib/libast/string/fmtgid.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * cached gid number -> name
 */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide getgrgid
#else
#define getgrgid	______getgrgid
#endif

#include <ast.h>
#include <hash.h>
#include <grp.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide getgrgid
#else
#undef	getgrgid
#endif

extern struct group*	getgrgid(gid_t);

/*
 * return gid name given gid number
 */

char*
fmtgid(int gid)
{
	register char*		name;
	register struct group*	gr;

	static Hash_table_t*	gidtab;
	static char		buf[sizeof(int) * 3 + 1];

	if (!gidtab && !(gidtab = hashalloc(NiL, HASH_set, HASH_ALLOCATE, HASH_namesize, sizeof(gid), HASH_name, "gidnum", 0))) sfsprintf(name = buf, sizeof(buf), "%d", gid);
	else if (!(name = hashget(gidtab, &gid)))
	{
		if (gr = getgrgid(gid)) name = gr->gr_name;
		else sfsprintf(name = buf, sizeof(buf), "%d", gid);
		hashput(gidtab, NiL, name = strdup(name));
	}
	return(name);
}
