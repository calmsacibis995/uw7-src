/*----------------------------------------------------------------------

            T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade and Mike Seibel
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   Administration Builiding, AG-44
   Seattle, Washington, 98195, USA
   Internet: lgl@CAC.Washington.EDU
             mikes@CAC.Washington.EDU

   Please address all bugs and comments to "pine-bugs@cac.washington.edu"


   Pine and Pico are registered trademarks of the University of Washington.
   No commercial use of these trademarks may be made without prior written
   permission of the University of Washington.

   Pine, Pico, and Pilot software and its included text are Copyright
   1989-1996 by the University of Washington.

   The full text of our legal notices is contained in the file called
   CPYRIGHT, included with this distribution.


   Pine is in part based on The Elm Mail System:
    ***********************************************************************
    *  The Elm Mail System  -  Revision: 2.13                             *
    *                                                                     *
    * 			Copyright (c) 1986, 1987 Dave Taylor              *
    * 			Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************
 

  ----------------------------------------------------------------------*/

/*======================================================================

 This contains most of Pine's interface to the local operating system
and hardware.  Hopefully this file, os-xxx.h and makefile.xxx are the
only ones that have to be modified for most ports.  Signals.c, ttyin.c,
and ttyout.c also have some dependencies.  See the doc/tech-notes for
notes on porting Pine to other platforms.  Here is a list of the functions
required for an implementation:


  File System Access
     can_access          -- See if a file can be accessed
     name_file_size      -- Return the number of bytes in the file (by name)
     fp_file_size        -- Return the number of bytes in the file (by FILE *)
     name_file_mtime     -- Return the mtime of a file (by name)
     fp_file_mtime       -- Return the mtime of a file (by FILE *)
     file_attrib_copy    -- Copy attributes of one file to another.
     is_writable_dir     -- Check to see if directory exists and is writable
     create_mail_dir     -- Make a directory
     rename_file         -- change name of a file
     build_path          -- Put together a file system path
     last_cmpnt          -- Returns pointer to last component of path
     expand_foldername   -- Expand a folder name to full path
     fnexpand            -- Do filename exansion for csh style "~"
     filter_filename     -- Make sure file name hasn't got weird chars
     cntxt_allowed       -- Check whether a pathname is allowed for read/write
     disk_quota          -- Check the user's disk quota
     read_file           -- Read whole file into memory (for small files)
     create_tmpfile      -- Just like ANSI C tmpfile function
     temp_nam            -- Almost like common tempnam function
     fget_pos,fset_pos   -- Just like ANSI C fgetpos, fsetpos functions

  Abort
     coredump            -- Abort running Pine dumping core if possible

  System Name and Domain
     hostname            -- Figure out the system's host name, only
                              used internally in this file.
     getdomainnames      -- Figure out the system's domain name
     canonical_name      -- Returns canonical form of host name

  Job Control
     have_job_control    -- Returns 1 if job control exists
     stop_process        -- What to do to stop process when it's time to stop
			      (only used if have_job_control returns 1)

  System Error Messages (in case given one is a problem)
     error_description   -- Returns string describing error

  System Password and Accounts
     gcos_name           -- Parses full name from system, only used
			      locally in this file so if you don't use it you
			      don't need it
     get_user_info       -- Finds in login name, full name, and homedir
     local_name_lookup   -- Get full name of user on system
     change_passwd       -- Calls system password changer

  MIME utilities
     mime_can_display    -- Can we display this type/subtype?
     exec_mailcap_cmd    -- Run the mailcap command to view a type/subtype.
     exec_mailcap_test_cmd -- Run mailcap test= test command.

  Other stuff
     srandom             -- Dummy srandom if you don't have this function
     init_debug
     do_debug
     save_debug_on_crash

  ====*/


#include "headers.h"



/*----------------------------------------------------------------------
       Check if we can access a file in a given way

   Args: file      -- The file to check
         mode      -- The mode ala the access() system call, see ACCESS_EXISTS
                      and friends in pine.h.

 Result: returns 0 if the user can access the file according to the mode,
         -1 if he can't (and errno is set).
 ----*/
int
can_access(file, mode)
    char *file;
    int   mode;
{
    return(access(file, mode));
}


/*----------------------------------------------------------------------
       Check if we can access a file in a given way in the given path

   Args: path     -- The path to look for "file" in
	 file      -- The file to check
         mode      -- The mode ala the access() system call, see ACCESS_EXISTS
                      and friends in pine.h.

 Result: returns 0 if the user can access the file according to the mode,
         -1 if he can't (and errno is set).
 ----*/
can_access_in_path(path, file, mode)
    char *path, *file;
    int   mode;
{
    char tmp[MAXPATH], *path_copy, *p, *t;
    int  rv = -1;

    if(!path || !*path || *file == '/'){
	rv = access(file, mode);
    }
    else if(*file == '~'){
	strcpy(tmp, file);
	rv = fnexpand(tmp, sizeof(tmp)) ? access(tmp, mode) : -1;
    }
    else{
	for(p = path_copy = cpystr(path); p && *p; p = t){
	    if(t = strindex(p, ':'))
	      *t++ = '\0';

	    sprintf(tmp, "%s/%s", p, file);
	    if((rv = access(tmp, mode)) == 0)
	      break;
	}

	fs_give((void **)&path_copy);
    }

    return(rv);
}

/*----------------------------------------------------------------------
      Return the number of bytes in given file

    Args: file -- file name

  Result: the number of bytes in the file is returned or
          -1 on error, in which case errno is valid
 ----*/
long
name_file_size(file)
    char *file;
{
    struct stat buffer;

    if(stat(file, &buffer) != 0)
      return(-1L);

    return((long)buffer.st_size);
}


/*----------------------------------------------------------------------
      Return the number of bytes in given file

    Args: fp -- FILE * for open file

  Result: the number of bytes in the file is returned or
          -1 on error, in which case errno is valid
 ----*/
long
fp_file_size(fp)
    FILE *fp;
{
    struct stat buffer;

    if(fstat(fileno(fp), &buffer) != 0)
      return(-1L);

    return((long)buffer.st_size);
}


/*----------------------------------------------------------------------
      Return the modification time of given file

    Args: file -- file name

  Result: the time of last modification (mtime) of the file is returned or
          -1 on error, in which case errno is valid
 ----*/
time_t
name_file_mtime(file)
    char *file;
{
    struct stat buffer;

    if(stat(file, &buffer) != 0)
      return((time_t)(-1));

    return(buffer.st_mtime);
}


/*----------------------------------------------------------------------
      Return the modification time of given file

    Args: fp -- FILE * for open file

  Result: the time of last modification (mtime) of the file is returned or
          -1 on error, in which case errno is valid
 ----*/
time_t
fp_file_mtime(fp)
    FILE *fp;
{
    struct stat buffer;

    if(fstat(fileno(fp), &buffer) != 0)
      return((time_t)(-1));

    return(buffer.st_mtime);
}


/*----------------------------------------------------------------------
      Copy the mode, owner, and group of sourcefile to targetfile.

    Args: targetfile -- 
	  sourcefile --
    
    We don't bother keeping track of success or failure because we don't care.
 ----*/
void
file_attrib_copy(targetfile, sourcefile)
    char *targetfile;
    char *sourcefile;
{
    struct stat buffer;

    if(stat(sourcefile, &buffer) == 0){
	chmod(targetfile, buffer.st_mode);
#if !defined(DOS) && !defined(OS2)
	chown(targetfile, buffer.st_uid, buffer.st_gid);
#endif
    }
}



/*----------------------------------------------------------------------
      Check to see if a directory exists and is writable by us

   Args: dir -- directory name

 Result:       returns 0 if it exists and is writable
                       1 if it is a directory, but is not writable
                       2 if it is not a directory
                       3 it doesn't exist.
  ----*/
is_writable_dir(dir)
    char *dir;
{
    struct stat sb;

    if(stat(dir, &sb) < 0)
      /*--- It doesn't exist ---*/
      return(3);

    if(!(sb.st_mode & S_IFDIR))
      /*---- it's not a directory ---*/
      return(2);

    if(can_access(dir, 07))
      return(1);
    else
      return(0);
}



/*----------------------------------------------------------------------
      Create the mail subdirectory.

  Args: dir -- Name of the directory to create
 
 Result: Directory is created.  Returns 0 on success, else -1 on error
	 and errno is valid.
  ----*/
create_mail_dir(dir)
    char *dir;
{
    if(mkdir(dir, 0700) < 0)
      return(-1);

    (void)chmod(dir, 0700);
    /* Some systems need this, on others we don't care if it fails */
    (void)chown(dir, getuid(), getgid());
    return(0);
}



/*----------------------------------------------------------------------
      Rename a file

  Args: tmpfname -- Old name of file
        fname    -- New name of file
 
 Result: File is renamed.  Returns 0 on success, else -1 on error
	 and errno is valid.
  ----*/
rename_file(tmpfname, fname)
    char *tmpfname, *fname;
{
    return(rename(tmpfname, fname));
}



/*----------------------------------------------------------------------
      Paste together two pieces of a file name path

  Args: pathbuf      -- Put the result here
        first_part   -- of path name
        second_part  -- of path name
 
 Result: New path is in pathbuf.  No check is made for overflow.  Note that
	 we don't have to check for /'s at end of first_part and beginning
	 of second_part since multiple slashes are ok.

BUGS:  This is a first stab at dealing with fs naming dependencies, and others 
still exist.
  ----*/
void
build_path(pathbuf, first_part, second_part)
    char *pathbuf, *first_part, *second_part;
{
    if(!first_part)
      strcpy(pathbuf, second_part);
    else
      sprintf(pathbuf, "%s%s%s", first_part,
	      (*first_part && first_part[strlen(first_part)-1] != '/')
	        ? "/" : "",
	      second_part);
}


