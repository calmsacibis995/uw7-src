#ident	"@(#)ksh93:src/lib/libcmd/chgrp.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * chgrp+chown
 */

static const char id[] = "\n@(#)chgrp (AT&T Bell Laboratories) 05/09/95\0\n";

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide lchown
#else
#define lchown		______lchown
#endif

#include <cmdlib.h>
#include <hash.h>
#include <ls.h>
#include <ctype.h>
#include <ftwalk.h>

#include "FEATURE/symlink"

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide lchown
#else
#undef	lchown
#endif

typedef struct				/* uid/gid map			*/
{
	HASH_HEADER;			/* hash bucket header		*/
	int	uid;			/* id maps to this uid		*/
	int	gid;			/* id maps to this gid		*/
} Map_t;

#define NOID		(-1)

#define F_CHOWN		(1<<0)		/* chown			*/
#define F_DONT		(1<<1)		/* show but don't do		*/
#define F_FORCE		(1<<2)		/* ignore errors		*/
#define F_GID		(1<<3)		/* have gid			*/
#define F_RECURSE	(1<<4)		/* ftw				*/
#define F_UID		(1<<5)		/* have uid			*/

extern int	lchown(const char*, uid_t, gid_t);

/*
 * parse uid and gid from s
 */

static void
getids(register char* s, char** e, int* uid, int* gid, int flags)
{
	register char*	t;
	register int	n;
	char*		z;
	char		buf[64];

	*uid = *gid = NOID;
	while (isspace(*s)) s++;
	for (t = s; (n = *t) && n != ':' && n != '.' && !isspace(n); t++);
	if (n)
	{
		flags |= F_CHOWN;
		if ((n = t++ - s) >= sizeof(buf))
			n = sizeof(buf) - 1;
		*((s = (char*)memcpy(buf, s, n)) + n) = 0;
		while (isspace(*t)) t++;
	}
	if (flags & F_CHOWN)
	{
		if (*s)
		{
			if ((n = struid(s)) == NOID)
			{
				n = (int)strtol(s, &z, 0);
				if (*z) error(ERROR_exit(1), gettxt(":279", "%s: unknown user"), s);
			}
			*uid = n;
		}
		for (s = t; (n = *t) && !isspace(n); t++);
		if (n)
		{
			if ((n = t++ - s) >= sizeof(buf))
				n = sizeof(buf) - 1;
			*((s = (char*)memcpy(buf, s, n)) + n) = 0;
		}
	}
	if (*s)
	{
		if ((n = strgid(s)) == NOID)
		{
			n = (int)strtol(s, &z, 0);
			if (*z) error(ERROR_exit(1), gettxt(":280","%s: unknown group"), s);
		}
		*gid = n;
	}
	if (e) *e = t;
}

