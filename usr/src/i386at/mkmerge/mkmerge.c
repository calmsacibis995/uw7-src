/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)mkmerge.c	16.1    98/03/03"
#ident  "$Header$"

/*	Options:
**	-c: clean (unlink or rm all files from merged tree)
**	-d: specify a directory
**	-h: use symbolis links to merge
**	-r: use relative symbolic links to merge
**	-l: use `hard' links to merge trees
**	-m: non-linked copies of makefiles to merge trees
**	-n: show what would be merged, but don't merge
**	-u: update (for non-symbolic linked trees), only merge
**		the files that have different checksums
**	-v: verbose
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <regexpr.h>
#include <stdlib.h>
#ifdef SVR4
#include <libgen.h>
#else
extern char *basename();
extern int mkdirp();
extern char *strerror();
typedef int mode_t;
#define MAXPATHLEN 1024
#endif

#define SUCCESS	1
#define FAIL	0


char *targetdir, *cmd, **dirs, **trees;
int	ndir,
	clean,
	use_link,
	show,
	verbose,
	error,
	makefile_copy,
	update,
	can_copy,
	use_symbolic,
	use_relative;
char root[MAXPATHLEN];
char realtargetdir[MAXPATHLEN];

main(argc, argv)
int argc;
char **argv;
{
	extern int optind;
	extern char *optarg;
	int opt, i;
	struct stat statb;
	char pathname[MAXPATHLEN];

	cmd = basename(argv[0]);

	while ((opt = getopt(argc, argv, "cd:hrlmnuv")) != EOF){
		switch(opt){
		case 'c':
			++clean;
			break;
		case 'd': /* Specify a directory */
			if (ndir == 0){
				if ((dirs = (char **)malloc(sizeof(char *)))
					 			== NULL)
					out_of_mem();
			} else {
				if ((dirs = (char **)realloc(dirs,
					    sizeof(char *) * (ndir + 1)))
								== NULL)
					out_of_mem();
			}
			dirs[ndir++] = optarg;
			break;
		case 'h': /* Use symbolic links instead of hard links */
			use_symbolic++;
			break;
		case 'r':
			use_relative++;
			break;
		case 'l':
			++use_link;
			break;
		case 'm':
			++makefile_copy;
			break;
		case 'n':
			++show;
			break;
		case 'u':
			++update;
			fprintf(stderr,
				"%s: `-%c' option not implemented, yet...ignored\n",
					cmd, opt);
			up_date();
			break;
		case 'v': /* Verbose mode */
			verbose++;
			break;
		default:
			usage();
		}
	}

	/*
	 * Verify the arguments
	 */
	if (optind >= argc - 1){
		fprintf(stderr, "%s: Must specify at least two trees\n", cmd);
		usage();
	}

	trees = argv + optind;

	for (i = 0; trees[i] ; ++i) {
		targetdir = trees[i];
	}
	--i;
	trees[i] = NULL;

	trees = argv + optind;

	if (clean)
		cleanup();

	/*
	 * Verify the existence of source trees
	 */
	for (i = 0 ; trees[i] ; i++){
		if (stat(trees[i], &statb) == -1){
			fprintf(stderr, "%s: Cannot access tree %s: %s\n",
				cmd, trees[i], strerror(errno));
			exit(1);
		}
		if ((statb.st_mode & S_IFMT) != S_IFDIR){
			fprintf(stderr, "%s: Not a directory: %s\n", cmd,
				trees[i]);
			exit(1);
		}
	}
	for (i = 0 ; i < ndir ; i++){
		int sdir = 0;
		int j;
		for (j = 0 ; trees[j] ; j++){
			sprintf(pathname, "%s/%s", trees[j], dirs[i]);
			if (stat(pathname, &statb) == -1)
				continue;
			if ((statb.st_mode & S_IFMT) != S_IFDIR){
				fprintf(stderr, "%s: Not a directory: %s\n",
					cmd, pathname);
				exit(1);
			}
			sdir++;
		}
		if (!sdir){
			fprintf(stderr,
				"%s: Directory does not exist in the trees: %s\n",
				cmd, dirs[i]);
			exit(1);
		}
	}

	if (!show) {
		if ((access(targetdir, 00)) == -1) {
			if (mkdirp(targetdir, 0777) == -1){
				perror(targetdir);
				exit (1);
			}
		}
	}

	/*
	 * Need the current directory for symbolic links
	 */
	if (use_symbolic){
		if (getcwd(root, sizeof root) == NULL){
			fprintf(stderr,
				"%s: Cannot determine current directory: %s\n",
				cmd, strerror(errno));
			exit(1);
		}
		if (use_relative && !show) {
			/*
			 * Make sure targetdir is an absolute path
			 */
			if (chdir(targetdir) == -1) {
				fprintf(stderr, "%s: Cannot chdir to %s: %s\n",
					cmd, targetdir, strerror(errno));
				exit(1);
			}
			if (getcwd(realtargetdir, sizeof realtargetdir) == NULL) {
				fprintf(stderr, "%s: Cannot determine real target directory name: %s\n",
					cmd, strerror(errno));
				exit(1);
			}
			if (chdir(root) == -1) {
				fprintf(stderr, "%s: Cannot chdir to %s: %s\n",
					cmd, root, strerror(errno));
				exit(1);
			}
			targetdir = realtargetdir;
		}
	}

	/*
	 * Go for it
	 */
	for (i = 0 ; trees[i] ; i++){

		/*
		 * update local .tree file
		 */

		if (ndir){
			int j;
			for (j = 0 ; j < ndir ; j++){
				char pathtarget[MAXPATHLEN];
				if (use_symbolic)
					sprintf(pathname, "%s/%s/%s", root,
						trees[i], dirs[j]);
				else
					sprintf(pathname, "%s/%s", trees[i],
						dirs[j]);
				(void)stat(pathname, &statb);
				sprintf(pathtarget, "%s/%s", targetdir, dirs[j]);
				if (!mkmerge(basename(trees[i]), pathname,
					pathtarget, statb.st_mode))
					exit(1);
			}
		} else {
			(void)stat(trees[i], &statb);
			if (use_symbolic){
				sprintf(pathname, "%s/%s", root, trees[i]);
				if (!mkmerge(basename(trees[i]), pathname,
					targetdir, statb.st_mode)){
					exit(1);
				}
			} else if (!mkmerge(basename(trees[i]), trees[i],
				targetdir, statb.st_mode)){
				exit(1);
			}
		}
	}
	exit(error ? 1 : 0);
}

