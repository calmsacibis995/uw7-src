/*
 * Program:	UNIX environment routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	15 October 1996
 *
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* c-client environment parameters */

static char *myUserName = NIL;	/* user name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myLocalHost = NIL;	/* local host name */
static char *myNewsrc = NIL;	/* newsrc file name */
static char *sysInbox = NIL;	/* system inbox name */
static char *newsActive = NIL;	/* news active file */
static char *newsSpool = NIL;	/* news spool */
static char *blackBoxDir = NIL;	/* black box directory name */
static int blackBox = NIL;	/* is a black box */
static long mbx_protection = 0600;
static long sub_protection = 0600;
static long lock_protection = 0666;
static long disableFcntlLock =	/* flock() emulator is a no-op */
#ifdef SVR4_DISABLE_FLOCK
  T
#else
  NIL
#endif
  ;
static long lockEaccesError =	/* warning on EACCES errors on .lock files */
#ifdef WARN_LOCK_EACCES_ERRORS
  T
#else
  NIL
#endif
  ;
static MAILSTREAM *defaultProto = NIL;
static char *userFlags[NUSERFLAGS] = {NIL};

#include "write.c"		/* include safe writing routines */
#include "writev.c"		/* include safe writing routines */

/* Environment manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *env_parameters (function,value)
	long function;
	void *value;
{
  switch ((int) function) {
  case SET_USERNAME:
    if (myUserName) fs_give ((void **) &myUserName);
    myUserName = cpystr ((char *) value);
    break;
  case GET_USERNAME:
    value = (void *) myUserName;
    break;
  case SET_HOMEDIR:
    if (myHomeDir) fs_give ((void **) &myHomeDir);
    myHomeDir = cpystr ((char *) value);
    break;
  case GET_HOMEDIR:
    value = (void *) myHomeDir;
    break;
  case SET_LOCALHOST:
    if (myLocalHost) fs_give ((void **) &myLocalHost);
    myLocalHost = cpystr ((char *) value);
    break;
  case GET_LOCALHOST:
    value = (void *) myLocalHost;
    break;
  case SET_NEWSRC:
    if (myNewsrc) fs_give ((void **) &myNewsrc);
    myNewsrc = cpystr ((char *) value);
    break;
  case GET_NEWSRC:
    value = (void *) myNewsrc;
    break;
  case SET_NEWSACTIVE:
    if (newsActive) fs_give ((void **) &newsActive);
    newsActive = cpystr ((char *) value);
    break;
  case GET_NEWSACTIVE:
    value = (void *) newsActive;
    break;
  case SET_NEWSSPOOL:
    if (newsSpool) fs_give ((void **) &newsSpool);
    newsSpool = cpystr ((char *) value);
    break;
  case GET_NEWSSPOOL:
    value = (void *) newsSpool;
    break;
  case SET_SYSINBOX:
    if (sysInbox) fs_give ((void **) &sysInbox);
    sysInbox = cpystr ((char *) value);
    break;
  case GET_SYSINBOX:
    value = (void *) sysInbox;
    break;

  case SET_MBXPROTECTION:
    mbx_protection = (long) value;
    break;
  case GET_MBXPROTECTION:
    value = (void *) mbx_protection;
    break;
  case SET_SUBPROTECTION:
    sub_protection = (long) value;
    break;
  case GET_SUBPROTECTION:
    value = (void *) sub_protection;
    break;
  case SET_LOCKPROTECTION:
    lock_protection = (long) value;
    break;
  case GET_LOCKPROTECTION:
    value = (void *) lock_protection;
    break;
  case SET_DISABLEFCNTLLOCK:
    disableFcntlLock = (long) value;
    break;
  case GET_DISABLEFCNTLLOCK:
    value = (void *) disableFcntlLock;
    break;
  case SET_LOCKEACCESERROR:
    lockEaccesError = (long) value;
    break;
  case GET_LOCKEACCESERROR:
    value = (void *) lockEaccesError;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Write current time
 * Accepts: destination string
 *	    optional format of day-of-week prefix
 *	    format of date and time
 *	    flag whether to append symbolic timezone
 */

