#ident	"@(#)dosfs.cmds:fstyp.c	1.2"

#include	<errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/fs/bootsect.h>

extern int	optind;

/*
 * Examine the specified device to see if it
 * contains a recognizable DOS file system.
 */
main(argc,argv)
int	argc;
char	*argv[];
{
	boolean_t	error;
	char		*devnode;
	int		dev;
	char		sector0[512];
	union bootsector	*bsp;
	int ret;


	(void)setlocale(LC_ALL,"");
	(void)setcat("uxfstyp");

	(void)setlabel("UX:dosfs fstyp");

	/*
	 * Process the command-line arguments.
	 */
	error = B_FALSE;

	/*
	 * There should be exactly one more argument which
	 * specifies the device to be examined.
	 */
	if (optind == argc-1) {
		devnode = argv[optind];

	} else if (optind < argc-1) {
		error = B_TRUE;

	} else if (optind > argc-1) {
		error = B_TRUE;
	}

	if (error == B_TRUE) {
		pfmt(stderr, MM_ACTION, ":1:Usage: %s special\n", "fstyp");
		exit(1);
	}

	/*
	 * Open the device.
	 */
	dev = open(devnode, O_RDONLY);
	if (dev < 0) {
		pfmt(stderr, MM_ERROR, ":2:Unable to open device: %s\n",
			devnode);
		exit(1);
	}

	/*
	Read the boot sector of the filesystem, and then
	check the boot signature.  If not a dos boot sector
	then error out.
	*/

	ret = read(dev, sector0, 512);
	if (ret != 512) {
		pfmt(stderr, MM_ERROR, ":3:Unable to access media\n");
		(void)close(dev);
		exit(1);
	}

	bsp = (union bootsector *)sector0;

	if (bsp->bs50.bsBootSectSig != BOOTSIG)
	{
		(void)close(dev);
		exit(1);
	}

	pfmt(stdout, MM_NOSTD, ":4:%s\n", "dosfs");
	(void)close(dev);
	exit(0);
}

