
/*
 *
 *  Written for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 */

#ident	"@(#)dosfs.cmds:mount.c	1.2"

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<limits.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<pwd.h>
#include	<grp.h>
#include	<time.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<sys/mnttab.h>
#include	<sys/mount.h>
#include	<sys/types.h>
#include	<sys/signal.h>
#include	<sys/fs/bpb.h>
#include	<sys/fs/dosfs_filsys.h>

#define	TIMAX	16

#define WRONG	0
#define RDONLY	1
#define RDWR	2
#define REMOUNT	4
#define USER	8
#define GROUP	16
#define MASK	32


char	typename[NAME_MAX], *progname;
char*	fstype   = "dosfs";
char*	mounttab = "/etc/mnttab";
char	temp[30];

struct optarg {

	int		type;
	int		subarg;
	char*	string;

} optargs[] = {

		{ RDONLY,	0,	"ro"	  },
		{ RDWR,		0,	"rw"	  },
		{ REMOUNT,	0,	"remount" },
		{ USER,		1,	"user"    },
		{ GROUP,	1,	"group"   },
		{ MASK,		1,	"mask"    },
		{ WRONG,	0,	NULL	  }};


dosfsmnt_args_t	dosfs_margs = { 
	DEFUSR, DEFGRP, 0 };
dosfsmnt_args_t	*dosfs_mptr;


uid_t
getuserid (string)

char*	string;
{
	char*			end;
	uid_t			uid;
	struct passwd*	pwdbuf;

	uid = (uid_t) strtol (string, &end, 0);

	if (*end == '\0' && uid > 0 && uid < UID_MAX)
	{
		return uid;
	}

	pwdbuf = getpwnam (string);

	if (pwdbuf == NULL)
	{
		pfmt(stderr, MM_ACTION,
			":127:`%s' not an existing user\n", string);
		exit    (1);
	}

	return pwdbuf->pw_uid;
}


gid_t
getgroupid (string)

char*	string;
{
	char*			end;
	gid_t			gid;
	struct group*	grpbuf;

	gid = (gid_t) strtol (string, &end, 0);

	if (*end == '\0' && gid > 0 && gid < UID_MAX)
	{
		return gid;
	}

	grpbuf = getgrnam (string);

	if (grpbuf == NULL)
	{
		pfmt(stderr, MM_ACTION,
			":128:`%s' not an existing group\n", string);
		exit    (1);
	}

	return grpbuf->gr_gid;
}


main (argc, argv)

int	argc;
char**	argv;
{
	unsigned		typeflag;
	int				flags;
	char*			special;
	char*			mpoint;
	char			timbuf[TIMAX];
	char*			ptr;
	uid_t			uid;
	gid_t			gid;
	int				mask;
	FILE*			fp;
	FILE*			ftp;
	struct optarg*	argptr;
	struct mnttab	mtab;
	char			label[NAME_MAX];


	typeflag = 0;
	flags    = 0;
	special  = NULL;
	mpoint   = NULL;
	ftp      = NULL;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmount");

	progname = strrchr (argv[0], '/');

	if (progname)
	{
		progname++;
	}
	else
	{
		progname = argv[0];
	}
	sprintf(typename, "%s %s", fstype, progname);
	strcpy(label,"UX:");
	strcat(label,typename);
	(void)setlabel(label);

	while (ptr = *++argv)
	{
		switch (*ptr)  {

		case '-':
			if (!strcmp(ptr, "-r"))
			{
				if (typeflag & RDWR)
				{
					conflict();
				}

				typeflag |= RDONLY;
			}
			else if (ptr[1] == 'o')
			{
				char*	eql;
				char*	subarg;
				char*	nextp;

				if (ptr[2] == '\0')
				{
					if ((ptr = *++argv) == NULL)
					{
						usage();
					}
				}
				else
				{
					ptr = &ptr[2];
				}

				do {

					if (nextp = strchr (ptr, ','))
					{
						*nextp = '\0';
					}

					for (argptr=optargs; argptr->type != WRONG; argptr++)
					{
						if (argptr->subarg)
						{
							eql = strchr (ptr, '=');

							if (eql == NULL)
							{
								continue;
							}

							*eql   = '\0';
							subarg = eql + 1;
						}

						if (!strcmp(argptr->string, ptr))
							break;

						if (argptr->subarg)
						{
							*eql = '=';
						}
					}

					switch (argptr->type)  {

					case WRONG:	
						usage();
						break;

					case RDONLY:	
						if (typeflag & RDWR)
						{
							conflict();
						}

						typeflag |= RDONLY;
						break;

					case RDWR:		
						if (typeflag & RDONLY)
						{
							conflict();
						}

						typeflag |= RDWR;
						break;

					case REMOUNT:	
						typeflag |= REMOUNT;
						break;

					case USER:    	
						uid = getuserid(subarg);

						typeflag |= argptr->type;
						break;

					case GROUP:		
						gid = getgroupid(subarg);

						typeflag |= argptr->type;
						break;

					case MASK:		
						mask = strtol(subarg, NULL, 0) & 0777;

						typeflag |= argptr->type;
						break;
					}

					ptr = nextp + 1;

				} while (nextp != NULL);
			}
			else
			{
				usage();
			}

			break;

		default:
			if (special)
			{
				if (mpoint)
				{
					usage();
				}
				else
				{
					mpoint = ptr;
				}
			}
			else
			{
				special = ptr;
			}

			break;
		}
	}

	if (mpoint == NULL)
	{
		usage();
	}

	/*
	Everything okay now; copy the flags
	suitable for the mount systemcall.
	*/

	if (typeflag & RDONLY )   flags |= MS_RDONLY;
	if (typeflag & REMOUNT)   flags |= MS_REMOUNT;

	if ( typeflag & (USER|GROUP|MASK) )
	{
		if (typeflag & REMOUNT)
		{
			conflict();
		}

		if (typeflag & USER )	dosfs_margs.uid  = uid;
		if (typeflag & GROUP)	dosfs_margs.gid  = gid;
		if (typeflag & MASK )	dosfs_margs.mask = mask;

		dosfs_mptr = &dosfs_margs;
	}
	else
	{
		dosfs_mptr = NULL;
	}

	/*
	Make a new mount table entry.
	*/

	mtab.mnt_special = special;
	mtab.mnt_mountp  = mpoint;
	mtab.mnt_fstype  = fstype;

	if (flags & MS_RDONLY)
	{
		mtab.mnt_mntopts = "ro";
	}
	else
	{
		mtab.mnt_mntopts = "rw";
	}

	sprintf (timbuf, "%ld", time(0));

	mtab.mnt_time = timbuf;

	/*
	Update the mount table file, do it
	locked to prevent concurrent updates.
	*/

	fp = fopen (mounttab, "r+");

	if (flags & MS_REMOUNT)
	{
		sprintf (temp, "%s.%ld", mounttab, getpid());

		ftp = fopen (temp, "w");
	}

	if ( fp == NULL			   ||
	    lockf (fileno(fp), F_LOCK, 0) < 0  ||
	    fseek (fp, 0, 2)		   ||
	    (flags & MS_REMOUNT) && ftp == NULL	)
	{
		if (fp  != NULL)	fclose (fp );

		if (ftp != NULL)
		{
			unlink (temp);
			fclose (ftp );
		}

		pfmt(stderr, MM_ERROR,
			":129:cannot update %s\n", mounttab);
		exit    (3);
	}

	/*
	Don't get interrupted while mounting
	and updating the mount table.
	*/

	signal (SIGHUP,  SIG_IGN);
	signal (SIGINT,  SIG_IGN);
	signal (SIGQUIT, SIG_IGN);

	if ( !doit (special, mpoint, flags, dosfs_mptr) )
	{
		if (fp  != NULL)	fclose (fp );

		if (ftp != NULL)
		{
			unlink (temp);
			fclose (ftp );
		}

		exit (2);
	}

	/*
	In case of a remount we need to update
	mount table, we just create a scratch
	file, copy every entry save the one we
	need to update and append the new entry.
	*/

	if (flags & MS_REMOUNT)
	{
		struct mnttab	mget;
		int		stat;

		rewind (fp);

		while ((stat = getmntent(fp, &mget)) != -1)
		{
			if (stat == 0 &&
			    strcmp (mtab.mnt_special, mget.mnt_special))
			{
				putmntent (ftp, &mget);
			}
		}

		putmntent (ftp, &mtab);
		fclose (ftp);
		rename (temp, mounttab);
	}
	else
	{
		putmntent (fp, &mtab);
	}

	fclose (fp);

	return (0);

}


