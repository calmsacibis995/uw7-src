#ident	"@(#)scomsc1.c	11.2"


/*
 * SCO Message Store Version 1 cache routines.
 * reads and writes the folders, and manages the incore cache
 * Sendmail From lines are not visible from outside,
 * envelope sender and date are managed through the API
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <pwd.h>
#include <errno.h>
#include "mail.h"
#include "osdep.h"
#include "scomsc1.h"

#define debug printf

/* testing callback routine */
void (*scomsc1_rebuild_callback)();

/*
 * check if a folder is one we can open, we only guess by looking at
 * the first line or two.  Optimized for speed for now.
 * allow sendmail, MMDF, and scoms1 formats
 * returns TRUE if yes, FALSE if not
 */
int
scomsc1_valid(char *path)
{
	int fd;
	register mrc_t *mrc;
	int ret;

	msc1_init(0);
	mrc = malloc(sizeof(mrc_t));
	if (mrc == 0)
		return(0);
	msc1_init_mrc(mrc, path);
	fd = nolink_open(mrc->mrc_path, 0);
	if (fd < 0) {
		errno = ENOENT;
		free(mrc);
		return(0);
	}
	mrc->mrc_folder_fp = fdopen(fd, "r");
	ret = msc1_folder_valid(mrc->mrc_folder_fp);
	fclose(mrc->mrc_folder_fp);
	free(mrc);
	return(ret ? 1 : 0);
}

/*
 * open a folder, an opaque pointer is returned.
 *   convert means to automatically convert to system defined default
 *   folder type (MMDF or sendmail), used primarily by the folder
 *   conversion utility
 * error returns are:
 *	0 - failed, unspecified error
 *	1 - invalid mailbox format
 *	2 - could not get lock type requested
 *	any other value is a valid pointer
 */
void *
scomsc1_open(char *path, int access, int convert)
{
	register mrc_t *mrc;
	int fd;
	int ret;
	int mmdf;

	msc1_init(0);
	mrc = malloc(sizeof(mrc_t));
	if (mrc == 0)
		return(0);
	msc1_init_mrc(mrc, path);
	/* try create only if inbox */
	if (access&ACCESS_WRITE) {
		if (msc1_strccmp(path, "INBOX") == 0)
			ret = msc1_create_folder(mrc, access|ACCESS_BOTH,
				SCOMS_INBOX_PERM&conf_umask);
		else
			ret = msc1_open_folder(mrc, access|ACCESS_BOTH);
	}
	else
		ret = msc1_read_folder(mrc, access|ACCESS_BOTH);
	if (ret == 0) {
		msc1_close(mrc);
		return((void *)2);
	}
	/* check if main mailbox file is valid */
	if ((mmdf = msc1_folder_valid(mrc->mrc_folder_fp)) == 0) {
		msc1_close(mrc);
		return((void *)1);
	}
	mmdf--;
	if (mmdf == conf_mmdf)
		convert = 0;
	/* read index file, rebuild it if needed */
	if (msc1_index(mrc, convert) == 0) {
		msc1_close(mrc);
		return(0);
	}
	/* put back locks */
	if (msc1_lock(mrc, access) == 0) {
		msc1_close(mrc);
		return(0);
	}
	return(mrc);
}

/*
 * attempt to change our access type
 * return 1 success, 0 fail, on failure old access is preserved
 * if possible, downgrade access should not fail so we should
 * not have any problems if we are careful
 */
int
scomsc1_chopen(void *handle, int newaccess)
{
	mrc_t *mrc;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	if (msc1_lock(mrc, newaccess) == 0)
		return(0);
	return(1);
}

/*
 * files are closed, resources are freed, locks are removed
 * and garbage is collected if necessary
 */
void
scomsc1_close(void *handle)
{
	register mrc_t *mrc;
	int oldaccess;

	mrc = handle;
	if (mrc == 0)
		return;
	if ((mrc->mrc_dead == 0) && (mrc->mrc_access&ACCESS_SE)) {
		oldaccess = mrc->mrc_access;
		if (msc1_lock(mrc, mrc->mrc_access|ACCESS_REBUILD)) {
			/* expunge only on session */
			/* only clear old flags during session */
			msc1_set_old(mrc);
			if (scomsc1_check(mrc)) {
				msc1_expunge(mrc);
				msc1_set_consistency(mrc);
			}
			if (conf_fsync) {
				fsync(fileno(mrc->mrc_index_fp));
				fsync(fileno(mrc->mrc_folder_fp));
			}
		}
	}
	msc1_close(mrc);
	return;
}

/*
 * given a handle and a message number, fetch the message
 * if buf is null then we malloc a buffer that the caller must free
 * otherwise buf is filled, it is assumed to be large enough
 * to hold the message including the null terminator byte at the end.
 * the filled buffer is always returned, null if failed
 * if body is false, then fetch the header, else fetch the body.
 * We strip the "From " line as we fetch only the rfc822 part
 * We also strip all internal Status lines as they are exposed
 * elsewhere and that is the way C-Client wants it.
 */
char *
scomsc1_fetch(void *handle, char *buf, int msg, int body)
{
	register mrc_t *mrc;
	register prc_t *prc;
	prc_t tprc;
	int malloced;		/* need to free buf on error */
	int off;
	int size;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(0);
	prc = mrc->mrc_prc + msg;
	/* check file munged */
	if (conf_extended_checks && mrc->mrc_index_fp) {
		tprc = *prc;
		if (msc1_read_prc(mrc, msg) == 0) {
			mrc->mrc_dead = 1;
			return(0);
		}
		if (memcmp(&tprc, prc, sizeof(prc_t))) {
			mrc->mrc_dead = 1;
			return(0);
		}
	}
	if (body == 1) {
		off = prc->prc_bodystart;
		size = prc->prc_size - (prc->prc_bodystart - prc->prc_start);
	}
	else if (body == 0) {
		off = prc->prc_hdrstart;
		if (prc->prc_stat_start)
			size = prc->prc_stat_start - prc->prc_hdrstart;
		else
			/* full header if no index? */
			size = prc->prc_bodystart - prc->prc_hdrstart;
	}
	else {
		/* fetch sendmail From line, only used internally */
		off = prc->prc_start;
		size = prc->prc_hdrstart - off;
	}
	if (buf == 0) {
		buf = malloc(size+2);
		malloced = 1;
		if (buf == 0)
			return(0);
	}
	else
		malloced = 0;
	buf[0] = 0;
	if (size) {
		if (fseek(mrc->mrc_folder_fp, off, SEEK_SET) < 0)
			goto fetch_err;
		if (fread(buf, 1, size, mrc->mrc_folder_fp) != size)
			goto fetch_err;
		buf[size] = 0;
	}
	/* add terminating new line to header */
	if ((body == 0) && prc->prc_stat_start) {
		buf[size++] = '\n';
		buf[size] = 0;
	}
	return(buf);
fetch_err:
	if (malloced)
		free(buf);
	mrc->mrc_dead = 1;
	return(0);
}

/*
 * fetch the size of a message header or body
 * -1 returned on error (bad msg id)
 * we strip the "From " line from sizes as we count only the rfc822 part
 */
int
scomsc1_fetchsize(void *handle, int msg, int body)
{
	register mrc_t *mrc;
	register prc_t *prc;
	int size;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(-1);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(-1);
	prc = mrc->mrc_prc + msg;
	if (body)
		size = prc->prc_size - (prc->prc_bodystart - prc->prc_start);
	else {
		if (prc->prc_stat_start)
			size = (prc->prc_stat_start - prc->prc_hdrstart) + 1;
		else
			size = prc->prc_bodystart - prc->prc_hdrstart;
	}
	return(size);
}

/*
 * fetch line counts for header and body
 * saves a little time in newline conversion calculations
 */
int
scomsc1_fetchlines(void *handle, int msg, int body)
{
	register mrc_t *mrc;
	register prc_t *prc;
	int lines;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(-1);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(-1);
	prc = mrc->mrc_prc + msg;
	if (body)
		lines = prc->prc_bodylines;
	else {
		lines = prc->prc_hdrlines;
		if (prc->prc_stat_start)
			lines -= STAT_LINES;
		if (prc->prc_content_len)
			lines--;
	}
	return(lines);
}

/*
 * fetch the internal date of a message
 * 1 is returned on success
 * 0 returned on error (bad msg id)
 * MESSAGECACHE is filled in with the internal date values
 */
int
scomsc1_fetchdate(void *handle, int msg, register MESSAGECACHE *elt)
{
	mrc_t *mrc;
	register prc_t *prc;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(0);
	prc = mrc->mrc_prc + msg;
	elt->hours = prc->prc_hours;
	elt->minutes = prc->prc_minutes;
	elt->seconds = prc->prc_seconds;
	elt->day = prc->prc_day;
	elt->month = prc->prc_month;
	elt->year = prc->prc_year;
	elt->zoccident = prc->prc_zoccident;
	elt->zhours = prc->prc_zhours;
	elt->zminutes = prc->prc_zminutes;
	return(1);
}

/*
 * fetch the uid of a message
 * uid is returned on success
 * 0 returned on error (bad msg id)
 */
int
scomsc1_fetchuid(void *handle, int msg)
{
	register mrc_t *mrc;
	register prc_t *prc;
	int size;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(0);
	prc = mrc->mrc_prc + msg;
	return(prc->prc_uid);
}

/*
 * fetch the internal sender of a message
 * 1 is returned on success
 * 0 returned on error (bad msg id)
 */
int
scomsc1_fetchsender(void *handle, int msg, char *out)
{
	mrc_t *mrc;
	register prc_t *prc;
	char buf[512];
	char sender[512];
	char *hdr;
	char *cp;
	char *cp1;
	int c;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(0);
	prc = mrc->mrc_prc + msg;
	/* have a from line */
	if (prc->prc_start != prc->prc_hdrstart) {
		if (scomsc1_fetch(handle, buf, msg+1, 2) == 0) {
			mrc->mrc_dead = 1;
			return(0);
		}
		if (sendmail_parse_from(buf, sender, 0, 0) == 0) {
			mrc->mrc_dead = 1;
			return(0);
		}
	}
	/* must fetch the header and search for From: */
	else {
		strcpy(sender, "unknown");
		hdr = scomsc1_fetch(handle, 0, msg+1, 0);
		if (hdr) {
			for (cp = hdr; *cp; cp = cp1) {
				cp1 = strchr(cp, '\n');
				if (cp1 == 0)
					cp1 = cp + strlen(cp);
				c = *cp1;
				*cp1 = 0;
				if (msc1_strhccmp("From", cp, ':') == 0) {
					cp = strchr(cp, ':') + 1;
					while ((*cp == ' ') || (*cp == '\t'))
						cp++;
					if (*cp)
						msc1_parse_address(sender, cp);
					*cp1 = c;
					break;
				}
				*cp1 = c;
				if (*cp1)
					cp1++;
			}
			free(hdr);
		}
	}
	strcpy(out, sender);
	return(1);
}

/*
 * set a flag
 * value is 1 to set the flag, 0 to clear the flag
 * 1 is returned on success, 0 on failure
 */
int
scomsc1_setflags(void *handle, int msg, int flags)
{
	register mrc_t *mrc;
	register prc_t *prc;
	prc_t ptmp;
	int oldaccess;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(0);
	prc = mrc->mrc_prc + msg;
	if (prc->prc_status == flags)
		return(1);
	/* must have session lock to change flags */
	if ((mrc->mrc_access&ACCESS_SE) == 0)
		return(0);
	oldaccess = mrc->mrc_access;
	/* must grab folder lock to change flags */
	if (msc1_lock(mrc, mrc->mrc_access|ACCESS_BOTH) == 0)
		goto setflagerr;
	/* check file contents for validity */
	if (conf_extended_checks) {
		ptmp = mrc->mrc_prc[msg];
		if (msc1_read_prc(mrc, msg) == 0)
			goto setflagerr;
		if (memcmp(mrc->mrc_prc + msg, &ptmp, sizeof(prc_t)))
			goto setflagerr;
	}
	if (scomsc1_check(mrc) == 0)
		goto setflagerr;
	/* prc can change after check */
	prc = mrc->mrc_prc + msg;
	prc->prc_status = flags;
	if (msc1_status_out(mrc, prc, msg, 0) == 0)
		goto setflagerr;
	/* must write out modified prc entry as well */
	if (msc1_write_prc(mrc, msg) == 0)
		goto setflagerr;
	if (fflush(mrc->mrc_folder_fp) == EOF)
		goto setflagerr;
	if (fflush(mrc->mrc_index_fp) == EOF)
		goto setflagerr;
	if (msc1_set_consistency(mrc) == 0)
		goto setflagerr;
	if (conf_fsync) {
		fsync(fileno(mrc->mrc_folder_fp));
		fsync(fileno(mrc->mrc_index_fp));
	}
	msc1_lock(mrc, oldaccess);
	return(1);
setflagerr:
	msc1_lock(mrc, oldaccess);
	mrc->mrc_dead = 1;
	return(0);
}

/*
 * get the flags for a message
 * returns flags or -1 for error
 */
int
scomsc1_getflags(void *handle, int msg)
{
	register mrc_t *mrc;
	register prc_t *prc;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(-1);
	msg--;
	if ((msg < 0) || (msg >= mrc->mrc_msgs))
		return(-1);
	prc = mrc->mrc_prc + msg;
	return(prc->prc_status&~FLAGS_INTERNAL);
}

/*
 * this is our IMAP-like expunge routine
 * just take all messages that are flagged as deleted
 * and really remve them from the mailbox
 *
 * lock the master record so we can change the index
 * we do not change any offsets in the folder, but the
 * index is rebuilt.
 *
 * we mark messages as deleted by clobbering the header
 * then we renumber the messages
 */
