#ident  "@(#)relax.C	1.3"
#ident  "$Header$"

/******************************************************************************
 *	relax.C
 *-----------------------------------------------------------------------------
 * Comments:
 * relax utility for configuring system security parameters. relax
 * expects the name of a directory in the /tcb/lib/relax directory as
 * the only parameter. Three files are looked for in this directory:
 *
 * etc_def:	updates to the /etc/default files
 * script:	shell script passed to the bourne shell for execution.
 *
 * Usage: relax directory_name
 *
 * Only a user with appropriate privileges can use this program.
 *
 *-----------------------------------------------------------------------------
 *      @(#)relax.C	7.5	97/08/30
 *
 *      Copyright (C) The Santa Cruz Operation, 1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and should be treated as Confidential.
 *-----------------------------------------------------------------------------
 * Revision History:
 *
 *	Thu Dec 19 13:19:29 PST 1996	louisi
 *		Created file.
 *
 *	Mon Dec 30 16:54:40 PST 1996    louisi
 *		- If the configureation directory cannot be accessed, then
 *                generate an error indicating that the user is not authorized
 *                to run the program.  This allows for tfadmin execution to
 *                work for non-root users.
 *
 *  Tue Sep  9 11:39:17 BST 1997    andrewma
 *      Default dir now /etc/security/seclevel. Was /usr/lib/scoadmin/security.
 *      Changed includes to avoid dependance on scoadmin part of tree and to
 *      use xenv instead.
 *============================================================================*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <basicIncl.h>
#include "relax.msgd.h"
#include "relax.msg.h"

#define	EXIT_OK		0		/* no errors */
#define	EXIT_ERROR	1		/* an error occurred */
#define	EXIT_USAGE	2		/* usage message */
#define	ARBSTRSIZ	100		/* ARBitrary STRing SIZe */
#define	BOURNE_SHELL	"/sbin/sh"
#define	DELETE		0
#define	ADD_OR_CHANGE	1
#define	COMPLETED	2


static void	usage(void);
static void	cleanup(char *, char *, FILE *, FILE *);
static int	check_name(char *);
static int	update_defaults(char *);
static int	execute_script(char *);
static int	process_line(char *);
static int	update_file(char *, char *, int);
static int	install_new_file(char *, char *);      /*L006*/
static int	match_str(char *, char *);
static char	*make_path(char	*, char	*);

static char     *command_name="relax";
static char	*system_defs = "/etc_def";
static char	*shell_script = "/script";
static char	*etc_default = "/etc/default/";
static char	*default_dir = "/etc/security/seclevel/";
static char	*safe_env[] = {"PATH=/bin:/usr/bin:/etc","IFS= \t\n",(char *)0};

