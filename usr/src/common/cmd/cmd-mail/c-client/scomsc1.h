#ident	"@(#)scomsc1.h	11.2"
/*
 * SCO Message Store version 1 Cache header file
 */

/*
 * the index file contains one master record followed by
 * as many per-message records as needed
 * the size of the per-message records is fixed allowing
 * direct access via a simple offset calculation
 */

/* the following are masked */
#define SCOMS_PERM		0666	/* created folder permission */
#define SCOMS_DPERM		0777	/* created directory permission */
/* this is not masked */
#define SCOMS_INBOX_PERM	0600	/* INBOX is always 0700 */

/* data type lengths */
#define SCOMS_VSN		40	/* version string length */
#define SCOMS_INT		4	/* integer byte length */
#define SCOMS_CONSISTENCY	100	/* consistency string len */

/* access types, (lock types) */
#define ACCESS_SE	0x1		/* session access, lock file contents */
#define ACCESS_AP	0x2		/* append access, ability to append */
#define ACCESS_RDX	0x4		/* read exclusive, lock out readers */
#define ACCESS_RD	0x8		/* read access, locks out RDX */
#define ACCESS_TMP	0x10		/* lock the temp file */
#define ACCESS_FOLDER	0x20		/* folder transient lock */
#define ACCESS_INDEX	0x40		/* index transient lock */
#define ACCESS_BOTH	(ACCESS_FOLDER|ACCESS_INDEX)
#define ACCESS_REBUILD	(ACCESS_SE|ACCESS_AP|ACCESS_RDX|ACCESS_BOTH)
#define ACCESS_WRITE	(ACCESS_SE|ACCESS_AP|ACCESS_RDX)

#define MMDFSTR		"\n"
#define MMDFSIZ		(sizeof(MMDFSTR)-1)
#define STAT_RESERVE	6		/* number of status bytes to reserve */
#define STAT_LINES	4		/* number of internal status lines */

#define STATUS_STR	"Status"
#define STATUS_SIZ	(sizeof(STATUS_STR)-1)
#define XSTATUS_STR	"X-Status"
#define XSTATUS_SIZ	(sizeof(XSTATUS_STR)-1)
#define UID_STR		"X-SCO-UID"
#define UID_SIZ		(sizeof(UID_STR)-1)
#define PAD_STR		"X-SCO-PAD"
#define PAD_SIZ		(sizeof(PAD_STR)-1)
#define CONT_STR	"Content-Length"
#define CONT_SIZ	(sizeof(CONT_STR)-1)
#define DATE_STR	"Date"
#define MAGIC_STR	"UNIX Message Store "
#define MAGIC_VSN	"1.0a"
#define DELETED_EFROM	"From deleted Thu Aug  1 10:20:30 1996\n"
#define DELETED_EFROMSIZ (sizeof(DELETED_EFROM) - 1)
#define DELETED_FROM	"From: deleted\n"
#define DELETED_SUBJECT	"Subject: deleted\n"
#define DATE_SIZ	25

#define MSC1_UPDATE	0x200		/* index being updated */
#define FLAGS_INTERNAL	(MSC1_UPDATE)

#define MSCBUFSIZ	10240		/* large buffer */
#define MSCSBUFSIZ	512		/* small buffer */

/* per message record on disk format, marshaled to little endian */
typedef struct msc1_message_rec_disk {
	unsigned char prd_start[SCOMS_INT];	/* strt of msg, skip ctrl-A's */
	unsigned char prd_size[SCOMS_INT];	/* size of msg, skip ctrl-A's */
	unsigned char prd_hdrstart[SCOMS_INT];	/* start of rfc822 header */
	unsigned char prd_hdrlines[SCOMS_INT];	/* lines in hdr (LF count) */
	unsigned char prd_bodystart[SCOMS_INT];	/* header size, wi envelope */
	unsigned char prd_bodylines[SCOMS_INT];	/* lines in body (LF count) */
	unsigned char prd_uid[SCOMS_INT];	/* UID of message */
	unsigned char prd_stat_start[SCOMS_INT];/* offset of status headers */
	unsigned char prd_content_len[SCOMS_INT];/* content-length present */
	unsigned char prd_status[SCOMS_INT];	/* status flags */
	/* MESSAGECACHE compatible date values */
	unsigned char prd_year;			/* offset from 1969 */
	unsigned char prd_month;		/* month 1-12, 0 is invalid */
	unsigned char prd_day;			/* day 1-n */
	unsigned char prd_hours;		/* hours */
	unsigned char prd_minutes;		/* minutes 0-59 */
	unsigned char prd_seconds;		/* seconds 0-59 */
	unsigned char prd_zoccident;		/* timezone sign */
	unsigned char prd_zhours;		/* timezone hours */
	unsigned char prd_zminutes;		/* timezone minutes */
} prd_t;