int
scomsc1_expunge(void *handle)
{
	register prc_t *prc;
	register mrc_t *mrc;
	int oldaccess;
	int src;		/* source msg */
	int dst;		/* destination msg */
	int msg;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	if ((mrc->mrc_access&ACCESS_SE) == 0)
		return(0);
	oldaccess = mrc->mrc_access;
	/* if lock fails then just don't expunge */
	if (msc1_lock(mrc, mrc->mrc_access|ACCESS_REBUILD) == 0)
		return(0);
	/* after lock make sure we our in core copy is up to date */
	if (scomsc1_check(mrc) == 0) {
		msc1_lock(mrc, oldaccess);
		return(0);
	}
	mrc->mrc_oldaccess = oldaccess;
	mrc->mrc_status |= MSC1_UPDATE;
	if (msc1_write_mrc(mrc) == 0) {
		mrc->mrc_dead = 1;
		return(0);
	}

	for (msg = 0; msg < mrc->mrc_msgs; msg++) {
		prc = mrc->mrc_prc + msg;
		if ((prc->prc_status&fDELETED) == 0)
			continue;
		if (fseek(mrc->mrc_folder_fp, prc->prc_start, SEEK_SET) < 0)
			goto expunge_err;
		if (fputs(DELETED_EFROM, mrc->mrc_folder_fp) == EOF)
			goto expunge_err;
		if (fputs(DELETED_FROM, mrc->mrc_folder_fp) == EOF)
			goto expunge_err;
		if (fputs(DELETED_SUBJECT, mrc->mrc_folder_fp) == EOF)
			goto expunge_err;
		/* X prevents exposing >From as From (unlikely but possible) */
		if (fputs("\nX", mrc->mrc_folder_fp) == EOF)
			goto expunge_err;
		if (fflush(mrc->mrc_folder_fp) == EOF)
			goto expunge_err;
	}

	/* kill prc entries in core and write to disk */
	dst = 0;
	for (src = 0; src < mrc->mrc_msgs; src++) {
		if (mrc->mrc_prc[src].prc_status&fDELETED)
			continue;

		if (src != dst) {
			/* copy one down */
			memcpy(mrc->mrc_prc + dst, mrc->mrc_prc + src,
				sizeof(prc_t));
		}
		dst++;
	}
	mrc->mrc_msgs = dst;
	if (msc1_write_index(mrc) == 0)
		goto expunge_err;
	/* truncate file */
	if (msc1_expand_index(mrc) == 0)
		goto expunge_err;
	/* expunge gets it's own locks so we must keep the FOLDER lock */
	if (msc1_expunge(mrc) == 0)
		goto expunge_err;
	mrc->mrc_status &= ~MSC1_UPDATE;
	if (msc1_set_consistency(mrc) == 0)
		goto expunge_err;
	if (msc1_lock(mrc, mrc->mrc_oldaccess) == 0) {
		mrc->mrc_dead = 1;
		return(0);
	}
	if (conf_fsync) {
		fsync(fileno(mrc->mrc_index_fp));
		fsync(fileno(mrc->mrc_folder_fp));
	}
	return(1);
expunge_err:
	mrc->mrc_dead = 1;
	msc1_lock(mrc, mrc->mrc_oldaccess);
	return(0);
}

/*
 * Wrapper to scomsc1_deliverf for incore messages that come in
 * from mail_append and mail_copy.
 * Put message into unlinked tmp file in MAILSPOOL and call deliverf.
 * We use MAILSPOOL as the tmp location because /tmp is a tiny ramdisk.
 * Leading "From " line can be sent to this routine, but it is ignored.
 */
int
scomsc1_deliver(char *path, char *sender, char *date, char *message, int messagelen, int flags)
{
	char tmpf[80];
	long time();
	FILE *fp;
	int ret;

	sprintf(tmpf, "%s/%d.%d", MAILSPOOL, getpid(), time(0));
	fp = fopen(tmpf, "w+");
	if (fp == 0)
		return(0);
	unlink(tmpf);
	if (fwrite(message, 1, messagelen, fp) != messagelen) {
		fclose(fp);
		return(0);
	}
	rewind(fp);
	ret = scomsc1_deliverf(path, sender, date, fp, flags);
	fclose(fp);
	return(ret);
}

/*
 * deliver (append) a message to a given folder name
 * optimized for speed, does not do a rebuild
 * if the index is there and valid it will be used, otherwise
 * an old fashioned textual append will be done.
 * we depend on the input file being a real file to stat it's size.
 * returns 1 - success, 0 - error
 */
int
scomsc1_deliverf(char *path, char *sender, char *date, FILE *mfp, int flags)
{
	register mrc_t *mrc;
	static char *buf;
	int buflen;
	prc_t prc;
	int first;
	int fd;
	int newaccess;
	int hdrlen;
	int bodylen;
	int elen;		/* envelope line len */
	int clen;		/* size of content-length line */
	char uid[25];
	char cont[80];
	struct stat sbuf;
	int mmdf;
	int old_msgs;
	off_t old_fsize;
	int addnewline;
	int perm;
	int i;
	int c;
	int ret;

	msc1_init(0);
	mrc = malloc(sizeof(mrc_t));
	memset(mrc, 0, sizeof(mrc_t));
	if (mrc == 0)
		return(0);
	if (buf == 0)
		buf = malloc(MSCBUFSIZ);
	if (buf == 0) {
		free(mrc);
		return(0);
	}
	msc1_init_mrc(mrc, path);

	if (msc1_strccmp(path, "INBOX") == 0)
		ret = msc1_create_folder(mrc, ACCESS_AP|ACCESS_BOTH,
					SCOMS_INBOX_PERM&conf_umask);
	else
		ret = msc1_create_folder(mrc, ACCESS_AP|ACCESS_BOTH,
					SCOMS_PERM&conf_umask);
	if (ret == 0)
		goto deliver_err;

	/* stat folder for size */
	if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0)
		goto deliver_err;

	/* check if main mailbox folder is valid */
	if ((mmdf = msc1_folder_valid(mrc->mrc_folder_fp)) == 0)
		goto deliver_err;
	mrc->mrc_mmdf = mmdf - 1;

	/* read index file mr only */
	if (msc1_read_mrc(mrc) == 0) {
		/* index is bad */
		if (sbuf.st_size) {
			/* set simple delivery if index is bad */
			fclose(mrc->mrc_index_fp);
			mrc->mrc_index_fp = 0;
		}
		else {
			/* rebuild index if first message */
			/* uid_next is preserved if it was there */
			if (mrc->mrc_uid_next == 0)
				mrc->mrc_uid_next = 1;
			if (scomsc1_rebuild_callback)
				(*scomsc1_rebuild_callback)();
		}
	}
	/* hand init the mrc if simple delivery */
	if (mrc->mrc_index_fp == 0) {
		mrc->mrc_fsize = sbuf.st_size;
		mrc->mrc_uid_next = 1;
	}
	old_msgs = mrc->mrc_msgs;
	old_fsize = mrc->mrc_fsize;

	/* start with the message header, copy it stripping and padding */
	memset(&prc, 0, sizeof(prc_t));
	if (fseek(mrc->mrc_folder_fp, mrc->mrc_fsize, SEEK_SET) < 0)
		goto deliver_err;
	if (mrc->mrc_mmdf) {
		if (fwrite(MMDFSTR, 1, MMDFSIZ, mrc->mrc_folder_fp) != MMDFSIZ)
			goto deliver_terr;
	}
	if (fprintf(mrc->mrc_folder_fp, "From %s %s\n", sender, date) < 0)
		goto deliver_terr;
	hdrlen = 0;
	first = 0;
	while (buflen = msc1_fgets(buf, MSCBUFSIZ, mfp)) {
		/* end of header */
		if ((buflen == 1) && (buf[0] == '\n')) {
			/* eat blank line, put it back later */
			break;
		}
		if ((msc1_strhccmp(STATUS_STR, buf, ':') == 0) ||
			(msc1_strhccmp(XSTATUS_STR, buf, ':') == 0) ||
			(msc1_strhccmp(PAD_STR, buf, ':') == 0) ||
			(msc1_strhccmp(CONT_STR, buf, ':') == 0) ||
			(msc1_strhccmp(UID_STR, buf, ':') == 0)) {
			/* skip to next line */
			first = 1;
			continue;
		}
		else if (strncmp(buf, "From ", 5) == 0) {
			/* eat leading From line if it is there */
			if (first == 0)
				continue;
			if (mrc->mrc_mmdf == 0) {
				/* pad other From lines if sendmail */
				if (fputc('>', mrc->mrc_folder_fp) == EOF)
					goto deliver_terr;
				hdrlen++;
			}
		}
		if (fwrite(buf, 1, buflen, mrc->mrc_folder_fp) != buflen)
			goto deliver_terr;
		prc.prc_hdrlines++;
		hdrlen += buflen;
		first = 1;
	}

	/* have header, now discover body size */
	if (fstat(fileno(mfp), &sbuf) < 0)
		goto deliver_err;
	bodylen = ftell(mfp);
	bodylen = sbuf.st_size - bodylen;

	/* hdrlen includes the blank line */

	/* body must be terminated by a newline */
	addnewline = 0;
	if (bodylen) {
		i = ftell(mfp);
		if (fseek(mfp, -1, SEEK_END) < 0)
			goto deliver_terr;
		if ((c = fgetc(mfp)) == EOF)
			goto deliver_terr;
		/* add newline if needed */
		if (c != '\n') {
			addnewline = 1;
			bodylen++;
		}
		/* restore input pointer for body */
		if (fseek(mfp, i, SEEK_SET) < 0)
			goto deliver_terr;
	}

	/* calculate prc values */
	/* strlen(From sender date\n) */
	elen = strlen(sender) + strlen(date) + 7;
	prc.prc_start = mrc->mrc_fsize + (mrc->mrc_mmdf ? MMDFSIZ : 0);
	prc.prc_hdrstart = prc.prc_start + elen;
	prc.prc_size = hdrlen + elen + STATUS_SIZ + XSTATUS_SIZ + PAD_SIZ
		+ STAT_RESERVE + 9;
	sprintf(uid, "%s: %d\n", UID_STR, mrc->mrc_uid_next);
	prc.prc_size += strlen(uid);
	sprintf(cont, "%s: %d\n", CONT_STR, bodylen);
	clen = strlen(cont);
	prc.prc_size += clen;
	prc.prc_size++;		/* blank line for end of header */
	prc.prc_stat_start = prc.prc_start + hdrlen + elen;
	prc.prc_content_len = 1;
	prc.prc_bodystart = prc.prc_start + prc.prc_size;
	prc.prc_uid = mrc->mrc_uid_next;
	prc.prc_hdrlines += 5;	/* three status lines, Content-Len, blank */
	msc1_date_sendmail_to_prc(&prc, date);
	prc.prc_size += bodylen;

	prc.prc_status = flags;		/* initial flags */
	if (msc1_status_out(mrc, &prc, mrc->mrc_msgs, 0) == 0)
		goto deliver_terr;

	/* only output uid header if full delivery */
	if (mrc->mrc_index_fp) {
		prc.prc_hdrlines++;
		if (fprintf(mrc->mrc_folder_fp, "%s", uid) < 0)
			goto deliver_terr;
	}

	if (fprintf(mrc->mrc_folder_fp, "%s", cont) < 0)
		goto deliver_terr;

	if (fprintf(mrc->mrc_folder_fp, "\n") < 0)
		goto deliver_terr;
	/* end of header */

	/* write message body */
	while (buflen = msc1_fgets(buf, MSCBUFSIZ, mfp)) {
		if (fwrite(buf, 1, buflen, mrc->mrc_folder_fp) != buflen)
			goto deliver_terr;
		prc.prc_bodylines++;
	}
	if (addnewline) {
		if (fputc('\n', mrc->mrc_folder_fp) == EOF)
			goto deliver_terr;
		prc.prc_bodylines++;
	}
	if (mrc->mrc_mmdf) {
		if (fwrite(MMDFSTR, 1, MMDFSIZ, mrc->mrc_folder_fp) != MMDFSIZ)
			goto deliver_terr;
	}
	if (fflush(mrc->mrc_folder_fp) == EOF)
		goto deliver_terr;

	/* write prc and updated mrc */
	if (mrc->mrc_index_fp) {
		mrc->mrc_fsize += prc.prc_size + (mrc->mrc_mmdf ? MMDFSIZ*2 : 0);
		if (msc1_write_prcp(mrc, &prc, mrc->mrc_msgs) == 0)
			goto deliver_terr;
		mrc->mrc_msgs++;
		mrc->mrc_uid_next++;
		if (msc1_set_consistency(mrc) == 0)
			goto deliver_terr;
	}
	if (conf_fsync) {
		fsync(fileno(mrc->mrc_index_fp));
		fsync(fileno(mrc->mrc_folder_fp));
	}
	msc1_close(mrc);
	free(buf);
	buf = 0;
	return(1);
/* truncate folder back as deliver failed */
deliver_terr:
	mrc->mrc_msgs = old_msgs;
	mrc->mrc_fsize = old_fsize;
	msc1_expand_folder(mrc);
	if (mrc->mrc_index_fp) {
		msc1_expand_index(mrc);
		msc1_set_consistency(mrc);
		if (conf_fsync)
			fsync(fileno(mrc->mrc_index_fp));
	}
	if (conf_fsync) {
		fsync(fileno(mrc->mrc_folder_fp));
	}
deliver_err:
	msc1_close(mrc);
	free(buf);
	buf = 0;
	return(0);
}

/*
 * check if new mail received, return updated stats on mailbox
 * 0 only if fatal error
 */
scomsc1_stat_t *
scomsc1_check(void *handle)
{
	register mrc_t *mrc;
	mrc_t nmrc;
	prc_t *prc;
	static scomsc1_stat_t folder_stat;
	int i;
	int err;
	struct stat sbuf;
	int newaccess;

	mrc = (mrc_t *)handle;
	if (mrc->mrc_dead)
		return(0);
	mrc->mrc_coldaccess = mrc->mrc_access;
	if (mrc->mrc_index_fp)
		newaccess = ACCESS_BOTH;
	else
		newaccess = ACCESS_FOLDER;
	newaccess |= mrc->mrc_access;
	if (msc1_lock(mrc, newaccess) == 0) {
		/* if could not get lock then just don't update */
		goto check_nolock;
	}

	err = 0;
	nmrc = *mrc;
	/* read-only access with no index is different */
	if ((mrc->mrc_access&ACCESS_RD) && (mrc->mrc_index_fp == 0)) {
		if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0) {
			err = 1;
			goto check_done;
		}
		if (mrc->mrc_fsize > sbuf.st_size) {
			err = 1;
			goto check_done;
		}
		if (msc1_rebuild(mrc, 0, 1) == 0)
			err = 1;
		goto check_done;
	}
	/* read in new index data */
	/*
	 * if mrc bad then probably a non-MS compatible delivery occurred
	 * treat like read-only append, but do a rebuild
	 */
	if (msc1_read_mrc(&nmrc) == 0) {
		if (msc1_rebuild(mrc, 0, 1) == 0)
			err = 1;
		goto check_done;
	}
	if (nmrc.mrc_msgs <= mrc->mrc_msgs)
		goto check_done;

	/* got some new ones */
	if (mrc->mrc_psize < nmrc.mrc_msgs) {
		nmrc.mrc_prc = malloc(sizeof(prc_t)*nmrc.mrc_msgs);
		nmrc.mrc_psize = nmrc.mrc_msgs;
		if (nmrc.mrc_prc == 0) {
			err = 1;
			goto check_done;
		}
		memcpy(nmrc.mrc_prc, mrc->mrc_prc, sizeof(prc_t)*mrc->mrc_msgs);
	}
	for (i = mrc->mrc_msgs; i < nmrc.mrc_msgs; i++)
		if (msc1_read_prc(&nmrc, i) == 0)
			goto check_done;
	/* got them, swap prc arrays if needed so free works later */
	if (nmrc.mrc_prc != mrc->mrc_prc) {
		prc = nmrc.mrc_prc;
		nmrc.mrc_prc = mrc->mrc_prc;
		mrc->mrc_prc = prc;
		mrc->mrc_psize = nmrc.mrc_psize;
	}
	mrc->mrc_msgs = nmrc.mrc_msgs;
	mrc->mrc_fsize = nmrc.mrc_fsize;
	mrc->mrc_uid_next = nmrc.mrc_uid_next;