out_of_mem()
{
	fprintf(stderr, "%s: Out of memory\n", cmd);
	exit(1);
}

usage()
{
	fprintf(stderr,
		"%s: Usage: %s [-chrlnuv] [-d dir] tree1 tree2...target_tree\n",
		cmd, cmd);
	exit(2);
}



#ifndef SVR4
symlink(pathname, pathtarget)
char *pathname;
char *pathtarget;
{
	char buf[MAXPATHLEN * 2];

/*
 * symlink() is not in the C library.
 * csymlink() does not seem to work.
 * However, "ln -s" does work in the ucb universe.
 * Well, well, well.
 */

	sprintf(buf, "ucb ln -s %s %s", pathname, pathtarget);

	if (system(buf) != 0)
		return -1;

	return 0;
}
#endif

/*
 * Returns the relative path from path1 to path2.
 * Eg. /work/tree/usr/src/common/cmd/ls 
 *     /work/tree/usr/src/uts gives "../../../uts"
 *     
 * The return value is pointer to static data.
 */
char *
relative_path(path1, path2)
char *path1;
char *path2;
{
	static char relpath[MAXPATHLEN * 2];
	char *p1, *p2, *np1, *np2;

	relpath[0] = '\0';
	p1 = path1;
	p2 = path2;
	while ((np1 = strchr(p1, '/')) != NULL) {
		if ((np2 = strchr(p2, '/')) == NULL)
			break;
		if ((np1 - path1) != (np2 - path2))
			break;
		*np1 = '\0';
		*np2 = '\0';
		if (strcmp(p1, p2) != 0) {
			*np1 = '/';
			*np2 = '/';
			break;
		}
		*np1 = '/';
		*np2 = '/';
		p1 = ++np1;
		p2 = ++np2;
	}

	/*
	 * p1 and p2 are at first non-common component
	 * add to relpath "../" times the number of components left in p1
	 */
	do {
		strcat(relpath, "../");
	} while ((p1 = strchr(p1, '/')) != NULL && *++p1);
	/*
	 * add remaining components of p2 into relpath
	 */
	strcat(relpath, p2);

	if (verbose > 1)
		printf("relative_path(%s, %s) -> %s\n", path1, path2, relpath);

	return relpath;
}


