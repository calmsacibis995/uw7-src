/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/switchout.c	1.19.6.1"
#ident "$Header$"

#include	<stdio.h>
#include 	<limits.h>
#include	<sys/fstyp.h>  
#include	<sys/errno.h>  
#include	<sys/vfstab.h>

#define	FSTYPE_MAX	8
#define	FULLPATH_MAX	64
#define	ARGV_MAX	1024
#define	VFS_PATH	"/usr/lib/fs"
#define	ALT_PATH	"/etc/fs"

extern	int errno;
char	*special = NULL;  /*  device special name  */
char	*fstype = NULL;	  /*  fstype name is filled in here  */
char	*basename;	  /* name of command */
char	*newargv[ARGV_MAX]; 	/* args for the fstype specific command  */
char	vfstab[] = VFSTAB;
int	newargc = 2;

struct commands {
	char *c_basename;
	char *c_optstr;
	char *c_usgstr;
} cmd_data[] = {
	"mkfs", "F:o:mb:?V", 
		"[-F FSType] [-V] [-m] [current_options] [-o specific_options] special [operands]",
	"dcopy", "F:o:s:a:f:?dvV", 
		"[-F FSType] [-V] [current_options] [-o specific_options] inputfs outputfs",
	"fsdb", "F:o:z:?V", 
		"[-F FSType] [-V] [current_options] [-o specific_options] special",
	"labelit", "F:o:?nV",
		"[-F FSType] [-V] [current_options] [-o specific_options] special [operands]",
	"edquota", "F:o:p:t?V",
		"[-F FSType] [-V] [current_options] username ...]",
	"quot", "F:o:?Vacfhnv",
		"[-F FSTYPE] [-V] [current_options] [filesystems ...]",
	"quota", "F:o:v?V",
		"[-F FSType] [-V] [current_options] [username]",
	"quotacheck", "F:o:avp?V",
		"[-F, FSType] [-V] [current_options] [filesystem ...]",
	"quotaon", "F:o:av?V",
		"[-F FSType] [-V] [current_options] filesystem ...",
	"quotaoff", "F:o:av?V",
		"[-F FSType] [-V] [current_options] filesystem ...",
	"repquota", "F:o:av?V",
		"[-F FSType] [-V] [current_options] [filesystem ...]",	
	NULL, "F:o:?V",
	 	"[-F FSType] [-V] [-o specific_options] special [operands]"
	};
struct 	commands *c_ptr;

/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage */
#define	RET_EXEC	2	/* cannot execute <file system dependent path> */

#define	RET_FS_MORE	3	/* more than one FSType specified */
#define	RET_FS_LONG	4	/* FSType exceeds max characters */
#define	RET_FS_UNK	5	/* FSType cannot be identified */
#define	RET_FS_SET	6	/* FSType not installed in the kernel */
#define	RET_FS_OPER	7	/* operation not applicable for FSType */

#define	RET_DEV_UNK	8	/* special device not specified */

#define	RET_VFS_OPEN	9	/* cannot open vfstab */
#define	RET_VFS_LONG	10	/* line in vfstab exceeds max characters */
#define	RET_VFS_FEW	11	/* line in vfstab has too few entries */