check_done:
	if (nmrc.mrc_prc && (nmrc.mrc_prc != mrc->mrc_prc))
		free(nmrc.mrc_prc);
	msc1_lock(mrc, mrc->mrc_coldaccess);
	if (err) {
		mrc->mrc_dead = 1;
		return(0);
	}
check_nolock:
	folder_stat.m_validity = mrc->mrc_validity;
	folder_stat.m_msgs = mrc->mrc_msgs;
	return(&folder_stat);
}

/*
 * create a folder or a directory
 * must create the folder and index, lock them, then write the index
 * zero length folder is created.
 * return 1 successful, 0 failed
 */
int
scomsc1_create(char *path)
{
	int fd;
	struct stat sbuf;
	register mrc_t *mrc;
	int i;
	int ret;
	char *realpath;
	int perm;

	msc1_init(0);
	if (msc1_strccmp("INBOX", path) == 0)
		realpath = conf_inbox;
	else
		realpath = path;
	/* fail if file/dir exists */
	if (stat(realpath, &sbuf) == 0)
		return(0);
	/* check if directory */
	i = strlen(realpath) - 1;
	if (realpath[i] == '/') {
		realpath[i] = 0;
		ret = mkdir(realpath, SCOMS_DPERM&conf_umask);
		realpath[i] = '/';
		return((ret < 0) ? 0 : 1);
	}
	/* a folder */
	perm = SCOMS_PERM;
	/* restrict permissions if inbox */
	if (msc1_strccmp(path, "INBOX") == 0)
		perm = SCOMS_INBOX_PERM;
	perm &= conf_umask;
	fd = nolink_open(realpath, O_CREAT|O_RDWR|O_EXCL, perm);
	if (fd < 0)
		return(0);
	close(fd);
	/* try to create it */
	mrc = scomsc1_open(realpath, ACCESS_SE, 0);
	if ((mrc == 0) || ((int)mrc == 1) || ((int)mrc == 2))
		return(0);
	scomsc1_close(mrc);
	return(1);
}

/*
 * remove a folder, open the folder, get all locks, then unlink and close
 * returns 1 successful, 0 fail
 */
int
scomsc1_remove(char *path)
{
	register mrc_t *mrc;
	struct stat sbuf;
	int i;
	int ret;
	char *realpath;

	msc1_init(0);
	if (msc1_strccmp("INBOX", path) == 0)
		realpath = conf_inbox;
	else
		realpath = path;
	/* check if directory */
	if (stat(realpath, &sbuf))
		return(0);
	if (S_ISDIR(sbuf.st_mode)) {
		ret = rmdir(realpath);
		return((ret < 0) ? 0 : 1);
	}
	/* a folder */
	mrc = malloc(sizeof(mrc_t));
	if (mrc == 0)
		return(0);
	msc1_init_mrc(mrc, realpath);
	ret = msc1_open_folder(mrc, ACCESS_REBUILD);
	if (ret == 0)
		return(0);
	if (unlink(mrc->mrc_path)) {
		msc1_close(mrc);
		return(0);
	}
	if (unlink(mrc->mrc_ipath)) {
		msc1_close(mrc);
		return(0);
	}
	msc1_close(mrc);
	return(1);
}

/*
 * rename a folder, open it, get all locks, then rename (via link and unlink)
 * both links are created before unlink is called to allow for backout
 */
int
scomsc1_rename(char *new, char *old)
{
	struct stat sbuf;
	mrc_t *mrc;
	mrc_t nmrc;
	char *realnew;
	char *realold;
	int ret;

	msc1_init(0);
	if (msc1_strccmp("INBOX", new) == 0)
		realnew = conf_inbox;
	else
		realnew = new;
	if (msc1_strccmp("INBOX", old) == 0)
		realold = conf_inbox;
	else
		realold = old;
	/* check if same */
	if (strcmp(realnew, realold) == 0)
		return(0);
	/* check if directory */
	if (stat(realold, &sbuf))
		return(0);
	if (S_ISDIR(sbuf.st_mode)) {
		/* new cannot exist */
		if (stat(realnew, &sbuf) == 0)
			return(0);
		ret = rename(realold, realnew);
		return((ret < 0) ? 0 : 1);
	}
	/* it is a folder, is it valid */
	mrc = malloc(sizeof(mrc_t));
	if (mrc == 0)
		return(0);
	msc1_init_mrc(mrc, realold);
	ret = msc1_open_folder(mrc, ACCESS_REBUILD);
	if (ret == 0)
		return(0);
	msc1_init_mrc(&nmrc, realnew);
	/* use links instead of renames so we can back out on failure */
	if (link(mrc->mrc_path, nmrc.mrc_path)) {
		/*
		 * if the failure is because they are not on the same.
		 * filesystem then a copy will succeed.
		 */
		if (msc1_fcopy(nmrc.mrc_path, mrc->mrc_path) == 0) {
			msc1_close(mrc);
			return(0);
		}
		if (msc1_fcopy(nmrc.mrc_ipath, mrc->mrc_ipath) == 0) {
			unlink(nmrc.mrc_path);
			msc1_close(mrc);
			return(0);
		}
	}
	else if (link(mrc->mrc_ipath, nmrc.mrc_ipath)) {
		unlink(nmrc.mrc_path);
		msc1_close(mrc);
		return(0);
	}
	/* committed at this point */
	unlink(mrc->mrc_path);
	unlink(mrc->mrc_ipath);
	msc1_close(mrc);
	return(1);
}

/* test API routines */

/* we need to know during testing if a rebuild occurred */
void
scomsc1_set_rebuild_callback(void(*entry)())
{
	scomsc1_rebuild_callback = entry;
}

/* end of exposed API routines */

/*
 * files are closed, resources are freed, locks are removed
 */
void
msc1_close(mrc_t *mrc)
{
	if (mrc == 0)
		return;
	if (mrc->mrc_access)
		msc1_lock(mrc, 0);
	if (mrc->mrc_index_fp)
		fclose(mrc->mrc_index_fp);
	if (mrc->mrc_folder_fp)
		fclose(mrc->mrc_folder_fp);
	if (mrc->mrc_temp_fp) {
		fclose(mrc->mrc_temp_fp);
		unlink(mrc->mrc_tpath);
	}
	if (mrc->mrc_prc)
		free(mrc->mrc_prc);
	free(mrc);
	return;
}

void
msc1_abort(mrc_t *mrc)
{
	mrc->mrc_access = 0;
	if (mrc->mrc_folder_fp)
		fclose(mrc->mrc_folder_fp);
	if (mrc->mrc_index_fp)
		fclose(mrc->mrc_index_fp);
	if (mrc->mrc_temp_fp)
		fclose(mrc->mrc_temp_fp);
	mrc->mrc_folder_fp = 0;
	mrc->mrc_index_fp = 0;
	mrc->mrc_temp_fp = 0;
}

/*
 * Change lock status, on failure old lock status is preserved if possible.
 * Some locks fail immediately if the blocking lock is not a temporary lock.
 * These include SESSION and RDX because SESSION and RD are long term locks.
 * Temporary locks are AP and FOLDER.
 * Long term locks are SESSION and RD.
 * returns 0 - successful, 1 - fail
 */
int
msc1_lock(mrc_t *mrc, int newaccess)
{
	int oldaccess;
	int i;
	int wr;

	wr = (newaccess&ACCESS_WRITE);
	oldaccess = mrc->mrc_access;
	for (i = 0; i < conf_lock_timeout; i++) {
		mrc->mrc_access = msc1_lock_work(mrc, newaccess, wr);
		if (mrc->mrc_access == newaccess)
			return(1);
		/* fail immediately if ACCESS_SE is requested but fails */
		if ((newaccess&ACCESS_SE) && ((mrc->mrc_access&ACCESS_SE) == 0))
			break;
		/* fail immediately if ACCESS_RDX is requested but fails */
		if ((newaccess&ACCESS_RDX) && ((mrc->mrc_access&ACCESS_RDX) == 0))
			break;
		sleep(1);
	}
	/* reset to old lock status */
	mrc->mrc_access = msc1_lock_work(mrc, oldaccess, wr);
	return(0);
}

/*
 * lock work routine, called multiple times until timeout for write locks
 * accepts the mask of new locks requested.
 * returns mask of locks actually achieved.
 */
int
msc1_lock_work(mrc_t *mrc, int newaccess, int wr)
{
	struct flock lock;
	int ret;
	int set;
	int mask;
	int fd;

	mask = mrc->mrc_access;
	/* change of session lock status */
	if ((newaccess&ACCESS_SE) ^ (mrc->mrc_access&ACCESS_SE)) {
		set = (newaccess&ACCESS_SE) ? F_WRLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 0;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_folder_fp), F_SETLK, &lock);
		/* lock/unlock the file if it is non-zero length */
		/* now deal with folder.lock */
		if ((ret == 0) && (conf_file_locking)) {
			if (newaccess&ACCESS_SE) {
				fd = nolink_open(mrc->mrc_lpath,
					O_CREAT|O_EXCL|O_RDONLY, 0600);
				/* lock failed, release kernel locks */
				if (fd < 0) {
					lock.l_type = F_UNLCK;
					fcntl(fileno(mrc->mrc_folder_fp),
						F_SETLK, &lock);
					ret = -1;
				}
				else
					close(fd);
			}
			else {
				unlink(mrc->mrc_lpath);
			}
		}
		/* always sucessful on unlock */
		if (set == F_UNLCK)
			mask &= ~ACCESS_SE;
		else if (ret == 0)
			mask |= ACCESS_SE;
	}
	/* change of append lock status */
	if ((newaccess&ACCESS_AP) ^ (mrc->mrc_access&ACCESS_AP)) {
		set = (newaccess&ACCESS_AP) ? F_WRLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 1;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_folder_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_AP;
		else if (ret == 0)
			mask |= ACCESS_AP;
	}
	/* change of folder lock status */
	if ((newaccess&ACCESS_FOLDER) ^ (mrc->mrc_access&ACCESS_FOLDER)) {
		if (wr)
			set = (newaccess&ACCESS_FOLDER) ? F_WRLCK : F_UNLCK;
		else
			set = (newaccess&ACCESS_FOLDER) ? F_RDLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 3;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_folder_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_FOLDER;
		else if (ret == 0)
			mask |= ACCESS_FOLDER;
	}
	/* change of index lock status */
	if ((newaccess&ACCESS_INDEX) ^ (mrc->mrc_access&ACCESS_INDEX)) {
		if (wr)
			set = (newaccess&ACCESS_INDEX) ? F_WRLCK : F_UNLCK;
		else
			set = (newaccess&ACCESS_INDEX) ? F_RDLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 3;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_index_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_INDEX;
		else if (ret == 0)
			mask |= ACCESS_INDEX;
	}
	/* change of read exclusive lock status */
	if ((newaccess&ACCESS_RDX) ^ (mrc->mrc_access&ACCESS_RDX)) {
		set = (newaccess&ACCESS_RDX) ? F_WRLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 2;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_folder_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_RDX;
		else if (ret == 0)
			mask |= ACCESS_RDX;
	}
	/* change of read lock status */
	if ((newaccess&ACCESS_RD) ^ (mrc->mrc_access&ACCESS_RD)) {
		set = (newaccess&ACCESS_RD) ? F_RDLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 2;
		lock.l_len = 1;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_folder_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_RD;
		else if (ret == 0)
			mask |= ACCESS_RD;
	}
	/* change of tmp lock status */
	/* note, tmp lock must mimic ACCESS_REBUILD */
	if ((newaccess&ACCESS_TMP) ^ (mrc->mrc_access&ACCESS_TMP)) {
		set = (newaccess&ACCESS_TMP) ? F_WRLCK : F_UNLCK;
		lock.l_type = set;
		lock.l_whence = 0;
		lock.l_start = 0;
		lock.l_len = 4;
		lock.l_sysid = 0;
		lock.l_pid = 0;
		ret = fcntl(fileno(mrc->mrc_temp_fp), F_SETLK, &lock);

		if (set == F_UNLCK)
			mask &= ~ACCESS_TMP;
		else if (ret == 0)
			mask |= ACCESS_TMP;
	}
	return(mask);
}

/*
 * the tmp folder has been renamed to the folder.
 * we had a tmp lock now convert it to a folder lock
 * our locking strategy makes a tmp lock look like ACCESS_REBUILD
 * all we have to do is clear the ACCESS_TMP bit
 */
void
msc1_lock_tmp_to_folder(mrc_t *mrc)
{
	mrc->mrc_access &= ~(ACCESS_TMP);
	return;
}


/*
 * set old flag on all messages, only the on-disk copy
 * only for ACCESS_SE
 */
int
msc1_set_old(register mrc_t *mrc)
{
	register prc_t *prc;
	prd_t prd;
	int i;
	int old;

	old = 0;
	if (mrc->mrc_dead)
		return(0);
	for (i = 0; i < mrc->mrc_msgs; i++) {
		prc = mrc->mrc_prc + i;
		if (prc->prc_status&fOLD)
			continue;
		old = 1;
		prc->prc_status |= fOLD;
		if (msc1_status_out(mrc, prc, i, 0) == 0) {
			mrc->mrc_dead = 1;
			return(0);
		}
		if (msc1_write_prc(mrc, i) == 0) {
			mrc->mrc_dead = 1;
			return(0);
		}
	}
	if (old) {
		if (fflush(mrc->mrc_index_fp) == EOF) {
			mrc->mrc_dead = 1;
			return(0);
		}
		if (fflush(mrc->mrc_folder_fp) == EOF) {
			mrc->mrc_dead = 1;
			return(0);
		}
	}
	return(1);
}

/*
 * check if the folder contents are valid, just check first line or two
 * full parse done later will catch any errors not detected here
 * it can confuse the listings of valid folders, but that is all.
 * a zero length file is considered to be a valid folder
 * returns zero not valid
 * returns 1 for sendmail, two if MMDFSTR was detected
 */
int
msc1_folder_valid(FILE *fp)
{
	char buf[MSCSBUFSIZ];
	struct stat sbuf;
	int fd;
	int mmdf;

	if (fstat(fileno(fp), &sbuf) < 0) {
		errno = ENOENT;
		return(0);
	}
	/* not a regular file */
	if (!S_ISREG(sbuf.st_mode)) {
		errno = EINVAL;
		return(0);
	}
	/* zero length files ok */
	if (sbuf.st_size == 0)
		return(conf_mmdf ? 2 : 1);

	buf[0] = 0;
	rewind(fp);
	fgets(buf, MSCSBUFSIZ, fp);
	/* MMDF check */
	mmdf = 1;
	if (strcmp(buf, MMDFSTR) == 0) {
		buf[0] == 0;
		fgets(buf, MSCSBUFSIZ, fp);
		mmdf = 2;
		/* don't check lines that are not From */
		if (strncmp(buf, "From ", 5))
			return(mmdf);
	}
	/* check for From line */
	if (sendmail_parse_from(buf, 0, 0, 0))
		return(mmdf);
	errno = EINVAL;
	return(0);
}

