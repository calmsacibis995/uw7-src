#ident	"@(#)ksh93:src/lib/libast/string/fmtuid.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * uid number -> name
 */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide getpwuid
#else
#define getpwuid	______getpwuid
#endif

#include <ast.h>
#include <hash.h>
#include <pwd.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide getpwuid
#else
#undef	getpwuid
#endif

extern struct passwd*	getpwuid(uid_t);

/*
 * return uid name given uid number
 */

char*
fmtuid(int uid)
{
	register char*		name;
	register struct passwd*	pw;

	static Hash_table_t*	uidtab;
	static char		buf[sizeof(int) * 3 + 1];

	if (!uidtab && !(uidtab = hashalloc(NiL, HASH_set, HASH_ALLOCATE, HASH_namesize, sizeof(uid), HASH_name, "uidnum", 0))) sfsprintf(name = buf, sizeof(buf), "%d", uid);
	else if (!(name = hashget(uidtab, &uid)))
	{
		if (pw = getpwuid(uid)) name = pw->pw_name;
		else sfsprintf(name = buf, sizeof(buf), "%d", uid);
		hashput(uidtab, NiL, name = strdup(name));
	}
	return(name);
}