static void do_date (date,prefix,fmt,suffix)
	char *date;
	char *prefix;
	char *fmt;
	int suffix;
{
  time_t tn = time (0);
  struct tm *t = gmtime (&tn);
  int zone = t->tm_hour * 60 + t->tm_min;
  int julian = t->tm_yday;
  t = localtime (&tn);		/* get local time now */
				/* minus UTC minutes since midnight */
  zone = t->tm_hour * 60 + t->tm_min - zone;
  /* julian can be one of:
   *  36x  local time is December 31, UTC is January 1, offset -24 hours
   *    1  local time is 1 day ahead of UTC, offset +24 hours
   *    0  local time is same day as UTC, no offset
   *   -1  local time is 1 day behind UTC, offset -24 hours
   * -36x  local time is January 1, UTC is December 31, offset +24 hours
   */
  if (julian = t->tm_yday -julian)
    zone += ((julian < 0) == (abs (julian) == 1)) ? -24*60 : 24*60;
  if (prefix) {			/* want day of week? */
    sprintf (date,prefix,days[t->tm_wday]);
    date += strlen (date);	/* make next sprintf append */
  }
				/* output the date */
  sprintf (date,fmt,t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60);
				/* append timezone suffix if desired */
  if (suffix) rfc822_timezone (date,(void *) t);
}


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void rfc822_date (date)
	char *date;
{
  do_date (date,"%s, ","%d %s %d %02d:%02d:%02d %+03d%02d",T);
}


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (date)
	char *date;
{
  do_date (date,NIL,"%02d-%s-%d %02d:%02d:%02d %+03d%02d",NIL);
}

/* Initialize environment
 * Accepts: user name
 *	    home directory name
 * Returns: T, always
 */