int
main(
	int	argc,
	char	**argv
){
	char	*path;
	struct stat	stbuf;
	errStatus_cl  eStk;

	/* check a single parameter is supplied */
	if (argc != 2) {
		usage();
		/* NOTREACHED */
	}

	/* check a valid directory name has been supplied */
	if (check_name(argv[1]) < 0) {
		eStk.Push(&SCO_RELAX_ERR_BAD_DIR, argv[1]);
		eStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	if ((path = make_path(default_dir, argv[1])) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	/* ensure the specified directory exists */
	if (stat(path, &stbuf) < 0) {
	    if (errno == EACCES) {
		eStk.Push(&SCO_RELAX_ERR_SU, path);
		eStk.Output(stderr);
		exit(EXIT_ERROR);
	    } else
		eStk.Push(&SCO_RELAX_ERR_NO_DIR, path);
		eStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}
	/* check a directory has been specified */
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		eStk.Push(&SCO_RELAX_ERR_NOT_DIR, path);
		eStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	/* execute shell script */			/* L000 start */
	if (execute_script(path) < 0) {
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}						/* L000 end */

	/* update the /etc/default files */
	if (update_defaults(path) < 0) {
		exit(EXIT_ERROR);
		/* NOTREACHED */
	}

	exit(EXIT_OK);
	/* NOTREACHED */
}

/*
 * Display the usage message on stderr and exit.
 */
static void
usage(
	void /* no args */
){
	errStatus_cl eStk;

	eStk.Push(&SCO_RELAX_ERR_USAGE, command_name);
	eStk.Output(stderr);
	exit(EXIT_USAGE);
	/* NOTREACHED */
}

/*
 * Return -1 if the name supplied is an invalid filename otherwise return 0.
 */
static int
check_name(
	char	*name
){
	/* check a non empty string was supplied */
	if (strlen(name) < 1) {
		return(-1);
	}
	/* check the current directory "." is not specified */
	if (strcmp(name, ".") == 0) {
		return(-1);
	}
	/* ensure name contains no ".." or / characters */
	if (strchr(name, '/') != (char *)0) {
		return(-1);
	}
	while (*name && *(name + 1)) {
		if ((*name == '.') && (*(name + 1) == '.')) {
			return(-1);
		}
		name++;
	}
	return(0);
}

/*
 * Update the system default files. Return 0 on success, otherwise -1.
 */
static int
update_defaults(
	char	*path
){
	char	*defaults, *buf, *pos, c;
	int	length, lineno;
	FILE	*sys_fp;
	errStatus_cl eStk;

	/* make the pathname of the defaults updates file */
	if ((defaults = make_path(path, system_defs)) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		eStk.Output(stderr);
		return(-1);
	}

	/* check the defaults file exists, if not just return */
	if (access(defaults, R_OK) < 0)
	{
		delete defaults;
		return(0);
	}

	if ((sys_fp = fopen(defaults, "r")) == (FILE *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_OPEN, defaults);
		eStk.Output(stderr);
		delete defaults;
		return(-1);
	}

	buf = new char[BUFSIZ];

	lineno = 0;
	while ((pos = fgets(buf, BUFSIZ, sys_fp)) != (char *)0) {
		lineno++;
		length = strlen(pos) - 1;
		if (pos[length] != '\n') {
			/* skip the remainder of the line */
			while ((c = getc(sys_fp)) != EOF && c != '\n') {
				;
			}
			pos[BUFSIZ - 2] = '\0';
			eStk.Push(&SCO_RELAX_ERR_TRUNC, defaults, lineno);
			eStk.Output(stderr);
		}
		else {
			pos[length] = '\0';
		}
		if (process_line(pos) < 0) {
			(void)fclose(sys_fp);
			delete defaults;
			delete buf;
			return(-1);
		}
	}
	delete buf;
	if (ferror(sys_fp)) {
		(void)fclose(sys_fp);
		eStk.Push(&SCO_RELAX_ERR_READ, defaults);
		eStk.Output(stderr);
		delete defaults;
		return(-1);
	}
	delete defaults;
	(void)fclose(sys_fp);
	return(0);
}

/*
 * Pass the script in the specified directory to the shell for execution.
 * Return 0 on success, otherwise -1.
 */
static int
execute_script(
	char	*path
){
	char	*script;
	pid_t	child_pid;				/* L003 */
	int	wait_stat;
	struct stat	stbuf;
	errStatus_cl eStk;

	/* make the pathname of the script file */
	if ((script = make_path(path, shell_script)) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		eStk.Output(stderr);
		return(-1);
	}

	/* check the script exists and is a regular file */
	if (stat(script, &stbuf) < 0) {
		delete script;
		return(0);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFREG) {
		eStk.Push(&SCO_RELAX_ERR_NOT_REG_FILE, script);
		eStk.Output(stderr);
		delete script;
		return(-1);
	}

	/* check the shell exists and is executable */
	if (access(BOURNE_SHELL, X_OK) < 0)
	{
		eStk.Push(&SCO_RELAX_ERR_NO_EXEC, BOURNE_SHELL);
		eStk.Output(stderr);
		delete script;
		return(-1);
	}

	switch (child_pid = fork()) {
	case -1:	/* fork failed */
		eStk.PushUnixErr(errno);
		eStk.Push(&SCO_RELAX_ERR_NO_FORK);
		eStk.Output(stderr);
		delete script;
		return(-1);
		/* NOTREACHED */
	case 0: 	/* child */
		(void)execle(BOURNE_SHELL, BOURNE_SHELL, script, (char *)0,
			     safe_env);
		eStk.PushUnixErr(errno);
		eStk.Push(&SCO_RELAX_ERR_NO_EXEC2);
		eStk.Output(stderr);
		exit(EXIT_ERROR);
		/* NOTREACHED */
	default:	/* parent */
		if (waitpid(child_pid, &wait_stat, 0) < 0) {
		    eStk.PushUnixErr(errno);
		    eStk.Push(&SCO_RELAX_ERR_WAIT);
		    eStk.Output(stderr);
		    delete script;
		    return(-1);
		}
		if (WIFEXITED(wait_stat)) {
			delete script;
			return(0);
		}
		else {
			/* if child didn't terminate normally */
			delete script;
			return(-1);
		}
		/* NOTREACHED */
	}
}

/*
 * Return a pointer to a new string created by appending <file> to <directory>.
 */
static char	*
make_path(
	char	*directory,
	char	*file
){
	char	*path;

	path = new char[strlen(directory) + strlen(file) + 1];
	if (path == (char *)0) {
		return((char *)0);
	}
	(void)strcpy(path, directory);
	(void)strcat(path, file);
	return(path);
}

/*
 * Process a line from the system file, return 0 on success, otherwise -1.
 */
static int
process_line(
	char	*line
){
	int	operation, result;
	char	*def_file, *filename;
	errStatus_cl eStk;

	/* check the first character of the line for + or - */
	switch (*line++) {
		case '+':
			operation = ADD_OR_CHANGE;
			break;
		case '-':
			operation = DELETE;
			break;
		default:
			return(0);
			/* NOTREACHED */
	}

	/* the next characters are a filename terminated by a ":" */
	filename = line;
	while (*line && (*line != ':')) {
		line++;
	}
	if (*line == '\0') {
		return(0);
	}
	*line++ = '\0';

	if (check_name(filename) < 0) {
		eStk.Push(&SCO_RELAX_ERR_BAD_FILENAME, filename);
		eStk.Output(stderr);
		return(0);
	}

	if ((def_file = make_path(etc_default, filename)) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		eStk.Output(stderr);
		return(-1);
	}

	result = update_file(def_file, line, operation);
	delete def_file;
	return(result);
}

/*
 * Update the system default <file>, with <string>, performing the
 * specified <operation>. Return 0 on success, otherwise -1.
 */
static int
update_file(
	char	*file,
	char	*string,
	int	operation
){
	static char	*buffer = (char *)0;
	int	lineno, length, result, old_umask;
	char	*lck_file, *tmp_file, *pos, c;
	FILE	*in_fp, *out_fp;
	int     lck_fd;
	errStatus_cl eStk;

	if (buffer == (char *)0) {
		buffer=new char[BUFSIZ];
		if (buffer == (char *)0) {
		    eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		    eStk.Output(stderr);
		    return(-1);
		}
	}

	/* check the file exists, if not just return */
	if (access(file, R_OK) < 0)
	{
		return(0);
	}
	if ((in_fp = fopen(file, "r")) == (FILE *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_UPDATE, file);
		eStk.Output(stderr);
		return(-1);
	}

	old_umask = umask(0777);
	if ((lck_file = make_path(file, "-t")) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		eStk.Output(stderr);
		return(-1);
	}
	if ((lck_fd = creat(lck_file, 7000)) == -1) {
		eStk.Push(&SCO_RELAX_ERR_NO_OPEN2, lck_file);
		eStk.Output(stderr);
		delete lck_file;
		(void)fclose(in_fp);
		return(-1);
	}
	close(lck_fd);
	(void)umask(old_umask);

	if ((tmp_file = make_path(file, "-tmp")) == (char *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_ALLOC);
		eStk.Output(stderr);
		unlink(lck_file);
		return(-1);
	}

	(void)unlink(tmp_file);
	if ((out_fp = fopen(tmp_file, "w")) == (FILE *)0) {
		eStk.Push(&SCO_RELAX_ERR_NO_OPEN2, tmp_file);
		eStk.Output(stderr);
		delete tmp_file;
		(void)fclose(in_fp);
		unlink(lck_file);
		return(-1);
	}

	lineno = 0;
	while ((pos = fgets(buffer, BUFSIZ, in_fp)) != (char *)0) {
		lineno++;
		length = strlen(pos) - 1;
		if (pos[length] != '\n') {
			/* skip the remainder of the line */
			while ((c = getc(in_fp)) != EOF && c != '\n') {
				;
			}
			pos[BUFSIZ - 2] = '\0';
			eStk.Push(&SCO_RELAX_ERR_TRUNC,file,lineno);
			eStk.Output(stderr);
		}
		else {
			pos[length] = '\0';
		}
		/* compare with edit string */
		if ((operation != COMPLETED) && match_str(pos, string)) {
			if (operation == ADD_OR_CHANGE) {
				if (fputs(string, out_fp) == EOF) {
					cleanup(lck_file, tmp_file, in_fp, out_fp);
					return(-1);
				}
				if (fputc('\n', out_fp) == EOF) {
					cleanup(lck_file, tmp_file, in_fp, out_fp);
					return(-1);
				}
				/* only do one edit */
				operation = COMPLETED;
			}
			else {
				/* only do one edit */
				operation = COMPLETED;
			}
		}
		else if (pos[0] != '\0') {
			if (fputs(pos, out_fp) == EOF) {
				cleanup(lck_file, tmp_file, in_fp, out_fp);
				return(-1);
			}
			if (fputc('\n', out_fp) == EOF) {
				cleanup(lck_file, tmp_file, in_fp, out_fp);
				return(-1);
			}
		}
	}
	/* if an existing string not replaced, add at the end of the file */
	if (operation == ADD_OR_CHANGE) {
		if (fputs(string, out_fp) == EOF) {
			cleanup(lck_file, tmp_file, in_fp, out_fp);
			return(-1);
		}
		if (fputc('\n', out_fp) == EOF) {
			cleanup(lck_file, tmp_file, in_fp, out_fp);
			return(-1);
		}
	}
	(void)fclose(in_fp);
	(void)fclose(out_fp);

	result = install_new_file(tmp_file, file);
	(void)unlink(tmp_file);					      
	(void)unlink(lck_file);					      
	return(result);
}

/*
 * Replace the default file <path> with the updated <tmppath>. Return
 * 0 on success and -1 on failure.
 */
static int
install_new_file(
	char    *tmpfile,
	char    *file
){
	struct stat statbuf;
	errStatus_cl eStk;

	// First update the file mode.
	if(stat(file, &statbuf) == -1) {
	    eStk.PushUnixErr(errno);
	    eStk.Push(&SCO_RELAX_ERR_UPDATE_FAIL, file);
	    eStk.Output(stderr);
	    return(-1);
	}
	if(chown(tmpfile, statbuf.st_uid, statbuf.st_gid) == -1) {
	    eStk.PushUnixErr(errno);
	    eStk.Push(&SCO_RELAX_ERR_UPDATE_FAIL, tmpfile);
	    eStk.Output(stderr);
	    return(-1);
	}
	if(chmod(tmpfile, statbuf.st_mode) == -1) {
	    eStk.PushUnixErr(errno);
	    eStk.Push(&SCO_RELAX_ERR_UPDATE_FAIL, tmpfile);
	    eStk.Output(stderr);
	    return(-1);
	}
	if(rename(tmpfile, file) == -1 ) {
	    eStk.PushUnixErr(errno);
	    eStk.Push(&SCO_RELAX_ERR_UPDATE_FAIL, file);
	    eStk.Output(stderr);
	    return(-1);
	}
	return(0);
}

/*
 * Return 1 if <new> and <current> are considered to match.
 */
static int
match_str(
	char	*current,
	char	*newstr
){
	if ((strlen(current) == 0) || (strlen(newstr) == 0)) {
		return(0);
	}
	while (*current && *newstr && (*newstr != '=') && (*current == *newstr)) {
		current++;
		newstr++;
	}
	if (((*newstr == '=') || (*newstr == '\0')) &&
	    ((*current == '=') || (*current == '\0'))) {
		return(1);
	}
	return(0);
}

/*
 * Cleanup routine called by update_file.
 */
static void
cleanup(
	char	*lckpath,
	char	*path,
	FILE	*fp1,
	FILE	*fp2
){
	(void)fclose(fp1);
	(void)fclose(fp2);
	(void)unlink(lckpath);
	delete lckpath;
	(void)unlink(path);
	delete path;
}