/*
 * read index file into core, rebuild it if necessary
 * must get all locks if rebuilding, expects folder and index locks on entry.
 */
int
msc1_index(register mrc_t *mrc, int convert)
{
	int t;
	int ret;

	/* do validation checking */
	if ((msc1_read_index(mrc) == 0) || convert) {
		/* rebuild on write access, build an incore index on read */
		t = mrc->mrc_access;
		if ((mrc->mrc_access&ACCESS_WRITE)) {
			/* upgrade lock to rebuild */
			if (msc1_lock(mrc, ACCESS_REBUILD) == 0)
				return(0);
		}
		else {
			/* we are building a new read-only in-core only index */
			/* must close the old bad folder fp if it is there */
			if (mrc->mrc_index_fp) {
				fclose(mrc->mrc_index_fp);
				mrc->mrc_index_fp = 0;
			}
		}

		/* rebuild index and (on write) the folder */
		ret = msc1_rebuild(mrc, convert, 0);

		/* put locks back to user request */
		if (msc1_lock(mrc, t) == 0)
			return(0);

		if (ret == 0)
			return(0);
	}
	return(1);
}

/*
 * msc1_rebuild
 * rebuild index file and if necessary the folder file
 * both files are expected to be locked before this routine is called
 *
 * a simple algorithm is used here:
 * we copy the folder file to the temp file a message at
 * a time, reformatting headers and building the index as we go.
 * folder format conversion is also performed (adding or deleting ctrl-A's).
 * To preserve locks we lock the new folder and move it into place
 * while keeping it open to preserve the lock.
 *
 * mrc structure is assumed to be initialized (mostly to zero)
 * an fsm is used to parse the file
 *
 * additionally, if the folder is opened read only, then only an in-core
 * index is built and no files are modified.
 *
 * later we added the append flag, it is used for read-only files
 * that have no index so the new file contents needs to be parsed.
 * It is also used to read in new messages if the appender did not update
 * the index (backwards compatible mail delivery agents).
 * In the non MS compatible appender mode, only the new messages
 * are copied to the temp file, and are copied back to the original
 * folder.
 */
/* fsm input types */
#define I_REG		0	/* regular line */
#define I_BLANK		1	/* blank line */
#define I_MMDF		2	/* four ctrl-A's and a newline */
#define I_FROM		3	/* sendmail From header line */
#define I_DELE		4	/* found a delete from line */
#define I_STAT		5	/* Status: header line */
#define I_XSTAT		6	/* X-Status: header line */
#define I_PAD		7	/* X-SCO-Pad: header line */
#define I_UID		8	/* X-SCO-UID: header line */
#define I_DATE		9	/* Date: header */
#define I_EOF		10	/* end of file */
#define I_COUNT		11	/* input count */
#define I_CONT		11	/* pseudo input for Content-Length checking */
/* fsm states */
#define S_START		0	/* start state */
#define S_HDR		1	/* initial header state */
#define S_SRCH		2	/* inside header state */
#define S_BODY		3	/* in body of state */
#define S_DELE		4	/* found a deleted message to skip */
#define S_COUNT		5	/* count of states */
#define S_DONE		5	/* magic state that means end of fsm */
/* fsm outputs */
#define O_ERR		0	/* parse error */
#define O_NULL		1	/* do nothing */
#define O_SOM		2	/* start of message */
#define O_NSOM		3	/* start of message */
#define O_OUT		4	/* simple output line */
#define O_EOH		5	/* end of header */
#define O_EOB		6	/* end of both (eom within header) */
#define O_PAD		7	/* pad From line to >From */
#define O_SAVES		8	/* save Status: header */
#define O_SAVEX		9	/* save X-Status: header */
#define O_SAVEU		10	/* save UID: header */
#define O_SAVED		11	/* save date header */
#define O_EOM		12	/* End of message */
#define O_LOOP		13	/* repeat token scan of SOM or DELE */
/* fsm tables: mmdf are for MMDF input folders, others are for sendmail input */
unsigned char msc1_fsm_mmdf_output[I_COUNT][S_COUNT] = {
/*		START	HDR	SRCH	BODY	DELETED */
/*REG  */{	O_ERR,	O_NSOM,	O_OUT,	O_OUT,	O_NULL,	},
/*BLANK*/{	O_ERR,	O_ERR,	O_EOH,	O_OUT,	0,	},
/*MMDF */{	O_NULL,	O_ERR,	O_EOB,	O_EOM,	O_NULL,	},
/*FROM */{	O_ERR,	O_SOM,	O_PAD,	O_OUT,	O_NULL,	},
/*DELET*/{	O_ERR,	O_NULL,	O_PAD,	O_OUT,	O_NULL,	},
/*STAT */{	O_ERR,	O_NSOM,	O_SAVES,O_ERR,	0,	},
/*XSTAT*/{	O_ERR,	O_NSOM,	O_SAVEX,O_ERR,	0,	},
/*PAD  */{	O_ERR,	O_NSOM,	O_NULL,	O_ERR,	0,	},
/*UID  */{	O_ERR,	O_NSOM,	O_SAVEU,O_ERR,	0,	},
/*DATE */{	O_ERR,	O_NSOM,	O_SAVED,O_ERR,	0,	},
/*EOF  */{	O_NULL,	O_ERR,	O_EOB,	O_EOM,	O_NULL,	},
};
unsigned char msc1_fsm_mmdf_next[I_COUNT][S_COUNT] = {
/*		START	HDR	SRCH	BODY	DELETED */
/*REG  */{	0,	S_SRCH,	S_SRCH,	S_BODY,	S_DELE,	},
/*BLANK*/{	0,	0,	S_BODY,	0,	0,	},
/*MMDF */{	S_HDR,	0,	S_START,S_START,S_START,},
/*FROM */{	0,	S_SRCH,	S_SRCH,	S_BODY,	S_DELE,	},
/*DELET*/{	0,	S_DELE,	S_SRCH,	S_BODY,	S_DELE,	},
/*STAT */{	0,	S_SRCH,	S_SRCH,	0,	0,	},
/*XSTAT*/{	0,	S_SRCH,	S_SRCH,	0,	0,	},
/*PAD  */{	0,	S_SRCH,	S_SRCH,	0,	0,	},
/*UID  */{	0,	S_SRCH,	S_SRCH,	0,	0,	},
/*DATE */{	0,	S_SRCH,	S_SRCH,	0,	0,	},
/*EOF  */{	S_DONE,	0,	S_DONE,	S_DONE,	S_DONE,	},
};
unsigned char msc1_fsm_output[I_COUNT][S_COUNT] = {
/*		START	HDR	SRCH	BODY	DELETED */
/*REG  */{	O_ERR,	0,	O_OUT,	O_OUT,	O_NULL,	},
/*BLANK*/{	O_ERR,	0,	O_EOH,	0,	0,	},
/*MMDF */{	0,	0,	0,	0,	0,	},
/*FROM */{	O_SOM,	0,	O_EOB,	O_EOM,	O_LOOP,	},
/*DELET*/{	O_NULL,	0,	O_EOB,	O_EOM,	O_LOOP,	},
/*STAT */{	O_ERR,	0,	O_SAVES,0,	0,	},
/*XSTAT*/{	O_ERR,	0,	O_SAVEX,0,	0,	},
/*PAD  */{	O_ERR,	0,	O_NULL,	0,	0,	},
/*UID  */{	O_ERR,	0,	O_SAVEU,0,	0,	},
/*DATE */{	O_ERR,	0,	O_OUT,  0,	0,	},
/*EOF  */{	O_NULL,	0,	O_EOB,	O_EOM,	O_NULL,	},
};
unsigned char msc1_fsm_next[I_COUNT][S_COUNT] = {
/*		START	HDR	SRCH	BODY	DELETED */
/*REG  */{	0,	0,	S_SRCH,	S_BODY,	S_DELE,	},
/*BLANK*/{	0,	0,	S_BODY,	0,	0,	},
/*MMDF */{	0,	0,	0,	0,	0,	},
/*FROM */{	S_SRCH,	0,	S_START,S_START,S_START,},
/*DELET*/{	S_DELE,	0,	S_START,S_START,S_START,},
/*STAT */{	0,	0,	S_SRCH,	0,	0,	},
/*XSTAT*/{	0,	0,	S_SRCH,	0,	0,	},
/*PAD  */{	0,	0,	S_SRCH,	0,	0,	},
/*UID  */{	0,	0,	S_SRCH,	0,	0,	},
/*DATE */{	0,	0,	S_SRCH,	0,	0,	},
/*EOF  */{	S_DONE,	0,	S_DONE,	S_DONE,	S_DONE,	},
};

