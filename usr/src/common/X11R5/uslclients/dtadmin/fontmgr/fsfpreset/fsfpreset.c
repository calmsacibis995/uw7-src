#ident	"@(#)dtadmin:fontmgr/fsfpreset/fsfpreset.c	1.1"
/*
	fs_xset will send a SIGUSR1 signal to the fontserver 
	so the fontserver will reread it's configuration files
	and will update it's internal font tables to be in
	sync with the actual state of the directory. This is needed
	after adding or deleting fonts while the fontserver is 
	running.
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
static int FileOK();
static int GetFontserverPid();

main(argc,argv)
int argc;
char *argv[];
{

/*
	Get the pid of the fontserver and send a SIGUSR1 signal
	pid is stored in file /dev/X/fs.#.pid where # is the port
	number which is used to connect to fs. port # is part of the
	fontpath so get the ports from the XGetFontPath command.
 */
    char filename[256];
    int result, port, pid;


	/* send SIGUSR1 to each fontserver in the fontpath */
	/* this will cause the fontserver to reread it's configuration
		data, we need this to get the newly installed/deleted
		fonts recognized */

	if (argc !=2 ) exit(1);
	port = atoi(argv[1]);
#ifdef DEBUG
	fprintf(stderr,"port==%d\n",port);
#endif
	sprintf(filename, "%s%d%s", "/dev/X/fs.", port, ".pid");
	pid = GetFontserverPid(filename);
#ifdef DEBUG
	fprintf(stderr,"pid for kill=%d\n",pid);
#endif
	if (pid == 0) exit(1);
#ifdef DEBUG
	fprintf(stderr,"sending SIGUSR1 signal to %d\n",pid);
#endif
	result = kill(pid, SIGUSR1);
#ifdef DEBUG
	if (result !=0) fprintf(stderr,"errno=%ld\n",errno);
	fprintf(stderr,"result of kill is %d\n", result);
#endif
	exit(0);

}
	

/*
 * search for X fontserver pid in /dev/X/fs.port#.pid file 
 */
static int
GetFontserverPid(char *filename)
{
    int pid=0;
    FILE *file;
    char buf[256];

    file = fopen(filename, "r");

    if (FileOK(file)) {
        while (fgets(buf, 256, file) != NULL) {
		pid = atoi(buf);
	    }
        }

    fclose(file);
    return pid;
}


static int
FileOK(file)
     FILE *file;
{
  struct stat statb;
  ushort not_normal_file;

  if (file == 0)
    return 0;
  if (fstat (fileno(file), &statb) == -1) {
    fclose(file);
    return 0;
  }

  not_normal_file = statb.st_mode & 070000;  /* octal mask */
  /* if file is a directory or special */
  if (not_normal_file)
      return 0;
  else
      return 1;
} /* end of FileOK */

