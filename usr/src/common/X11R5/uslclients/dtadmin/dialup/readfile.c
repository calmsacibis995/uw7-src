#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/readfile.c	1.1"
#endif

/*
 */

#include <X11/Xos.h>			/* for types.h */
#include <sys/stat.h>
#include <stdio.h>

extern char *malloc();

/*
 * get_data_from_file - read data from a file into a single buffer; meant 
 * for small files containing messages.
 */
static char *get_data_from_file (filename)
    char *filename;
{
    FILE *fp;
    struct stat statbuf;
    char *cp;


    if (stat (filename, &statbuf) != 0 || statbuf.st_size < 0) {
	perror(filename);
	return NULL;
    }

    cp = malloc (statbuf.st_size + 1);
    if (!cp) {
	perror("malloc");
	return NULL;
    }

    fp = fopen (filename, "r");
    if (!fp) {
	(void) free (cp);
	return NULL;
    }

    if (fread (cp, 1, statbuf.st_size, fp) != statbuf.st_size) {
	(void) free (cp);
	(void) fclose (fp);
	return NULL;
    }

    cp[statbuf.st_size] = '\0';		/* since we allocated one extra */
    (void) fclose (fp);
    return cp;
}

/*
 * get_data_from_stdin - copy data from stdin to file, use get_data_from_file,
 * and then remove file.  Reads data twices, but avoid mallocing extra memory.
 * Meant for small files.
 */
static char *get_data_from_stdin ()
{
    char filename[80];
    char buf[BUFSIZ];
    int mfile;
    int n;
    char *cp;

    strcpy (filename, "/tmp/xmsg-XXXXXX");
    mktemp (filename);
    if (!filename[0])
	return NULL;

    mfile = creat(filename, 0600);
    if (mfile < 0) return NULL;
    while ((n = fread (buf, 1, BUFSIZ, stdin)) > 0) {
	(void) write (mfile, buf, n);
    }
    (void) close (mfile);

    cp = get_data_from_file (filename);
    (void) unlink (filename);
    return cp;
}


/*
 * read_file - read data from indicated file and return pointer to malloced
 * buffer.  Returns NULL on error or if no such file.
 */
char *read_file (filename)
    char *filename;
{
    if (filename[0] == '-' && filename[1] == '\0') {
	return (get_data_from_stdin ());
    } else {
	return (get_data_from_file (filename));
    }
}
