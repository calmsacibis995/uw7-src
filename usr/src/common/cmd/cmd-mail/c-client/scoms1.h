#ident	"@(#)scoms1.h	11.1"
/*
 * SCO Message Store version 1
 */

/* Build parameters */

#define KODRETRY 15		/* kiss-of-death retry in seconds */
#define LOCKTIMEOUT 5		/* lock timeout in minutes */

/* SCOMS1 I/O stream local data */

typedef struct scoms1_local {
  void *handle;			/* our message store handle */
  int rdonly;			/* our access type */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} SCOMS1LOCAL;


/* Convenient access to local data */

#define LOCAL ((SCOMS1LOCAL *) stream->local)

/* Function prototypes */

DRIVER *scoms1_valid (char *name);
long scoms1_isvalid (char *name,char *tmp);
long scoms1_isvalid_fd (int fd,char *tmp);
void *scoms1_parameters (long function,void *value);
void scoms1_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void scoms1_list (MAILSTREAM *stream,char *ref,char *pat);
void scoms1_lsub (MAILSTREAM *stream,char *ref,char *pat);
long scoms1_create (MAILSTREAM *stream,char *mailbox);
long scoms1_delete (MAILSTREAM *stream,char *mailbox);
long scoms1_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *scoms1_open (MAILSTREAM *stream);
void scoms1_close (MAILSTREAM *stream,long options);
void scoms1_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void scoms1_fetchflags (MAILSTREAM *stream,char *sequence,long flags);
ENVELOPE *scoms1_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
				 BODY **body,long flags);
char *scoms1_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			  STRINGLIST *lines,unsigned long *len,long flags);
char *scoms1_fetchtext (MAILSTREAM *stream,unsigned long msgno,
			unsigned long *len,long flags);
char *scoms1_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
			unsigned long *len,long flags);
void scoms1_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void scoms1_clearflag (MAILSTREAM *stream,char *sequence,char *flag,
		       long flags);
long scoms1_ping (MAILSTREAM *stream);
void scoms1_check (MAILSTREAM *stream);
void scoms1_expunge (MAILSTREAM *stream);
long scoms1_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
		  long options);
long scoms1_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		    STRING *message);
long scoms1_append_putc (int fd,char *s,int *i,char c);
void scoms1_gc (MAILSTREAM *stream,long gcflags);
long scoms1_deliver (MAILSTREAM *stream,char *mailbox,
		char *sender,FILE *fd);

/* internal protos */
void scoms1_update_status (MAILSTREAM *stream, MESSAGECACHE *elt);
void scoms1_abort (MAILSTREAM *stream);

/* no longer needed ?
char *scoms1_file (char *dst,char *name);
int scoms1_lock (char *file,int flags,int mode,char *lock,int op);
void scoms1_unlock (int fd,MAILSTREAM *stream,char *lock);
int scoms1_parse (MAILSTREAM *stream);
char *scoms1_eom (char *som,char *sod,long i);
int scoms1_extend (MAILSTREAM *stream,int fd,char *error);
void scoms1_save (MAILSTREAM *stream,int fd);
unsigned long scoms1_pseudo (MAILSTREAM *stream,char *hdr);
unsigned long scoms1_xstatus (char *status,FILECACHE *m,long flag);
*/
