#ident	"@(#)ksh93:src/lib/libast/string/fmtfs.c	1.1"
#pragma prototyped

/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return fs type string given stat
 */

#include <ast.h>
#include <ls.h>

#include "FEATURE/fs"

#if _str_st_fstype || !_hdr_mntent && !_hdr_mnttab

char*
fmtfs(struct stat* st)
{
#if _str_st_fstype
	return(st->st_fstype);
#else
	return(FS_default);
#endif
}

#else

#include <stdio.h>
#include <hash.h>

#if _hdr_mntent && _lib_getmntent

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide endmntent getmntent
#else
#define endmntent	______endmntent
#define getmntent	______getmntent
#endif

#include <mntent.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide endmntent getmntent
#else
#undef	endmntent
#undef	getmntent
#endif

extern int		endmntent(FILE*);
extern struct mntent*	getmntent(FILE*);

#else

/*
 * shortened for tw info
 */

#if _hdr_mnttab
#define MOUNTED		"/etc/mnttab"
#define SEP		'\t'
#else
#define MOUNTED		"/etc/mtab"
#define SEP		' '
#endif
#define	MNTTYPE_IGNORE	"ignore"
#define setmntent(n,f)	fopen(n,f)

struct	mntent
{
	char	mnt_dir[256];
	char	mnt_type[32];
};

static struct mntent*
getmntent(FILE* mp)
{
	register int		c;
	register char*		s;
	register char*		m;
	register int		q;
	static struct mntent	e;

	q = 0;
	s = m = 0;
	for (;;) switch (c = getc(mp))
	{
	case EOF:
		return(0);
	case SEP:
		switch (++q)
		{
		case 1:
			s = e.mnt_dir;
			m = s + sizeof(e.mnt_dir) - 1;
			break;
		case 2:
			*s = 0;
			s = e.mnt_type;
			m = s + sizeof(e.mnt_type) - 1;
			break;
		case 3:
			*s = 0;
			s = m = 0;
			break;
		}
		break;
	case '\n':
		if (q >= 3) return(&e);
		q = 0;
		s = m = 0;
		break;
	default:
		if (s < m) *s++ = c;
		break;
	}
}

static int
endmntent(FILE* mp)
{
	fclose(mp);
	return(1);
}

#endif

#ifndef MOUNTED
#ifdef	MNT_MNTTAB
#define MOUNTED		MNT_MNTTAB
#else
#if _hdr_mnttab
#define MOUNTED		"/etc/mnttab"
#else
#define MOUNTED		"/etc/mtab"
#endif
#endif
#endif

char*
fmtfs(struct stat* st)
{
	register FILE*		mp;
	register struct mntent*	mnt;
	register char*		s;
	struct stat		rt;

	static Hash_table_t*	tab;
	static char		typ[16];

	if ((tab || (tab = hashalloc(NiL, HASH_set, HASH_ALLOCATE, HASH_namesize, sizeof(dev_t), HASH_name, "fstype", 0))) && (s = (char*)hashget(tab, &st->st_dev)))
		return(s);
	s = FS_default;
	if (mp = setmntent(MOUNTED, "r"))
	{
		while ((mnt = getmntent(mp)) && (!strcmp(mnt->mnt_type, MNTTYPE_IGNORE) || stat(mnt->mnt_dir, &rt) || rt.st_dev != st->st_dev));
		endmntent(mp);
		if (mnt && mnt->mnt_type && (!tab || !(s = strdup(mnt->mnt_type))))
			return(strncpy(typ, mnt->mnt_type, sizeof(typ)));
	}
	if (tab) hashput(tab, NiL, s);
	return(s);
}

#endif