/* per message record in-core copy */
typedef struct msc1_message_rec_core {
	int prc_start;				/* strt of msg, skip ctrl-A's */
	int prc_size;				/* size of msg, skip ctrl-A's */
	int prc_hdrstart;			/* start of rfc822 header */
	int prc_hdrlines;			/* # of header lines */
	int prc_bodystart;			/* header size, wi envelope */
	int prc_bodylines;			/* # of lines in body */
	int prc_uid;				/* UID of message */
	int prc_stat_start;			/* offset of status headers */
	int prc_content_len;			/* content-length present */
	int prc_status;		/* status flags as standard C-Client bits */
	/* MESSAGECACHE compatible date values */
	unsigned char prc_year;			/* offset from 1969 */
	unsigned char prc_month;		/* month 1-12, 0 is invalid */
	unsigned char prc_day;			/* day 0-n */
	unsigned char prc_hours;		/* hours */
	unsigned char prc_minutes;		/* minutes 0-59 */
	unsigned char prc_seconds;		/* seconds 0-59 */
	unsigned char prc_zoccident;		/* timezone sign */
	unsigned char prc_zhours;		/* timezone hours */
	unsigned char prc_zminutes;		/* timezone minutes */
} prc_t;

/* master record on disk copy (marshaled to little endian byte order) */
typedef struct msc1_master_rec_disk {
	char mrd_magic[SCOMS_VSN];		/* version stamp */
	unsigned char mrd_fsize[SCOMS_INT];	/* size of folder on disk */
	unsigned char mrd_mtime[SCOMS_INT];	/* mod time of folder */
	unsigned char mrd_validity[SCOMS_INT];	/* creation date of folder */
	unsigned char mrd_uid_next[SCOMS_INT];	/* next uid to be allocated */
	unsigned char mrd_msgs[SCOMS_INT];	/* message count */
	unsigned char mrd_mmdf[SCOMS_INT];	/* Sendmail or MMDF folder */
	unsigned char mrd_status[SCOMS_INT];	/* marks partial operations */
	/* consistency string, last N bytes of folder */
	char mrd_consistency[SCOMS_CONSISTENCY];/* null terminated last N bytes*/
} mrd_t;

/* master record in-core copy, all per folder info here */
typedef struct msc1_master_rec_core {
	int mrc_fsize;
	int mrc_mtime;
	int mrc_validity;
	int mrc_uid_next;
	int mrc_msgs;
	int mrc_mmdf;
	int mrc_status;
	char mrc_consistency[SCOMS_CONSISTENCY];
	/* in-core only variables */
	char mrc_path[MAXPATHLEN*2];		/* pathname of folder file */
	char mrc_ipath[MAXPATHLEN*2];		/* pathname of index file */
	char mrc_tpath[MAXPATHLEN*2];		/* pathname of temp file */
	char mrc_lpath[MAXPATHLEN*2];		/* pathname of lock file */
	FILE *mrc_folder_fp;			/* file descriptor of folder */
	FILE *mrc_index_fp;			/* index file descriptor */
	FILE *mrc_temp_fp;			/* temp file descriptor */
	int mrc_access;				/* access type: SE, AP, RD */
	int mrc_oldaccess;			/* saves previous lock state */
	int mrc_coldaccess;			/* same but for scomsc1_check */
	prc_t *mrc_prc;				/* per message array */
	int mrc_psize;				/* size of prc array */
	int mrc_org_uid_next;			/* used during rebuild */
	int mrc_dead;				/* mbox killed under us */
} mrc_t;

/* folder statistics struct, returned by check routine */
typedef struct msc1_folder_stat {
	int m_validity;		/* folder validity value */
	int m_msgs;		/* total messages in folder */
} scomsc1_stat_t;