/* comments above, before fsm tables */
int 
msc1_rebuild(register mrc_t *mrc, int convert, int append)
{
	int fd;
	static char *buf;	/* incore message header */
	int buflen;		/* length read */
	char *cp;		/* temp pointer */
	unsigned char *ucp;	/* temp unsigned pointer */
	register prc_t *prc;	/* current message structure */
	int mmdfin;		/* is our input mmdf? */
	int state;		/* fsm state */
	int input;		/* line type */
	int output;		/* fsm output */
	int len;
	int oldlen;
	int oldmsgs;
	int i;
	int nsom;
	struct stat sbuf;
	char date[50];
	MESSAGECACHE elt;
	int content_valid;	/* inside valid content length in body */
	int content_len;	/* found a content-len, not validated yet */
	int folder_len;		/* length of our input folder */

	if (scomsc1_rebuild_callback)
		(*scomsc1_rebuild_callback)();
	if (append == 0) {
		/* check if index file is valid to overwrite */
		if (msc1_ok_to_rebuild_index(mrc) == 0)
			return(0);
	}
	mrc->mrc_org_uid_next = mrc->mrc_uid_next;
	if (mrc->mrc_access&ACCESS_WRITE) {
		fd = nolink_open(mrc->mrc_tpath, O_CREAT|O_RDWR, SCOMS_PERM&conf_umask);
		if (fd < 0)
			return(0);
		mrc->mrc_temp_fp = fdopen(fd, "w+");
		if (msc1_match_perm(mrc->mrc_tpath, mrc->mrc_path) == 0)
			return(0);
		if (msc1_lock(mrc, mrc->mrc_access|ACCESS_TMP) == 0)
			return(0);
	}
	if (buf == 0)
		buf = malloc(MSCBUFSIZ);
	if (buf == 0)
		return(0);
	if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0)
		return(0);
	folder_len = sbuf.st_size;
	if (append == 0) {
		rewind(mrc->mrc_folder_fp);
		if (mrc->mrc_prc)
			free(mrc->mrc_prc);
		mrc->mrc_psize = 128;		/* start at a reasonable size */
		mrc->mrc_prc = malloc(sizeof(prc_t)*mrc->mrc_psize);
		if (mrc->mrc_prc == 0)
			return(0);
		/* mmdf input check */
		buf[0] = 0;
		fgets(buf, MSCBUFSIZ, mrc->mrc_folder_fp);
		if (buf[0]) {
			if (strcmp(buf, MMDFSTR) == 0)
				mmdfin = 1;
			else
				mmdfin = 0;
		}
		else {
			/* zero length folders can be either type */
			mmdfin = conf_mmdf;
		}
		/* auto-convert option */
		mrc->mrc_mmdf = convert ? conf_mmdf : mmdfin;
		rewind(mrc->mrc_folder_fp);
		mrc->mrc_uid_next = 1;
		mrc->mrc_msgs = 0;
	}
	else {
		/* folder shrunk */
		if (folder_len < mrc->mrc_fsize)
			return(0);
		if (fseek(mrc->mrc_folder_fp, mrc->mrc_fsize, SEEK_SET) < 0)
			return(0);
		oldmsgs = mrc->mrc_msgs;
		mmdfin = mrc->mrc_mmdf;
	}

	state = 0;
	content_valid = 0;
	content_len = 0;
	while (1) {
		buf[0] = 0;
		buflen = msc1_fgets(buf, MSCBUFSIZ, mrc->mrc_folder_fp);
		if (buflen == 0)
			input = I_EOF;

/* label allows From lines to used as both SOM and EOM with non-MMDF folders */
top:
		/* id line type */
		if (input != I_EOF) {
			input = I_REG;
			if (mmdfin && (strcmp(buf, MMDFSTR) == 0))
				input = I_MMDF;
			else if (strcmp(buf, DELETED_EFROM) == 0)
				input = I_DELE;
			else if (sendmail_parse_from(buf, 0, 0, 0))
				input = I_FROM;
			else if ((state != S_BODY) && (state != S_DELE)) {
				/* header checks, ignore if in body, faster */
				if (strcmp(buf, "\n") == 0)
					input = I_BLANK;
				else if (msc1_strhccmp(STATUS_STR, buf, ':') == 0)
					input = I_STAT;
				else if (msc1_strhccmp(XSTATUS_STR, buf, ':') == 0)
					input = I_XSTAT;
				else if (msc1_strhccmp(UID_STR, buf, ':') == 0)
					input = I_UID;
				else if (msc1_strhccmp(PAD_STR, buf, ':') == 0)
					input = I_PAD;
				else if (msc1_strhccmp(DATE_STR, buf, ':') == 0)
					input = I_DATE;
				else if (msc1_strhccmp(CONT_STR, buf, ':') == 0)
					input = I_CONT;
			}
		}

		/* if in valid content length then check current token */
		if (content_valid) {
			i = ftell(mrc->mrc_folder_fp) - buflen;
			/* within a valid content-length area */
			if (i < content_valid) {
				switch (input) {
				case I_MMDF:
				case I_DELE:
				case I_FROM:
					input = I_REG;
				}
			}
		}

		/* save Content-Length for later (end of header check) */
		if (input == I_CONT) {
			ucp = (unsigned char *)strchr(buf, ':') + 1;
			while ((*ucp == ' ') || (*ucp == '\t'))
				ucp++;
			content_len = atoi((char *)ucp);
			if (content_len < 0)
				content_len = 0;
			/* drop if rebuilding */
			if (mrc->mrc_access&ACCESS_WRITE)
				continue;
			input = I_REG;
		}

		if (mmdfin)
			output = msc1_fsm_mmdf_output[input][state];
		else
			output = msc1_fsm_output[input][state];
		nsom = 0;
		switch (output) {
		case O_ERR:
			/* parse error */
			return(0);
		case O_NULL:
			if ((state == S_SRCH) &&
				(mrc->mrc_access&ACCESS_WRITE) == 0)
				prc->prc_hdrlines++;
			break;
		case O_NSOM:
			nsom = 1;
		case O_SOM:
			content_valid = 0;
			content_len = 0;
			/* valid start of new message found */
			/* grow prc array if needed */
			if (mrc->mrc_msgs == mrc->mrc_psize) {
				mrc->mrc_psize *= 2;
				mrc->mrc_prc = realloc(mrc->mrc_prc,
					sizeof(prc_t)*mrc->mrc_psize);
			}
			prc = mrc->mrc_prc + mrc->mrc_msgs;
			memset(prc, 0, sizeof(prc_t));
			if (mrc->mrc_access&ACCESS_WRITE) {
				if (mrc->mrc_mmdf)
					if (fputs(MMDFSTR, mrc->mrc_temp_fp) == EOF)
						return(0);
				prc->prc_start = ftell(mrc->mrc_temp_fp);
				if (nsom == 0) {
					/* put our From line in the file */
					if (fwrite(buf, 1, buflen, mrc->mrc_temp_fp) != buflen)
						return(0);
				}
				prc->prc_hdrstart = ftell(mrc->mrc_temp_fp);
			}
			else {
				/* read only just calculate offsets */
				prc->prc_hdrstart = ftell(mrc->mrc_folder_fp);
				prc->prc_start = prc->prc_hdrstart - strlen(buf);
				if (nsom)
					prc->prc_hdrstart = prc->prc_start;
			}
			cp = buf;
			if (nsom == 0) {
				sendmail_parse_from(cp, 0, date, 0);
				msc1_date_sendmail_to_prc(prc, date);
			}
			else {
				/* loop back MMDF no From line */
				/* still have to do next state processing */
				state = msc1_fsm_mmdf_next[input][state];
				goto top;
			}
			break;
		case O_OUT:
			if (state == S_SRCH)
				prc->prc_hdrlines++;
			else
				prc->prc_bodylines++;
			if (mrc->mrc_access&ACCESS_WRITE)
				if (fwrite(buf, 1, buflen, mrc->mrc_temp_fp) != buflen)
					return(0);
			break;
		case O_SAVEU:
			if ((mrc->mrc_access&ACCESS_WRITE) == 0)
				prc->prc_hdrlines++;
			/* save uid */
			prc->prc_uid = 0;
			cp = buf + 9;
			while ((*cp == ' ') || (*cp == '\t'))
				cp++;
			if (*cp != ':')
				break;
			cp++;
			while ((*cp == ' ') || (*cp == '\t'))
				cp++;
			if (*cp)
				prc->prc_uid = atoi(cp);
			if (prc->prc_uid < 0)
				prc->prc_uid = 0;
			break;
		case O_SAVED:
			prc->prc_hdrlines++;
			if (mrc->mrc_access&ACCESS_WRITE)
				if (fwrite(buf, 1, buflen, mrc->mrc_temp_fp) != buflen)
					return(0);
			/* got a From line */
			if (prc->prc_start != prc->prc_hdrstart)
				break;
			/* parse date into our structure */
			cp = strchr(buf, ':') + 1;
			while ((*cp == ' ') || (*cp == '\t'))
				cp++;
			memset(&elt, 0, sizeof(MESSAGECACHE));
			mail_parse_date(&elt, cp);
			prc->prc_hours = elt.hours;
			prc->prc_minutes = elt.minutes;
			prc->prc_seconds = elt.seconds;
			prc->prc_day = elt.day;
			prc->prc_month = elt.month;
			prc->prc_year = elt.year;
			prc->prc_zoccident = elt.zoccident;
			prc->prc_zhours = elt.zhours;
			prc->prc_zminutes = elt.zminutes;
			break;
		case O_EOH:
			/* end of header */
			content_valid = msc1_content_valid(mrc, folder_len, content_len, buf, mmdfin);
			if (msc1_eoh(mrc, buf, content_valid ? content_len : 0) == 0)
				return(0);
			break;
		case O_EOB:
			/*
			 * end of both message and header: null body
			 * we tack on a newline although it is not necessary
			 */
			if (msc1_eoh(mrc, buf, 0) == 0)
				return(0);
			/* fall through */
		case O_EOM:
			/* end of message processing */
			if (msc1_eom(mrc, buf) == 0)
				return(0);
			/* fall through */
		case O_LOOP:
			/*
			 * now do special loop check
			 * this makes every From line be processed twice
			 * by the fsm so it can act as both and EOM
			 * and an SOM marker, it makes the MMDF and
			 * non MMDF fsm actions identical for SOM and EOM
			 */
			content_valid = 0;
			content_len = 0;
			if (!mmdfin && ((input == I_FROM) || (input == I_DELE))) {
				/* still have to do next state processing */
				state = msc1_fsm_next[input][state];
				goto top;
			}
			break;
		case O_PAD:
			if (state == S_SRCH)
				prc->prc_hdrlines++;
			else
				prc->prc_bodylines++;
			if ((mrc->mrc_access&ACCESS_WRITE) == 0)
				break;
			/* pad a from line if converting to sendmail */
			if (mrc->mrc_mmdf == 0) {
				if (fputc('>', mrc->mrc_temp_fp) == EOF)
					return(0);
			}
			if (fwrite(buf, 1, buflen, mrc->mrc_temp_fp) != buflen)
				return(0);
			break;
		case O_SAVES:
			if ((mrc->mrc_access&ACCESS_WRITE) == 0)
				prc->prc_hdrlines++;
			/* save status */
			cp = buf + STATUS_SIZ + 1;
			while (*cp) {
				switch (*cp++) {
				case 'R':
					prc->prc_status |= fSEEN;
					break;
				case 'O':
					prc->prc_status |= fOLD;
					break;
				}
			}
			break;
		case O_SAVEX:
			if ((mrc->mrc_access&ACCESS_WRITE) == 0)
				prc->prc_hdrlines++;
			/* save X-Status */
			cp = buf + XSTATUS_SIZ + 1;
			while (*cp) {
				switch (*cp++) {
				case 'D':
					prc->prc_status |= fDELETED;
					break;
				case 'F':
					prc->prc_status |= fFLAGGED;
					break;
				case 'A':
					prc->prc_status |= fANSWERED;
					break;
				case 'T':
					prc->prc_status |= fDRAFT;
					break;
				}
			}
		}
		if (mmdfin)
			state = msc1_fsm_mmdf_next[input][state];
		else
			state = msc1_fsm_next[input][state];
		if (state == S_DONE)
			break;
	}
	if ((mrc->mrc_access&ACCESS_WRITE) == 0) {
		/* finished if read-only */
		mrc->mrc_fsize = ftell(mrc->mrc_folder_fp);
		free(buf);
		buf = 0;
		if (append == 0) {
			/* set validity value */
			if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0)
				return(0);
			/* new validity is mtime */
			mrc->mrc_validity = sbuf.st_mtime;
			mrc->mrc_mtime = sbuf.st_mtime;
		}
		return(1);
	}
	/* the temp file now has our new data and it is locked */
	if (append == 0) {
		mrc->mrc_fsize = ftell(mrc->mrc_temp_fp);
		/*
		 * we remove the orignal and rename the new file
		 */
		fclose(mrc->mrc_folder_fp);
		mrc->mrc_folder_fp = 0;
		unlink(mrc->mrc_path);
		if (rename(mrc->mrc_tpath, mrc->mrc_path) < 0) {
			/*
			 * Unable to rename the new folder
			 * give up completely.  The folder.pid
			 * file contains the user's data but
			 * there is no way to inform the user.
			 */
			fclose(mrc->mrc_temp_fp);
			/*
			 * this prevents the temp file from
			 * being removed by the close routine
			 * thus the user is given some recovery
			 */
			mrc->mrc_temp_fp = 0;
			return(0);
		}
		mrc->mrc_folder_fp = mrc->mrc_temp_fp;
		msc1_lock_tmp_to_folder(mrc);
	}
	else {
		oldlen = mrc->mrc_fsize;
		mrc->mrc_fsize += ftell(mrc->mrc_temp_fp);
		/*
		 * here we copy the temp file data back to the original folder
		 * we truncate the folder if the copy back fails
		 */
		if (msc1_expand_folder(mrc) == 0)
			goto copyback_failed;
		if (fseek(mrc->mrc_folder_fp, oldlen, SEEK_SET) < 0)
			goto copyback_failed;
		rewind(mrc->mrc_temp_fp);
		while (len = fread(buf, 1, MSCBUFSIZ, mrc->mrc_temp_fp))
			if (fwrite(buf, 1, len, mrc->mrc_folder_fp) != len)
				goto copyback_failed;

		fclose(mrc->mrc_temp_fp);
		unlink(mrc->mrc_tpath);
		/* now update new prc's */
		for (i = oldmsgs; i < mrc->mrc_msgs; i++) {
			prc = mrc->mrc_prc + i;
			prc->prc_start += oldlen;
			prc->prc_hdrstart += oldlen;
			prc->prc_bodystart += oldlen;
			prc->prc_stat_start += oldlen;
		}
	}
	mrc->mrc_temp_fp = 0;
	if (fflush(mrc->mrc_folder_fp) == EOF)
		return(0);
	if (conf_fsync)
		fsync(fileno(mrc->mrc_folder_fp));
	free(buf);
	buf = 0;
	/* set or restore validity value */
	if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0)
		return(0);
	/* new validity needed ? */
	if (mrc->mrc_validity == 0)
		mrc->mrc_validity = sbuf.st_mtime;
	mrc->mrc_mtime = sbuf.st_mtime;
	/* now write out index file */
	if (msc1_write_index(mrc) == 0)
		return(0);
	/* truncate index file */
	if (msc1_expand_index(mrc) == 0)
		return(0);
	return(1);
copyback_failed:
	mrc->mrc_fsize = oldlen;
	msc1_expand_folder(mrc);
	fclose(mrc->mrc_temp_fp);
	mrc->mrc_temp_fp = 0;
	unlink(mrc->mrc_tpath);
	return(0);
}

/*
 * our index has been rebuilt, write it to the index file
 * caller gets the locks, we need ACCESS_BOTH and maybe all the others
 * if a full rebuild is taking place
 */
int
msc1_write_index(register mrc_t *mrc)
{
	register prc_t *prc;
	prd_t prd;
	mrd_t mrd;
	int i;

	mrc->mrc_status |= MSC1_UPDATE;
	if (msc1_write_mrc(mrc) == 0)
		return(0);
	if (fflush(mrc->mrc_index_fp) == EOF)
		return(0);
	mrc->mrc_status &= ~MSC1_UPDATE;	/* reset in-core copy */

	for (i = 0; i < mrc->mrc_msgs; i++)
		if (msc1_write_prc(mrc, i) == 0)
			break;

	/* both error and normal gets here */
	/* try to reset update flag */
	if (msc1_write_mrc(mrc) == 0)
		return(0);
	if (conf_fsync)
		fsync(fileno(mrc->mrc_index_fp));
	return(1);
}

/*
 * write out our mr, do fflush, caller does fsync
 */
int
msc1_write_mrc(register mrc_t *mrc)
{
	mrd_t mrd;

	if (msc1_get_consistency(mrc, mrc->mrc_consistency) == 0)
		return(0);
	rewind(mrc->mrc_index_fp);
	msc1_mrc_out(&mrd, mrc);
	if (fwrite(&mrd, 1, sizeof(mrd_t), mrc->mrc_index_fp) != sizeof(mrd_t))
		return(0);
	if (fflush(mrc->mrc_index_fp) == EOF)
		return(0);
	return(1);
}

/*
 * write out a prc, caller does fflush and fsync
 */
int
msc1_write_prc(register mrc_t *mrc, int msg)
{
	register prc_t *prc;
	prd_t prd;
	int offset;

	prc = mrc->mrc_prc + msg;
	return(msc1_write_prcp(mrc, prc, msg));
}

/*
 * write out a prc from pointer, caller does fflush and fsync
 */
int
msc1_write_prcp(register mrc_t *mrc, register prc_t *prc, int msg)
{
	prd_t prd;
	int offset;
	int status;

	offset = sizeof(mrd_t) + msg*sizeof(prd_t);
	if (fseek(mrc->mrc_index_fp, offset, SEEK_SET) < 0)
		return(0);
	status = prc->prc_status;
	msc1_prc_out(&prd, prc);
	if (fwrite(&prd, 1, sizeof(prd_t), mrc->mrc_index_fp) != sizeof(prd_t))
		return(0);
	return(1);
}

/*
 * marshal out an mrc
 */
void
msc1_mrc_out(register mrd_t *mrd, register mrc_t *mrc)
{
	memset(mrd, 0, sizeof(mrd_t));
	strcpy(mrd->mrd_magic, MAGIC_STR);
	strcat(mrd->mrd_magic, MAGIC_VSN);
	msc1_int_out(mrd->mrd_fsize, mrc->mrc_fsize);
	msc1_int_out(mrd->mrd_mtime, mrc->mrc_mtime);
	msc1_int_out(mrd->mrd_validity, mrc->mrc_validity);
	msc1_int_out(mrd->mrd_uid_next, mrc->mrc_uid_next);
	msc1_int_out(mrd->mrd_msgs, mrc->mrc_msgs);
	msc1_int_out(mrd->mrd_mmdf, mrc->mrc_mmdf);
	msc1_int_out(mrd->mrd_status, mrc->mrc_status);
	strcpy(mrd->mrd_consistency, mrc->mrc_consistency);
}

/*
 * marshal out a prc
 */
void
msc1_prc_out(register prd_t *prd, register prc_t *prc)
{
	msc1_int_out(prd->prd_start, prc->prc_start);
	msc1_int_out(prd->prd_size, prc->prc_size);
	msc1_int_out(prd->prd_hdrstart, prc->prc_hdrstart);
	msc1_int_out(prd->prd_hdrlines, prc->prc_hdrlines);
	msc1_int_out(prd->prd_bodystart, prc->prc_bodystart);
	msc1_int_out(prd->prd_bodylines, prc->prc_bodylines);
	msc1_int_out(prd->prd_uid, prc->prc_uid);
	msc1_int_out(prd->prd_stat_start, prc->prc_stat_start);
	msc1_int_out(prd->prd_content_len, prc->prc_content_len);
	msc1_int_out(prd->prd_status, prc->prc_status);
	prd->prd_year = prc->prc_year;
	prd->prd_month = prc->prc_month;
	prd->prd_day = prc->prc_day;
	prd->prd_hours = prc->prc_hours;
	prd->prd_minutes = prc->prc_minutes;
	prd->prd_seconds = prc->prc_seconds;
	prd->prd_zoccident = prc->prc_zoccident;
	prd->prd_zhours = prc->prc_zhours;
	prd->prd_zminutes = prc->prc_zminutes;
}

/*
 * update our on disk copy to match our in-core flags
 * temp flag is use temp file or folder file
 */
int
msc1_status_out(mrc_t *mrc, register prc_t *prc, int msg, int temp)
{
	char buf[80];
	register char *cp;
	int i;
	FILE *fp;

	if (temp)
		fp = mrc->mrc_temp_fp;
	else
		fp = mrc->mrc_folder_fp;
	i = 0;
	sprintf(buf, "%s: ", STATUS_STR);
	cp = buf + strlen(buf);
	if (prc->prc_status&fSEEN) {
		i++;
		*cp++ = 'R';
	}
	if (prc->prc_status&fOLD) {
		i++;
		*cp++ = 'O';
	}
	sprintf(cp, "\n%s: ", XSTATUS_STR);
	cp += strlen(cp);
	if (prc->prc_status&fDELETED) {
		i++;
		*cp++ = 'D';
	}
	if (prc->prc_status&fFLAGGED) {
		i++;
		*cp++ = 'F';
	}
	if (prc->prc_status&fANSWERED) {
		i++;
		*cp++ = 'A';
	}
	if (prc->prc_status&fDRAFT) {
		i++;
		*cp++ = 'T';
	}
	sprintf(cp, "\n%s: ", PAD_STR);
	cp += strlen(cp);
	while (i++ < STAT_RESERVE)
		*cp++ = 'X';
	*cp++ = '\n';
	*cp++ = 0;
	if (fseek(fp, prc->prc_stat_start, SEEK_SET) < 0)
		return(0);
	if (fputs(buf, fp) == EOF)
		return(0);
	return(1);
}

