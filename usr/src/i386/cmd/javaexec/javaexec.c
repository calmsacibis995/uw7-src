/*	copyright	"%c%"	*/
#ident	"@(#)javaexec:javaexec.c	1.2"

#include <errno.h>
#include <stdio.h>
#include <pfmt.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#define MAX_PATHNAME	1024
#define DEFAULT_JDKPATH	"/usr/java"

/*
 * This is supposed to be invoked by the kernel upon execution of a Java
 * first class (See uts/proc/obj/java.c).
 * The command must be:
 * [<path>/]CLASS[.class]
 * /usr/bin/javaexec will pass only CLASS to the real Java virtual machine
 * minimally by parsing the command line.
 * If the variable JAVA_HOME is not defined, it will use /usr/java for
 * default.
 * The following variables are set by putenv():
 * => CLASSPATH
 * => LD_LIBRARY_PATH
 * => THREADS_TYPE
 *
 * REMARKS:
 *	The error messages are not proper at this moment, especially, in terms
 *	of L10N.
 * 
 */
main(int argc, char **argv)
{
	char *java_home, *classpath, *threads_flag, *threads_type, *class,
		*ld_library_path, *prog, *env, *cmd, *s;
	int error;

	java_home = getenv("JAVA_HOME");
	classpath = malloc(MAX_PATHNAME);
	ld_library_path = malloc(MAX_PATHNAME);
	prog = malloc(MAX_PATHNAME);

	if (argc != 2) {
		error = EINVAL;
		perror("javaexec: invalid argument");		
		goto exit_java;
	}

	/*
	 * Parse the command line
	 */
	cmd = argv[1];		/* should be [<path>/]CLASS[.clas] */

	s = cmd;
	if ((s = rindex(cmd, '/'))) { /* '/' found */
		cmd = ++s;
	}

	if ((s = strstr(cmd, ".class"))) { /* ".class" found */
		*s = NULL;	/* terminiate the string */
	}
	
	if (classpath == NULL || ld_library_path == NULL || prog == NULL) {
		error = ENOMEM;
		perror("javaexec: memory");		
		goto exit_java;
	}
	
	if (java_home != NULL) {
		printf("JAVA_HOME = %s\n", java_home);
	}
	else {
		java_home = DEFAULT_JDKPATH;
	}

	env = getenv("CLASSPATH");

	if (env == NULL) {
		sprintf(classpath, "CLASSPATH=.:%s/classes:%s/lib/classes.zip",
			java_home, java_home);
#ifdef DEBUG		
			printf("classpath = %s\n", classpath);
#endif			
	} else {
		sprintf(classpath,
			"CLASSPATH=%s:%s/classes:%s/lib/classes.zip",
			env, java_home, java_home);
		
	}

	threads_flag = getenv("THREADS_FLAG");
	if (strcmp(threads_flag, "native")) {
		putenv("THREADS_TYPE=green_threads"); /* if not native */
		threads_type = "green_threads";
	} else {
		putenv("THREADS_TYPE=native_threads");
		threads_type = "native_threads";
	}	
			
	sprintf(ld_library_path, "LD_LIBRARY_PATH=%s:%s/lib/x86at/%s",
		env, java_home, threads_type);

	/* put env */
	putenv(classpath);
	putenv(ld_library_path);

	sprintf(prog, "%s/bin/x86at/%s/java", java_home, threads_type);

	error = execl(prog, "java", cmd, 0);
	if (error)
		perror("javaexec: exec error");
exit_java:	
	return error;
}

