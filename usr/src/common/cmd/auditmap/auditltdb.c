/*		copyright	"%c%" 	*/

#ident	"@(#)auditltdb.c	1.3"
#ident  "$Header$"

/*LINTLIBRARY*/
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <audit.h>
#include <locale.h>
#include <pfmt.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include "auditmap.h"

extern void adumprec();
extern void ch_name();
extern int lck_file();

extern char *argvp;
extern char *mapdir;
extern struct stat statbuf;

/*
 * Procedure:     cr_ltdb
 *
 * Restrictions:
                 pfmt: None
                 open(2): None
                 fcntl(2): None
                 stat(2): None
                 write(2): None
                 unlink(2): None
                 chown(2): None
*/
/* The cr_ltdb() routine will create the LTDB in the auditmap      */
/* directory.  For each file, the routine does the following: 	   */
/* 1. open the "from" and "to" files,				   */
/* 2. get the size of the "from" file,				   */
/* 3. divided the size by number of bytes in each entry,    	   */
/* 4. copy one entry at a time till all entries are copied.	   */
void
cr_ltdb(newf,oldf,existf,size)
char *newf;
char *oldf;
char *existf;
int size;
{
	struct	stat status;
	int 	msize,fd, fd2,i,rc;
	char	*mapfile, *entry;
	short   incomplete;
	struct flock wlck, rlck;

	/*Allocate space for the pathname of the file*/
	msize=strlen(mapdir)+strlen(newf)+1;
	if ((mapfile = ((char *)malloc(msize))) ==  (char *)NULL) {
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/*Concat the full pathname of file */
	(void)strcpy(mapfile,mapdir);
	(void)strcat(mapfile,newf);

	/*Create buffer for read/write operations*/
	if ((entry = (char *)calloc(size, sizeof(char))) == NULL)
	{
                (void)pfmt(stderr,MM_ERROR,MSG_MALLOC);
		adumprec(ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	rc=incomplete=0;

	/*If the LTDB file already exists - rename the file*/
	if ((fd=open(mapfile,O_WRONLY)) >0 )
	{
		/*Establish a write lock on the entire file*/
		wlck.l_type=F_WRLCK;
		wlck.l_whence=0;  /*start at beginning of file*/
		wlck.l_start=0L;  /*relative offset*/
		wlck.l_len=0L;    /*until EOF*/

		if (lck_file(fd,mapfile,&wlck) == 0)
		{
			/*Rename the existing file to ofile*/
			ch_name(newf,oldf);
			(void)fcntl(fd,F_UNLCK,&wlck);
			(void)close(fd);
		}
		else
		{
			/*The file is locked by another process */
			/*Continue on to the next audit map file*/
			(void)close(fd);
                	(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,existf,mapfile);
			return;
		}
	}
	else {
		/*It is not an error if there is no existing auditmap file*/
		/*ENOENT = O_CREAT not set and the file doesn't exist     */
		if (errno != ENOENT)
		{
			if (errno == EACCES) {
                                (void)pfmt(stderr, MM_ERROR, NOPERM);
                                adumprec(ADT_NOPERM,strlen(argvp),argvp);
                                exit(ADT_NOPERM);
			}
       	       		(void)pfmt(stderr,MM_ERROR,MSG_NO_WRITE,mapfile);
			adumprec(ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}
	}

	if ((fd=open(mapfile,O_WRONLY|O_CREAT, 0660))> 0)
	{

		if ((fd2=open(existf, O_RDONLY)) > 0)
		{
			/*Establish a write lock on the to file*/
			wlck.l_type=F_WRLCK;
			wlck.l_whence=0;  /*start at beginning of file*/
			wlck.l_start=0L;  /*relative offset*/
			wlck.l_len=0L;    /*until EOF*/

			if (lck_file(fd,mapfile,&wlck) == 0)
			{
				/*Establish a read lock on the from file*/
				rlck.l_type=F_RDLCK;
				rlck.l_whence=0;  /*start at beginning of file*/
				rlck.l_start=0L;  /*relative offset*/
				rlck.l_len=0L;    /*until EOF*/

 				if (lck_file(fd2,existf,&rlck) == 0)
				{
					(void)stat(existf,&status);
					rc=(status.st_size) / size;
					for (i=0; i<rc; i++)
					{
						if (read(fd2,entry,size) != size)
						{
							i=rc;
							incomplete=1;
						}
						if (write(fd,entry,size) != size)
						{
							i=rc;
							incomplete=1;
						}
					}
				}
				else
				{
					/*The master LTDB file is locked by another process*/
					/*Continue on to the next audit map file           */
					(void)fcntl(fd,F_UNLCK,&wlck);
					(void)close(fd);
					(void)close(fd2);
                			(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,existf,mapfile);
					return;
				}
			}
			else
			{ 
				/*The local LTDB file is locked by another process*/
				/*Continue on to the next audit map file          */
				(void)close(fd);
				(void)close(fd2);
                		(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,existf,mapfile);
				return;
			}
		}
		else
		{
			/*Unable to open a master LTDB file for reading    */
			/*If a master LTDB file does not exist, continue to*/
                        /*next audit map file.                             */
			if (errno != ENOENT) {
				if (errno == EACCES){
					(void)close(fd);
					(void)unlink(mapfile);
                       			(void)pfmt(stderr, MM_ERROR, NOPERM);
                       			adumprec(ADT_NOPERM,strlen(argvp),argvp);
                        		exit(ADT_NOPERM);
				}
				(void)close(fd);
				(void)unlink(mapfile);
				(void)pfmt(stderr,MM_ERROR,MSG_NO_READ,existf);
				adumprec(ADT_FMERR,strlen(argvp),argvp);
				exit(ADT_FMERR);
			}
			(void)close(fd);
			(void)unlink(mapfile);
                	(void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,existf,mapfile);
			return;
		}
	}
	else {
		/*Failed to open a local LTDB file*/
		if (errno == EACCES){
                        (void)pfmt(stderr, MM_ERROR, NOPERM);
                        adumprec(ADT_NOPERM,strlen(argvp),argvp);
                        exit(ADT_NOPERM);
		}
		if (errno == ENOENT)
               	 	(void)pfmt(stderr,MM_ERROR,MSG_NO_DIR,mapfile);
		else
               	 	(void)pfmt(stderr,MM_ERROR,MSG_NO_WRITE,mapfile);
		adumprec(ADT_FMERR,strlen(argvp),argvp);
		exit(ADT_FMERR);
	}

	(void)fcntl(fd,F_UNLCK,&wlck);
	(void)fcntl(fd2,F_UNLCK,&rlck);
	(void)close(fd);
	(void)close(fd2);
	if (incomplete || (chown(mapfile,statbuf.st_uid,statbuf.st_gid) == -1))
	{
                (void)pfmt(stdout,MM_WARNING,MSG_INCOMPLETE,existf,mapfile);
		(void)unlink(mapfile);
	}
	free(mapfile);
	free(entry);
}