main(argc, argv)
int	argc;
char	*argv[];
{
	register char *ptr;
	char	full_path[FULLPATH_MAX];
	char	*vfs_path = VFS_PATH;
	char	*alt_path = ALT_PATH;
	int	i; 
        int	verbose = 0;		/* set if -V is specified */
	int	F_flg = 0;
	int	mflag = 0;
	int	usgflag = 0; 
	int	nospecial = 0;		/* unset if special isn't needed */
	int	arg;			/* argument from getopt() */
	extern	char *optarg;		/* getopt specific */
	extern	int optind;
	extern	int opterr;

	basename = ptr = argv[0];
	while (*ptr) {
		if (*ptr++ == '/')
			basename = ptr;
	}
	if (argc == 1) {
		for (c_ptr = cmd_data; ((c_ptr->c_basename != NULL) && (strcmp(c_ptr->c_basename, basename) != 0));  c_ptr++) 
		;
		usage(basename, c_ptr->c_usgstr);
		exit(RET_USAGE);
	}

	for (c_ptr = cmd_data; ((c_ptr->c_basename != NULL) && (strcmp(c_ptr->c_basename, basename) != 0));  c_ptr++) 
		; 
	while ((arg = getopt(argc,argv,c_ptr->c_optstr)) != -1) {
			switch(arg) {
			case 'V':	/* echo complete command line */
				verbose = 1;
				break;
			case 'F':	/* FSType specified */
				F_flg++;
				fstype = optarg;
				break;
			case 'o':	/* FSType specific arguments */
				newargv[newargc++] = "-o";
				newargv[newargc++] = optarg; 
				break;
			case '?':	/* print usage message */
				newargv[newargc++] = "-?";
				usgflag = 1;
				break;
			case 'm':	/* FSType specific arguments */
				mflag=1;
				newargv[newargc] = (char *)malloc(3);
				sprintf(newargv[newargc++], "-%c", arg);
				if (optarg)
					newargv[newargc++] = optarg;
				break;
			default:
				newargv[newargc] = (char *)malloc(3);
				sprintf(newargv[newargc++], "-%c", arg);
				if (optarg)
					newargv[newargc++] = optarg;
				break;
			}
			optarg = NULL;
	}
	if (F_flg > 1) {
		fprintf(stderr, "%s: more than one FSType specified\n",
			basename);
		usage(basename, c_ptr->c_usgstr);
		exit(RET_FS_MORE);
	}
	if (fstype != NULL) {
		if (strlen(fstype) > FSTYPE_MAX) {
			fprintf(stderr, "%s: FSType %s exceeds %d characters\n",
				basename, fstype, FSTYPE_MAX);
			exit(RET_FS_LONG);
		}
	}

	/*  perform a lookup if fstype is not specified  */
	special = argv[optind];
	optind++;
	if (special == NULL) {
		/* These commands (edquota|quota||quot|repquota) can have
		 * no special/arguments	
		 */
		if (strncmp(basename,"edquota",7) == 0 ||
			strncmp(basename, "quot", 4) == 0 ||
			strncmp(basename, "quota", 5) == 0 ||
			strncmp(basename, "repquota", 8) == 0) {
			nospecial = 1;
		}
	}
		
	if ((special == NULL) && (!usgflag) && (nospecial == 0)) {
		fprintf(stderr,"%s: special not specified\n", basename);	
		usage(basename, c_ptr->c_usgstr);
		exit(RET_DEV_UNK);
	}
	if ( (fstype == NULL)  && (usgflag)) {
		usage(basename, c_ptr->c_usgstr);
		exit(RET_USAGE);
	}
	if (fstype == NULL && nospecial == 0) 
		lookup();
	if (fstype == NULL) {
		fprintf(stderr, "%s: FSType cannot be identified\n",
			basename);
		usage(basename, c_ptr->c_usgstr);
		exit(RET_FS_UNK);
	}
	if (nospecial == 0)
		newargv[newargc++] = special;
	for (; optind < argc; optind++) 
		newargv[newargc++] = argv[optind];

	/*  build the full pathname of the fstype dependent command  */
	sprintf(full_path, "%s/%s/%s",vfs_path, fstype, basename);

	newargv[1] = &full_path[FULLPATH_MAX];
	while (*newargv[1]-- != '/');
	newargv[1] += 2;

	if (verbose) {
		printf("%s -F %s ", basename, fstype);
		for (i = 2; newargv[i]; i++)
			printf("%s ", newargv[i]);
		printf("\n");
		exit(RET_OK);
	}

	/*
	 *  Execute the FSType specific command.
	 */
		if (!strncmp(basename,"mkfs",4)) {
			if ((!access(full_path,2)) && (!mflag) && (!usgflag)) {
				fprintf(stdout,"Mkfs: make %s file system? \n(DEL if wrong)\n",fstype);
				fflush(stdout);
				sleep(10);	/* 10 seconds to DEL */
			}
		}
	execv(full_path, &newargv[1]);
	if ((errno == ENOENT) || (errno == EACCES)) {
		/*  build the alternate pathname */
		sprintf(full_path, "%s/%s/%s",alt_path, fstype, basename);
		if (verbose) {
			printf("%s -F %s ", basename, fstype);
			for (i = 2; newargv[i]; i++)
				printf("%s ", newargv[i]);
			printf("\n");
			exit(RET_OK);
		}
		if (!strncmp(basename,"mkfs",4)) {
			if ((!access(full_path,2)) && (!mflag) && (!usgflag)) {
				fprintf(stdout,"Mkfs: make %s file system? \n(DEL if wrong)\n",fstype);
				fflush(stdout);
				sleep(10);	/* 10 seconds to DEL */
			}
		}
		execv(full_path, &newargv[1]);
	}
	if (errno == ENOEXEC) {
		if (!strncmp(basename,"mkfs",4)) {
			if ((!access(full_path,2)) && (!mflag) && (!usgflag)) {
				fprintf(stdout,"Mkfs: make %s file system? \n(DEL if wrong)\n",fstype);
				fflush(stdout);
				sleep(10);	/* 10 seconds to DEL */
			}
		}
		newargv[0] = "sh";
		newargv[1] = full_path;
		execv("/sbin/sh", &newargv[0]);
	}
	if (errno != ENOENT) {
		perror(basename);
		fprintf(stderr, "%s: cannot execute %s\n", basename, full_path);
		exit(RET_EXEC);
	}

	if (sysfs(GETFSIND, fstype) == (-1)) {
		fprintf(stderr, "%s: FSType %s not installed in the kernel\n",
			basename, fstype);
		exit(RET_FS_SET);
	}
	fprintf(stderr, "%s: operation not applicable for FSType %s \n",
		basename, fstype);
	exit(RET_FS_OPER);
}

usage (cmd, usg)
char *cmd, *usg;
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "%s %s\n", cmd, usg);
}


/*
 *  This looks up the /etc/vfstab entry given the device 'special'.
 *  It is called when the fstype is not specified on the command line.
 *
 *  The following global variables are used:
 *	special, fstype
 */

lookup()
{
	FILE	*fd;
	int	ret;
	struct vfstab	vget, vref;

	if ((fd = fopen(vfstab, "r")) == NULL) {
		fprintf (stderr, "%s: cannot open vfstab\n", basename);
		exit(RET_VFS_OPEN);
	}
	vfsnull(&vref);
	vref.vfs_special = special;
	ret = getvfsany(fd, &vget, &vref);
	if (ret == -1) {
		rewind(fd);
		vfsnull(&vref);
		vref.vfs_fsckdev = special;
		ret = getvfsany(fd, &vget, &vref);
	}
	fclose(fd);

	switch (ret) {
	case -1:
		fprintf(stderr, "%s: FSType cannot be identified\n",
			basename);
		exit(RET_FS_UNK);
		break;
	case 0:
		fstype = vget.vfs_fstype;
		break;
	case VFS_TOOLONG:
		fprintf(stderr, "%s: line in vfstab exceeds %d characters\n",
			basename, VFS_LINE_MAX-2);
		exit(RET_VFS_LONG);
		break;
	case VFS_TOOFEW:
		fprintf(stderr, "%s: line in vfstab has too few entries\n",
			basename);
		exit(RET_VFS_FEW);
		break;
	}
}
