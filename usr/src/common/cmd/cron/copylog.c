/*		copyright	"%c%" 	*/

#ident	"@(#)cron:common/cmd/cron/copylog.c	1.1.3.4"

#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pfmt.h>
#include <priv.h>

#define BACKUPLOG	"/var/cron/_backuplog"
#define MAX_TRY		100

static off_t bfgetc();

/****************************************************************
*  COPYLOG handles the actual copying of the log files.  It 	*
*  takes care of locking and unlocking the log files during	*
*  the actual copy procedure.					*
****************************************************************/
/*
 * from  - name of the log file 
 *       - as a result of this operation it will only contain
 *       - the most recent number of lines as specified by the argument
 *       - lines
 * to    - backup copy of the log file 
 *       - as a result of this operation it will only contain
 *       - the removed lines
 *       - if the argument is null or undefined no backup copy
 *       - will be created
 * lines - how many lines you want to keep in the log file
 */
void
copylog(from, to, klines)
char *from, *to;
int klines;
{
int 	current, backup;
int	count = 0;
off_t 	roffset;
char 	buffer[BUFSIZ];
flock_t l1;
int	try = 0;
int	no_old_log = 0;
struct	stat	sb;

	if (from == (char *)NULL || klines <= 0)
		return;

	if (to == (char *)NULL || *to == '\0')
		no_old_log = 1;

	/* OPEN & LOCK CURRENT LOG */

	current = open(from, O_RDWR);
	if (current == -1) {
		/* 
		 * We need MAC write priv to create file in /var/cron
		 * directory since we're running at SYS_PRIVATE and
		 * the directory is at SYS_PUBLIC.
		 */
		(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
		current = creat(from, 0664);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (current == -1) {
			pfmt(stderr, MM_ERROR, 
				":773:failed to create file %s\n", from);
			return;
		}
		close(current);
		current = open(from, O_RDWR);
		if (current == -1) {
			pfmt(stderr, MM_ERROR, 
				":774:failed to open file %s for reading and writing\n", from);
			return;
		}
	}

	l1.l_type = F_WRLCK;
	l1.l_whence = (short)0;
	l1.l_start = l1.l_len = (long)0;

	/*
	 * Attempt locking MAX_TRY times before giving up
	 */
	while (fcntl(current, F_SETLK, &l1) < 0) {
		if (errno == EAGAIN || errno == EACCES) {
			if (++try < MAX_TRY) {
				(void) sleep(4);
				continue;
			}
			pfmt(stderr, MM_ERROR, 
				":775:file %s busy; try again later!\n", from);
			(void) close(current);
			return;
		}
		pfmt(stderr, MM_ERROR, ":776:cannot lock file %s\n", from);
		(void) close(current);
		return;
	}

	/* find the size of the file */

	if (stat(from, &sb) < 0) {
		pfmt(stderr, MM_ERROR, ":777:stat failed on file %s\n", from);
		(void) close(current);
		return;
	}

	if (sb.st_size == 0) {
		(void) close(current);
		return;
	}


	/* FIND THE nlines RECORD OFFSET */

	roffset = bfgetc(current, klines, sb.st_size);

	/* WRITE nlines RECORDS FROM CURRENT TO TEMP LOG */
	
	if (klines) {
		/* 
		 * We need MAC write priv to create file in /var/cron
		 * directory since we're running at SYS_PRIVATE and
		 * the directory is at SYS_PUBLIC.
		 */
		(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
		backup = creat(BACKUPLOG, 0644);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (backup == -1) {
			pfmt(stderr, MM_ERROR, ":778:cannot create file %s\n",
				BACKUPLOG);
			(void) close(current);
			return;
		}
		(void) lseek(current, roffset, 0);
		while ((count = read(current,buffer,sizeof(buffer))) > 0) {
			if (write(backup, buffer, count) != count) {
				pfmt(stderr, MM_ERROR, 
					":779:write fail on file %s\n",
					BACKUPLOG);
				(void) close(current);
				return;
			}
		}
		(void) close(backup);
	}

	/* TRUNCATE THE CURRENT LOG FILE */

	lseek(current, 0L, 0);
	truncate(from, roffset);

	/* WRITE CURRENT LOG TO BACKUP LOG */

	if (roffset && no_old_log == 0) {
		(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
		backup = creat(to, 0644);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (backup == -1) {
			pfmt(stderr, MM_ERROR, 
				":778:cannot create file %s\n", BACKUPLOG);
			(void) close(current);
			return;
		}
		while ((count = read(current,buffer,sizeof(buffer))) > 0) {
			if (write(backup, buffer, count) != count) {
				pfmt(stderr, MM_ERROR,
					":779:write fail on file %s\n",
					BACKUPLOG);
				(void) close(current);
				return;
			}
		}
		(void) close(backup);
	}

	/* TRUNCATE THE CURRENT LOG FILE TO ZERO */

	lseek(current, 0L, 0);
	truncate(from, 0L);

	/* WRITE TEMP LOG BACK TO CURRENT LOG */

	if (klines) {
		backup = open(BACKUPLOG, O_RDONLY);
		if (backup == -1) {
			pfmt(stderr, MM_ERROR,
				":780:failed to open file %s\n", BACKUPLOG);
			(void) close(current);
			return;
		}
		while ((count = read(backup,buffer,sizeof(buffer))) > 0) {
			if (write(current, buffer, count) != count) {
				pfmt(stderr, MM_ERROR,
					":779:write fail on file %s\n", current);
				(void) close(current);
				return;
			}
		}
		(void) close(backup);
	}

	(void) close(current);
	(void) unlink(BACKUPLOG);
}
	

/****************************************************************
*  BFGETC locates the offset used for file truncation prior to	*
*  copying a log file.						*
****************************************************************/
static off_t
bfgetc(fd, klines, size)
int fd, klines;
off_t size;
{
char	buffer[BUFSIZ];
off_t	offset;
int	count;

	int	num_bytes;
	off_t   old_offset;

	/* FIND THE END OF FILE OFFSET */

	klines++;
	if ((offset = lseek(fd, 0L, 2)) == -1) {
		pfmt(stderr, MM_ERROR, ":781:failed to seek: %s\n",
				strerror(errno));
		return(-1);
	}

	old_offset = size;

	while (klines && offset) {

		num_bytes = BUFSIZ;

		/* SEEK BACK A BUFFER LENGTH */

		if ((lseek(fd, (long) -BUFSIZ, 1)) == -1) {
			num_bytes = offset; 
			(void) lseek(fd, 0L, 0);
		}
		

		/* READ THE BUFFER INTO MEMORY */

		if ((count = read(fd, buffer, num_bytes)) == -1) {
			offset = lseek(fd, 0L, 0);
			break;
		}

		(void) lseek(fd, old_offset, 0);
		offset = old_offset;

		/* COUNT NEWLINES FOR RECORD DELIMITER */

		while (--count && --offset) {
			if (buffer[count] == '\n')
				--klines;
			if (!klines)
				return (++offset);
		}

		/* SEEK TO BEGINNING BUFFER LOCATION */

		old_offset = offset;
		(void) lseek(fd, offset, 0);
		if (offset == 1L) {
			offset = 0L;
			break;
		}
	}	

	/* RETURN THE OFFSET LOCATION */
	return (offset);
}