/*
 * read in status flags, called twice, once for Status, once for X-status
 * flags are or'd with passed value on second call
 */
int
msc1_status_in(char *ibuf, int status)
{
	int newstatus;
	register char *cp;
	register int c;

	cp = strchr(ibuf, ':') + 1;
	newstatus = status;
	while (c = *cp++) {
		switch (c) {
		case 'R':
			newstatus |= fSEEN;
			break;
		case 'O':
			newstatus |= fOLD;
			break;
		case 'D':
			newstatus |= fDELETED;
			break;
		case 'F':
			newstatus |= fFLAGGED;
			break;
		case 'A':
			newstatus |= fANSWERED;
			break;
		case 'T':
			newstatus |= fDRAFT;
			break;
		}
	}
	return(newstatus);
}

/*
 * This routine is a reincarnation of the bezerk.h Valid macro
 * Known formats are:
 * 		From user Wed Dec  2 05:53 1992
 * BSD		From user Wed Dec  2 05:53:22 1992
 * SysV		From user Wed Dec  2 05:53 PST 1992
 * rn		From user Wed Dec  2 05:53:22 PST 1992
 *		From user Wed Dec  2 05:53 -0700 1992
 *		From user Wed Dec  2 05:53:22 -0700 1992
 *		From user Wed Dec  2 05:53 1992 PST
 *		From user Wed Dec  2 05:53:22 1992 PST
 *		From user Wed Dec  2 05:53 1992 -0700
 * Solaris	From user Wed Dec  2 05:53:22 1992 -0700
 *
 * Plus all of the above with `` remote from xxx'' after it.
 */
int
sendmail_parse_from(char *str, char *sender, char *date, char **rest)
{
	register char *cp;
	int len;
	int i;
	int have;
	char *sendstart;
	char *sendend;
	int t;
	int zone;
	int zoccident;
	int zhours;
	int zminutes;
	char nbuf[4];

#define HAVE_YEAR	1	/* have year */
#define	HAVE_CZONE	2	/* have character timezone name */
#define HAVE_NZONE	4	/* have numeric timezone definition */

	zone = 0;
	if (strncmp(str, "From ", 5))
		return(0);
	if (sender) {
		sendstart = str + 4;
		while (*sendstart == ' ')
			sendstart++;
	}
	len = strlen(str);
	if (len < 27)
		return(0);
	/* scan forward until a valid day found */
	for (cp = str; *cp; cp++)
		for (i = 0; i < 7; i++)
			if (strncmp(cp, days[i], 3) == 0)
				goto day_found;
day_found:
	if (i == 7)
		return(0);
	/* found a day */
	if (sender) {
		sendend = cp - 1;
		if (*sendend != ' ')
			return(0);
		while (sendend[-1] == ' ')
			sendend--;
		if (sendend <= sendstart)
			return(0);
		*sendend = 0;
		strcpy(sender, sendstart);
		*sendend = ' ';
	}
	if (strlen(cp) < 22)
		return(0);
	if (cp[3] != ' ')
		return(0);
	if (date) {
		/* up to the point where it diverges (seconds) */
		if (cp[16] == ':') {
			strncpy(date, cp, 20);
			date[20] = 0;
		}
		else {
			strncpy(date, cp, 16);
			date[16] = ':';
			date[17] = '0';
			date[18] = '0';
			date[19] = ' ';
			date[20] = 0;
		}
	}
	cp += 4;
	for (i = 0; i < 12; i++)
		if (strncmp(cp, months[i], 3) == 0)
			break;
	if (i == 12)
		return(0);
	/* have a month */
	if (cp[3] != ' ')
		return(0);
	cp += 4;
	/* now we look for the day either two digits or a blank and a digit */
	if ((*cp != ' ') && (*cp < '0' || *cp > '9'))
		return(0);
	cp++;
	if (*cp < '0' || *cp > '9')
		return(0);
	cp++;
	if (*cp != ' ')
		return(0);
	cp++;
	/* now we look for time with or without seconds */
	/* each loop wants two digits followed by an optional colon */
	for (i = 0; i < 3; i++) {
		if (*cp == ' ')
			break;
		if (*cp < '0' || *cp > '9')
			return(0);
		cp++;
		if (*cp < '0' || *cp > '9')
			return(0);
		cp++;
		if (*cp == ' ')
			continue;
		if (*cp++ != ':')
			return(0);
		if (*cp == ' ')
			return(0);
	}
	/* must have found at least two */
	if (i < 2)
		return(0);
	cp++;
	/*
	 * three things can come next PST YEAR or +/-HHMM in any order
	 * only one of each can occur and at least year must be present.
	 * we use a token recognition algorithm to parse it.
	 */
	have = 0;
	while (*cp) {
		/* what do we have */
		/* have +/-HHMM */
		if ((*cp == '+') || (*cp == '-')) {
			if (have&(HAVE_CZONE|HAVE_NZONE))
				return(0);
			cp++;
			for (i = 0; i < 4; i++) {
				if (*cp < '0' || *cp > '9')
					return(0);
				cp++;
			}
			have |= HAVE_NZONE;
			zone = 1;
			zoccident = (cp[-5] == '-') ? 1 : 0;
			nbuf[0] = cp[-4];
			nbuf[1] = cp[-3];
			nbuf[2] = 0;
			zhours = atoi(nbuf);
			nbuf[0] = cp[-2];
			nbuf[1] = cp[-1];
			zminutes = atoi(nbuf);

			if (*cp == ' ') {
				cp++;
				continue;
			}
			if (*cp == '\n')
				break;
			return(0);
		}
		/* have a character zone (three upper case characters) */
		if (*cp >= 'A' && *cp <= 'Z') {
			if (have&(HAVE_CZONE|HAVE_NZONE))
				return(0);
			cp++;
			for (i = 0; i < 2; i++) {
				if (*cp < 'A' || *cp > 'Z') 
					return(0);
				cp++;
			}
			have |= HAVE_CZONE;
			if (*cp == ' ') {
				cp++;
				continue;
			}
			if (*cp == '\n')
				break;
			return(0);
		}
		/* have year */
		if (*cp >= '0' && *cp <= '9') {
			if (have&HAVE_YEAR)
				return(0);
			if (date) {
				date[20] = cp[0];
				date[21] = cp[1];
				date[22] = cp[2];
				date[23] = cp[3];
				date[24] = 0;
			}
			cp++;
			for (i = 0; i < 3; i++) {
				if (*cp < '0' || *cp > '9')
					return(0);
				cp++;
			}
			have |= HAVE_YEAR;
			if (*cp == ' ') {
				cp++;
				continue;
			}
			if (*cp == '\n')
				break;
			return(0);
		}
		/* try for remote from */
		if (strncmp(cp, "remote from ", 12) == 0) {
			cp = strchr(cp, '\n');
			break;
		}
		/* unrecognized token */
		return(0);
	}
	if (cp == 0)
		return(0);
	if (*cp != '\n')
		return(0);
	if ((have&HAVE_YEAR) == 0)
		return(0);
	if (rest)
		*rest = cp + 1;
	/* append timezone on date field */
	if (date) {
		if (zone == 0) {
			tzset();
			/* local timezone */
			t = timezone/60;
			zoccident = 1;
			zhours = t/60;
			zminutes = t%60;
		}
		cp = date + strlen(date);
		sprintf(cp, " %c%02d%02d", zoccident ? '-' : '+', zhours, zminutes);
	}
	return(1);
}

/*
 * read in index and do validation checking
 * if index is there with correct version stamp the uid is preserved in mrc
 * even if the validation checks fail
 */
int
msc1_read_index(mrc_t *mrc)
{
	/* check for read only case of non-indexed folder */
	if (mrc->mrc_index_fp == 0)
		return(0);
	if (msc1_read_mrc(mrc) == 0)
		return(0);
	if (msc1_read_prcs(mrc) == 0)
		return(0);
	return(1);
}

/*
 * read in master record and check it
 */
int
msc1_read_mrc(register mrc_t *mrc)
{
	mrd_t mrd;
	mrc_t nmrc;
	char cbuf[SCOMS_CONSISTENCY];
	struct stat sbuf;
	int strt;
	int len;

	rewind(mrc->mrc_index_fp);
	if (fread(&mrd, 1, sizeof(mrd_t), mrc->mrc_index_fp) != sizeof(mrd_t))
		return(0);
	/* version stamp */
	strcpy(cbuf, MAGIC_STR);
	strcat(cbuf, MAGIC_VSN);
	if (strcmp(mrd.mrd_magic, cbuf))
		return(0);

	/* cannonify it */
	nmrc = *mrc;			/* save in-core stuff */
	msc1_mrc_in(&nmrc, &mrd);	/* add in on-disk stuff */

	/* preserve uid and validity even if consistency checks fail */
	mrc->mrc_uid_next = nmrc.mrc_uid_next;
	mrc->mrc_validity = nmrc.mrc_validity;

	/* check if folder size not match */
	if (fstat(fileno(mrc->mrc_folder_fp), &sbuf) < 0)
		return(0);
	if (nmrc.mrc_fsize != sbuf.st_size)
		return(0);

	/* check transient flags */
	if (nmrc.mrc_status&MSC1_UPDATE)
		return(0);

	/* check consistency string */
	if (msc1_get_consistency(&nmrc, cbuf) == 0)
		return(0);
	if (strcmp(cbuf, nmrc.mrc_consistency))
		return(0);
	/* validity check */
	if (nmrc.mrc_mtime != sbuf.st_mtime)
		return(0);

	/* copy in everything */
	*mrc = nmrc;
	return(1);
}

/*
 * read in per message records and check them
 * extended checking is done if configured
 */
int
msc1_read_prcs(register mrc_t *mrc)
{
	int i;

	mrc->mrc_psize = mrc->mrc_msgs + 20;	/* leave some room to save RAM*/
	mrc->mrc_prc = malloc(sizeof(prc_t)*mrc->mrc_psize);
	if (mrc->mrc_prc == 0)
		goto prd_err;
	for (i = 0; i < mrc->mrc_msgs; i++)
		if (msc1_read_prc(mrc, i) == 0)
			goto prd_err;
	return(1);
prd_err:
	if (mrc->mrc_prc)
		free(mrc->mrc_prc);
	mrc->mrc_prc = 0;
	mrc->mrc_msgs = 0;
	return(0);
}

/*
 * read in a single prc and check it
 */
int
msc1_read_prc(register mrc_t *mrc, int msg)
{
	register prc_t *prc;
	prd_t prd;
	int len;
	int uid;
	int status;
	char buf[MSCSBUFSIZ];

	prc = mrc->mrc_prc + msg;
	memset(prc, 0, sizeof(prc_t));
	if (fseek(mrc->mrc_index_fp,
		sizeof(mrd_t) + msg*sizeof(prd_t), SEEK_SET) < 0)
		return(0);
	if (fread(&prd, 1, sizeof(prd_t), mrc->mrc_index_fp) != sizeof(prd_t))
		return(0);
	/* cannonify it */
	msc1_prc_in(prc, &prd);

	if (conf_extended_checks == 0)
		return(1);
	/* check From line, it does not have to be present if MMDF */
	if (fseek(mrc->mrc_folder_fp, prc->prc_start, SEEK_SET) < 0)
		return(0);
	if (fgets(buf, MSCSBUFSIZ, mrc->mrc_folder_fp) == 0)
		return(0);
	if (strncmp(buf, "From ", 5) == 0) {
		if (sendmail_parse_from(buf, 0, 0, 0) == 0)
			return(0);
	}
	else {
		if (mrc->mrc_mmdf == 0)
			return(0);
	}

	/* check status headers */
	if (fseek(mrc->mrc_folder_fp, prc->prc_stat_start, SEEK_SET) < 0)
		return(0);
	/* get Status: header */
	if (fgets(buf, MSCSBUFSIZ, mrc->mrc_folder_fp) == 0)
		return(0);
	if (msc1_strxccmp(STATUS_STR, buf, ':'))
		return(0);
	len = strlen(buf + STATUS_SIZ + 1);
	status = msc1_status_in(buf, 0);
	/* get X-Status: */
	if (fgets(buf, MSCSBUFSIZ, mrc->mrc_folder_fp) == 0)
		return(0);
	if (msc1_strxccmp(XSTATUS_STR, buf, ':'))
		return(0);
	len += strlen(buf + XSTATUS_SIZ + 1);
	/* status must match prc value */
	status = msc1_status_in(buf, status);
	if (status != prc->prc_status)
		return(0);
	/* get X-PAD */
	if (fgets(buf, MSCSBUFSIZ, mrc->mrc_folder_fp) == 0)
		return(0);
	if (msc1_strxccmp(PAD_STR, buf, ':'))
		return(0);
	len += strlen(buf + PAD_SIZ + 1);
	len -= 6;		/* expect blanks and newlines */
	if (len != STAT_RESERVE)
		return(0);

	/* check uid */
	if (fgets(buf, MSCSBUFSIZ, mrc->mrc_folder_fp) == 0)
		return(0);
	if (msc1_strxccmp(UID_STR, buf, ':'))
		return(0);
	if (buf[UID_SIZ + 1] != ' ')
		return(0);
	uid = atoi(buf + UID_SIZ + 1);
	if (uid != prc->prc_uid)
		return(0);
	return(1);
}

/*
 * convert mrd to mrc (disk format to core format)
 */
void
msc1_mrc_in(mrc_t *mrc, mrd_t *mrd)
{
	mrc->mrc_fsize = msc1_int_in(mrd->mrd_fsize);
	mrc->mrc_mtime = msc1_int_in(mrd->mrd_mtime);
	mrc->mrc_validity = msc1_int_in(mrd->mrd_validity);
	mrc->mrc_uid_next = msc1_int_in(mrd->mrd_uid_next);
	mrc->mrc_msgs = msc1_int_in(mrd->mrd_msgs);
	mrc->mrc_mmdf = msc1_int_in(mrd->mrd_mmdf);
	mrc->mrc_status = msc1_int_in(mrd->mrd_status);
	mrd->mrd_consistency[SCOMS_CONSISTENCY-1] = 0;
	strcpy(mrc->mrc_consistency, mrd->mrd_consistency);
}

/*
 * convert prd to prc (disk format to core format)
 */