usage()
{
	pfmt(stderr, MM_ACTION,
		":130:Usage: %s [-r] special mount-point\n", progname);
	pfmt(stderr, MM_ACTION,
	    ":131:\t\t[-o {mask=<mask>}|user={user|uid}|group={group|gid}}]\n");
	pfmt(stderr, MM_ACTION,
		":132:       %s [-o remount] special mount-point\n", progname);
	exit    (1);
}


conflict()
{
	pfmt(stderr, MM_ACTION,
		":17:conflicting suboptions\n");
	usage();
}


doit (special, mpoint, flags, dosfs_mptr)

char*			special;
char*			mpoint;
int				flags;
dosfsmnt_args_t*	dosfs_mptr;
{
	int	datalen;

	datalen  = (dosfs_mptr == NULL) ? 0 : sizeof *dosfs_mptr;
	flags   |= MS_DATA;

	if ( mount (special, mpoint, flags, fstype, dosfs_mptr, datalen) )
	{
		error  (special, mpoint);
		return (0);
	}

	return (1);
}


error (special, mpoint)

char*	special;
char*	mpoint;
{
	switch (errno) {

	case ENXIO:
		pfmt(stderr, MM_ACTION,
			":5:%s no such device\n", special);
		break;

	case ENOTDIR:
	   pfmt(stderr, MM_ACTION,
	   ":6:%s not a directory\n\tor a component of %s is not a directory\n",
	   mpoint, special);
		break;

	case EPERM:
		pfmt(stderr, MM_ACTION,
			":20:permission denied\n");
		break;

	case ENOTBLK:
		pfmt(stderr, MM_ERROR,
			":10:%s not a block device\n", special);
		break;

	case ENOENT:
		pfmt(stderr, MM_ERROR,
			":21:%s or %s, no such file or directory or no previous mount was performed\n",
			special, mpoint);
		break;

	case EINVAL:
		pfmt(stderr, MM_ERROR,
			":22:%s is not an %s file system,\n\tor %s is busy.\n",
			special, fstype, mpoint);
		break;

	case EBUSY:
		pfmt(stderr, MM_ERROR,
			":9:%s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded\n",
			special, mpoint);
		break;

	case EROFS:
		pfmt(stderr, MM_ERROR,
			":11:%s write-protected\n", special);
		break;

	case ENOSPC:
		pfmt(stderr, MM_ERROR,
			":23:%s is corrupted. Needs checking\n", special);
		break;

	case ENODEV:
		pfmt(stderr, MM_ERROR,
			":24:%s no such device or device is write-protected\n",
			special);

	default:
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		pfmt(stderr, MM_ERROR,
			":15:cannot mount %s\n", special);
	}
}