long env_init (user,home)
	char *user;
	char *home;
{
#ifdef SCOMS
  extern void msc1_init(int);
  extern char *conf_inbox;
#endif
  extern MAILSTREAM STDPROTO;
  struct stat sbuf;
  char tmp[MAILTMPLEN];
  struct hostent *host_name;
  if (myUserName) fatal ("env_init called twice!");
  myUserName = cpystr (user);	/* remember user name */
  myHomeDir = cpystr (home);	/* and home directory */
#ifndef SCOMS
  sprintf (tmp,"%s/%s",MAILSPOOL,myUserName);
  sysInbox = cpystr (tmp);	/* system inbox is from mail spool */
#else
  msc1_init(0);
  sysInbox = cpystr(conf_inbox);
#endif
  dorc ("/etc/imapd.conf");	/* do systemwide */
  if (blackBoxDir) {		/* build black box directory name */
    sprintf (tmp,"%s/%s",blackBoxDir,myUserName);
				/* if black box if exists and directory */
    if (blackBox = (!stat (tmp,&sbuf)) && (sbuf.st_mode & S_IFDIR)) {
				/* flush standard values */
      fs_give ((void **) &myHomeDir);
      fs_give ((void **) &sysInbox);
      myHomeDir = cpystr (tmp);	/* set black box values in their place */
      sysInbox = cpystr (strcat (tmp,"/INBOX"));
    }
  }
  else blackBoxDir = "";	/* make sure user rc files don't try this */
				/* do user rc files */
  dorc (strcat (strcpy (tmp,myhomedir ()),"/.mminit"));
  dorc (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"));
  if (!myLocalHost) {		/* have local host yet? */
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    myLocalHost = cpystr ((host_name = gethostbyname (tmp)) ?
			  host_name->h_name : tmp);
  }
  if (!myNewsrc) {		/* set news file name if not defined */
    sprintf (tmp,"%s/.newsrc",myhomedir ());
    myNewsrc = cpystr (tmp);
  }
  if (!newsActive) newsActive = cpystr (ACTIVEFILE);
  if (!newsSpool) newsSpool = cpystr (NEWSSPOOL);
				/* force default prototype to be set */
  if (!defaultProto) defaultProto = &STDPROTO;
				/* re-do open action to get flags */
  (*defaultProto->dtb->open) (NIL);
  return T;
}

/* Return my user name
 * Returns: my user name
 */

char *myusername ()
{
  if (!myUserName) {		/* get user name if don't have it yet */
    char *name = (char *) getlogin ();
    char *dir = getenv ("HOME");
    struct passwd *pw;
    if (((name && *name) && (pw = getpwnam (name)) &&
	 (pw->pw_uid == geteuid ())) || (pw = getpwuid (geteuid ())))
      env_init (pw->pw_name,(dir && *dir) ? dir : pw->pw_dir);
    else fatal ("Unable to look up user in passwd file");
  }
  return myUserName;
}


/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost ()
{
  if (!myLocalHost) myusername ();
  return myLocalHost;
}


/* Return my home directory name
 * Returns: my home directory name
 */

char *myhomedir ()
{
  if (!myHomeDir) myusername ();/* initialize if first time */
  return myHomeDir;
}


/* Return system standard INBOX
 * Accepts: buffer string
 */

char *sysinbox ()
{
  if (!sysInbox) myusername ();	/* call myusername() if first time */
  return sysInbox;
}

/* Return mailbox file name
 * Accepts: destination buffer
 *	    mailbox name
 * Returns: file name or NIL if error
 */

char *mailboxfile (dst,name)
	char *dst;
	char *name;
{
  struct passwd *pw;
  char *s,*t,tmp[MAILTMPLEN];
  char *dir = myhomedir ();
  *dst = '\0';			/* erase buffer */
  if (blackBox && strstr (name,"/..")) dst = NIL;
  else switch (*name) {		/* dispatch based on first character */
  case '*':			/* bboard? */
    if ((pw = getpwnam ("ftp")) && pw->pw_dir && !strstr (name,"..") &&
	!strstr (name,"//") && !strstr (name,"/~")) {
      sprintf (dst,"%s/%s",pw->pw_dir,name + 1);
      break;
    }
				/* drops in */
  case '#':			/* namespace name? */
    dst = NIL;			/* definite error */
    break;			/* can't be used */
  default:			/* any other name, ignore extraneous context */
    if ((s = strstr (name,"/~")) || (s = strstr (name,"//"))) name = s + 1;
    switch (*name) {		/* dispatch based on first character */
    case '/':			/* absolute file path */
      if (blackBox && strncmp (name,sysInbox,strlen (myHomeDir)+1)) dst = NIL;
      else strcpy (dst,name);
      break;
    case '.':			/* these names may be private */
      if (blackBox && (name[1] == '.')) dst = NIL;
      else sprintf (dst,"%s/%s",dir,name);
      break;
    case '~':			/* home directory */
      if (blackBox) dst = NIL;	/* don't permit this for now */
      else {
	if (name[1] != '/') {	/* if not want my own home directory */
	  strcpy (tmp,name + 1);/* copy user name */
	  if (name = strchr (tmp,'/')) *name++ = '\0';
	  else name = "";	/* prevent code before from being surprised */
				/* look it up in password file */
	  if (!(dir = ((pw = getpwnam (tmp)) ? pw->pw_dir : NIL))) break;
	}
	else name += 2;		/* skip past user name */
	sprintf (dst,"%s/%s",dir,name);
      }
      break;
    default:			/* some other name */
				/* non-INBOX name is in home directory */
      if (strcmp (ucase (strcpy (tmp,name)),"INBOX"))
	sprintf (dst,"%s/%s",dir,name);
				/* blackbox INBOX is always in home dir */
      else if (blackBox) sprintf (dst,"%s/INBOX",dir);
				/* empty name means driver selects INBOX */
      break;
    }
  }
  return dst;
}

/* Build status lock file name
 * Accepts: scratch buffer
 *	    file name
 * Returns: name of file to lock or NIL if error
 */

char *lockname (tmp,fname)
	char *tmp;
	char *fname;
{
  char *s = strrchr (fname,'/');
  struct stat sbuf;
  if (stat (fname,&sbuf))	/* get file status */
    sprintf (tmp,"/tmp/.%s",s ? s : fname);
  else sprintf (tmp,"/tmp/.%hx.%lx",sbuf.st_dev,sbuf.st_ino);
  return chk_notsymlink (tmp,T) ? tmp : NIL;
}


/* Check to make sure not a symlink
 * Accepts: file name
 	    check hard link flag
 * Returns: T if not symlink, NIL if symlink
 */

long chk_notsymlink (name, chkhard)
	char *name;
	long chkhard;
{
  struct stat sbuf;
				/* OK if not exists */
  if (lstat (name,&sbuf)) return T;
  if ((sbuf.st_mode & S_IFMT) == S_IFLNK) {
    mm_log ("SECURITY ALERT: symbolic link on lock name!",ERROR);
    syslog (LOG_CRIT,"SECURITY PROBLEM: lock file %s is a symbolic link",name);
    return NIL;
  }
  if (chkhard && (sbuf.st_nlink > 1)) {
    mm_log ("SECURITY ALERT: hard link to lock name!",ERROR);
    syslog (LOG_CRIT,"SECURITY PROBLEM: lock file %s has a hard link",name);
    return NIL;
  }
  return T;			/* OK */
}

/* Determine default prototype stream to user
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto ()
{
  myusername ();		/* make sure initialized */
  return defaultProto;		/* return default driver's prototype */
}


