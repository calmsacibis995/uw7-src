#ident	"@(#)ksh93:src/lib/libcmd/id.c	1.1"
#pragma prototyped
/*
 * David Korn
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * id
 */

static const char id[] = "\n@(#)id (AT&T Bell Laboratories) 07/17/94\0\n";

#include <cmdlib.h>

#include "FEATURE/ids"

#include <grp.h>
#include <pwd.h>

#if _lib_fsid
#if _lib_getfsgid && ( _sys_fss || _hdr_fsg )
#define fss_grp		fs_grp
#define fss_id		fs_id
#define fss_mem		fs_mem
#define fss_passwd	fs_passwd
#define fss_shares	fs_shares
#if _sys_fss
#include <sys/fss.h>
#endif
#if _hdr_fsg
#include <fsg.h>
#endif
#if !_lib_isfsg && !defined(isfsg)
#define isfsg(p)	(!(p)->fs_id&&!(p)->fs_shares&&(!(p)->fs_passwd||!*(p)->fs_passwd))
#endif
#else
#undef _lib_fsid
#endif
#endif

#define power2(n)	(!((n)&((n)-1)))

#define GG_FLAG		(1<<0)
#define G_FLAG		(1<<1)
#define N_FLAG		(1<<2)
#define R_FLAG		(1<<3)
#define U_FLAG		(1<<4)
#define S_FLAG		(1<<5)
#define O_FLAG		(1<<6)
#define X_FLAG		(1<<7)

#if _lib_fsid
static void
getfsids(Sfio_t* sp, const char* name, int flags, register int lastchar)
{
	register struct fsg*	fs;
	register char*		s;
	register char**		p;
	char**			x;

	if (lastchar)
	{
		if (flags & O_FLAG) flags = 1;
		else flags = 0;
	}
	else if (flags & N_FLAG) flags = 1;
	else flags = -1;
	setfsgent();
	while (fs = getfsgnam(name))
		if (!isfsg(fs))
		{
			if (p = fs->fs_mem)
			{
				if (flags > 0) x = 0;
				else
				{
					register char**		q;
					register char*		t;
					register int		n;

					n = 0;
					q = p;
					while (s = *q++)
						n += strlen(s) + 1;
					if (!(x = newof(0, char*, q - p, n)))
						break;
					s = (char*)(x + (q - p));
					q = x;
					while (t = *p++)
					{
						*q++ = s;
						while (*s++ = *t++);
					}
					*q = 0;
					p = x;
				}
				while (s = *p++)
				{
					if (lastchar == '=')
					{
						lastchar = ',';
						sfputr(sp, " fsid=", -1);
					}
					else if (!lastchar) lastchar = ' ';
					else sfputc(sp, lastchar);
					if (flags > 0) sfprintf(sp, "%s", s);
					else
					{
						setfsgent();
						while (fs = getfsgnam(s))
							if (isfsg(fs))
							{
								if (flags < 0) sfprintf(sp, "%u", fs->fs_id);
								else sfprintf(sp, "%u(%s)", fs->fs_id, s);
								break;
							}
					}
				}
				if (x) free(x);
			}
			break;
		}
	endfsgent();
	if (lastchar == ' ') sfputc(sp, '\n');
}
#endif

static void
putid(Sfio_t* sp, int flags, const char* label, const char* name, long number)
{
	sfprintf(sp, "%s=", label);
	if (flags & O_FLAG)
	{
		if (name) sfputr(sp, name, -1);
		else sfprintf(sp, "%lu", number);
	}
	else
	{
		sfprintf(sp, "%u", number);
		if (name) sfprintf(sp, "(%s)", name);
	}
}

