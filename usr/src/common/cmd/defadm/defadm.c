/*		copyright	"%c%" 	*/

#ident	"@(#)defadm:defadm.c	1.7.2.2"
#ident  "$Header$"
/*
 * Command:	defadm
 *
 * Inheritable Privileges:	P_DACWRITE,P_MACREAD,P_MACWRITE,P_SETFLEVEL
 * Fixed Privileges:		none
 *
 * Level:	SYS_PRIVATE
 *
 * Files:	/etc/default/*
 *
 * Notes:	The defadm command is used to update or display various
 *		default values for several administrative commands that
 *		can be modified without recompiling the source code.
*/

/* LINTLIBRARY */
#include	<stdio.h>
#include	<deflt.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<fcntl.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<signal.h>
#include	<sys/mac.h>
#include	<locale.h>
#include	<pfmt.h>

#define	USAGE	":736:usage: defadm [ filename [name1[=value1]] [name2[=value2]] [...] ]\n       defadm [ -d filename name1 [name2] [...] ]\n"

extern	int	optind;
extern	char	*optarg;

extern	int	errno,
		getopt(),
		lvlfile();

static	void	def_exit();

static	int	defupdate(),
		display_dir(),
		display_file();

static	FILE	*defltfp;

static	int	dflg,
		errflg;

static	char	fn[MAXPATHLEN],
		filename[MAXNAMELEN];

static	const	char
	*noperm = ":737:permission denied\n",
	*noname = ":738:name %s does not exist\n",
	*unexpected = ":739:unexpected failure\n",
	*nofile_acc = ":740:cannot access file %s\n",
	*nofile_exist = ":741:file %s does not exist\n",
	*nodir_acc = ":742:cannot access directory %s\n",
	*nodir_exist = ":743:directory %s does not exist\n",
	*locked = ":744:file %s is locked, try again later\n",
	*no_option = ":745:invalid option\n";

/*
 * Procedure:	main
 *
 * Restrictions:
 *		stat(2):	P_MACREAD
 *		access(2):	P_DACWRITE;P_MACREAD;P_MACWRITE
 *		defopen:	P_MACREAD
*/
main(argc, argv)
	int argc;
	char **argv;
{
	int	c, i,
		ret = 0,
		rcode = 0;
	char	*ptr;
	struct	stat	dbuf;

	(void) setlocale(LC_ALL, "");
	setcat("uxcore");
	setlabel("UX:defadm");

	if (stat(DEFLT, &dbuf) < 0) {
		switch (errno) {

		case EACCES:
			(void) pfmt(stderr, MM_ERROR, nodir_acc, DEFLT);
			exit(3);
		/* FALLTHROUGH */
		case ENOENT:
		case ENOTDIR:
			(void) pfmt(stderr, MM_ERROR, nodir_exist, DEFLT);
			exit(2);
		default:
			(void) pfmt(stderr, MM_ERROR, unexpected);
			exit(1);
		}	
	}
	/*
	 * no ARGS given list files in DEFLT directory
	*/
	if (argc == 1)
		def_exit(display_dir());

	while ((c = getopt(argc,argv,"d")) != EOF) {
		switch(c) {
		case 'd':
			++dflg;
			break;
		case '?':
			++errflg;
			break;
		}
	}

	argv = &argv[optind];
	argc -= optind;

	filename[0] = fn[0] = '\0';

	(void) strcat(fn, DEFLT);
	(void) strcat(fn, (char *) "/");
	(void) strcat(fn, *argv);

	(void) strcpy(filename, *argv);

	if (access(fn, F_OK) == -1) {
		(void) pfmt(stderr, MM_ERROR, nofile_exist, filename);
		def_exit(4);
	}

	--argc;
	++argv;

	if (errflg || (dflg && !argc)) {
		(void) pfmt(stderr, MM_ERROR, no_option);
		(void) pfmt(stderr, MM_ACTION, USAGE);
		def_exit(1);
	}

	/*
	 * display contents of the given file
	*/
	if (!argc) {
		def_exit(display_file());
	}

	if ((defltfp = defopen(filename)) == NULL) {
		(void) pfmt(stderr, MM_ERROR, nofile_acc, filename);
		def_exit(5);
	}
	/*
	 * ignore all signals
	*/
	for (i = 1; i < NSIG; i++)
		(void) sigset (i, SIG_IGN);

	errno = 0;             /* For correcting sigset to SIGKILL */

	while (argc) {
		if (dflg) {
			if (ret = defupdate(DEF_DEL, filename, *argv)) {
				if (ret != 6) {
					def_exit(ret);
				}
				else {
					rcode = ret;
				}
			}
		}
		else if ((ptr = strchr(*argv, '=')) == NULL) {
			if ((ptr = defread(defltfp, *argv)) != NULL) {
				(void) printf("%s=%s\n", *argv, ptr);
			}
			else {
				(void) pfmt(stderr, MM_WARNING, noname, *argv);
				ret = 6;
			}
		}
		else if (ret = defupdate(DEF_WRITE, filename, *argv)) {
			def_exit(ret);
		}
		--argc;
		++argv;
	}
	def_exit(rcode ? rcode : ret);
	/* NOTREACHED */
}