/* configuration variables from /etc/default/mail */
int conf_mmdf;			/* default folder format */
char *conf_inbox;		/* inbox pathname, built from the others */
char *conf_inbox_dir;		/* default directory for inbox */
char *conf_inbox_name;		/* default name for inbox */
int conf_fsync;			/* use fsync during folder updates */
int conf_extended_checks;	/* use extended validity checking on open */
int conf_expunge_threshold;	/* percent expunge threshold */
int conf_long_cache;		/* use long cache */
int conf_file_locking;		/* enable folder.lock style locking */
int conf_lock_timeout;		/* locking timeout in seconds */
int conf_umask;			/* mail folder (and directory) umask */

/* cache API protos */
int scomsc1_valid(char *);
void *scomsc1_open(char *, int, int);
int scomsc1_chopen(void *, int);
void scomsc1_close(void *);
int scomsc1_create(char *);
int scomsc1_remove(char *);
int scomsc1_rename(char *, char *);
char *scomsc1_fetch(void *, char *, int, int);
int scomsc1_fetchsize(void *, int, int);
int scomsc1_fetchlines(void *, int, int);
int scomsc1_fetchdate(void *, int, MESSAGECACHE *);
int scomsc1_fetchuid(void *, int);
int scomsc1_fetchsender(void *, int, char *);
int scomsc1_setflags(void *, int, int);
int scomsc1_getflags(void *, int);
int scomsc1_expunge(void *);
int scomsc1_deliver(char *, char *, char *, char *, int, int);
int scomsc1_deliverf(char *, char *, char *, FILE *, int);
scomsc1_stat_t *scomsc1_check(void *);

/* testing API calls */
void scomsc1_set_rebuild_callback(void(*)());

/* internal utility protos */ 
void msc1_close(mrc_t *);
int msc1_lock(mrc_t *, int);
int msc1_lock_work(mrc_t *, int, int);
void msc1_lock_tmp_to_folder(mrc_t *);
int msc1_folder_valid(FILE *);
int msc1_index(mrc_t *, int);
int msc1_rebuild(mrc_t *, int, int);
int msc1_read_index(mrc_t *);
int msc1_read_mrc(mrc_t *);
int msc1_read_prcs(mrc_t *);
int msc1_read_prc(mrc_t *, int);
int msc1_set_old(mrc_t *);
int strxccmp(char *, char *, int);
int strhccmp(char *, char *, int);
int strccmp(char *, char *);
int strnccmp(char *, char *, int);
int msc1_status_out(mrc_t *, prc_t *, int, int);
int msc1_status_in(char *, int);
int msc1_eoh(mrc_t *, char *, int);
int msc1_eom(mrc_t *, char *);
int msc1_expand(FILE *, int);
int msc1_expand_folder(mrc_t *);
int msc1_expand_index(mrc_t *);
int msc1_expunge(mrc_t *);
int msc1_write_index(mrc_t *);
void msc1_mrc_in(mrc_t *, mrd_t *);
void msc1_prc_in(prc_t *, prd_t *);
int msc1_int_in(unsigned char *);
void msc1_int_out(unsigned char *, int);
int msc1_write_mrc(mrc_t *);
void msc1_mrc_out(mrd_t *, mrc_t *);
int msc1_write_prc(mrc_t *, int);
int msc1_write_prcp(mrc_t *, prc_t *, int);
void msc1_prc_out(prd_t *, prc_t *);
int msc1_get_consistency(mrc_t *, char *);
int msc1_set_consistency(mrc_t *);
void msc1_init_mrc(mrc_t *, char *);
void msc1_date_sendmail_to_prc(prc_t *, char *);
int msc1_ok_to_rebuild_index(mrc_t *mrc);
void msc1_init(int);
void msc1_init_parse(char *);
void msc1_build_home_dir(char *, struct passwd *);
void msc1_parse_address(char *, char *);
int msc1_fcopy(char *dst, char *src);
void msc1_strip_quotes(char *);
int msc1_fgets(char *, int, FILE *);
int msc1_match_perm(char *, char *);
int msc1_content_valid(mrc_t *, int, int, char *, int);
#ifdef OpenServer
void msc1_parse_mmdftailor();
#endif
int msc1_create_folder(mrc_t *, int, int);
int msc1_open_folder(mrc_t *, int);
int msc1_read_folder(mrc_t *, int);
void msc1_abort(mrc_t *);