static int
getids(Sfio_t* sp, const char* name, register int flags)
{
	register struct passwd*	pw;
	register struct group*	grp;
	register int		i;
	register int		j;
	register int		k;
#if _lib_fsid
	register struct fsg*	fs;
	const char*		fs_name;
	int			fs_id;
#endif
	char**			p;
	char*			s;
	int			lastchar;
	int			ngroups = 0;
	const char*		gname;
	uid_t			user;
	uid_t			euid;
	gid_t			group;
	gid_t			egid;

	static gid_t*		groups;

	if (flags & GG_FLAG)
	{
		static int	maxgroups;

		/*
		 * get supplemental groups if required
		 */

		if (!maxgroups)
		{
			/*
			 * first time
			 */

			if ((maxgroups = getgroups(0, groups)) <= 0)
				maxgroups = NGROUPS_MAX;
			groups = newof(0, gid_t, maxgroups + 1, 0);
		}
		ngroups = getgroups(maxgroups, groups);
		for (i = j = 0; i < ngroups; i++)
		{
			for (k = 0; k < j && groups[k] != groups[i]; k++);
			if (k >= j) groups[j++] = groups[i];
		}
		ngroups = j;
	}
	if (name)
	{
		flags |= X_FLAG;
		if (!(flags & N_FLAG) || (flags & (G_FLAG|GG_FLAG)))
		{
			if (!(pw = getpwnam(name)))
			{
				user = strtol(name, &s, 0);
				if (*s || !(pw = getpwuid(user)))
					error(ERROR_exit(1), gettxt(":227","%s: name not found"), name);
				name = pw->pw_name;
			}
			user = pw->pw_uid;
			group = pw->pw_gid;
		}
#if _lib_fsid
		if (!(flags & N_FLAG) || (flags & S_FLAG))
		{
			setfsgent();
			do
                        {
                                if (!(fs = getfsgnam(name)))
                                        error(ERROR_exit(1), gettxt(":276","%u: fss name not found"), name);
                        } while (isfsg(fs));
                        fs_id = fs->fs_id;
		}
#endif
	}
	else
	{
		if (flags & G_FLAG)
			group = (flags & R_FLAG) ? getgid() : getegid();
		if (flags & (GG_FLAG|N_FLAG|U_FLAG))
			user = (flags & R_FLAG) ? getuid() : geteuid();
#if _lib_fsid
		if (flags & S_FLAG)
			fs_id = fsid(0);
#endif
		if (flags & N_FLAG)
			name = (pw = getpwuid(user)) ? pw->pw_name : (char*)0;
	}
	if (ngroups == 1 && groups[0] == group)
		ngroups = 0;
	if ((flags & N_FLAG) && (flags & G_FLAG))
		gname = (grp = getgrgid(group)) ? grp->gr_name : (char*)0;
#if _lib_fsid
	if ((flags & N_FLAG) && (flags & S_FLAG))
	{
		setfsgent();
		fs_name = (fs = getfsgid(fs_id)) ? fs->fs_grp : (char*)0;
	}
#endif
	if ((flags & (U_FLAG|G_FLAG|S_FLAG)) == (U_FLAG|G_FLAG|S_FLAG))
	{
		putid(sp, flags, "uid", name, user);
		putid(sp, flags, " gid", gname, group);
		if ((flags & X_FLAG) && name)
		{
#if _lib_getgrent
#if _lib_setgrent
			setgrent();
#endif
			lastchar = '=';
			while (grp = getgrent())
				if (p = grp->gr_mem)
					while (s = *p++)
						if (streq(s, name))
						{
							if (lastchar == '=')
								sfputr(sp, " groups", -1);
							sfputc(sp, lastchar);
							lastchar = ',';
							if (flags & O_FLAG)
								sfprintf(sp, "%s", grp->gr_name);
							else sfprintf(sp, "%u(%s)", grp->gr_gid, grp->gr_name);
						}
#if _lib_endgrent
			endgrent();
#endif
#endif
#if _lib_fsid
			getfsids(sp, name, flags, '=');
#endif
		}
		else
		{
			if ((euid = geteuid()) != user)
				putid(sp, flags, " euid", (pw = getpwuid(euid)) ? pw->pw_name : (char*)0, euid);
			if ((egid = getegid()) != group)
				putid(sp, flags, " egid", (grp = getgrgid(egid)) ? grp->gr_name : (char*)0, egid);
			if (ngroups > 0)
			{
				sfputr(sp, " groups", -1);
				lastchar = '=';
				for (i = 0; i < ngroups; i++)
				{
					group = groups[i];
					sfputc(sp, lastchar);
					if (grp = getgrgid(group))
					{
						if (flags & O_FLAG) sfprintf(sp, "%s", grp->gr_name);
						else sfprintf(sp, "%u(%s)", group, grp->gr_name);
					}
					else sfprintf(sp, "%u", group);
					lastchar = ',';
				}
			}
#if _lib_fsid
			putid(sp, flags, " fsid", fs_name, fs_id);
#endif
		}
		sfputc(sp,'\n');
		return(0);
	}
	if (flags & U_FLAG)
	{
		if ((flags & N_FLAG) && name) sfputr(sp, name, '\n');
		else sfprintf(sp, "%u\n", user);
	}
	else if (flags & G_FLAG)
	{
		if ((flags & N_FLAG) && gname) sfputr(sp, gname, '\n');
		else sfprintf(sp, "%u\n", group);
	}
	else if (flags & GG_FLAG)
	{
		if ((flags & X_FLAG) && name)
		{
#if _lib_getgrent
#if _lib_setgrent
			setgrent();
#endif
			i = 0;
			while (grp = getgrent())
				if (p = grp->gr_mem)
					while (s = *p++)
						if (streq(s, name))
						{
							if (i++) sfputc(sp, ' ');
							if (flags & N_FLAG) sfprintf(sp, "%s", grp->gr_name);
							else sfprintf(sp, "%u", grp->gr_gid);
						}
#if _lib_endgrent
			endgrent();
#endif
			if (i) sfputc(sp, '\n');
#endif
		}
		else if (ngroups > 0)
		{
			for (i = 0;;)
			{
				group = groups[i];
				if ((flags & N_FLAG) && (grp = getgrgid(group)))
					sfprintf(sp, "%s", grp->gr_name);
				else sfprintf(sp, "%u", group);
				if (++i >= ngroups) break;
				sfputc(sp, ' ');
			}
			sfputc(sp, '\n');
		}
	}
#if _lib_fsid
	else if (flags & S_FLAG)
	{
		if ((flags & X_FLAG) && name) getfsids(sp, name, flags, 0);
		else if ((flags & N_FLAG) && fs_name) sfputr(sp, fs_name, '\n');
		else sfprintf(sp, "%u\n", fs_id);
	}
#endif
	return(0);
}

int
b_id(int argc, char *argv[])
{
	register int	flags = 0;
	register int	n;

	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, "[Ggus]nr [user]")) switch (n)
	{
	case 'G':
		flags |= GG_FLAG;
		break;
	case 'g':
		flags |= G_FLAG;
		break;
	case 'n':
		flags |= N_FLAG;
		break;
	case 'r':
		flags |= R_FLAG;
		break;
	case 's':
		flags |= S_FLAG;
		break;
	case 'u':
		flags |= U_FLAG;
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
	n = (flags & (GG_FLAG|G_FLAG|S_FLAG|U_FLAG));
	if (!power2(n))
		error(2, gettxt(":298","incompatible options selected"));
	if (error_info.errors || argc > 1)
		error(ERROR_usage(2), optusage(NiL));
	if (!(flags & ~N_FLAG))
	{
		if (flags & N_FLAG) flags |= O_FLAG;
		flags |= (U_FLAG|G_FLAG|N_FLAG|R_FLAG|S_FLAG|GG_FLAG);
	}
	error_info.errors = getids(sfstdout, *argv, flags);
	return(error_info.errors);
}
