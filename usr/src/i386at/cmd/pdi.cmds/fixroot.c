#ident	"@(#)pdi.cmds:fixroot.c	1.1"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/ksym.h>
#include <sys/mount.h>
#include <sys/fs/memfs.h>

#include "fixroot.h"

main(int argc, char **argv)
{
	int	i, fd, ret, mntflag;
	dev_t	nextdev;
	char	*special, *mountp, *fstype, fullname[512];
	struct mioc_rksym	ksym;
	struct memfs_args	margs;
#ifdef KSTUFF
	dev_t	standdev = NODEV, dumpdev = NODEV, swapdev = NODEV;
	int		standindex, swapindex;
#endif

	/*
	 * First, let's construct the memfs
	 */

	margs.swapmax = 4096;
	margs.rootmode = S_IRWXU|S_IRWXG|S_IRWXO;
	margs.sfp = 0;

	special = specials[1].dir;
	mountp = specials[1].dir;
	fstype = "memfs";
	mntflag = MS_DATA;

	ret = mount(special, mountp, mntflag, fstype, &margs, sizeof(struct memfs_args));

	if ( ret == -1 )
		exit(1);

	/*
	 * now determine the devs and write them.
	 */

	fd = open("/dev/kmem", O_RDONLY);

	if ( fd == -1 )
		exit(1);

	ksym.mirk_buf = &nextdev;
	ksym.mirk_buflen = sizeof(nextdev);

#ifdef KSTUFF
	standindex = -1;
#endif

	for (i = 0; i < (sizeof(specials) / sizeof(io_template_t)); i++) {

		ksym.mirk_symname = specials[i].global;
#ifdef KSTUFF
		if ( !strcmp( ksym.mirk_symname, "swapdev" ) ) {
			swapindex = i;
		}
		else if ( !strcmp( ksym.mirk_symname, "standdev" ) ) {
			standindex = i;
		}
#endif

		ret = ioctl(fd, MIOC_READKSYM, &ksym);

		if ( ret == -1 )
			continue;

#ifdef KSTUFF
		if ( !strcmp( ksym.mirk_symname, "swapdev" ) ) {
			swapdev = nextdev;
		}
		else if ( !strcmp( ksym.mirk_symname, "dumpdev" ) ) {
			dumpdev = nextdev;
		}
		else if ( !strcmp( ksym.mirk_symname, "rootdev" ) ) {
			standdev = nextdev + 9;
		}
#endif

		/*
		 * now we make the special files.
		 */

		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFBLK|S_IRUSR|S_IWUSR, nextdev);

		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/r%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFCHR|S_IRUSR|S_IWUSR, nextdev);
	}

	(void)close(fd);
#ifdef KSTUFF
	if ( swapdev == NODEV  && dumpdev != NODEV ) {
		i = swapindex;
		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFBLK|S_IRUSR|S_IWUSR, dumpdev);

		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/r%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFCHR|S_IRUSR|S_IWUSR, dumpdev);
	}
	if ( standindex != -1 ) {
		i = standindex;
		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFBLK|S_IRUSR|S_IWUSR, standdev);

		sprintf(fullname, "%s/%s", specials[i].dir, specials[i].device);
		mkdir(fullname, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
		sprintf(fullname, "%s/%s/r%s", specials[i].dir, specials[i].device, specials[i].node);
		unlink(fullname);
		mknod(fullname, S_IFCHR|S_IRUSR|S_IWUSR, standdev);
	}
#endif
}