/*----------------------------------------------------------------------
  Test to see if the given file path is absolute

  Args: file -- file path to test

 Result: TRUE if absolute, FALSE otw

  ----*/
int
is_absolute_path(path)
    char *path;
{
    return(path && (*path == '/' || *path == '~'));
}



/*----------------------------------------------------------------------
      Return pointer to last component of pathname.

  Args: filename     -- The pathname.
 
 Result: Returned pointer points to last component in the input argument.
  ----*/
char *
last_cmpnt(filename)
    char *filename;
{
    register char *p = NULL, *q = filename;

    while(q = strchr(q, '/'))
      if(*++q)
	p = q;

    return(p);
}



/*----------------------------------------------------------------------
      Expand a folder name, taking account of the folders_dir and `~'.

  Args: filename -- The name of the file that is the folder
 
 Result: The folder name is expanded in place.  
         Returns 0 and queues status message if unsuccessful.
         Input string is overwritten with expanded name.
         Returns 1 if successful.

BUG should limit length to MAXPATH
  ----*/
int
expand_foldername(filename)
    char *filename;
{
    char         temp_filename[MAXPATH+1];

    dprint(5, (debugfile, "=== expand_foldername called (%s) ===\n",filename));

    /*
     * We used to check for valid filename chars here if "filename"
     * didn't refer to a remote mailbox.  This has been rethought
     */

    strcpy(temp_filename, filename);
    if(strucmp(temp_filename, "inbox") == 0) {
        strcpy(filename, ps_global->VAR_INBOX_PATH == NULL ? "inbox" :
               ps_global->VAR_INBOX_PATH);
    } else if(temp_filename[0] == '{') {
        strcpy(filename, temp_filename);
    } else if(ps_global->restricted
	        && (strindex("./~", temp_filename[0]) != NULL
		    || srchstr(temp_filename,"/../"))){
	q_status_message(SM_ORDER, 0, 3, "Can only open local folders");
	return(0);
    } else if(temp_filename[0] == '*') {
        strcpy(filename, temp_filename);
    } else if(ps_global->VAR_OPER_DIR && srchstr(temp_filename,"..")){
	q_status_message(SM_ORDER, 0, 3,
			 "\"..\" not allowed in folder name");
	return(0);
    } else if (temp_filename[0] == '~'){
        if(fnexpand(temp_filename, sizeof(temp_filename)) == NULL) {
            char *p = strindex(temp_filename, '/');
    	    if(p != NULL)
    	      *p = '\0';
    	    q_status_message1(SM_ORDER, 3, 3,
                    "Error expanding folder name: \"%s\" unknown user",
    	       temp_filename);
    	    return(0);
        }
        strcpy(filename, temp_filename);
    } else if(temp_filename[0] == '/') {
        strcpy(filename, temp_filename);
    } else if(F_ON(F_USE_CURRENT_DIR, ps_global)){
	strcpy(filename, temp_filename);
    } else if(ps_global->VAR_OPER_DIR){
	build_path(filename, ps_global->VAR_OPER_DIR, temp_filename);
    } else {
	build_path(filename, ps_global->home_dir, temp_filename);
    }
    dprint(5, (debugfile, "returning \"%s\"\n", filename));    
    return(1);
}



struct passwd *getpwnam();

/*----------------------------------------------------------------------
       Expand the ~ in a file ala the csh (as home directory)

   Args: buf --  The filename to expand (nothing happens unless begins with ~)
         len --  The length of the buffer passed in (expansion is in place)

 Result: Expanded string is returned using same storage as passed in.
         If expansion fails, NULL is returned
 ----*/
char *
fnexpand(buf, len)
    char *buf;
    int len;
{
    struct passwd *pw;
    register char *x,*y;
    char name[20];
    
    if(*buf == '~') {
        for(x = buf+1, y = name; *x != '/' && *x != '\0'; *y++ = *x++);
        *y = '\0';
        if(x == buf + 1) 
          pw = getpwuid(getuid());
        else
          pw = getpwnam(name);
        if(pw == NULL)
          return((char *)NULL);
        if(strlen(pw->pw_dir) + strlen(buf) > len) {
          return((char *)NULL);
        }
        rplstr(buf, x - buf, pw->pw_dir);
    }
    return(len ? buf : (char *)NULL);
}



/*----------------------------------------------------------------------
    Filter file names for strange characters

   Args:  file  -- the file name to check
 
 Result: Returns NULL if file name is OK
         Returns formatted error message if it is not
  ----*/
char *
filter_filename(file)
    char *file;
{
#ifdef ALLOW_WEIRD
    static char illegal[] = {'\177', '\0'};
#else
    static char illegal[] = {'"', '#', '$', '%', '&', '\'','(', ')','*',
                          ',', ':', ';', '<', '=', '>', '?', '[', ']',
                          '\\', '^', '|', '\177', '\0'};
#endif
    static char error[100];
    char ill_file[MAXPATH+1], *ill_char, *ptr, e2[10];
    int i;

    for(ptr = file; *ptr == ' '; ptr++) ; /* leading spaces gone */

    while(*ptr && *ptr > ' ' && strindex(illegal, *ptr) == 0)
      ptr++;

    if(*ptr != '\0') {
        if(*ptr == ' ') {
            ill_char = "<space>";
        } else if(*ptr == '\n') {
            ill_char = "<newline>";
        } else if(*ptr == '\r') {
            ill_char = "<carriage return>";
        } else if(*ptr == '\t') {
    	    ill_char = "<tab>";
        } else if(*ptr < ' ') {
            sprintf(e2, "control-%c", *ptr + '@');
            ill_char = e2;
        } else if (*ptr == '\177') {
    	    ill_char = "<del>";
        } else {
    	    e2[0] = *ptr;
    	    e2[1] = '\0';
    	    ill_char = e2;
        }
        if(ptr != file) {
            strncpy(ill_file, file, ptr - file);
            ill_file[ptr - file] = '\0';
            sprintf(error,
		    "Character \"%s\" after \"%s\" not allowed in file name",
		    ill_char, ill_file);
        } else {
            sprintf(error,
                    "First character, \"%s\", not allowed in file name",
                    ill_char);
        }
            
        return(error);
    }

    if((i=is_writable_dir(file)) == 0 || i == 1){
	sprintf(error, "\"%s\" is a directory", file);
        return(error);
    }

    if(ps_global->restricted || ps_global->VAR_OPER_DIR){
	for(ptr = file; *ptr == ' '; ptr++) ;	/* leading spaces gone */

	if((ptr[0] == '.' && ptr[1] == '.') || srchstr(ptr, "/../")){
	    sprintf(error, "\"..\" not allowed in filename");
	    return(error);
	}
    }

    return((char *)NULL);
}


/*----------------------------------------------------------------------
    Check to see if user is allowed to read or write this folder.

   Args:  s  -- the name to check
 
 Result: Returns 1 if OK
         Returns 0 and posts an error message if access is denied
  ----*/
int
cntxt_allowed(s)
    char *s;
{
    struct variable *vars = ps_global->vars;
    int retval = 1;
    MAILSTREAM stream; /* fake stream for error message in mm_notify */

    if(ps_global->restricted
         && (strindex("./~", s[0]) || srchstr(s, "/../"))){
	stream.mailbox = s;
	mm_notify(&stream, "Restricted mode doesn't allow operation", WARN);
	retval = 0;
    }
    else if(VAR_OPER_DIR
	    && s[0] != '{' && !(s[0] == '*' && s[1] == '{')
	    && strucmp(s,ps_global->inbox_name) != 0
	    && strcmp(s, ps_global->VAR_INBOX_PATH) != 0){
	char *p, *free_this = NULL;

	p = s;
	if(strindex(s, '~')){
	    p = strindex(s, '~');
	    free_this = (char *)fs_get(strlen(p) + 200);
	    strcpy(free_this, p);
	    fnexpand(free_this, strlen(p)+200);
	    p = free_this;
	}
	else if(p[0] != '/'){  /* add home dir to relative paths */
	    free_this = p = (char *)fs_get(strlen(s)
					    + strlen(ps_global->home_dir) + 2);
	    build_path(p, ps_global->home_dir, s);
	}
	
	if(!in_dir(VAR_OPER_DIR, p)){
	    char err[200];

	    sprintf(err, "Not allowed outside of %s", VAR_OPER_DIR);
	    stream.mailbox = p;
	    mm_notify(&stream, err, WARN);
	    retval = 0;
	}
	else if(srchstr(p, "/../")){  /* check for .. in path */
	    stream.mailbox = p;
	    mm_notify(&stream, "\"..\" not allowed in name", WARN);
	    retval = 0;
	}

	if(free_this)
	  fs_give((void **)&free_this);
    }
    
    return retval;
}



#if defined(USE_QUOTAS)

#include <sys/fs/ufs_quota.h>
#define MTABNAME "/etc/mnttab"

static char *device_name();

/*----------------------------------------------------------------------
   Return space left in disk quota on file system which given path is in.

    Args: path - Path name of file or directory on file system of concern
          over - pointer to flag that is set if the user is over quota

 Returns: If *over = 0, the number of bytes free in disk quota as per
          the soft limit.
	  If *over = 1, the number of bytes *over* quota.
          -1 is returned on an error looking up quota
           0 is returned if there is no quota

BUG:  If there's more than 2.1Gb free this function will break
  ----*/