void
msc1_prc_in(prc_t *prc, prd_t *prd)
{
	prc->prc_start = msc1_int_in(prd->prd_start);
	prc->prc_size = msc1_int_in(prd->prd_size);
	prc->prc_hdrstart = msc1_int_in(prd->prd_hdrstart);
	prc->prc_hdrlines = msc1_int_in(prd->prd_hdrlines);
	prc->prc_bodystart = msc1_int_in(prd->prd_bodystart);
	prc->prc_bodylines = msc1_int_in(prd->prd_bodylines);
	prc->prc_uid = msc1_int_in(prd->prd_uid);
	prc->prc_stat_start = msc1_int_in(prd->prd_stat_start);
	prc->prc_content_len = msc1_int_in(prd->prd_content_len);
	prc->prc_status = msc1_int_in(prd->prd_status);
	prc->prc_year = prd->prd_year;
	prc->prc_month = prd->prd_month;
	prc->prc_day = prd->prd_day;
	prc->prc_hours = prd->prd_hours;
	prc->prc_minutes = prd->prd_minutes;
	prc->prc_seconds = prd->prd_seconds;
	prc->prc_zoccident = prd->prd_zoccident;
	prc->prc_zhours = prd->prd_zhours;
	prc->prc_zminutes = prd->prd_zminutes;
}

/*
 * unmarshal an int
 */
int
msc1_int_in(register unsigned char *cp)
{
	int i;
	register int val;

	val = 0;
	for (i = 0; i < SCOMS_INT; i++) {
		val <<= 8;
		val |= *cp++;
	}
	return(val);
}

/*
 * marshal an int
 */
void
msc1_int_out(register unsigned char *cp, int val)
{
	int i;

	cp += SCOMS_INT - 1;
	for (i = 0; i < SCOMS_INT; i++) {
		*cp-- = val&0xff;
		val >>= 8;
	}
}

/*
 * end of header processing for rebuild
 */
int
msc1_eoh(register mrc_t *mrc, char *buf, int content_valid)
{
	register prc_t *prc;
	char sbuf[80];

	prc = mrc->mrc_prc + mrc->mrc_msgs;
	/*
	 * first check uid
	 * we are expecting uids to be in the file in order
	 * we just verify and continue
	 * once one is found that violates, we must
	 * reassign all uids after that, because UIDS
	 * must be in increasing order
	 */
	if (mrc->mrc_org_uid_next) {
		if (mrc->mrc_uid_next <= prc->prc_uid) {
			/* uid ok, set for next one */
			mrc->mrc_uid_next = prc->prc_uid + 1;
		}
		else {
			/* reset and assign all new uids */
			if (mrc->mrc_org_uid_next > mrc->mrc_uid_next)
				mrc->mrc_uid_next = mrc->mrc_org_uid_next;
			mrc->mrc_org_uid_next = 0;
			/* new uid for this one */
			prc->prc_uid = mrc->mrc_uid_next++;
		}
	}
	else {
		/* new uid for this one */
		prc->prc_uid = mrc->mrc_uid_next++;
	}
	if (mrc->mrc_access&ACCESS_WRITE) {
		/* now output our padded headers */
		prc->prc_stat_start = ftell(mrc->mrc_temp_fp);
		msc1_status_out(mrc, prc, mrc->mrc_msgs, 1);
		/* now output uid and end of header */
		sprintf(sbuf, "%s: %d\n", UID_STR, prc->prc_uid);
		if (fputs(sbuf, mrc->mrc_temp_fp) == EOF)
			return(0);
		if (content_valid) {
			prc->prc_content_len = 1;
			sprintf(sbuf, "%s: %d\n", CONT_STR, content_valid);
			if (fputs(sbuf, mrc->mrc_temp_fp) == EOF)
				return(0);
			prc->prc_hdrlines++;
		}
		if (fprintf(mrc->mrc_temp_fp, "\n") < 0)
			return(0);
		prc->prc_bodystart = ftell(mrc->mrc_temp_fp);
		prc->prc_hdrlines += 5;		/* status lines and blank */
	}
	else {
		/* read-only case, must include the blank line if there */
		prc->prc_bodystart = ftell(mrc->mrc_folder_fp);
		prc->prc_bodystart -= strlen(buf);
		if (strcmp(buf, "\n") == 0) {
			prc->prc_bodystart++;
			prc->prc_hdrlines++;
		}
	}
	return(1);
}

/*
 * end of message processing for rebuild
 */
int
msc1_eom(register mrc_t *mrc, char *buf)
{
	register prc_t *prc;

	prc = mrc->mrc_prc + mrc->mrc_msgs++;
	if (mrc->mrc_access&ACCESS_WRITE) {
		prc->prc_size = ftell(mrc->mrc_temp_fp) - prc->prc_start;
		if (mrc->mrc_mmdf)
			if (fputs(MMDFSTR, mrc->mrc_temp_fp) == EOF)
				return(0);
	}
	else {
		/* read-only case */
		prc->prc_size = ftell(mrc->mrc_folder_fp) - prc->prc_start;
		prc->prc_size -= strlen(buf);
	}
	return(1);
}

/*
 * expand or contract our original folder to match the requested size
 * expansion is done by writing to the file to guarantee disk space
 * if expansion fails, the file will be restored to it's original length
 * contraction is not recovereable, if it fails, who knows what happened anyway
 */
int
msc1_expand_folder(register mrc_t *mrc)
{
	return(msc1_expand(mrc->mrc_folder_fp, mrc->mrc_fsize));
}

/*
 * expand our index file to match the new size requested
 */
int
msc1_expand_index(register mrc_t *mrc)
{
	int size;

	size = sizeof(mrd_t) + sizeof(prd_t)*mrc->mrc_msgs;
	return(msc1_expand(mrc->mrc_index_fp, size));
}

/*
 * lower level expand routine
 * will restore file to previous size if expand fails
 */
int
msc1_expand(FILE *fp, int fsize)
{
	int org_fsize;		/* original file size */
	struct stat sbuf;
	char buf[MSCSBUFSIZ];	/* expansion output buffer */
	int i;
	int j;

	if (fstat(fileno(fp), &sbuf) < 0)
		return(0);
	org_fsize = sbuf.st_size;
	if (org_fsize > fsize) {
		/* contract */
		if (fflush(fp) == EOF)
			return(0);
		if (ftruncate(fileno(fp), fsize) < 0)
			return(0);
	}
	else if (org_fsize < fsize) {
		/* expand */
		memset(buf, 0, MSCSBUFSIZ);
		if (fseek(fp, org_fsize, SEEK_SET) < 0)
			return(0);
		for (i = org_fsize; i < fsize; ) {
			j = fsize - org_fsize;
			if (j > MSCSBUFSIZ)
				j = MSCSBUFSIZ;
			if (fwrite(buf, 1, j, fp) != j) {
				/* here the expand failed, truncate back */
				ftruncate(fileno(fp), org_fsize);
				fflush(fp);
				return(0);
			}
			i += j;
		}
		if (fflush(fp) == EOF)
			return(0);
	}
	return(1);
}

/*
 * check if expunge needed and do it if possible
 * expected that we do this as preamble to close
 * expunge algorithm is to copy down within the file and truncate at the end
 * therefore we should avoid out of disk space problems
 * index file is modified as messages move, but it's size does not change
 * caller must have session lock, we add all the other locks
 * as this equates to a rebuild, failure to get locks merely prevents
 * the low level expunge, no error is returned on lock failure.
 *
 * the copy down algorithm is designed so that after each write
 * operation the folder is in a consistent state, the index could be left
 * invalid so it is marked for update.
 *
 * return 0 only on fatal error.
 */
int
msc1_expunge(register mrc_t *mrc)
{
	int used;
	int percent;
	register prc_t *prc;
	int mmdf;
	int newaccess;
	int oldaccess;
	int err;
	int outsiz;
	int diff;
	int i;
	int start;
	int size;
	int optr;		/* output offset in file */

	/* our copy buffer */
	char *buf;
	int bufsize;
	int bufsizereq;

	/* never automatically expunge */
	if (conf_expunge_threshold == 0)
		return(1);
	/* nothing to do */
	if (mrc->mrc_fsize == 0)
		return(1);
	/* err to not have session lock */
	if ((mrc->mrc_access&ACCESS_SE) == 0)
		return(0);
	mmdf = (mrc->mrc_mmdf ? MMDFSIZ*2 : 0);
	used = 0;
	for (prc = mrc->mrc_prc; prc < mrc->mrc_prc + mrc->mrc_msgs; prc++)
		used += prc->prc_size + mmdf;
	/* use floats to avoid int overflow */
	if (mrc->mrc_fsize)
		percent = ((float)used / (float)mrc->mrc_fsize) * 100;
	else
		percent = 0;
	/* if conf_percent == 0 always expunge */
	if ((percent != 100) && (percent > conf_expunge_threshold))
		return(1);

	/* something to do, get lock */
	oldaccess = mrc->mrc_access;
	newaccess = mrc->mrc_access | ACCESS_REBUILD;
	if (msc1_lock(mrc, newaccess) == 0)
		return(1);
	/* make our in core copy good */
	if (scomsc1_check(mrc) == 0)
		return(0);

	/* prepare to update */
	mrc->mrc_status |= MSC1_UPDATE;
	if (msc1_write_mrc(mrc) == 0)
		return(0);
	if (fflush(mrc->mrc_index_fp) == EOF)
		return(0);
	mrc->mrc_status &= ~MSC1_UPDATE;	/* reset in-core copy */

	/* now copy down within the file, update prcs as we go */
	bufsize = 0;
	buf = 0;
	optr = 0;
	err = 0;
	for (i = 0; i < mrc->mrc_msgs; i++) {
		prc = mrc->mrc_prc + i;
		start = prc->prc_start;
		size = prc->prc_size;
		if (mrc->mrc_mmdf) {
			start -= MMDFSIZ;
			size += MMDFSIZ*2;
		}
		/* this message already in place */
		if (start == optr) {
			optr += size;
			continue;
		}
		/* must copy it, make our buffer big enough */
		bufsizereq = size + DELETED_EFROMSIZ + mmdf + 1;
		if (bufsize < bufsizereq) {
			if (buf) {
				free(buf);
				buf = 0;
				bufsize = 0;
			}
			/* enough to move a message in one operation */
			buf = malloc(bufsizereq);
			if (buf == 0) {
				err = 1;
				break;
			}
			bufsize = bufsizereq;
		}
		if (fseek(mrc->mrc_folder_fp, start, SEEK_SET) < 0) {
			err = 1;
			break;
		}
		if (fread(buf, 1, size, mrc->mrc_folder_fp) != size) {
			err = 1;
			break;
		}
		/* delete old message in place */
		if (fseek(mrc->mrc_folder_fp, prc->prc_start, SEEK_SET) < 0) {
			err = 1;
			break;
		}
		outsiz = DELETED_EFROMSIZ;
		if (fwrite(DELETED_EFROM, 1, outsiz, mrc->mrc_folder_fp) != outsiz) {
			err = 1;
			break;
		}
		fflush(mrc->mrc_folder_fp);
		/* append deleted from header to
		 * avoid leaving mailbox in a temporary invalid state */
		if (mrc->mrc_mmdf) {
			strcpy(buf + size, MMDFSTR);
			strcpy(buf + size + MMDFSIZ, DELETED_EFROM);
			outsiz = size + MMDFSIZ + DELETED_EFROMSIZ;
		}
		else {
			strcpy(buf + size, DELETED_EFROM);
			outsiz = size + DELETED_EFROMSIZ;
		}
		if (fseek(mrc->mrc_folder_fp, optr, SEEK_SET) < 0) {
			err = 1;
			break;
		}
		if (fwrite(buf, 1, outsiz, mrc->mrc_folder_fp) != outsiz) {
			err = 1;
			break;
		}
		if (fflush(mrc->mrc_folder_fp) == EOF) {
			err = 1;
			break;
		}
		/* update prc */
		diff = start - optr;
		prc->prc_start -= diff;
		prc->prc_stat_start -= diff;
		prc->prc_hdrstart -= diff;
		prc->prc_bodystart -= diff;
		if (msc1_write_prc(mrc, i) == 0) {
			err = 1;
			break;
		}
		fflush(mrc->mrc_index_fp);

		optr += size;
	}
	if (buf)
		free(buf);

	/* now truncate file */
	if (optr != mrc->mrc_fsize) {
		mrc->mrc_fsize = optr;
		if (msc1_expand(mrc->mrc_folder_fp, mrc->mrc_fsize) == 0)
			err = 1;
	}

	/* try to reset update flag and file size */
	if (msc1_write_mrc(mrc) == 0)
		return(0);

	msc1_lock(mrc, oldaccess);
	if (err)
		return(0);
	return(1);
}

int
msc1_get_consistency(register mrc_t *mrc, char *str)
{
	int strt;
	int len;

	/* get consistency string */
	strt = mrc->mrc_fsize - (SCOMS_CONSISTENCY-1);
	len = SCOMS_CONSISTENCY-1;
	if (strt < 0) {
		len -= 0 - strt;
		strt = 0;
	}
	if (fseek(mrc->mrc_folder_fp, strt, SEEK_SET) < 0)
		return(0);
	if (fread(str, 1, len, mrc->mrc_folder_fp) != len)
		return(0);
	str[len] = 0;
	return(1);
}

/*
 * update the consistency string and the mod time in the index
 */
int
msc1_set_consistency(register mrc_t *mrc)
{
	mrc_t nmrc;
	int ret;
	struct stat sbuf;

	fstat(fileno(mrc->mrc_folder_fp), &sbuf);
	mrc->mrc_mtime = sbuf.st_mtime;
	ret = msc1_write_mrc(mrc);
	return(ret);
}

/*
 * fill in some values to an mrc, INBOX translation is here
 */
void
msc1_init_mrc(register mrc_t *mrc, char *path)
{
	char buf[20];
	char *cp;
	int i;

	memset(mrc, 0, sizeof(mrc_t));
	/* have INBOX ? */
	if (msc1_strccmp("INBOX", path) == 0)
		strcpy(mrc->mrc_path, conf_inbox);
	else
		strcpy(mrc->mrc_path, path);
	cp = strrchr(mrc->mrc_path, '/');
	if (cp)
		cp++;
	else
		cp = mrc->mrc_path;
	strcpy(mrc->mrc_ipath, mrc->mrc_path);
	if (cp && (*cp != '.')) {
		i = cp - mrc->mrc_path;
		mrc->mrc_ipath[i++] = '.';
		strcpy(mrc->mrc_ipath+i, cp);
	}
	strcat(mrc->mrc_ipath, ".index");
	strcpy(mrc->mrc_lpath, mrc->mrc_path);
	strcat(mrc->mrc_lpath, ".lock");
	strcpy(mrc->mrc_tpath, mrc->mrc_path);
	sprintf(buf, ".%d", getpid());
	strcat(mrc->mrc_tpath, buf);
}

/*
 * take addresses in one of the following forms:
 *	addr
 *	"name" <addr>		(quotes are optional)
 *	addr (name)
 * and reduce it to just address
 */