/*
 * Procedure:	defupdate
 *
 * Restrictions:
 *		stat(2):	None
 *		access(2):	None
 *		open(2):	None
 *		unlink(2):	None
 *		rewind:		None
 *		fclose:		None
 *		rename(2):	None
 *		lvlfile:	None
 *		chown(2):	None
 *		fopen:		None
 *
 * Notes:	called whenever an addition or deletion to a default
 *		file is made.  Requestor must have write access to the
 *		target file.
*/
static	int
defupdate(cmd, fname, defnamp)
	int	cmd;
	char	*fname,
		*defnamp;
{
	int	patlen,
		fildes,
		ret = 0,
		found = 0;
	FILE	*tmpfp = NULL;
	char	buf[256],
		tmpf[MAXPATHLEN],
		*tmpfl = &tmpf[0];
	struct	stat statbuf;

	tmpf[0] = '\0';
	(void) strcat(tmpfl, DEFLT);
	(void) strcat(tmpfl, "/.");
	(void) strcat(tmpfl, fname);

	patlen = strcspn(defnamp, "=");

	if (stat(fn, &statbuf) < 0) {
		(void) pfmt(stderr, MM_ERROR, unexpected);
		return 1;
	}
	/*
	 * check the write access of the target file by this process.
	*/
	if (access(fn, W_OK) != 0) {
		/*
		 * If the process doesn't have write access to the
		 * target file, disallow this update.  The reason
		 * for this is the principle of least privilege is
		 * enforced by only allowing users with the required
		 * access to update files in ``/etc/default''.
		*/
		(void) pfmt(stderr, MM_ERROR, noperm);
		return 1;
	}

	(void) umask(~(statbuf.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)));

	if ((fildes = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0444)) == -1) {
		(void) pfmt(stderr, MM_ERROR, locked, fname);
		return 7;
	}

	if ((tmpfp = fdopen(fildes, "w")) == NULL) {
		(void) close(fildes);
		(void) unlink(tmpfl);
		(void) pfmt(stderr, MM_ERROR, unexpected);
		return 1;
	}

	rewind(defltfp);

	while (fgets(buf, sizeof(buf), defltfp)) {
		if ((strncmp(defnamp, buf, patlen) == 0) 
			&& (buf[patlen] == '=')) {
			found++;
			if (cmd == DEF_WRITE) {
				(void) fputs(defnamp, tmpfp);
				(void) fputs("\n", tmpfp);
				continue;
			}
			else {
				continue;
			}
		}
		(void) fputs(buf, tmpfp);
	}

	if (!found) {
		if (cmd == DEF_WRITE) {
			(void) fputs(defnamp, tmpfp);
			(void) fputs("\n", tmpfp);
		}
		else {
			(void) close(fildes);
			(void) unlink(tmpfl);
			(void) pfmt(stderr, MM_WARNING, noname, defnamp);
			return 6;
		}
	}

	if (fflush(tmpfp) || close(fildes) || fclose(defltfp)) { 
		(void) unlink(tmpfl);
		(void) pfmt(stderr, MM_ERROR, unexpected);
		return 1;
	}

	if ((ret = rename(tmpfl, fn)) == -1) {
		(void) pfmt(stderr, MM_ERROR, unexpected);
		(void) unlink(tmpfl);
	}

	(void) lvlfile(fn, MAC_SET, &statbuf.st_level);

	(void) chown(fn, statbuf.st_uid, statbuf.st_gid); 
	/*
	 * re-open the file just updated for "read" only.
	*/
	defltfp = fopen(fn, "r");

	return ret;
}


/*
 * Procedure:	display_dir
 *
 * Restrictions:
 *		opendir:	None
 *		stat(2):	None
 *
 * Notes:	used when the contents of the directory DEFLT
 *		are displayed.
*/
static	int
display_dir()
{
	DIR	*dirp;
	char	tfile[MAXPATHLEN],
		*tfilep = &tfile[0];
	struct	stat	statbuf;
	struct	dirent *direntp;
	
	errno = 0;
        if ((dirp = opendir(DEFLT)) == NULL) {
		switch (errno) {
		case EACCES:
			(void) pfmt(stderr, MM_ERROR, nodir_acc, DEFLT);
			return 3;
		case ENOTDIR:
			(void) pfmt(stderr, MM_ERROR, nodir_exist, DEFLT);
			return 2;
		default:
			(void) pfmt(stderr, MM_ERROR, unexpected);
			return 1;
		}
	}

	while ((direntp = readdir(dirp)) != NULL) {
		tfile[0] = '\0';
		(void) strcat(tfilep, DEFLT);
		(void) strcat(tfilep, "/");
		(void) strcat(tfilep, direntp->d_name);

		if (stat(tfilep, &statbuf) == 0) {
			if ((statbuf.st_mode & S_IFMT) == S_IFREG)
				(void) printf("%s\n", direntp->d_name);
		}
	}

	(void) closedir(dirp);

	return 0;
}


/*
 * Procedure:	display_file
 *
 * Restrictions:
 *		defopen:	None
 *
 * Notes:	used when the contents of a particular file are
 *		to be displayed.
*/
static	int
display_file()
{
	char	*defp;

	if ((defltfp = defopen(filename)) == NULL) {
		(void) pfmt(stderr, MM_ERROR, nofile_acc, filename);
		return 5;
	}

	while((defp = defread(defltfp, NULL)) != NULL) {
		if (strlen(defp)) {
			(void) printf("%s\n", defp);
		}
	}

	defclose(defltfp);

	return 0;
}

	
/*
 * Procedure:	def_exit
 *
 * Notes:	does some clean up and then exits with the value
 *		passwd as ``val''.
*/
static	void
def_exit(val)
int val;
{
	(void) defclose(defltfp);
	exit(val);
}