long
disk_quota(path, over)
    char *path;
    int  *over;
{
    static int   no_quota = 0;
    struct stat  statx;
    struct dqblk quotax;
    long         q;
    char        *dname;

    if(no_quota)
      return(0L); /* If no quota the first time, then none the second. */

    dprint(5, (debugfile, "quota_check path: %s\n", path));
    if(stat(path, &statx) < 0) {
        return(-1L);
    }

    *over = 0;
    errno = 0;

    dname = device_name(statx.st_dev);
    if(dname == NULL)
      return(-1L);

    dprint(7, (debugfile, "Quota check: UID:%d  device: %s\n", 
           getuid(), dname));
    if(quotactl(Q_GETQUOTA, dname, getuid(), (char *)&quotax) < 0) {
        dprint(5, (debugfile, "Quota failed : %s\n",
                   error_description(errno)));
        return(-1L); /* Something went wrong */
    }

    dprint(5,(debugfile,"Quota: bsoftlimit:%d  bhardlimit:%d  curblock:%d\n",
          quotax.dqb_bsoftlimit, quotax.dqb_bhardlimit, quotax.dqb_curblocks));

    if(quotax.dqb_bsoftlimit == -1)
      return(-1L);

    q = (quotax.dqb_bsoftlimit - quotax.dqb_curblocks) * 512;    

    if(q < 0) {
        q = -q;
        *over = 1;
    }
    dprint(5, (debugfile, "disk_quota returning :%d,  over:%d\n", q, *over));
    return(q);
}


/*----------------------------------------------------------------------
 *		devNumToName
 *
 *	This routine is here so that ex can get a device name to check
 *	disk quotas.  One might wonder, why not use getmntent(), rather
 *	than read /etc/mtab in this crude way?  The problem with getmntent
 *	is that it uses stdio, and ex/vi pointedly doesn't.
 ----*/
static  char
*device_name(st_devArg)
    dev_t st_devArg;
{
#ifndef MTABNAME
#define MTABNAME "/etc/mtab"
#endif
    char *mtab;
    static char devName[48];
    static char *answer = (char *) 0;
    struct stat devStat;
    static dev_t st_dev;
    int nb, cur, bol;
    char c;
    int dname;

    if (st_devArg == st_dev)
      return answer;

    mtab = read_file(MTABNAME);
    if(mtab == NULL)
      return((char *)NULL);

    /* Initialize save data. */
    st_dev = st_devArg;
    answer = (char *) 0;
    nb = strlen(mtab);

    for (cur=bol=0, dname=1; cur < nb; ++cur) {

	if (dname && (mtab[cur] <= ' ')) {
	/*	Space, tab or other such character has been found,
		presumably marking the end of the device name string. */
	
	    dname = 0;
	    c = mtab[cur];	/* Save current character. */
	    mtab[cur] = 0;	/* C zero-terminated string. */

	    /*	Get device number, via stat().  If it's the right
		number, copy the string and return its address. */
	    if (stat (&mtab[bol], &devStat) == 0) {
		if (devStat.st_rdev == st_dev) {
		    if ((cur - bol + 1) < sizeof (devName)) {
			strcpy (devName, &mtab[bol]);
                        answer = &devName[0];
			return(answer);
		    }
		}
	    }
	    mtab[cur] = c;
	}
	if (mtab[cur] == '\n') {
	    dname = 1;
	    bol = cur + 1;
	}
    }
    answer = NULL;

    return(answer);
}
#endif /* USE_QUOTAS */



/*----------------------------------------------------------------------
    Read whole file into memory

  Args: filename -- path name of file to read

  Result: Returns pointer to malloced memory with the contents of the file
          or NULL

This won't work very well if the file has NULLs in it and is mostly
intended for fairly small text files.
 ----*/
char *
read_file(filename)
    char *filename;
{
    int         fd;
    struct stat statbuf;
    char       *buf;
    int         nb;

    fd = open(filename, O_RDONLY);
    if(fd < 0)
      return((char *)NULL);

    fstat(fd, &statbuf);

    buf = fs_get((size_t)statbuf.st_size + 1);

    /*
     * On some systems might have to loop here, if one read isn't guaranteed
     * to get the whole thing.
     */
    if((nb = read(fd, buf, (int)statbuf.st_size)) < 0)
      fs_give((void **)&buf);		/* NULL's buf */
    else
      buf[nb] = '\0';

    close(fd);
    return(buf);
}



/*----------------------------------------------------------------------
   Create a temporary file, the name of which we don't care about 
and that goes away when it is closed.  Just like ANSI C tmpfile.
  ----*/
FILE  *
create_tmpfile()
{
    return(tmpfile());
}



/*
 * This routine is derived from BSD4.3 code,
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 */
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)tmpnam.c	4.5 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */
/*----------------------------------------------------------------------
      Return a unique file name in a given directory.  This is not quite
      the same as the usual tempnam() function, though it is very similar.
      We want it to use the TMP environment variable only if dir is NULL,
      instead of using TMP regardless if it is set.

  Args: dir      -- The directory to create the name in
        prefix   -- Prefix of the name
 
 Result: Malloc'd string equal to new name is returned.  It must be free'd
	 by the caller.  Returns the string on success and NULL on failure.
  ----*/
char *
temp_nam(dir, prefix)
    char *dir, *prefix;
{
    struct stat buf;
    char *f, *name;
    char *our_mktemp();

    if (!(name = malloc((unsigned int)MAXPATHLEN)))
        return((char *)NULL);

    if (!dir && (f = getenv("TMPDIR")) && !stat(f, &buf) &&
                         (buf.st_mode&S_IFMT) == S_IFDIR &&
			 !can_access(f, WRITE_ACCESS|EXECUTE_ACCESS)) {
        (void)strcpy(name, f);
        goto done;
    }

    if (dir && !stat(dir, &buf) &&
                         (buf.st_mode&S_IFMT) == S_IFDIR &&
	                 !can_access(dir, WRITE_ACCESS|EXECUTE_ACCESS)) {
        (void)strcpy(name, dir);
        goto done;
    }

#ifndef P_tmpdir
#define	P_tmpdir	"/usr/tmp"
#endif
    if (!stat(P_tmpdir, &buf) &&
                         (buf.st_mode&S_IFMT) == S_IFDIR &&
			 !can_access(P_tmpdir, WRITE_ACCESS|EXECUTE_ACCESS)) {
        (void)strcpy(name, P_tmpdir);
        goto done;
    }

    if (!stat("/tmp", &buf) &&
                         (buf.st_mode&S_IFMT) == S_IFDIR &&
			 !can_access("/tmp", WRITE_ACCESS|EXECUTE_ACCESS)) {
        (void)strcpy(name, "/tmp");
        goto done;
    }
    free((void *)name);
    return((char *)NULL);

done:
    if(*(f = &name[strlen(name) - 1]) != '/')
      *++f = '/';

    f++;
    if (prefix)
      sstrcpy(&f, prefix);

    sstrcpy(&f, "XXXXXX");
    return(our_mktemp(name));
}


/*
 * This routine is derived from BSD4.3 code,
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * We use this instead of mktemp() since we know of at least one stupid
 * mktemp() (AIX3.2) which breaks things.
 */
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mktemp.c	5.7 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