void
msc1_parse_address(char *sender, char *addr)
{
	register char *cp;
	register char *cp1;
	int c;

	cp = strchr(addr, '<');
	if (cp) {
		cp1 = strchr(cp, '>');
		if (cp1 == 0)
			return;
		c = *cp1;
		*cp1 = 0;
		strcpy(sender, cp+1);
		*cp1 = c;
	}
	else if (cp = strchr(addr, '(')) {
		c = *cp;
		*cp = 0;
		strcpy(sender, addr);
		*cp = c;
	}
	else
		strcpy(sender, addr);
	/* strip white space */
	cp1 = sender;
	for (cp = sender; *cp; cp++) {
		if (*cp == ' ')
			continue;
		if (*cp == '\t')
			continue;
		*cp1++ = *cp;
	}
	*cp1 = 0;
}

/*
 * check if index file is ok to overwrite
 * zero length is ok
 * if non-zero it must have our index stamp at the beginning
 * otherwise it is somebody else's data and we should not clobber it
 */
int
msc1_ok_to_rebuild_index(register mrc_t *mrc)
{
	struct stat sbuf;
	char buf[SCOMS_VSN];

	if ((mrc->mrc_access&ACCESS_WRITE) == 0)
		return(1);
	if (fstat(fileno(mrc->mrc_index_fp), &sbuf) < 0)
		return(0);
	/* zero len ok */
	if (sbuf.st_size == 0)
		return(1);
	/* must be big enough for a master record */
	if (sbuf.st_size < sizeof(mrd_t))
		return(0);
	rewind(mrc->mrc_index_fp);
	if (fread(buf, 1, SCOMS_VSN, mrc->mrc_index_fp) != SCOMS_VSN)
		return(0);
	buf[SCOMS_VSN-1] = 0;
	/* MAGIC minus version string must match */
	if (strncmp(buf, MAGIC_STR, strlen(MAGIC_STR)))
		return(0);
	return(1);
}

/*
 * special copy routine, duplicates old file in new place
 * will succeed if the source file does not exist and the
 * new place will not exist either.  Preserves times.
 * output files are locked (FOLDER and INDEX locks) before proceeding with copy.
 * input file is assumed to be locked already.
 */
int
msc1_fcopy(char *dst, char *src)
{
	struct flock lock;
	struct stat sbuf;
	struct utimbuf times;
	FILE *ifd;
	FILE *ofd;
	int fd;
	char buf[MSCBUFSIZ];
	int ret;
	int size;

	/* destination must not exist */
	if (stat(dst, &sbuf) == 0)
		return(0);
	/* if source not exist then we are ok */
	if (stat(src, &sbuf))
		return(1);
	/* now we must copy */
	ifd = fopen(src, "r");
	if (ifd == 0)
		return(0);
	fd = nolink_open(dst, O_CREAT|O_RDWR|O_EXCL, SCOMS_PERM&conf_umask);
	if (fd < 0) {
		fclose(ifd);
		return(0);
	}
	ofd = fdopen(fd, "w+");

	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 3;
	lock.l_len = 1;
	lock.l_sysid = 0;
	lock.l_pid = 0;
	ret = fcntl(fileno(ofd), F_SETLK, &lock);
	if (ret < 0) {
		fclose(ifd);
		fclose(ofd);
		return(0);
	}
	ret = 0;
	if (msc1_match_perm(dst, src) == 0)
		goto fcopy_err;
	while (size = fread(buf, 1, MSCBUFSIZ, ifd))
		if (fwrite(buf, 1, size, ofd) != size)
			goto fcopy_err;
	ret = 1;
fcopy_err:
	fclose(ifd);
	fclose(ofd);
	if (ret == 0) {
		unlink(dst);
	}
	else {
		times.actime = sbuf.st_atime;
		times.modtime = sbuf.st_mtime;
		utime(dst, &times); 
	}
	return(ret);
}

/* externally visible date conversion routines */

/*
 * convert from sendmail date format to c-client internal date structure
 * we use the local timezone to generate a plausible imap date string
 *
 * expecting: Mon Sep 23 09:57:13 1996
 * fixed length string, day of month is blank filled if only one digit
 */

static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
void
msc1_date_sendmail_to_prc(prc_t *prc, char *sm)
{
	int i;
	int t;

	tzset();
	prc->prc_hours = atoi(sm + 11);
	prc->prc_minutes = atoi(sm + 14);
	prc->prc_seconds = atoi(sm + 17);
	if (sm[8] != ' ')
		prc->prc_day = atoi(sm + 8);
	else
		prc->prc_day = atoi(sm + 9);
	prc->prc_month = 0;
	for (i = 0; i < 12; i++) {
		if (strncmp(months[i], sm + 4, 3) == 0) {
			prc->prc_month = i+1;
			break;
		}
	}
	prc->prc_year = atoi(sm + 20) - BASEYEAR;
	/* local timezone */
	t = timezone/60;
	prc->prc_zoccident = 1;
	prc->prc_zhours = t/60;
	prc->prc_zminutes = t%60;
}

/*
 * routine to act like fgets but allow null characters in the data,
 * returns length of data read, 0 is EOF, it still null terminates the string
 */
int
msc1_fgets(char *buf, int maxlen, FILE *fd)
{
	int inlen;
	int c;

	inlen = 0;
	maxlen--;
	while (inlen < maxlen) {
		c = getc(fd);
		if (c == EOF)
			break;
		buf[inlen++] = c;
		if (c == '\n')
			break;
	}
	/* null terminated just in case */
	buf[inlen] = 0;
	return(inlen);
}

/*
 * duplicate permissions and ownerships on new file
 */
int
msc1_match_perm(char *dst, char *src)
{
	struct stat sbuf;
	int oldmask;
	int ret;

	if (stat(src, &sbuf) < 0)
		return(0);
	oldmask = umask(0);
	ret = chmod(dst, sbuf.st_mode);
	umask(oldmask);
	if (ret < 0)
		return(0);
	/* now give the file away if needed */
	if ((sbuf.st_uid != geteuid()) || (sbuf.st_gid != getegid())) {
		ret = chown(dst, sbuf.st_uid, sbuf.st_gid);
		if (ret < 0)
			return(0);
	}
	return(1);
}

/*
 * check if a Content-Length header is valid
 * if it is not valid then it is dropped
 * we support it for both MMDF and Sendmail folders
 * can only get here if a blank line was found in the input stream
 * the current seek position therefore must be the start of a message body
 * the main input buffer gets reused here
 */
int
msc1_content_valid(mrc_t *mrc, int folder_len, int content_len, char *buf, int mmdfin)
{
	int curseek;		/* start here, end here */
	int newseek;		/* potential start of next message or EOF */
	int buflen;

	/* case for a message that has no content-len header */
	if (content_len == 0)
		return(0);
	curseek = ftell(mrc->mrc_folder_fp);
	newseek = curseek + content_len;
	/* EOF case only valid for Sendmail format */
	if ((newseek == folder_len) && (mmdfin == 0))
		return(newseek);
	if (fseek(mrc->mrc_folder_fp, newseek, SEEK_SET) < 0)
		return(0);
	buflen = msc1_fgets(buf, MSCBUFSIZ, mrc->mrc_folder_fp);
	if (fseek(mrc->mrc_folder_fp, curseek, SEEK_SET) < 0)
		return(0);
	if (buflen == 0)
		return(0);
	if (mmdfin) {
		if (strcmp(buf, MMDFSTR) == 0)
			return(newseek);
	}
	else {
		if (sendmail_parse_from(buf, 0, 0, 0))
			return(newseek);
	}
	return(0);
}

/*
 * create a folder and lock it
 * complicated protocol to avoid race conditions
 */
int
msc1_create_folder(mrc_t *mrc, int access, int perm)
{
	int fd;
	int fd1;
	struct stat sb;
	struct stat sb1;
	int i;
	int newaccess;
	int match;
	int err;

	for (i = 0; i < conf_lock_timeout; (i++, msc1_abort(mrc), sleep(1))) {
		match = 0;
		/* create or open index and lock it */
		fd = nolink_open(mrc->mrc_ipath, O_CREAT|O_RDWR|O_EXCL, perm);
		if (fd < 0) {
			if (errno != EEXIST)
				break;
			fd = nolink_open(mrc->mrc_ipath, O_RDWR);
			if (fd < 0) {
				if (errno == ENOENT)
					continue;
				break;
			}
			mrc->mrc_index_fp = fdopen(fd, "r+");
		}
		else {
			match |= 2;		/* index was created */
			mrc->mrc_index_fp = fdopen(fd, "w+");
		}
		if (msc1_lock_work(mrc, ACCESS_INDEX, 1) != ACCESS_INDEX)
			continue;
		/* verify our index file was not unlinked out from under us */
		err = 0;
		err |= fstat(fd, &sb);
		err |= stat(mrc->mrc_ipath, &sb1);
		if (err || (sb.st_ino != sb1.st_ino))
			continue;
		/* create or open folder and lock it */
		fd1 = nolink_open(mrc->mrc_path, O_CREAT|O_RDWR|O_EXCL, perm);
		if (fd1 < 0) {
			if (errno != EEXIST)
				break;
			fd1 = nolink_open(mrc->mrc_path, O_RDWR);
			if (fd < 0) {
				if (errno == ENOENT)
					continue;
				break;
			}
			mrc->mrc_folder_fp = fdopen(fd1, "r+");
		}
		else {
			match |= 1;		/* folder was created */
			mrc->mrc_folder_fp = fdopen(fd1, "w+");
		}
		if (msc1_lock_work(mrc, ACCESS_FOLDER, 1) != ACCESS_FOLDER)
			continue;
		/* success, now patch perms if needed, no retry on this one */
		err = 0;
		if (match == 1)
			err = !msc1_match_perm(mrc->mrc_path, mrc->mrc_ipath);
		if (match == 2)
			err = !msc1_match_perm(mrc->mrc_ipath, mrc->mrc_path);
		if (err)
			break;
		mrc->mrc_access = ACCESS_BOTH;
		/* now try to upgrade lock to user request */
		if (mrc->mrc_access != access)
			if (msc1_lock(mrc, access|ACCESS_BOTH) == 0)
				break;
		return(1);
	}
	msc1_abort(mrc);
	return(0);
}

/*
 * open a folder and lock it, create index file if it is not there
 * complicated protocol to avoid race conditions
 */
int
msc1_open_folder(mrc_t *mrc, int access)
{
	int fd;
	int fd1;
	struct stat sb;
	struct stat sb1;
	int i;
	int match;
	int err;

	for (i = 0; i < conf_lock_timeout; (i++, msc1_abort(mrc), sleep(1))) {
		match = 0;
		/* open folder and lock it */
		fd = nolink_open(mrc->mrc_path, O_RDWR);
		if (fd < 0)
			break;
		mrc->mrc_folder_fp = fdopen(fd, "r+");
		if (msc1_lock_work(mrc, ACCESS_FOLDER, 1) != ACCESS_FOLDER)
			continue;
		/* create or open index and lock it */
		fd = nolink_open(mrc->mrc_ipath, O_CREAT|O_RDWR|O_EXCL,
			SCOMS_PERM&conf_umask);
		if (fd < 0) {
			if (errno != EEXIST)
				break;
			fd = nolink_open(mrc->mrc_ipath, O_RDWR);
			if (fd < 0) {
				if (errno == ENOENT)
					continue;
				break;
			}
			mrc->mrc_index_fp = fdopen(fd, "r+");
		}
		else {
			match |= 2;		/* index was created */
			mrc->mrc_index_fp = fdopen(fd, "w+");
		}
		if (msc1_lock_work(mrc, ACCESS_INDEX, 1) != ACCESS_INDEX)
			continue;
		/* verify our folder file was not unlinked out from under us */
		err = 0;
		err |= fstat(fileno(mrc->mrc_folder_fp), &sb);
		err |= stat(mrc->mrc_path, &sb1);
		if (err || (sb.st_ino != sb1.st_ino))
			continue;
		/* verify our index file was not unlinked out from under us */
		err = 0;
		err |= fstat(fileno(mrc->mrc_index_fp), &sb);
		err |= stat(mrc->mrc_ipath, &sb1);
		if (err || (sb.st_ino != sb1.st_ino))
			continue;
		/* success, now patch perms if needed, no retry on this one */
		err = 0;
		if (match == 2)
			err = !msc1_match_perm(mrc->mrc_ipath, mrc->mrc_path);
		if (err)
			break;
		mrc->mrc_access = ACCESS_BOTH;
		/* now try to upgrade lock to user request */
		if (mrc->mrc_access != access)
			if (msc1_lock(mrc, access|ACCESS_BOTH) == 0)
				break;
		return(1);
	}
	msc1_abort(mrc);
	return(0);
}

/*
 * open a folder read only and lock it, open index file if it is there
 * complicated protocol to avoid race conditions
 */
int
msc1_read_folder(mrc_t *mrc, int access)
{
	int fd;
	int fd1;
	struct stat sb;
	struct stat sb1;
	int i;
	int match;
	int err;

	for (i = 0; i < conf_lock_timeout; (i++, msc1_abort(mrc), sleep(1))) {
		match = 0;
		/* open folder and lock it */
		fd = nolink_open(mrc->mrc_path, O_RDONLY);
		if (fd < 0)
			break;
		mrc->mrc_folder_fp = fdopen(fd, "r");
		if (msc1_lock_work(mrc, ACCESS_FOLDER, 0) != ACCESS_FOLDER)
			continue;
		mrc->mrc_access = ACCESS_FOLDER;
		/* create or open index and lock it */
		fd = nolink_open(mrc->mrc_ipath, O_RDONLY,
			SCOMS_PERM&conf_umask);
		if (fd >= 0) {
			mrc->mrc_index_fp = fdopen(fd, "r");
			if (msc1_lock_work(mrc, ACCESS_BOTH, 0) != ACCESS_BOTH)
				continue;
			mrc->mrc_access |= ACCESS_INDEX;
		}
		/* verify our folder file was not unlinked out from under us */
		err = 0;
		err |= fstat(fileno(mrc->mrc_folder_fp), &sb);
		err |= stat(mrc->mrc_path, &sb1);
		if (err || (sb.st_ino != sb1.st_ino))
			continue;
		if (mrc->mrc_index_fp) {
			/* verify index file not unlinked out from under us */
			err = 0;
			err |= fstat(fileno(mrc->mrc_index_fp), &sb);
			err |= stat(mrc->mrc_ipath, &sb1);
			if (err || (sb.st_ino != sb1.st_ino))
				continue;
		}
		/* now try to upgrade lock to user request */
		if (mrc->mrc_access != access)
			if (msc1_lock(mrc, access|ACCESS_BOTH) == 0)
				break;
		return(1);
	}
	msc1_abort(mrc);
	return(0);
}

klog(a, b, c, d)
char *a;
{
	static int fd = -1;
	static int pidlen;
	static char buf[128];

	if (fd < 0)
		fd = open("/gem/kmh/usr/src/common/cmd/cmd-mail/tests/system/log", O_APPEND|O_WRONLY);
	if (pidlen == 0) {
		sprintf(buf, "%d %d ", getpid(), getuid());
		pidlen = strlen(buf);
	}	
	if (fd >= 0) {
		sprintf(buf + pidlen, a, b, c, d);
		write(fd, buf, strlen(buf));
	}
}