/* Set up user flags for stream
 * Accepts: MAIL stream
 * Returns: MAIL stream with user flags set up
 */

MAILSTREAM *user_flags (stream)
	MAILSTREAM *stream;
{
  int i;
  myusername ();		/* make sure initialized */
  for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = userFlags[i];
  return stream;
}

/* Process rc file
 * Accepts: file name
 */

void dorc (file)
	char *file;
{
  int i;
  char *s,*t,*k,tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  extern MAILSTREAM STDPROTO;
  DRIVER *d;
  FILE *f = fopen (file,"r");
  if (!f) return;		/* punt if no file */
  while ((s = fgets (tmp,MAILTMPLEN,f)) && (t = strchr (s,'\n'))) {
    *t++ = '\0';		/* tie off line, find second space */
    if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
      *k++ = '\0';		/* tie off two words*/
      lcase (s);		/* make case-independent */
      if (!(defaultProto || strcmp (s,"set empty-folder-format"))) {
	if (!strcmp (lcase (k),"same-as-inbox"))
	  defaultProto = ((d = mail_valid (NIL,"INBOX",NIL)) &&
			  strcmp (d->name,"dummy")) ?
			    ((*d->open) (NIL)) : &STDPROTO;
	else if (!strcmp (k,"system-standard")) defaultProto = &STDPROTO;
	else {			/* see if a driver name */
	  for (d = (DRIVER *) mail_parameters (NIL,GET_DRIVERS,NIL);
	       d && strcmp (d->name,k); d = d->next);
	  if (d) defaultProto = (*d->open) (NIL);
	  else {		/* duh... */
	    sprintf (tmpx,"Unknown empty folder format in %s: %s",file,k);
	    mm_log (tmpx,WARN);
	  }
	}
      }
      else if (!(userFlags[0] || strcmp (s,"set keywords"))) {
	k = strtok (k,", ");	/* yes, get first keyword */
				/* copy keyword list */
	for (i = 0; k && i < NUSERFLAGS; ++i) {
	  if (userFlags[i]) fs_give ((void **) &userFlags[i]);
	  userFlags[i] = cpystr (k);
	  k = strtok (NIL,", ");
	}
      }
      else if (!strcmp (s,"set from-widget"))
	mail_parameters (NIL,SET_FROMWIDGET,strcmp (lcase (k),"header-only") ?
			 (void *) T : NIL);
      else if (!strcmp (s,"set black-box-directory")) {
	if (blackBoxDir)	/* users aren't allowed to do this */
	  mm_log ("Can't set black-box-directory in user init",ERROR);
	else blackBoxDir = cpystr (k);
      }
      else if (!strcmp (s,"set local-host")) {
	fs_give ((void **) &myLocalHost);
	myLocalHost = cpystr (k);
      }
      else if (!strcmp (s,"set news-active-file")) {
	fs_give ((void **) &newsActive);
	newsActive = cpystr (k);
      }
      else if (!strcmp (s,"set news-spool-directory")) {
	fs_give ((void **) &newsSpool);
	newsSpool = cpystr (k);
      }
    }
    s = t;			/* try next line */
  }
  fclose (f);			/* flush the file */
}