static
_gettemp(as)
	char	*as;
{
	extern int	errno;
	register char	*start, *trv;
	struct stat	sbuf;
	unsigned	pid;

	pid = (unsigned)getpid();

	/* extra X's get set to 0's */
	for (trv = as; *trv; ++trv);
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/*
	 * check for write permission on target directory; if you have
	 * six X's and you can't write the directory, this will run for
	 * a *very* long time.
	 */
	for (start = ++trv; trv > as && *trv != '/'; --trv);
	if (*trv == '/') {
		*trv = '\0';
		if (stat(as==trv ? "/" : as, &sbuf)
		    || !(sbuf.st_mode & S_IFDIR))
			return(0);
		*trv = '/';
	}
	else if (stat(".", &sbuf) == -1)
		return(0);

	for (;;) {
		if (stat(as, &sbuf))
			return(errno == ENOENT ? 1 : 0);

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv)
				return(0);
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit((unsigned char)*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	/*NOTREACHED*/
}

char *
our_mktemp(as)
	char	*as;
{
	return(_gettemp(as) ? as : (char *)NULL);
}



/*----------------------------------------------------------------------
     Abort with a core dump
 ----*/
void
coredump()
{
    abort();
}



#include <sys/utsname.h>

/*----------------------------------------------------------------------
       Call system gethostname

  Args: hostname -- buffer to return host name in 
        size     -- Size of buffer hostname is to be returned in

 Result: returns 0 if the hostname is correctly set,
         -1 if not (and errno is set).
 ----*/
hostname(hostname,size) 
    char *hostname;
    int size;
{
    /** This routine compliments of Scott McGregor at the HP
	    Corporate Computing Center **/
     
    int uname();
    struct utsname name;

    (void)uname(&name);
    (void)strncpy(hostname,name.nodename,size-1);

    hostname[size - 1] = '\0';
    return 0;
}



/*----------------------------------------------------------------------
       Get the current host and domain names

    Args: hostname   -- buffer to return the hostname in
          hsize      -- size of buffer above
          domainname -- buffer to return domain name in
          dsize      -- size of buffer above

  Result: The system host and domain names are returned. If the full host
          name is akbar.cac.washington.edu then the domainname is
          cac.washington.edu.

On Internet connected hosts this look up uses /etc/hosts and DNS to
figure all this out. On other less well connected machines some other
file may be read. If there is no notion of a domain name the domain
name may be left blank. On a PC where there really isn't a host name
this should return blank strings. The .pinerc will take care of
configuring the domain names. That is, this should only return the
native system's idea of what the names are if the system has such
a concept.
 ----*/
void
getdomainnames(hostname, hsize, domainname, dsize)
    char *hostname, *domainname;
    int   hsize, dsize;
{
    char           *dn, hname[MAX_ADDRESS+1];
    struct hostent *he;

    gethostname(hname, MAX_ADDRESS);

    he = gethostbyname(hname);

    if(he == NULL && strlen(hname) == 0) {
        strcpy(hostname, "");
    } else if(he == NULL) {
        strncpy(hostname, hname, hsize - 1);
    } else {
        strncpy(hostname, he->h_name, hsize-1);
    }
    hostname[hsize-1] = '\0';


    if((dn = strindex(hostname, '.')) != NULL) {
        strncpy(domainname, dn+1, dsize-1);
    } else {
        strncpy(domainname, hostname, dsize-1);
    }
    domainname[dsize-1] = '\0';
}



/*----------------------------------------------------------------------
       Return canonical form of host name ala c-client (UNIX version).

   Args: host      -- The host name

 Result: Canonical form, or input argument (worst case)
 ----*/
char *
canonical_name(host)
    char *host;
{
    struct hostent *hent;
    char hostname[MAILTMPLEN];
    char tmp[MAILTMPLEN];
    extern char *lcase();
                                /* domain literal is easy */
    if (host[0] == '[' && host[(strlen (host))-1] == ']')
      return host;

    strcpy (hostname,host);       /* UNIX requires lowercase */
                                /* lookup name, return canonical form */
    return (hent = gethostbyname (lcase (strcpy (tmp,host)))) ?
      hent->h_name : host;
}



/*----------------------------------------------------------------------
     This routine returns 1 if job control is available.  Note, thiis
     could be some type of fake job control.  It doesn't have to be
     real BSD-style job control.
  ----*/
have_job_control()
{
    return 1;
}


/*----------------------------------------------------------------------
    If we don't have job control, this routine is never called.
  ----*/
stop_process()
{
    kill(0, SIGSTOP); 
}



extern char *sys_errlist[];

/*----------------------------------------------------------------------
       Return string describing the error

   Args: errnumber -- The system error number (errno)

 Result:  long string describing the error is returned
  ----*/
char *
error_description(errnumber)
    int errnumber;
{
    static char buffer[50];

    strcpy(buffer, sys_errlist[errnumber]);

    return ( (char *) buffer);
}



/*----------------------------------------------------------------------
      Pull the name out of the gcos field if we have that sort of /etc/passwd

   Args: gcos_field --  The long name or GCOS field to be parsed
         logname    --  Replaces occurances of & with logname string

 Result: returns pointer to buffer with name
  ----*/
static char *
gcos_name(gcos_field, logname)
    char *logname, *gcos_field;
{
    static char fullname[MAX_FULLNAME+1];
    register char *fncp, *gcoscp, *lncp, *end;

    /* full name is all chars up to first ',' (or whole gcos, if no ',') */
    /* replace any & with logname in upper case */

    for(fncp = fullname, gcoscp= gcos_field, end = fullname + MAX_FULLNAME - 1;
        (*gcoscp != ',' && *gcoscp != '\0' && fncp != end);
	gcoscp++) {

	if(*gcoscp == '&') {
	    for(lncp = logname; *lncp; fncp++, lncp++)
		*fncp = toupper((unsigned char)(*lncp));
	} else {
	    *fncp++ = *gcoscp;
	}
    }
    
    *fncp = '\0';
    return(fullname);
}


/*----------------------------------------------------------------------
      Fill in homedir, login, and fullname for the logged in user.
      These are all pointers to static storage so need to be copied
      in the caller.

 Args: ui    -- struct pointer to pass back answers

 Result: fills in the fields
  ----*/
void
get_user_info(ui)
    struct user_info *ui;
{
    struct passwd *unix_pwd;

    unix_pwd = getpwuid(getuid());
    if(unix_pwd == NULL) {
      ui->homedir = cpystr("");
      ui->login = cpystr("");
      ui->fullname = cpystr("");
    }else {
      ui->homedir = cpystr(unix_pwd->pw_dir);
      ui->login = cpystr(unix_pwd->pw_name);
      ui->fullname = cpystr(gcos_name(unix_pwd->pw_gecos, unix_pwd->pw_name));
    }
}


/*----------------------------------------------------------------------
      Look up a userid on the local system and return rfc822 address

 Args: name  -- possible login name on local system

 Result: returns NULL or pointer to alloc'd string rfc822 address.
  ----*/
char *
local_name_lookup(name)
    char *name;
{
    struct passwd *pw = getpwnam(name);

    if(pw == NULL)
      return((char *)NULL);

    return(cpystr(gcos_name(pw->pw_gecos, name)));
}



/*----------------------------------------------------------------------
       Call the system to change the passwd
 
It would be nice to talk to the passwd program via a pipe or ptty so the
user interface could be consistent, but we can't count on the the prompts
and responses from the passwd program to be regular so we just let the user 
type at the passwd program with some screen space, hope he doesn't scroll 
off the top and repaint when he's done.
 ----*/        
change_passwd()
{
    char cmd_buf[100];

    ClearLines(1, ps_global->ttyo->screen_rows - 1);

    MoveCursor(5, 0);
    fflush(stdout);

    Raw(0);
    strcpy(cmd_buf, PASSWD_PROG);
    system(cmd_buf);
    sleep(3);
    Raw(1);
}



/*----------------------------------------------------------------------
       Can we display this type/subtype?

   Args: type       -- the MIME type to check
         subtype    -- the MIME subtype
         params     -- parameters
	 use_viewer -- tell caller he should run external viewer cmd to view

 Result: returns 1 if the type is displayable, 0 otherwise.
 Note: we always return 1 for type text and type message, but sometimes
       we set use_viewer and sometimes we don't.
 ----*/
mime_can_display(type, subtype, params, use_viewer)
int       type;
char      *subtype;
PARAMETER *params;
int       *use_viewer;
{
    int rv;

    /* give mailcap a crack at everything first */
    if(mailcap_can_display(type, subtype, params)){
	if(use_viewer)
	  *use_viewer = 1;

	rv = 1;
    }
    else{
	if(use_viewer)
	  *use_viewer = 0;

	switch(type){

	  /* if mailcap didn't want to handle these, we will */
	  case TYPETEXT:
	    if(subtype && !strucmp(subtype, "HTML")){
		rv = 0;
		break;
	    }
	  case TYPEMESSAGE:
	    rv = 1;
	    break;

	  case TYPEAPPLICATION:
	    rv = (subtype && !strucmp(subtype, "DIRECTORY"));
	    break;

	  default:
	    rv = 0;
	    break;
	}
    }

    return(rv);
}



/*----------------------------------------------------------------------
   This is just a call to the ANSI C fgetpos function.
  ----*/
fget_pos(stream, ptr)
FILE *stream;
fpos_t *ptr;
{
    return(fgetpos(stream, ptr));
}


/*----------------------------------------------------------------------
   This is just a call to the ANSI C fsetpos function.
  ----*/
fset_pos(stream, ptr)
FILE *stream;
fpos_t *ptr;
{
    return(fsetpos(stream, ptr));
}



/*----------------------------------------------------------------------
       Dummy srandom function.  Srandom isn't important.
 ----*/
void
srandom(i)
    int i;
{
}



/*======================================================================
    pipe
    
    Initiate I/O to and from a process.  These functions are similar to 
    popen and pclose, but both an incoming stream and an output file are 
    provided.
   
 ====*/

#ifndef	STDIN_FILENO
#define	STDIN_FILENO	0
#endif
#ifndef	STDOUT_FILENO
#define	STDOUT_FILENO	1
#endif
#ifndef	STDERR_FILENO
#define	STDERR_FILENO	2
#endif


/*
 * Defs to help fish child's exit status out of wait(2)
 */
#ifdef	HAVE_WAIT_UNION
#define WaitType	union wait
#ifndef	WIFEXITED
#define	WIFEXITED(X)	(!(X).w_termsig)	/* child exit by choice */
#endif
#ifndef	WEXITSTATUS
#define	WEXITSTATUS(X)	(X).w_retcode		/* childs chosen exit value */
#endif
#else
#define	WaitType	int
#ifndef	WIFEXITED
#define	WIFEXITED(X)	(!((X) & 0xff))		/* low bits tell how it died */
#endif
#ifndef	WEXITSTATUS
#define	WEXITSTATUS(X)	(((X) >> 8) & 0xff)	/* high bits tell exit value */
#endif
#endif


/*
 * Global's to helpsignal handler tell us child's status has changed...
 */
short	child_signalled;
short	child_jump = 0;
jmp_buf child_state;


/*
 * Internal Protos
 */
void pipe_error_cleanup PROTO((PIPE_S **, char *, char *, char *));
void zot_pipe PROTO((PIPE_S **));




/*----------------------------------------------------------------------
     Spawn a child process and optionally connect read/write pipes to it

  Args: command -- string to hand the shell
	outfile -- address of pointer containing file to receive output
	errfile -- address of pointer containing file to receive error output
	mode -- mode for type of shell, signal protection etc...
  Returns: pointer to alloc'd PIPE_S on success, NULL otherwise

  The outfile is either NULL, a pointer to a NULL value, or a pointer
  to the requested name for the output file.  In the pointer-to-NULL case
  the caller doesn't care about the name, but wants to see the pipe's
  results so we make one up.  It's up to the caller to make sure the
  free storage containing the name is cleaned up.

  Mode bits serve several purposes.
    PIPE_WRITE tells us we need to open a pipe to write the child's
	stdin.
    PIPE_READ tells us we need to open a pipe to read from the child's
	stdout/stderr.  *NOTE*  Having neither of the above set means 
	we're not setting up any pipes, just forking the child and exec'ing
	the command.  Also, this takes precedence over any named outfile.
    PIPE_STDERR means we're to tie the childs stderr to the same place
	stdout is going.  *NOTE* This only makes sense then if PIPE_READ
	or an outfile is provided.  Also, this takes precedence over any
	named errfile.
    PIPE_PROT means to protect the child from the usual nasty signals
	that might cause premature death.  Otherwise, the default signals are
	set so the child can deal with the nasty signals in its own way.     
    PIPE_NOSHELL means we're to exec the command without the aid of
	a system shell.  *NOTE* This negates the affect of PIPE_USER.
    PIPE_USER means we're to try executing the command in the user's
	shell.  Right now we only look in the environment, but that may get
	more sophisticated later.
    PIPE_RESET means we reset the terminal mode to what it was before
	we started pine and then exec the command.
 ----*/
PIPE_S *
open_system_pipe(command, outfile, errfile, mode)
    char  *command;
    char **outfile, **errfile;
    int    mode;
{
    PIPE_S *syspipe = NULL;
    char    shellpath[32], *shell;
    int     p[2], oparentd = -1, ochildd = -1, iparentd = -1, ichildd = -1;

    dprint(5, (debugfile, "Opening pipe: \"%s\" (%s%s%s%s%s%s)\n", command,
	       (mode & PIPE_WRITE)   ? "W":"", (mode & PIPE_READ)  ? "R":"",
	       (mode & PIPE_NOSHELL) ? "N":"", (mode & PIPE_PROT)  ? "P":"",
	       (mode & PIPE_USER)    ? "U":"", (mode & PIPE_RESET) ? "T":""));

    syspipe = (PIPE_S *)fs_get(sizeof(PIPE_S));
    memset(syspipe, 0, sizeof(PIPE_S));

    /*
     * If we're not using the shell's command parsing smarts, build
     * argv by hand...
     */
    if(mode & PIPE_NOSHELL){
	char   **ap, *p;
	size_t   n;

	/* parse the arguments into argv */
	for(p = command; *p && isspace((unsigned char)(*p)); p++)
	  ;					/* swallow leading ws */

	if(*p){
	    syspipe->args = cpystr(p);
	}
	else{
	    pipe_error_cleanup(&syspipe, "<null>", "execute",
			       "No command name found");
	    return(NULL);
	}

	for(p = syspipe->args, n = 2; *p; p++)	/* count the args */
	  if(isspace((unsigned char)(*p))
	     && *(p+1) && !isspace((unsigned char)(*(p+1))))
	    n++;

	syspipe->argv = ap = (char **)fs_get(n * sizeof(char *));
	memset(syspipe->argv, 0, n * sizeof(char *));

	for(p = syspipe->args; *p; ){		/* collect args */
	    while(*p && isspace((unsigned char)(*p)))
	      *p++ = '\0';

	    *ap++ = (*p) ? p : NULL;
	    while(*p && !isspace((unsigned char)(*p)))
	      p++;
	}

	/* make sure argv[0] exists in $PATH */
	if(can_access_in_path(getenv("PATH"), syspipe->argv[0],
			      EXECUTE_ACCESS) < 0){
	    pipe_error_cleanup(&syspipe, syspipe->argv[0], "access",
			       error_description(errno));
	    return(NULL);
	}
    }

    /* fill in any output filenames */
    if(!(mode & PIPE_READ)){
	if(outfile && !*outfile)
	  *outfile = temp_nam(NULL, "pine_p");	/* asked for, but not named? */

	if(errfile && !*errfile)
	  *errfile = temp_nam(NULL, "pine_p");	/* ditto */
    }

    /* create pipes */
    if(mode & (PIPE_WRITE | PIPE_READ)){
	if(mode & PIPE_WRITE){
	    pipe(p);				/* alloc pipe to write child */
	    oparentd = p[STDOUT_FILENO];
	    ichildd  = p[STDIN_FILENO];
	}

	if(mode & PIPE_READ){
	    pipe(p);				/* alloc pipe to read child */
	    iparentd = p[STDIN_FILENO];
	    ochildd  = p[STDOUT_FILENO];
	}
    }
    else if(!(mode & PIPE_SILENT)){
	flush_status_messages(0);		/* just clean up display */
	ClearScreen();
	fflush(stdout);
    }

    if((syspipe->mode = mode) & PIPE_RESET)
      Raw(0);

#ifdef	SIGCHLD
    /*
     * Prepare for demise of child.  Use SIGCHLD if it's available so
     * we can do useful things, like keep the IMAP stream alive, while
     * we're waiting on the child.
     */
    child_signalled = child_jump = 0;
#endif

    if((syspipe->pid = vfork()) == 0){
 	/* reset child's handlers in requested fashion... */
	(void)signal(SIGINT,  (mode & PIPE_PROT) ? SIG_IGN : SIG_DFL);
	(void)signal(SIGQUIT, (mode & PIPE_PROT) ? SIG_IGN : SIG_DFL);
	(void)signal(SIGHUP,  (mode & PIPE_PROT) ? SIG_IGN : SIG_DFL);
#ifdef	SIGCHLD
	(void) signal(SIGCHLD,  SIG_DFL);
#endif

	/* if parent isn't reading, and we have a filename to write */
	if(!(mode & PIPE_READ) && outfile){	/* connect output to file */
	    int output = creat(*outfile, 0600);
	    dup2(output, STDOUT_FILENO);
	    if(mode & PIPE_STDERR)
	      dup2(output, STDERR_FILENO);
	    else if(errfile)
	      dup2(creat(*errfile, 0600), STDERR_FILENO);
	}

	if(mode & PIPE_WRITE){			/* connect process input */
	    close(oparentd);
	    dup2(ichildd, STDIN_FILENO);	/* tie stdin to pipe */
	    close(ichildd);
	}

	if(mode & PIPE_READ){			/* connect process output */
	    close(iparentd);
	    dup2(ochildd, STDOUT_FILENO);	/* tie std{out,err} to pipe */
	    if(mode & PIPE_STDERR)
	      dup2(ochildd, STDERR_FILENO);
	    else if(errfile)
	      dup2(creat(*errfile, 0600), STDERR_FILENO);

	    close(ochildd);
	}

	if(mode & PIPE_NOSHELL){
	    execvp(syspipe->argv[0], syspipe->argv);
	}
	else{
	    if(mode & PIPE_USER){
		char *env, *sh;
		if((env = getenv("SHELL")) && (sh = strrchr(env, '/'))){
		    shell = sh + 1;
		    strcpy(shellpath, env);
		}
		else{
		    shell = "csh";
		    strcpy(shellpath, "/bin/csh");
		}
	    }
	    else{
		shell = "sh";
		strcpy(shellpath, "/bin/sh");
	    }

	    execl(shellpath, shell, command ? "-c" : 0, command, 0);
	}

	fprintf(stderr, "Can't exec %s\nReason: %s",
		command, error_description(errno));
	_exit(-1);
    }

    if(syspipe->pid > 0){
	syspipe->isig = signal(SIGINT,  SIG_IGN); /* Reset handlers to make */
	syspipe->qsig = signal(SIGQUIT, SIG_IGN); /* sure we don't come to  */
	syspipe->hsig = signal(SIGHUP,  SIG_IGN); /* a premature end...     */

	if(mode & PIPE_WRITE){
	    close(ichildd);
	    if(mode & PIPE_DESC)
	      syspipe->out.d = oparentd;
	    else
	      syspipe->out.f = fdopen(oparentd, "w");
	}

	if(mode & PIPE_READ){
	    close(ochildd);
	    if(mode & PIPE_DESC)
	      syspipe->in.d = iparentd;
	    else
	      syspipe->in.f = fdopen(iparentd, "r");
	}

	dprint(5, (debugfile, "PID: %d, COMMAND: %s\n",syspipe->pid,command));
    }
    else{
	if(mode & (PIPE_WRITE | PIPE_READ)){
	    if(mode & PIPE_WRITE){
		close(oparentd);
		close(ichildd);
	    }

	    if(mode & PIPE_READ){
		close(iparentd);
		close(ochildd);
	    }
	}
	else if(!(mode & PIPE_SILENT)){
	    ClearScreen();
	    ps_global->mangled_screen = 1;
	}

	if(mode & PIPE_RESET)
	  Raw(1);

#ifdef	SIGCHLD
	(void) signal(SIGCHLD,  SIG_DFL);
#endif
	if(outfile)
	  fs_give((void **) outfile);

	pipe_error_cleanup(&syspipe, command, "fork",error_description(errno));
    }

    return(syspipe);
}



/*----------------------------------------------------------------------
    Write appropriate error messages and cleanup after pipe error

  Args: syspipe -- address of pointer to struct to clean up
	cmd -- command we were trying to exec
	op -- operation leading up to the exec
	res -- result of that operation

 ----*/
void
pipe_error_cleanup(syspipe, cmd, op, res)
    PIPE_S **syspipe;
    char    *cmd, *op, *res;
{
    q_status_message3(SM_ORDER, 3, 3, "Pipe can't %s \"%.20s\": %s",
		      op, cmd, res);
    dprint(1, (debugfile, "* * PIPE CAN'T %s(%s): %s\n", op, cmd, res));
    zot_pipe(syspipe);
}



/*----------------------------------------------------------------------
    Free resources associated with the given pipe struct

  Args: syspipe -- address of pointer to struct to clean up

 ----*/
void
zot_pipe(syspipe)
    PIPE_S **syspipe;
{
    if((*syspipe)->args)
      fs_give((void **) &(*syspipe)->args);

    if((*syspipe)->argv)
      fs_give((void **) &(*syspipe)->argv);

    if((*syspipe)->tmp)
      fs_give((void **) &(*syspipe)->tmp);

    fs_give((void **)syspipe);
}



/*----------------------------------------------------------------------
    Close pipe previously allocated and wait for child's death

  Args: syspipe -- address of pointer to struct returned by open_system_pipe
  Returns: returns exit status of child or -1 if invalid syspipe
 ----*/
int
close_system_pipe(syspipe)
    PIPE_S **syspipe;
{
    WaitType stat;
    int	     status;

    if(!(syspipe && *syspipe))
      return(-1);

    if(((*syspipe)->mode) & PIPE_WRITE){
	if(((*syspipe)->mode) & PIPE_DESC){
	    if((*syspipe)->out.d >= 0)
	      close((*syspipe)->out.d);
	}
	else if((*syspipe)->out.f)
	  fclose((*syspipe)->out.f);
    }

    if(((*syspipe)->mode) & PIPE_READ){
	if(((*syspipe)->mode) & PIPE_DESC){
	    if((*syspipe)->in.d >= 0)
	      close((*syspipe)->in.d);
	}
	else if((*syspipe)->in.f)
	  fclose((*syspipe)->in.f);
    }

#ifdef	SIGCHLD
    {
	SigType (*alarm_sig)();
	int	old_cue = F_ON(F_SHOW_DELAY_CUE, ps_global);

	/*
	 * remember the current SIGALRM handler, and make sure it's
	 * installed when we're finished just in case the longjmp
	 * out of the SIGCHLD handler caused sleep() to lose it.
	 * Don't pay any attention to that man behind the curtain.
	 */
	alarm_sig = signal(SIGALRM, SIG_IGN);
	(void) signal(SIGALRM, alarm_sig);
	F_SET(F_SHOW_DELAY_CUE, ps_global, 0);
	ps_global->noshow_timeout = 1;
	while(!child_signalled){
	    new_mail(0, 2, 0);		/* wake up and prod server */

	    if(!child_signalled){
		if(setjmp(child_state) == 0){
		    child_jump = 1;	/* prepare to wake up */
		    sleep(600);		/* give it 5mins to happend */
		}
		else
		  pine_sigunblock(SIGCHLD);
	    }

	    child_jump = 0;
	}

	ps_global->noshow_timeout = 0;
	F_SET(F_SHOW_DELAY_CUE, ps_global, old_cue);
	(void) signal(SIGALRM, alarm_sig);
    }
#endif

    /*
     * Call c-client's pid reaper to wait() on the demise of our child,
     * then fish out its exit status...
     */
    grim_pid_reap_status((*syspipe)->pid, 0, &stat);
    status = WIFEXITED(stat) ? WEXITSTATUS(stat) : -1;

    /*
     * restore original handlers...
     */
    (void)signal(SIGINT,  (*syspipe)->isig);
    (void)signal(SIGHUP,  (*syspipe)->hsig);
    (void)signal(SIGQUIT, (*syspipe)->qsig);

    if((*syspipe)->mode & PIPE_RESET)		/* restore our tty modes */
      Raw(1);

    if(!((*syspipe)->mode & (PIPE_WRITE | PIPE_READ | PIPE_SILENT))){
	ClearScreen();				/* No I/O to forked child */
	ps_global->mangled_screen = 1;
    }

    zot_pipe(syspipe);

    return(status);
}

/*======================================================================
    post_reap
    
    Manage exit status collection of a child spawned to handle posting
 ====*/



#if	defined(BACKGROUND_POST) && defined(SIGCHLD)
/*----------------------------------------------------------------------
    Check to see if we have any posting processes to clean up after

  Args: none
  Returns: any finished posting process reaped
 ----*/
post_reap()
{
    WaitType stat;
    int	     r;

    if(ps_global->post && ps_global->post->pid){
	r = waitpid(ps_global->post->pid, &stat, WNOHANG);
	if(r == ps_global->post->pid){
	    ps_global->post->status = WIFEXITED(stat) ? WEXITSTATUS(stat) : -1;
	    ps_global->post->pid = 0;
	    return(1);
	}
	else if(r < 0 && errno != EINTR){ /* pid's become bogus?? */
	    fs_give((void **) &ps_global->post);
	}
    }

    return(0);
}
#endif

/*----------------------------------------------------------------------
    Routines used to hand off messages to local agents for sending/posting

 The two exported routines are:

    1) smtp_command()  -- used to get local transport agent to invoke
    2) post_handoff()  -- used to pass messages to local posting agent

 ----*/



/*
 * Protos for "sendmail" internal functions
 */
static char *mta_parse_post PROTO((METAENV *, BODY *, char *, char *));
static long  pine_pipe_soutr_nl PROTO((void *, char *));



/* ----------------------------------------------------------------------
   Figure out command to start local SMTP agent

  Args: errbuf   -- buffer for reporting errors (assumed non-NULL)

  Returns an alloc'd copy of the local SMTP agent invocation or NULL

  ----*/
char *
smtp_command(errbuf)
    char *errbuf;
{
#if	defined(SENDMAIL) && defined(SENDMAILFLAGS)
    char tmp[256];

    sprintf(tmp, "%s %s", SENDMAIL, SENDMAILFLAGS);
    return(cpystr(tmp));
#else
    strcpy(errbuf, "No default posting command.");
    return(NULL);
#endif
}



/*----------------------------------------------------------------------
   Hand off given message to local posting agent

  Args: envelope -- The envelope for the BCC and debugging
        header   -- The text of the message header
        errbuf   -- buffer for reporting errors (assumed non-NULL)
     
   ----*/
int
mta_handoff(header, body, errbuf)
    METAENV    *header;
    BODY       *body;
    char       *errbuf;
{
    char cmd_buf[256], *cmd = NULL;

    /*
     * A bit of complicated policy implemented here.
     * There are two posting variables sendmail-path and smtp-server.
     * Precedence is in that order.
     * They can be set one of 4 ways: fixed, command-line, user, or globally.
     * Precedence is in that order.
     * Said differently, the order goes something like what's below.
     * 
     * NOTE: the fixed/command-line/user precendence handling is also
     *	     indicated by what's pointed to by ps_global->VAR_*, but since
     *	     that also includes the global defaults, it's not sufficient.
     */

    if(ps_global->FIX_SENDMAIL_PATH
       && ps_global->FIX_SENDMAIL_PATH[0]){
	cmd = ps_global->FIX_SENDMAIL_PATH;
    }
    else if(!(ps_global->FIX_SMTP_SERVER
	      && ps_global->FIX_SMTP_SERVER[0])){
	if(ps_global->COM_SENDMAIL_PATH
	   && ps_global->COM_SENDMAIL_PATH[0]){
	    cmd = ps_global->COM_SENDMAIL_PATH;
	}
	else if(!(ps_global->COM_SMTP_SERVER
		  && ps_global->COM_SMTP_SERVER[0])){
	    if(ps_global->USR_SENDMAIL_PATH
	       && ps_global->USR_SENDMAIL_PATH[0]){
		cmd = ps_global->USR_SENDMAIL_PATH;
	    }
	    else if(!(ps_global->USR_SMTP_SERVER
		      && ps_global->USR_SMTP_SERVER[0])){
		if(ps_global->GLO_SENDMAIL_PATH
		   && ps_global->GLO_SENDMAIL_PATH[0]){
		    cmd = ps_global->GLO_SENDMAIL_PATH;
		}
#ifdef	DF_SENDMAIL_PATH
		/*
		 * This defines the default method of posting.  So,
		 * unless we're told otherwise use it...
		 */
		else if(!(ps_global->GLO_SMTP_SERVER
			  && ps_global->GLO_SMTP_SERVER[0])){
		    strcpy(cmd = cmd_buf, DF_SENDMAIL_PATH);
		}
#endif
	    }
	}
    }

    *errbuf = '\0';
    if(cmd){
	dprint(4, (debugfile, "call_mailer via cmd: %s\n", cmd));

	(void) mta_parse_post(header, body, cmd, errbuf);
	return(1);
    }
    else
      return(0);
}



/*----------------------------------------------------------------------
   Hand off given message to local posting agent

  Args: envelope -- The envelope for the BCC and debugging
        header   -- The text of the message header
        errbuf   -- buffer for reporting errors (assumed non-NULL)
     
  Fork off mailer process and pipe the message into it
  Called to post news via Inews when NNTP is unavailable
  
   ----*/
char *
post_handoff(header, body, errbuf)
    METAENV    *header;
    BODY       *body;
    char       *errbuf;
{
    char *err = NULL;
#ifdef	SENDNEWS
    char *s;

    if(s = strstr(header->env->date," (")) /* fix the date format for news */
      *s = '\0';

    if(err = mta_parse_post(header, body, SENDNEWS, errbuf))
      sprintf(err = errbuf, "News not posted: \"%s\": %s", SENDNEWS, err);

    if(s)
      *s = ' ';				/* restore the date */

#else /* !SENDNEWS */  /* this is the default case */
    sprintf(err = errbuf, "Can't post, NNTP-server must be defined!");
#endif /* !SENDNEWS */
    return(err);
}



/*----------------------------------------------------------------------
   Hand off message to local MTA; it parses recipients from 822 header

  Args: header -- struct containing header data
        body  -- struct containing message body data
	cmd -- command to use for handoff (%s says where file should go)
	errs -- pointer to buf to hold errors

   ----*/
static char *
mta_parse_post(header, body, cmd, errs)
    METAENV *header;
    BODY    *body;
    char    *cmd;
    char    *errs;
{
    char   *result = NULL;
    int     rv;
    PIPE_S *pipe;

    dprint(1, (debugfile, "=== mta_parse_post(%s) ===\n", cmd));

    if(pipe = open_system_pipe(cmd, &result, NULL,
		     PIPE_STDERR|PIPE_WRITE|PIPE_PROT|PIPE_NOSHELL|PIPE_DESC)){
	if(!pine_rfc822_output(header, body, pine_pipe_soutr_nl,
			       (TCPSTREAM *) pipe))
	  strcpy(errs, "Error posting.");

	if(close_system_pipe(&pipe) && !*errs){
	    sprintf(errs, "Posting program %s returned error", cmd);
	    if(result)
	      display_output_file(result, "POSTING ERRORS", errs);
	}
    }
    else
      sprintf(errs, "Error running \"%s\"", cmd);

    if(result){
	unlink(result);
	fs_give((void **)&result);
    }

    return(*errs ? errs : NULL);
}


/* 
 * pine_pipe_soutr - Replacement for tcp_soutr that writes one of our
 *		     pipes rather than a tcp stream
 */
static long
pine_pipe_soutr_nl (stream,s)
     void *stream;
     char *s;
{
    long    rv = T;
    char   *p;
    size_t  n;

    while(*s && rv){
	if(n = (p = strstr(s, "\015\012")) ? p - s : strlen(s))
	  while((rv = write(((PIPE_S *)stream)->out.d, s, n)) != n)
	    if(rv < 0){
		if(errno != EINTR){
		    rv = 0;
		    break;
		}
	    }
	    else{
		s += rv;
		n -= rv;
	    }

	if(p && rv){
	    s = p + 2;			/* write UNIX EOL */
	    while((rv = write(((PIPE_S *)stream)->out.d,"\n",1)) != 1)
	      if(rv < 0 && errno != EINTR){
		  rv = 0;
		  break;
	      }
	}
	else
	  break;
    }

    return(rv);
}

/* ----------------------------------------------------------------------
   Execute the given mailcap command

  Args: cmd           -- the command to execute
	image_file    -- the file the data is in
	needsterminal -- does this command want to take over the terminal?
  ----*/
void
exec_mailcap_cmd(cmd, image_file, needsterminal)
char *cmd;
char *image_file;
int   needsterminal;
{
    char   *command = NULL,
	   *result_file = NULL,
	   *p;
    char  **r_file_h;
    PIPE_S *syspipe;
    int     mode;

    p = command = (char *)fs_get((32 + strlen(cmd) + (2*strlen(image_file)))
			     * sizeof(char));
    if(!needsterminal)  /* put in background if it doesn't need terminal */
      *p++ = '(';
    sprintf(p, "%s ; rm -f %s", cmd, image_file);
    p += strlen(p);
    if(!needsterminal){
	*p++ = ')';
	*p++ = ' ';
	*p++ = '&';
    }
    *p++ = '\n';
    *p   = '\0';
    dprint(9, (debugfile, "exec_mailcap_cmd: command=%s\n", command));

    mode = PIPE_RESET;
    if(needsterminal == 1)
      r_file_h = NULL;
    else{
	mode       |= PIPE_WRITE | PIPE_STDERR;
	result_file = temp_nam(NULL, "pine_cmd");
	r_file_h    = &result_file;
    }

    if(syspipe = open_system_pipe(command, r_file_h, NULL, mode)){
	close_system_pipe(&syspipe);
	if(needsterminal == 1)
	  q_status_message(SM_ORDER, 0, 4, "VIEWER command completed");
	else if(needsterminal == 2)
	  display_output_file(result_file, "VIEWER", " command result");
	else
	  display_output_file(result_file, "VIEWER", " command launched");
    }
    else
      q_status_message1(SM_ORDER, 3, 4, "Cannot spawn command : %s", cmd);

    fs_give((void **)&command);
    if(result_file)
      fs_give((void **)&result_file);
}


/* ----------------------------------------------------------------------
   Execute the given mailcap test= cmd

  Args: cmd -- command to execute
  Returns exit status
  
  ----*/
int
exec_mailcap_test_cmd(cmd)
    char *cmd;
{
    PIPE_S *syspipe;

    return((syspipe = open_system_pipe(cmd, NULL, NULL, PIPE_SILENT))
	     ? close_system_pipe(&syspipe) : -1);
}



/*======================================================================
    print routines
   
    Functions having to do with printing on paper and forking of spoolers

    In general one calls open_printer() to start printing. One of
    the little print functions to send a line or string, and then
    call print_end() when complete. This takes care of forking off a spooler
    and piping the stuff down it. No handles or anything here because there's
    only one printer open at a time.

 ====*/



static char *trailer;  /* so both open and close_printer can see it */

/*----------------------------------------------------------------------
       Open the printer

  Args: desc -- Description of item to print. Should have one trailing blank.

  Return value: < 0 is a failure.
		0 a success.

This does most of the work of popen so we can save the standard output of the
command we execute and send it back to the user.
  ----*/
int
open_printer(desc)
    char     *desc;
{
    char command[201], prompt[200];
    int  cmd, rc, just_one;
    char *p, *init, *nick;
    char aname[100];
    char *printer;
    int	 done = 0, i, lastprinter, cur_printer = 0;
    HelpType help;
    char   **list;
    static ESCKEY_S ekey[] = {
	{'y', 'y', "Y", "Yes"},
	{'n', 'n', "N", "No"},
	{ctrl('P'), 10, "^P", "Prev Printer"},
	{ctrl('N'), 11, "^N", "Next Printer"},
	{-2,   0,   NULL, NULL},
	{'c', 'c', "C", "CustomPrint"},
	{KEY_UP,    10, "", ""},
	{KEY_DOWN,  11, "", ""},
	{-1, 0, NULL, NULL}};
#define PREV_KEY   2
#define NEXT_KEY   3
#define CUSTOM_KEY 5
#define UP_KEY     6
#define DOWN_KEY   7

    trailer      = NULL;
    init         = NULL;
    nick         = NULL;
    command[200] = '\0';

    if(ps_global->VAR_PRINTER == NULL){
        q_status_message(SM_ORDER | SM_DING, 3, 5,
	"No printer has been chosen.  Use SETUP on main menu to make choice.");
	return(-1);
    }

    /* Is there just one print command available? */
    just_one = (ps_global->printer_category!=3&&ps_global->printer_category!=2)
	       || (ps_global->printer_category == 2
		   && !(ps_global->VAR_STANDARD_PRINTER
			&& ps_global->VAR_STANDARD_PRINTER[0]
			&& ps_global->VAR_STANDARD_PRINTER[1]))
	       || (ps_global->printer_category == 3
		   && !(ps_global->VAR_PERSONAL_PRINT_COMMAND
			&& ps_global->VAR_PERSONAL_PRINT_COMMAND[0]
			&& ps_global->VAR_PERSONAL_PRINT_COMMAND[1]));

    if(F_ON(F_CUSTOM_PRINT, ps_global))
      ekey[CUSTOM_KEY].ch = 'c'; /* turn this key on */
    else
      ekey[CUSTOM_KEY].ch = -2;  /* turn this key off */

    if(just_one){
	ekey[PREV_KEY].ch = -2;  /* turn these keys off */
	ekey[NEXT_KEY].ch = -2;
	ekey[UP_KEY].ch   = -2;
	ekey[DOWN_KEY].ch = -2;
    }
    else{
	ekey[PREV_KEY].ch = ctrl('P'); /* turn these keys on */
	ekey[NEXT_KEY].ch = ctrl('N');
	ekey[UP_KEY].ch   = KEY_UP;
	ekey[DOWN_KEY].ch = KEY_DOWN;
	/*
	 * count how many printers in list and find the default in the list
	 */
	if(ps_global->printer_category == 2)
	  list = ps_global->VAR_STANDARD_PRINTER;
	else
	  list = ps_global->VAR_PERSONAL_PRINT_COMMAND;

	for(i = 0; list[i]; i++)
	  if(strcmp(ps_global->VAR_PRINTER, list[i]) == 0)
	    cur_printer = i;
	
	lastprinter = i - 1;
    }

    help = NO_HELP;
    ps_global->mangled_footer = 1;

    while(!done){
	if(init)
	  fs_give((void **)&init);

	if(trailer)
	  fs_give((void **)&trailer);

	if(just_one)
	  printer = ps_global->VAR_PRINTER;
	else
	  printer = list[cur_printer];

	parse_printer(printer, &nick, &p, &init, &trailer, NULL, NULL);
	strncpy(command, p, 200);
	fs_give((void **)&p);
	sprintf(prompt, "Print %susing \"%s\" ? ", desc ? desc : "",
	    *nick ? nick : command);

	fs_give((void **)&nick);
	
	cmd = radio_buttons(prompt, -FOOTER_ROWS(ps_global),
				 ekey, 'y', 'x', help, RB_NORM);
	
	switch(cmd){
	  case 'y':
	    q_status_message1(SM_ORDER, 0, 9,
		"Printing with command \"%s\"", command);
	    done++;
	    break;

	  case 10:
	    cur_printer = (cur_printer>0)
				? (cur_printer-1)
				: lastprinter;
	    break;

	  case 11:
	    cur_printer = (cur_printer<lastprinter)
				? (cur_printer+1)
				: 0;
	    break;

	  case 'n':
	  case 'x':
	    done++;
	    break;

	  case 'c':
	    done++;
	    break;

	  default:
	    break;
	}
    }

    if(cmd == 'c'){
	if(init)
	  fs_give((void **)&init);

	if(trailer)
	  fs_give((void **)&trailer);

	sprintf(prompt, "Enter custom command : ");
	command[0] = '\0';
	rc = 1;
	while(rc){
	    rc = optionally_enter(command, -FOOTER_ROWS(ps_global), 0,
		200, 1, 0, prompt, NULL, NO_HELP, 0);
	    
	    if(rc == 1){
		cmd = 'x';
		rc = 0;
	    }
	    else if(rc == 3)
	      q_status_message(SM_ORDER, 0, 3, "No help available");
	    else if(rc == 0){
		removing_trailing_white_space(command);
		removing_leading_white_space(command);
		q_status_message1(SM_ORDER, 0, 9,
		    "Printing with command \"%s\"", command);
	    }
	}
    }

    if(cmd == 'x' || cmd == 'n'){
	q_status_message(SM_ORDER, 0, 2, "Print cancelled");
	if(init)
	  fs_give((void **)&init);

	if(trailer)
	  fs_give((void **)&trailer);

	return(-1);
    }

    display_message('x');

    ps_global->print = (PRINT_S *)fs_get(sizeof(PRINT_S));
    memset(ps_global->print, 0, sizeof(PRINT_S));

    strcat(strcpy(aname, ANSI_PRINTER), "-no-formfeed");
    if(strucmp(command, ANSI_PRINTER) == 0
       || strucmp(command, aname) == 0){
        /*----------- Printer attached to ansi device ---------*/
        q_status_message(SM_ORDER, 0, 9,
	    "Printing to attached desktop printer...");
        display_message('x');
	xonxoff_proc(1);			/* make sure XON/XOFF used */
	crlf_proc(1);				/* AND LF->CR xlation */
        fputs("\033[5i", stdout);
        ps_global->print->fp = stdout;
        if(strucmp(command, ANSI_PRINTER) == 0){
	    /* put formfeed at the end of the trailer string */
	    if(trailer){
		int len = strlen(trailer);

		fs_resize((void **)&trailer, len+2);
		trailer[len] = '\f';
		trailer[len+1] = '\0';
	    }
	    else
	      trailer = cpystr("\f");
	}
    }
    else{
        /*----------- Print by forking off a UNIX command ------------*/
        dprint(4, (debugfile, "Printing using command \"%s\"\n", command));
	ps_global->print->result = temp_nam(NULL, "pine_prt");
	if(ps_global->print->pipe = open_system_pipe(command,
					 &ps_global->print->result, NULL,
					 PIPE_WRITE | PIPE_STDERR)){
	    ps_global->print->fp = ps_global->print->pipe->out.f;
	}
	else{
	    fs_give((void **)&ps_global->print->result);
            q_status_message1(SM_ORDER | SM_DING, 3, 4,
			      "Error opening printer: %s",
                              error_description(errno));
            dprint(2, (debugfile, "Error popening printer \"%s\"\n",
                      error_description(errno)));
	    if(init)
	      fs_give((void **)&init);

	    if(trailer)
	      fs_give((void **)&trailer);
	    
	    return(-1);
        }
    }

    ps_global->print->err = 0;
    if(init){
	if(*init)
	  fputs(init, ps_global->print->fp);

	fs_give((void **)&init);
    }

    return(0);
}



/*----------------------------------------------------------------------
     Close printer
  
  If we're piping to a spooler close down the pipe and wait for the process
to finish. If we're sending to an attached printer send the escape sequence.
Also let the user know the result of the print
 ----*/
void
close_printer()
{
    if(trailer){
	if(*trailer)
	  fputs(trailer, ps_global->print->fp);

	fs_give((void **)&trailer);
    }

    if(ps_global->print->fp == stdout) {
        fputs("\033[4i", stdout);
        fflush(stdout);
	xonxoff_proc(0);			/* turn off XON/XOFF */
	crlf_proc(0);				/* turn off CF->LF xlantion */
    } else {
	(void) close_system_pipe(&ps_global->print->pipe);
	display_output_file(ps_global->print->result, "PRINT", NULL);
	fs_give((void **)&ps_global->print->result);
    }

    fs_give((void **)&ps_global->print);

    q_status_message(SM_ASYNC, 0, 3, "Print command completed");
    display_message('x');
}



/*----------------------------------------------------------------------
     Print a single character

  Args: c -- char to print
  Returns: 1 on success, 0 on ps_global->print->err
 ----*/
int
print_char(c)
    int c;
{
    if(!ps_global->print->err && putc(c, ps_global->print->fp) == EOF)
      ps_global->print->err = 1;

    return(!ps_global->print->err);
}



/*----------------------------------------------------------------------
     Send a line of text to the printer

  Args:  line -- Text to print

  ----*/
    
void
print_text(line)
    char *line;
{
    if(!ps_global->print->err && fputs(line, ps_global->print->fp) == EOF)
      ps_global->print->err = 1;
}



/*----------------------------------------------------------------------
      printf style formatting with one arg for printer

 Args: line -- The printf control string
       a1   -- The 1st argument for printf
 ----*/
void
print_text1(line, a1)
    char *line, *a1;
{
    if(!ps_global->print->err
       && fprintf(ps_global->print->fp, line, a1) < 0)
      ps_global->print->err = 1;
}



/*----------------------------------------------------------------------
      printf style formatting with one arg for printer

 Args: line -- The printf control string
       a1   -- The 1st argument for printf
       a2   -- The 2nd argument for printf
 ----*/
void
print_text2(line, a1, a2)
    char *line, *a1, *a2;
{
    if(!ps_global->print->err
       && fprintf(ps_global->print->fp, line, a1, a2) < 0)
      ps_global->print->err = 1;
}



/*----------------------------------------------------------------------
      printf style formatting with one arg for printer

 Args: line -- The printf control string
       a1   -- The 1st argument for printf
       a2   -- The 2nd argument for printf
       a3   -- The 3rd argument for printf
 ----*/
void
print_text3(line, a1, a2, a3)
    char *line, *a1, *a2, *a3;
{
    if(!ps_global->print->err
       && fprintf(ps_global->print->fp, line, a1, a2, a3) < 0)
      ps_global->print->err = 1;
}

#ifdef DEBUG
/*----------------------------------------------------------------------
     Initialize debugging - open the debug log file

  Args: none

 Result: opens the debug logfile for dprints

   Opens the file "~/.pine-debug1. Also maintains .pine-debug[2-4]
   by renaming them each time so the last 4 sessions are saved.
  ----*/
void
init_debug()
{
    char nbuf[5];
    char newfname[MAXPATH+1], filename[MAXPATH+1];
    int i;

    if(!debug)
      return;

    for(i = NUMDEBUGFILES - 1; i > 0; i--){
        build_path(filename, ps_global->home_dir, DEBUGFILE);
        strcpy(newfname, filename);
        sprintf(nbuf, "%d", i);
        strcat(filename, nbuf);
        sprintf(nbuf, "%d", i+1);
        strcat(newfname, nbuf);
        (void)rename_file(filename, newfname);
    }

    build_path(filename, ps_global->home_dir, DEBUGFILE);
    strcat(filename, "1");

    debugfile = fopen(filename, "w+");
    if(debugfile != NULL){
	time_t now = time((time_t *)0);
	if(debug > 7)
	  setbuf(debugfile, NULL);

	if(NUMDEBUGFILES == 0){
	    /*
	     * If no debug files are asked for, make filename a tempfile
	     * to be used for a record should pine later crash...
	     */
	    if(debug < 9)
	      unlink(filename);
	}

	dprint(1, (debugfile, "Debug output of the Pine program (at debug"));
	dprint(1, (debugfile, " level %d).  Version %s\n%s\n",
		  debug, pine_version, ctime(&now)));
    }
}


/*----------------------------------------------------------------------
     Try to save the debug file if we crash in a controlled way

  Args: dfile:  pointer to open debug file

 Result: tries to move the appropriate .pine-debugx file to .pine-crash

   Looks through the four .pine-debug files hunting for the one that is
   associated with this pine, and then renames it.
  ----*/
void
save_debug_on_crash(dfile)
FILE *dfile;
{
    char nbuf[5], crashfile[MAXPATH+1], filename[MAXPATH+1];
    int i;
    struct stat dbuf, tbuf;
    time_t now = time((time_t *)0);

    if(!(dfile && fstat(fileno(dfile), &dbuf) != 0))
      return;

    fprintf(dfile, "\nsave_debug_on_crash: Version %s: debug level %d\n",
	pine_version, debug);
    fprintf(dfile, "\n                   : %s\n", ctime(&now));

    build_path(crashfile, ps_global->home_dir, ".pine-crash");

    fprintf(dfile, "\nAttempting to save debug file to %s\n", crashfile);
    fprintf(stderr,
	"\n\n       Attempting to save debug file to %s\n\n", crashfile);

    /* Blat out last n keystrokes */
    fputs("========== Latest keystrokes ==========\n", dfile);
    while((i = key_recorder(0, 1)) != -1)
      fprintf(dfile, "\t%s\t(0x%04.4x)\n", pretty_command(i), i);

    /* look for existing debug file */
    for(i = 1; i <= NUMDEBUGFILES; i++){
	build_path(filename, ps_global->home_dir, DEBUGFILE);
	sprintf(nbuf, "%d", i);
	strcat(filename, nbuf);
	if(stat(filename, &tbuf) != 0)
	  continue;

	/* This must be the current debug file */
	if(tbuf.st_dev == dbuf.st_dev && tbuf.st_ino == dbuf.st_ino){
	    rename_file(filename, crashfile);
	    break;
	}
    }

    /* if current debug file name not found, write it by hand */
    if(i > NUMDEBUGFILES){
	FILE *cfp;
	char  buf[1025];

	/*
	 * Copy the debug temp file into the 
	 */
	if(cfp = fopen(crashfile, "w")){
	    buf[1024] = '\0';
	    fseek(dfile, 0L, 0);
	    while(fgets(buf, 1025, dfile) && fputs(buf, cfp) != EOF)
	      ;

	    fclose(cfp);
	}
    }

    fclose(dfile);
}


#define CHECK_EVERY_N_TIMES 100
#define MAX_DEBUG_FILE_SIZE 200000L
/*
 * This is just to catch runaway Pines that are looping spewing out
 * debugging (and filling up a file system).  The stop doesn't have to be
 * at all precise, just soon enough to hopefully prevent filling the
 * file system.  If the debugging level is high (9 for now), then we're
 * presumably looking for some problem, so don't truncate.
 */
int
do_debug(debug_fp)
FILE *debug_fp;
{
    static int counter = CHECK_EVERY_N_TIMES;
    static int ok = 1;
    long filesize;

    if(debug == DEFAULT_DEBUG && ok && --counter <= 0){
	if((filesize = fp_file_size(debug_fp)) != -1L)
	  ok = (unsigned long)filesize < (unsigned long)MAX_DEBUG_FILE_SIZE;

	counter = CHECK_EVERY_N_TIMES;
	if(!ok){
	    fprintf(debug_fp, "\n\n --- No more debugging ---\n");
	    fprintf(debug_fp,
		"     (debug file growing too large - over %ld bytes)\n\n",
		MAX_DEBUG_FILE_SIZE);
	    fflush(debug_fp);
	}
    }
    return(ok);
}
#endif /* DEBUG */