#define SYM_LINK 1
#define HARD_LINK 2
#define HARD_COPY 3

mkmerge(tree, source, dest, mode)
char *tree, *source, *dest;
mode_t mode;
{
	struct stat statb;
	int copy_mode = HARD_COPY;
	DIR *dirp;
	struct dirent *entryp;
	char relpathsource[MAXPATHLEN * 2];

	if ((dirp = opendir(source)) == NULL){
		return SUCCESS;
	}

	/*
	 *
	 * Create target directory
	 */
	if (!show) {
	if (stat(dest, &statb) == 0){
		if ((statb.st_mode & S_IFMT) != S_IFDIR){
			fprintf(stderr,
				"%s: Target %s exists and is not a directory\n",
				cmd, dest);
			closedir(dirp);
			return FAIL;
		}
	} else {
		if (mkdirp(dest, mode | (mode_t) S_IWUSR ) == -1){
			fprintf(stderr,
				"%s: Cannot create target directory %s: %s\n",
				cmd, dest, strerror(errno));
			closedir(dirp);
			return FAIL;
		}
		if (verbose)
			fprintf(stdout, "d %s\n", dest);
	}
	}

	if (use_symbolic && use_relative)
		strcpy(relpathsource, relative_path(dest, source));
		
	/*
	 * Skip . and ..
	 */
	(void)readdir(dirp);
	(void)readdir(dirp);

	for (entryp = readdir(dirp) ; entryp ; entryp = readdir(dirp)){
		char pathname[MAXPATHLEN], pathtarget[MAXPATHLEN];
		sprintf(pathname, "%s/%s", source, entryp->d_name);
		sprintf(pathtarget, "%s/%s", dest, entryp->d_name);
		if (stat(pathname, &statb) == -1){
			fprintf(stderr, "%s: Cannot access %s: %s\n", cmd,
				pathname, strerror(errno));
			continue;
		}
		if ((statb.st_mode & S_IFMT) == S_IFDIR){
			if (!mkmerge(tree, pathname, pathtarget, statb.st_mode)){
				closedir(dirp);
				return FAIL;
			}
			continue;
		}
		if ((statb.st_mode & S_IFMT) != S_IFREG){
			fprintf(stderr, "%s: Not a regular file: %s\n", cmd,
				pathname);
			error++;
			continue;
		}
		if (show) {
			fprintf(stdout, "%s\n", pathname);
			continue;
		}

		if (use_symbolic !=0 ){
			if ( makefile_copy != 0 && ismakefile(pathname)!=0 )
					copy_mode = HARD_COPY;
			else
					copy_mode = SYM_LINK;;
		}else if (use_link !=0 )
			copy_mode = HARD_LINK;;

		switch (copy_mode) {
		case SYM_LINK: {
		    char relpath[MAXPATHLEN * 2], *pathsource;

		    if (use_relative) {
			    sprintf(relpath, "%s/%s",
				    relpathsource, entryp->d_name);
			    pathsource = relpath;
		    } else {
			    pathsource = pathname;
		    }

		    if (symlink(pathsource, pathtarget) == -1) {
			if (unlink(pathtarget) == -1 && errno != ENOENT) {
				fprintf(stderr,
					"%s: Cannot remove existing %s: %s\n",
					cmd, pathtarget, strerror(errno));
					error++;
				continue;
			}

			if (symlink(pathsource, pathtarget) == -1) {
				fprintf(stderr,
			"%s: Cannot create symbolic link from %s to %s: %s\n",
					cmd, pathsource, pathtarget,
					strerror(errno));
				closedir(dirp);
				return FAIL;
			}
		    }
		break;
		}

		case HARD_LINK:
			if (link(pathname, pathtarget) == -1){
				perror(cmd);
				closedir(dirp);
				return FAIL;
			}
			break;
		default:
			if (!copy_file(pathname, pathtarget,
				statb.st_mode)){
				closedir(dirp);
				fprintf(stderr,
					"%s: Cannot link %s to %s: %s\n",
					cmd, pathname, pathtarget,
					strerror(errno));
				closedir(dirp);
				return FAIL;
			}
			break;
		}

		if (verbose)
			fprintf(stdout, "%c %s\n", copy_mode == SYM_LINK? 'l' : 'f',
				pathtarget);
	}

	closedir(dirp);
	return SUCCESS;
}
		