int
b_chgrp(int argc, char *argv[])
{
	register int	flags = 0;
	register char*	file;
	register char*	s;
	register Map_t*	m;
	Hash_table_t*	map = 0;
	int		resolve;
	int		n;
	int		uid;
	int		gid;
	struct stat	st;
	char*		op;
	int		(*chownf)(const char*, uid_t, gid_t);
	int		(*statf)(const char*, struct stat*);

	NoP(id[0]);
	cmdinit(argv);
	resolve = ftwflags();
	if (error_info.id[2] == 'g')
		s = "fmnHLPR [owner:]group file...";
	else
	{
		flags |= F_CHOWN;
		s = "fmnHLPR owner[:group] file...";
	}
	while (n = optget(argv, s)) switch (n)
	{
	case 'f':
		flags |= F_FORCE;
		break;
	case 'm':
		if (!(map = hashalloc(NiL, HASH_set, HASH_ALLOCATE, HASH_namesize, sizeof(int), HASH_name, "ids", 0)))
			error(ERROR_exit(1), gettxt(":321","out of space space [id map]"));
		break;
	case 'n':
		flags |= F_DONT;
		break;
	case 'H':
		resolve |= FTW_META|FTW_PHYSICAL;
		break;
	case 'L':
		resolve &= ~(FTW_META|FTW_PHYSICAL);
		break;
	case 'P':
		resolve &= ~FTW_META;
		resolve |= FTW_PHYSICAL;
		break;
	case 'R':
		flags |= F_RECURSE;
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	argc -= opt_info.index;
	if (error_info.errors || argc < 2)
		error(ERROR_usage(2), optusage(NiL));
	s = *argv;
	if (map)
	{
		char*	t;
		Sfio_t*	sp;
		int	nuid;
		int	ngid;

		if (streq(s, "-"))
			sp = sfstdin;
		else if (!(sp = sfopen(NiL, s, "r")))
			error(ERROR_exit(1), gettxt(":322","%s: cannot read"), s);
		while (s = sfgetr(sp, '\n', 1))
		{
			getids(s, &t, &uid, &gid, flags);
			getids(t, NiL, &nuid, &ngid, flags);
			if (uid != NOID)
			{
				if (m = (Map_t*)hashlook(map, (char*)&uid, HASH_LOOKUP, NiL))
				{
					m->uid = nuid;
					if (m->gid == NOID)
						m->gid = ngid;
				}
				else if (m = (Map_t*)hashlook(map, NiL, HASH_CREATE|HASH_SIZE(sizeof(Map_t)), NiL))
				{
					m->uid = nuid;
					m->gid = ngid;
				}
				else error(ERROR_exit(1), gettxt(":321","out of space space [id map]"));
			}
			if (gid != NOID)
			{
				if (gid == uid || (m = (Map_t*)hashlook(map, (char*)&gid, HASH_LOOKUP, NiL)))
					m->gid = ngid;
				else if (m = (Map_t*)hashlook(map, NiL, HASH_CREATE|HASH_SIZE(sizeof(Map_t)), NiL))
				{
					m->uid = NOID;
					m->gid = ngid;
				}
				else error(ERROR_exit(1), gettxt(":321","out of space space [id map]"));
			}
		}
		if (sp != sfstdin)
			sfclose(sp);
	}
	else
	{
		getids(s, NiL, &uid, &gid, flags);
		if (uid != NOID)
			flags |= F_UID;
		if (gid != NOID)
			flags |= F_GID;
	}
	if (flags & F_RECURSE)
	{
		char*	pfxv[5];

		n = 0;
		if (flags & F_FORCE) pfxv[n++] = "-f";
		if (map) pfxv[n++] = "-m";
		if (flags & F_DONT) pfxv[n++] = "-n";
		if (resolve & FTW_META) pfxv[n++] = "-H";
		else if (resolve & FTW_PHYSICAL) pfxv[n++] = "-P";
		pfxv[n] = 0;
		return(cmdrecurse(argc, argv, n, pfxv));
	}
	statf = (resolve & FTW_PHYSICAL) ? lstat : stat;
#if _lib_lchown
	if (resolve & FTW_PHYSICAL)
	{
		chownf = lchown;
		op = "lchown";
	}
	else
#endif
	{
		chownf = chown;
		op = "chown";
	}
	while (file = *++argv)
	{
		if (!(*statf)(file, &st))
		{
			if (map)
			{
				flags &= ~(F_UID|F_GID);
				uid = st.st_uid;
				gid = st.st_gid;
				if ((m = (Map_t*)hashlook(map, (char*)&uid, HASH_LOOKUP, NiL)) && m->uid != NOID)
				{
					uid = m->uid;
					flags |= F_UID;
				}
				if (gid != uid)
					m = (Map_t*)hashlook(map, (char*)&gid, HASH_LOOKUP, NiL);
				if (m && m->gid != NOID)
				{
					gid = m->gid;
					flags |= F_GID;
				}
				if (!(flags & (F_UID|F_GID)))
					continue;
			}
			else
			{
				if (!(flags & F_UID))
					uid = st.st_uid;
				if (!(flags & F_GID))
					gid = st.st_gid;
			}
			if (flags & F_DONT)
				sfprintf(sfstdout, "%s uid:%05d->%05d gid:%05d->%05d %s\n", op, st.st_uid, uid, st.st_gid, gid, file);
			else if ((*chownf)(file, uid, gid) && !(flags & F_FORCE))
			{
				switch (flags & (F_UID|F_GID))
				{
				case F_UID:
					error(ERROR_system(0), gettxt(":283","%s: cannot change owner"), file);
					break;
				case F_GID:
					error(ERROR_system(0), gettxt(":282","%s: cannot change group"), file);
					break;
				case F_UID|F_GID:
					error(ERROR_system(0), gettxt(":284","%s: cannot change owner and group"), file);
					break;
				default:
					error(ERROR_system(0), gettxt(":281","%s: cannot change"), file);
					break;
				}
				error_info.errors++;
			}
		}
		else if (!(flags & F_FORCE))
		{
			error(ERROR_system(0), gettxt(":260","%s: not found"), file);
			error_info.errors++;
		}
	}
	return(error_info.errors != 0);
}
