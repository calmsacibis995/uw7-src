/*		copyright	"%c%" 	*/

#ident	"@(#)pt_chmod.c	1.2"
#ident  "$Header$"

#include <string.h>
#include <stdio.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mac.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>

#define DEFAULT_TTY_GROUP	"tty"

gid_t	gid;

/*
 * change the owner and mode of the pseudo terminal slave device.
 */
ch_func(cache_ptsname)
char *cache_ptsname;
{
	/* change slave side owner, modes, and MAC level */
	if ( chown( cache_ptsname, getuid(), gid))
		exit( -1);

	if ( chmod( cache_ptsname, 00620))
		exit( -1);
}

main( argc, argv)
int	argc;
char	**argv;
{
	int	fd;

	struct	group	*gr_name_ptr;
	char	*ptsname(), *cache_ptsname;
	extern int errno;
	level_t	proclevel,tmplevel;
	struct devstat attrib, slaveatrib;
	struct stat path_status;
	struct stat fd_status;

	char *ptmx_hilevel, *ptmx_lolevel,
		*devattr(), *strchr(), *strnchr();

	if (( gr_name_ptr = getgrnam( DEFAULT_TTY_GROUP)) != NULL)
		gid = gr_name_ptr->gr_gid;
	else
		gid = getgid();

	fd = atoi( argv[1]);

	if ( NULL==(cache_ptsname=ptsname( fd)))
		exit( -2);

	/* ptmx is an old-style clonable device.  Verify the
	 * (minor number to /dev/ptmx in the file system)
	 * equals the (major number of the file descriptor)
	 */
	if (( stat( "/dev/ptmx", &path_status) < 0 ) ||
	    ( fstat( fd, &fd_status) < 0 ) ||
	    ( minor(path_status.st_rdev) != major(fd_status.st_rdev)) ) 
		exit( -3);

	/* get MAC level - if not installed, just chmod/chown here */
	if (lvlproc(MAC_GET, &proclevel) != 0){
		/* if the error is not that MAC is not running, bail out */
		if (errno != ENOPKG)
			exit(-4);
		/* otherwise, do the chmod/chown */
		ch_func(cache_ptsname);
	} else {
		/* get and check master device attributes */
		if (fdevstat(fd, DEV_GET, &attrib) != 0)
			exit(-5);

		/* These checks are to preserve compatibility with SVR4
		 * pseudo-tty conventions
		 */
		if ((DEV_LASTCLOSE != attrib.dev_relflag) ||
		    (DEV_PRIVATE != attrib.dev_state) ||
			/* check device range&level for envelopment */
		    lvlvalid(&attrib.dev_lolevel) ||
		    lvlvalid(&attrib.dev_hilevel) ||
		    (lvldom(&attrib.dev_hilevel, &proclevel) <= 0) ||
		    (lvldom(&proclevel, &attrib.dev_lolevel) <= 0) ||
		    (devstat(cache_ptsname, DEV_GET, &slaveatrib) != 0) ||
		    (0 != slaveatrib.dev_usecount))
			exit(-6);

		/* We have verified the master.
		 * Work with the slave before the master is changed.
		 * When we set the master's state, it will be PUBLIC.
		 */
		attrib.dev_state = DEV_PUBLIC;

		/* The level on the pseudo device must not conflict with
		 * the range we are about to set.  We will delete the
		 * structure with the DEV_SYSTEM flag first.
		 */
		slaveatrib = attrib;
		slaveatrib.dev_relflag = DEV_SYSTEM;
		if (devstat(cache_ptsname, DEV_SET, &slaveatrib) != 0) 
			exit(-7);

		/* assign the owner, modes, level, and devstat attributes */
		ch_func(cache_ptsname);

		if (lvlfile(cache_ptsname, MAC_SET, &proclevel) != 0) {
			/* don't fail if it is already correct level */
			if ((lvlfile(cache_ptsname,MAC_GET,&tmplevel)!=0)||
			    (tmplevel != proclevel))
				exit(-12);
		}

		if (devstat(cache_ptsname, DEV_SET, &attrib) != 0)
			exit(-9);

		/* change the level of the master fd */
		if (flvlfile(fd, MAC_SET, &proclevel) != 0) {
			/* don't fail if it is already correct level */
			if ((lvlfile(cache_ptsname,MAC_GET,&tmplevel)!=0)||
			    (tmplevel != proclevel))
				exit(-13);
		}

		/* change the state of the master fd to public */
		if (fdevstat(fd, DEV_SET, &attrib) != 0)
			exit(-11);
	}

	exit( 0);
}