copy_file(source, dest, mode)
char *source, *dest;
mode_t mode;
{
	char buf[BUFSIZ];
	int src, dst, n;

	if ((src = open(source, O_RDONLY)) == -1){
		fprintf(stderr, "%s: Cannot open %s: %s\n", cmd, source,
			strerror(errno));
		return FAIL;
	}

	if ((dst = open(dest, O_WRONLY|O_CREAT|O_CREAT, mode)) == -1){
		fprintf(stderr, "%s: Cannot create %s: %s\n", cmd, dest,
			strerror(errno));
		close(src);
		return FAIL;
	}

	while ((n = read(src, buf, sizeof buf)) > 0){
		if (write(dst, buf, n) != n){
			fprintf(stderr, "%s: Write error in %s: %s\n", cmd,
				dest, strerror(errno));
			close(src);
			close(dst);
			return FAIL;
		}
	}
	if (n == -1){
		fprintf(stderr, "%s: Read error in %s: %s\n", cmd, source,
			strerror(errno));
		close(src);
		close(dst);
		return FAIL;
	}
	close(src);
	close(dst);
	return SUCCESS;
}


cleanup()
{
	FILE *fp;
	int i, o;
	unsigned int sum;
	char finame[MAXPATHLEN];
	char unpath[MAXPATHLEN];

	for (i = 0; trees[i]; ++i) {

		strcpy(finame, targetdir);
		strcat(finame, "/.");
		strcat(finame, trees[i]);

		if ((fp = fopen(finame, "r")) == NULL) {
			perror(finame);
			exit (0);
		}

		while ((fscanf(fp, "%s %u", finame, &sum)) != EOF) {
			o = sfnd(finame, trees[i]);
			o += (strlen(trees[i]) + 1);
			strcpy(unpath, targetdir);
			strcat(unpath, "/");
			strcat(unpath, &finame[o]);
			if (verbose)
				fprintf(stdout, "unlink %s\n", unpath);
			if ((unlink(unpath)) == -1) {
				perror(unpath);
			}
		}

		fclose(fp);
		strcpy(unpath, targetdir);
		strcat(unpath, "/.");
		strcat(unpath, trees[i]);
		if (verbose)
			fprintf(stdout, "unlink %s\n", unpath);
		if ((unlink(unpath)) == -1) {
			perror(unpath);
		}
	}

	exit (0);
}

#ifndef SVR4
char *
strerror()
{
	extern char *sys_errlist[];

	return (sys_errlist[errno]);
}
#endif	/* SVR4 */

up_date()
{
	exit (0);
}

sfnd(as1,as2)
char *as1,*as2;
{
	register char *s1,*s2;
	register char c;
	int offset;

	s1 = as1;
	s2 = as2;
	c = *s2;

	while (*s1)
		if (*s1++ == c) {
			offset = s1 - as1 - 1;
			s2++;
			while ((c = *s2++) == *s1++ && c) ;
			if (c == 0)
				return(offset);
			s1 = offset + as1 + 1;
			s2 = as2;
			c = *s2;
		}
	 return(-1);
}

int ismakefile( mk_pathname)
   char	*mk_pathname;
{
   char	*mk_expr;
   int t=0;

   mk_expr = regcmp(".*uts/.+\\.mk$",(char *)0);
   if ((t = (int)regex(mk_expr,mk_pathname)) == 0){
	free(mk_expr);
	mk_expr = regcmp(".*uts/.*[Mm]akefile$",(char *)0);
	t=(int)regex(mk_expr,mk_pathname);
   }

   free(mk_expr);
   return t;
}
