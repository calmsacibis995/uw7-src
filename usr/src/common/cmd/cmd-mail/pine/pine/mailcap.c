#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*----------------------------------------------------------------------

           T H E    P I N E    M A I L   S Y S T E M

   Laurence Lundblade, Mike Seibel and David Miller
   Networks and Distributed Computing
   Computing and Communications
   University of Washington
   4545 Builiding, JE-20
   Seattle, Washington, 98195, USA
   Internet: mikes@cac.washington.edu
             dlm@cac.wasington.edu

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
    *                   Copyright (c) 1986, 1987 Dave Taylor              *
    *                   Copyright (c) 1988, 1989 USENET Community Trust   *
    ***********************************************************************


   ******************************************************* 
   Mailcap support is based in part on Metamail version 2.7:
      Author:  Nathaniel S. Borenstein, Bellcore

   Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
 
   Permission to use, copy, modify, and distribute this material
   for any purpose and without fee is hereby granted, provided
   that the above copyright notice and this permission notice
   appear in all copies, and that the name of Bellcore not be
   used in advertising or publicity pertaining to this
   material without the specific, prior written permission
   of an authorized representative of Bellcore.  BELLCORE
   MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
   OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
   WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
   ******************************************************* 

   ******************************************************* 
   MIME-TYPE support based on code contributed
   by Hans Drexler <drexler@mpi.nl>.  His comment:

   Mime types makes mime assign attachment types according
   to file name extensions found in a system wide file
   ``/usr/local/lib/mime.types'' and a user specific file
   ``~/.mime.types'' . These files specify file extensions
   that will be connected to a mime type.
   ******************************************************* 


  ----------------------------------------------------------------------*/

/*======================================================================
       mailcap.c

       Functions to support "mailcap" (RFC1524) to help us read MIME
       segments of various types, and ".mime.types" ala NCSA Mosaic to
       help us attach various types to outbound MIME segments.

 ====*/

#include "headers.h"

/*
 * We've decided not to implement the RFC1524 standard minimum path, because
 * some of us think it is harder to debug a problem when you may be misled
 * into looking at the wrong mailcap entry.  Likewise for MIME.Types files.
 */
#if defined(DOS) || defined(OS2)
#define MC_PATH_SEPARATOR ';'
#define MT_PATH_SEPARATOR ';'
#define	MT_USER_FILE	  "mimetype"
#else /* !DOS */
#define MC_PATH_SEPARATOR ':'
#define MC_STDPATH         \
		".mailcap:/etc/mailcap:/usr/etc/mailcap:/usr/local/etc/mailcap"
#define MT_PATH_SEPARATOR ':'
#define MT_STDPATH         \
		".mime.types:/etc/mime.types:/usr/local/lib/mime.types"
#endif /* !DOS */

#define LINE_BUF_SIZE      2000

typedef struct mcap_entry {
    struct mcap_entry *next;
    int                needsterminal;
    char              *contenttype;
    char              *command;
    char              *testcommand;
    char              *label;           /* unused */
    char              *printcommand;    /* unused */
} MailcapEntry;

MailcapEntry *mc_head = NULL,
	     *mc_tail = NULL;
static int    mc_is_initialized = 0;


/*
 * Types used to pass parameters and operator functions to the
 * mime.types searching routines.
 */
#define MT_MAX_FILE_EXTENSION 3

/*
 * Struct passed mime.types search functions
 */
typedef struct {
    union {
	char		*ext;
	char		*mtype;
    } from;
    union {
	BODY		*body;
	char		*ext;
    } to;
} MT_MAP_T;

typedef int (* MT_OPERATORPROC) PROTO((MT_MAP_T *, FILE *));


/*
 * Internal prototypes
 */
void      mc_addtolist PROTO((MailcapEntry *));
char     *mc_bld_test_cmd PROTO((char *, int, char *, PARAMETER *));
char     *mc_cmd_bldr PROTO((char *, int, char *, PARAMETER *, char *));
int       mc_ctype_match PROTO((int, char *, char *));
MailcapEntry *mc_get_command PROTO((int, char *, PARAMETER *));
int       mc_get_entry PROTO((FILE *, MailcapEntry *));
char     *mc_get_next_piece PROTO((char *, char **));
void      mc_init PROTO((void));
int       mc_passes_test PROTO((MailcapEntry *, int, char *, PARAMETER *));
void      mc_process_file PROTO((char *));
void	  mt_get_file_ext PROTO((char *, char **));
int	  mt_srch_mime_type PROTO((MT_OPERATORPROC, MT_MAP_T *));
int	  mt_browse_types_file PROTO((MT_OPERATORPROC, MT_MAP_T *, char *));
int	  mt_translate_type PROTO((char *));
int	  mt_srch_by_ext PROTO((MT_MAP_T *, FILE *));
int	  mt_srch_by_type PROTO((MT_MAP_T *, FILE *));



/*
 * mc_init - Run down the path gathering all the mailcap entries.
 *           Returns with the Mailcap list built.
 */
void
mc_init()
{
    char *s,
	 *pathcopy,
	 *path;
  
    if(mc_is_initialized)
      return;

    dprint(2, (debugfile, "- mc_init -\n"));

    if(ps_global->VAR_MAILCAP_PATH)
      /* there may need to be an override specific to pine */
      pathcopy = cpystr(ps_global->VAR_MAILCAP_PATH);
    else if((path = getenv("MAILCAPS")) != (char *)NULL)
      /* rfc1524 specifies MAILCAPS as a path override */
      pathcopy = cpystr(path);
    else{
#if defined(DOS) || defined(OS2)
        /*
	 * This gets interesting.  Since we don't have any standard location
	 * for config/data files, look in the same directory as the PINERC
	 * and the same dir as PINE.EXE.  This is similar to the UNIX
	 * situation with personal config info coming before 
	 * potentially shared config data...
	 */
	s = last_cmpnt(ps_global->pinerc);
	if(s != NULL){
	    strncpy(tmp_20k_buf, ps_global->pinerc, s - ps_global->pinerc);
	    tmp_20k_buf[s - ps_global->pinerc] = '\0';
	}
	else
	  strcpy(tmp_20k_buf, ".\\");

	sprintf(tmp_20k_buf + strlen(tmp_20k_buf),
	        "MAILCAP%c%s\\MAILCAP",
	        MC_PATH_SEPARATOR,
		ps_global->pine_dir);
#else /* !DOS */
	build_path(tmp_20k_buf, ps_global->home_dir, MC_STDPATH);
#endif /* !DOS */
	pathcopy = cpystr(tmp_20k_buf);
    }

    path = pathcopy;			/* overloaded "path" */

    /*
     * Insert an entry for the image-viewer variable from .pinerc, if present.
     */
    if(ps_global->VAR_IMAGE_VIEWER && *ps_global->VAR_IMAGE_VIEWER){
	MailcapEntry mc;
	char *p;

	p = tmp_20k_buf;
	sstrcpy(&p, ps_global->VAR_IMAGE_VIEWER);
	sstrcpy(&p, " %s");
	mc.command       = cpystr(tmp_20k_buf);
	mc.contenttype   = cpystr("image/*");
	mc.testcommand   = (char *)NULL;
	mc.printcommand  = (char *)NULL;
	mc.needsterminal = 0;
	mc.label         = cpystr("Pine Image Viewer");
	mc.next          = (MailcapEntry *)NULL;

	mc_addtolist(&mc);
	dprint(5, (debugfile, "mailcap: using image-viewer=%s\n", 
		   ps_global->VAR_IMAGE_VIEWER));
    }

    dprint(7, (debugfile, "mailcap: path: %s\n", path));
    while(path){
	s = strindex(path, MC_PATH_SEPARATOR);
	if(s)
	  *s++ = '\0';
	mc_process_file(path);
	path = s;
    }
  
    if(pathcopy)
      fs_give((void **)&pathcopy);

    mc_is_initialized++;

#ifdef DEBUG
    if(debug >= 11){
	MailcapEntry *mc;
	int i = 0;

	dprint(11, (debugfile, "Collected mailcap entries\n"));
	for(mc = mc_head; mc; mc = mc->next){

	    dprint(11, (debugfile, "%d: ", i++));
	    if(mc->label)
	      dprint(11, (debugfile, "%s\n", mc->label));
	    if(mc->contenttype)
	      dprint(11, (debugfile, "   %s", mc->contenttype));
	    if(mc->command)
	      dprint(11, (debugfile, "   command: %s\n", mc->command));
	    if(mc->testcommand)
	      dprint(11, (debugfile, "   testcommand: %s", mc->testcommand));
	    if(mc->printcommand)
	      dprint(11, (debugfile, "   printcommand: %s", mc->printcommand));
	    dprint(11, (debugfile, "   needsterminal %d\n", mc->needsterminal));
	}
    }
#endif /* DEBUG */
}


/*
 * Add all the entries from this file onto the Mailcap list.
 */
void
mc_process_file(file)
char *file;
{
    MailcapEntry mc;
    int file_check;
    FILE *fp;
    char filebuf[MAXPATH+1];
  
    dprint(2, (debugfile, "mailcap: process_file: %s\n", file));

    (void)strncpy(filebuf, file, MAXPATH);
    filebuf[MAXPATH] = '\0';
    file = fnexpand(filebuf, MAXPATH);
    dprint(7, (debugfile, "mailcap: processing file: %s\n", file));
    file_check = is_writable_dir(file);
    switch(file_check){
      case 0: case 1: /* is a directory */
	dprint(2, (debugfile, "mailcap: %s is a directory, should be a file\n",
	    file));
	return;

      case 2: /* ok */
	break;

      case 3: /* doesn't exist */
	dprint(5, (debugfile, "mailcap: %s doesn't exist\n", file));
	return;

      default:
	panic("Programmer botch in mc_process_file");
	/*NOTREACHED*/
    }

    fp = fopen(file, "r");

    if(fp){
	while(mc_get_entry(fp, &mc))
	  mc_addtolist(&mc);
    
	(void)fclose(fp);
    }
}


/*
 * Find the next entry in the file pointed to by fp and fill in mc.
 *
 * Returns 1 if it found another entry, 0 if no more entries in file.
 */
int
mc_get_entry(fp, mc)
FILE *fp;
MailcapEntry *mc;
{
    char *s, *t, *cont, *raw_entry;
    char  linebuf[LINE_BUF_SIZE];
    int   raw_entry_alloc, len;

    mc->contenttype   = (char *)NULL;
    mc->command       = (char *)NULL;
    mc->testcommand   = (char *)NULL;
    mc->label         = (char *)NULL;
    mc->printcommand  = (char *)NULL;
    mc->needsterminal = 0;
    mc->next          = (MailcapEntry *)NULL;

#define ALLOC_INCREMENT  1000
    raw_entry_alloc = ALLOC_INCREMENT;
    raw_entry = (char *)fs_get((size_t)raw_entry_alloc + 1);
try_again:
    *raw_entry = '\0';

    /* handle continuation lines and comments */
    while(fgets(linebuf, LINE_BUF_SIZE, fp)){
	if(*linebuf == '#' || *linebuf == '\n') /* skip comments and blanks */
	  continue;
        len = strlen(linebuf);
        if(linebuf[len-1] == '\n')
	  linebuf[--len] = '\0';
        if(len + strlen(raw_entry) > raw_entry_alloc){
            raw_entry_alloc += ALLOC_INCREMENT;
            fs_resize((void **)&raw_entry, (size_t)raw_entry_alloc+1);
        }
        if(linebuf[len-1] == '\\'){
            linebuf[len-1] = '\0';
            (void)strcat(raw_entry, linebuf);
        }
	else{
            (void)strcat(raw_entry, linebuf);
            break;
        }
    }

    /* skip leading space in line */
    for(cont = raw_entry; *cont && isspace((unsigned char)*cont); ++cont)
      ;/* do nothing */
    if(!*cont){  /* blank entry */
	if(feof(fp)){  /* end of file */
	    if(raw_entry)
	      fs_give((void **)&raw_entry);
	    dprint(7, (debugfile, "mailcap: end of file\n"));
	    return 0;
	}
	goto try_again;
    }

    s = strindex(cont, ';');
    if(!s){
	dprint(2, (debugfile, "mailcap: ignoring entry, no semicolon: %s\n",
	    raw_entry));
	goto try_again;
    }

    dprint(8, (debugfile, "mailcap: processing entry: %s\n", raw_entry));

    *s++ = '\0';

    /* trim white space, convert to lowercase */
    mc->contenttype = cpystr(strclean(cont));
    dprint(9, (debugfile, "mailcap: content_type: %s\n", mc->contenttype));

    /* make implicit wildcard explicit */
    if (strindex(mc->contenttype, '/') == NULL){
	fs_resize((void **)&mc->contenttype, strlen(mc->contenttype) + 3);
        (void)strcat(mc->contenttype, "/*");
    }

    t = mc_get_next_piece(s, &mc->command);
    removing_leading_white_space(mc->command);
    dprint(9, (debugfile, "mailcap: command: %s\n", mc->command));

    if(!t){
	fs_give((void **)&raw_entry);
	return 1;
    }

    /* process fields */
    dprint(9, (debugfile, "mailcap: flags: %s\n", t));
    for(s = t; s; s = t){
	char *arg, *eq;
	
	t = mc_get_next_piece(s, &arg);
	if(*arg)
	  removing_leading_white_space(arg);
	eq = strindex(arg, '=');
	if(eq)
	  *eq++ = '\0';

	if(*arg)
	  removing_trailing_white_space(arg);
	if(*arg){
	    if(!strucmp(arg, "needsterminal")){
	        mc->needsterminal = 1;
	        dprint(9, (debugfile, "mailcap: set needsterminal\n"));
	    }
	    else if(!strucmp(arg, "copiousoutput")){
	        mc->needsterminal = 2;
	        dprint(9, (debugfile, "mailcap: set copiousoutput\n"));
	    }
	    else if(eq && !strucmp(arg, "test")){
	        mc->testcommand = cpystr(eq);
	        dprint(9, (debugfile, "mailcap: testcommand=%s\n",
		    mc->testcommand));
	    }
	    else if(eq && !strucmp(arg, "description")){
	        mc->label = cpystr(eq);
	        dprint(9, (debugfile, "mailcap: label=%s\n",
		    mc->label));
	    }
	    else if(eq && !strucmp(arg, "print")){
	        mc->printcommand = cpystr(eq);
	        dprint(9, (debugfile, "mailcap: printcommand=%s\n",
		    mc->printcommand));
	    }
	    else if(eq && !strucmp(arg, "compose")){
	        ;/* not used */
	        dprint(9, (debugfile, "mailcap: not using compose=%s\n", eq));
	    }
	    else if(eq && !strucmp(arg, "composetyped")){
	        ;/* not used */
	        dprint(9, (debugfile, "mailcap: not using composetyped=%s\n",
		    eq));
	    }
	    else if(eq && !strucmp(arg, "textualnewlines")){
	        ;/* not used */
	        dprint(9, (debugfile,
		    "mailcap: not using texttualnewlines=%s\n", eq));
	    }
	    else if(eq && !strucmp(arg, "edit")){
	        ;/* not used */
	        dprint(9, (debugfile, "mailcap: not using edit=%s\n", eq));
	    }
	    else if(eq && !strucmp(arg, "x11-bitmap")){
	        ;/* not used */
	        dprint(9, (debugfile, "mailcap: not using x11-bitmap=%s\n",
		    eq));
	    }
	    else
	      dprint(9, (debugfile, "mailcap: ignoring unknown flag: %s\n",
		  arg));
	}

	if(arg)
	  fs_give((void **)&arg);
    }

    fs_give((void **)&raw_entry);
    return 1;
}


/*
 * s is a pointer to the current position in the mailcap entry
 *
 * "result" is a pointer to point at an alloc'd copy of the current piece, up
 * to the next ";", and the return value is a pointer to the piece after
 * the ";" (the next "s").
 */
char *
mc_get_next_piece(s, result)
char *s, **result;
{
    char *s2;
    int quoted = 0;

    s2 = (char *)fs_get(strlen(s) * 2 + 1); /* absolute max, if all % signs */

    *result = s2;

    while(s && *s){
	if(quoted){
	    if(*s == '%') /* quoted % turns into %% */
	      *s2++ = '%';
      
	    *s2++ = *s++;
	    quoted = 0;
	}
	else{
	    if(*s == ';'){  
		*s2 = '\0';
		dprint(9, (debugfile, "mailcap: mc_get_next_piece: %s\n",
		    *result));
		return(++s);
	    }
#if defined(DOS) || defined(OS2)
	    /*
	     * RFC 1524 says that backslash is used to quote
	     * the next character, but since backslash is part of pathnames
	     * on DOS we're afraid people will not put double backslashes
	     * in their mailcap files.  Therefore, we violate the RFC by
	     * looking ahead to the next character.  If it looks like it
	     * is just part of a pathname, then we consider a single
	     * backslash to *not* be a quoting character, but a literal
	     * backslash instead.
	     */
	    else if(*s == '\\'){
		int t;

		t = *(s+1);
		/*
		 * If next char is any of these, treat the backslash
		 * that preceded it like a regular character.
		 */
		if(t && isascii(t) &&
		     (isalnum((unsigned char)t) || t == '_' || t == '+'
		      || t == '-' || t == '=' || t == '~')){
		    *s2++ = *s++;
		}
		/* otherwise, treat it as a quoting backslash */
		else{
		    quoted++;
		    ++s;
		}
	    }
#else  /* !DOS */
	    /* backslash quotes the next character */
	    else if(*s == '\\'){
		quoted++;
		++s;
	    }
#endif  /* !DOS */
	    else
	      *s2++ = *s++;
	} 
    }

    *s2 = '\0';

    dprint(9, (debugfile, "mailcap: mc_get_next_piece(last): %s\n", *result));

    return NULL;
}
     

/*
 * Returns the mailcap entry for type/subtype from the successfull
 * mailcap entry, or NULL if none.  Command string still contains % stuff.
 */
MailcapEntry *
mc_get_command(type, subtype, params)
int        type;
char      *subtype;
PARAMETER *params;
{
    MailcapEntry *mc;

    dprint(4, (debugfile, "- mc_get_command(%s/%s) -\n",
	body_type_names(type), subtype));

    for(mc = mc_head; mc; mc = mc->next){
	if(mc_ctype_match(type, subtype, mc->contenttype) &&
	   mc_passes_test(mc, type, subtype, params)){

	    dprint(9, (debugfile, 
		     "mc_get_command: type=%s/%s, command=%s\n", 
		     body_type_names(type), subtype, mc->command));
	    return(mc);
	}
    }
    return((MailcapEntry *)NULL);
}


/*
 * Check whether the pattern "pat" matches this type/subtype.
 * Returns 1 if it does, 0 if not.
 */
int
mc_ctype_match(type, subtype, pat)
int   type;
char *subtype;
char *pat;
{
    int  len, rv;
    char *type_txt;
    char *full_ctype;
    char *p;

    type_txt = body_type_names(type);
    len = strlen(subtype) + strlen(type_txt);
    full_ctype = (char *)fs_get((size_t)len + 1 + 1);
    p = full_ctype;
    sstrcpy(&p, type_txt);
    sstrcpy(&p, "/");
    sstrcpy(&p, subtype);

    dprint(4, (debugfile, 
	   "mc_ctype_match: trying entry %s for %s\n", pat, full_ctype));
	    
    rv = 0;

    /* check for exact match */
    if(!strucmp(full_ctype, pat))
      rv = 1;

    /* check for wildcard match */
    else if ((len=strlen(pat)) >= 2 
	 && (pat[--len] == '*')
         && (pat[--len] == '/')
         && (!struncmp(full_ctype, pat, len))
         && ((full_ctype[len] == '/') || (full_ctype[len] == '\0')))
      rv = 1;

    fs_give((void **)&full_ctype);
    return(rv);
}


/*
 * Run the test command for entry mc to see if this entry currently applies to
 * applies to this type/subtype.
 *
 * Returns 1 if it does pass test (exits with status 0), 0 otherwise.
 */
int
mc_passes_test(mc, type, subtype, params)
MailcapEntry *mc;
int           type;
char         *subtype;
PARAMETER    *params;
{
    char *cmd = NULL;
    int   rv;

    dprint(2, (debugfile, "- mc_passes_test -\n"));

    if(mc->testcommand && *mc->testcommand)
	cmd = mc_bld_test_cmd(mc->testcommand, type, subtype, params);
    
    if(!mc->testcommand || !cmd || !*cmd){
	if(cmd)
	  fs_give((void **)&cmd);
	dprint(7, (debugfile, "no test command, so Pass\n"));
	return 1;
    }

    rv = exec_mailcap_test_cmd(cmd);
    dprint(7, (debugfile, "mc_passes_test: \"%s\" %s (rv=%d)\n",
       cmd, rv ? "Failed" : "Passed", rv)) ;

    fs_give((void **)&cmd);

    return(!rv);
}


int
mailcap_can_display(type, subtype, params)
int       type;
char     *subtype;
PARAMETER *params;
{
    dprint(5, (debugfile, "- mailcap_can_display -\n"));

    if(!mc_is_initialized)
      mc_init();

    return(mc_get_command(type, subtype, params) ? 1 : 0);
}


char *
mailcap_build_command(body, tmp_file, needsterm)
BODY *body;
char *tmp_file;
int  *needsterm;
{
    MailcapEntry *mc;
    char         *command;

    dprint(2, (debugfile, "- mailcap_build_command -\n"));

    if(!mc_is_initialized)
      mc_init();

    mc = mc_get_command((int)body->type, body->subtype, body->parameter);
    if(!mc){
	q_status_message(SM_ORDER, 3, 4, "Error constructing viewer command");
	dprint(2, (debugfile,
		   "mailcap_build_command: no command string for %s/%s\n",
		   body_type_names((int)body->type), body->subtype));
	return((char *)NULL);
    }

    *needsterm = mc->needsterminal;

    command = mc_cmd_bldr(mc->command, (int)body->type,
			  body->subtype, body->parameter, tmp_file);

    dprint(2, (debugfile, "built command: %s\n", command));

    return(command);
}


/*
 * mc_bld_test_cmd - build the command to test if the given type flies
 *
 *    mc_cmd_bldr's tmp_file argument is NULL as we're not going to
 *    decode and write each and every MIME segment's data to a temp file
 *    when no test's going to use the data anyway.
 */
char *
mc_bld_test_cmd(controlstring, type, subtype, params)
    char      *controlstring;
    int        type;
    char      *subtype;
    PARAMETER *params;
{
    return(mc_cmd_bldr(controlstring, type, subtype, params, NULL));
}


/*
 * mc_cmd_bldr - construct a command string to execute
 *
 *    If tmp_file is null, then the contents of the given MIME segment
 *    is not provided.  This is useful for building the "test=" string
 *    as it doesn't operate on the segment's data.
 *
 *    The return value is an alloc'd copy of the command to be executed.
 */
char *
mc_cmd_bldr(controlstring, type, subtype, parameter, tmp_file)
    char *controlstring;
    int   type;
    char *subtype;
    PARAMETER *parameter;
    char *tmp_file;
{
    char        *from, *to, *s; 
    int	         prefixed = 0, used_tmp_file = 0;
    PARAMETER   *parm;

    dprint(8, (debugfile, "- mc_cmd_bldr -\n"));

    for(from = controlstring, to = tmp_20k_buf; *from; ++from){
	if(prefixed){			/* previous char was % */
	    prefixed = 0;
	    switch(*from){
	      case '%':			/* turned \% into this earlier */
		*to++ = '%';
		break;

	      case 's':			/* insert tmp_file name in cmd */
		if(tmp_file){
		    used_tmp_file = 1;
		    sstrcpy(&to, tmp_file);
		}
		else
		  dprint(2, (debugfile,
			     "mc_cmd_bldr: %%s in cmd but not supplied!\n"));

		break;

	      case 't':			/* insert MIME type/subtype */
		/* quote to prevent funny business */
		*to++ = '\'';
		sstrcpy(&to, body_type_names(type));
		*to++ = '/';
		sstrcpy(&to, subtype);
		*to++ = '\'';
		break;

	      case '{':			/* insert requested MIME param */
		s = strindex(from, '}');
		if(!s){
		    q_status_message1(SM_ORDER, 0, 4,
	"Ignoring ill-formed parameter reference in mailcap file: %s", from);
		    break;
		}

		*s = '\0';
		++from;    /* from is the part inside the brackets now */

		for(parm = parameter; parm; parm = parm->next)
		  if(!strucmp(from, parm->attribute))
		    break;

		dprint(9, (debugfile,
			   "mc_cmd_bldr: parameter %s = %s\n", 
			   from, parm ? parm->value : "(not found)"));

		/*
		 * Quote parameter values for /bin/sh.
		 * Put single quotes around the whole thing but every time
		 * there is an actual single quote put it outside of the
		 * single quotes with a backslash in front of it.  So the
		 * parameter value  fred's car
		 * turns into       'fred'\''s car'
		 */
		*to++ = '\'';  /* opening quote */
		if(parm){
		    char *p;

		    /*
		     * Copy value, but quote single quotes for /bin/sh
		     * Backslash quote is ignored inside single quotes so
		     * have to put those outside of the single quotes.
		     */
		    for(p = parm->value; *p; p++){
			if(*p == '\''){
			    *to++ = '\'';  /* closing quote */
			    *to++ = '\\';
			    *to++ = '\'';  /* below will be opening quote */
			}
			*to++ = *p;
		    }
		}

		*to++ = '\'';  /* closing quote for /bin/sh */

		*s = '}'; /* restore */
		from = s;
		break;

		/*
		 * %n and %F are used by metamail to support otherwise
		 * unrecognized multipart Content-Types.  Pine does
		 * not use these since we're only dealing with the individual
		 * parts at this point.
		 */
	      case 'n':
	      case 'F':  
	      default:  
		dprint(9, (debugfile, 
		   "Ignoring %s format code in mailcap file: %%%c\n",
		   (*from == 'n' || *from == 'F') ? "unimplemented"
						  : "unrecognized",
		   *from));
		break;
	    }
	}
	else if(*from == '%')		/* next char is special */
	  prefixed = 1;
	else				/* regular character, just copy */
	  *to++ = *from;
    }

    *to = '\0';

    /*
     * file not specified, redirect to stdin
     */
    if(!used_tmp_file && tmp_file)
      sprintf(to, " < %s", tmp_file);

    return(cpystr(tmp_20k_buf));
} 


/*
 * Adds element mc to the mailcap list.
 */
void
mc_addtolist(mc)
MailcapEntry *mc;
{
    register MailcapEntry *new_mc;

    new_mc = (MailcapEntry *)fs_get(sizeof(MailcapEntry));

    /*
     * Copy the structure, but don't copy pointed-to strings, just
     * copy the pointers themselves.
     */
    *new_mc = *mc;

    /* add to end of list so search order is correct */
    if(mc_tail)
	mc_tail->next = new_mc;
    else
	mc_head       = new_mc;

    mc_tail = new_mc;
}


void
mailcap_free()
{
    MailcapEntry *mc, *mc_next;

    dprint(2, (debugfile, "- mailcap_free -\n"));

    if(!mc_is_initialized)
      return;
    
    for(mc = mc_head; mc; mc = mc_next){
	mc_next = mc->next;
	if(mc->contenttype)
	fs_give((void **)&mc->contenttype);
	if(mc->command)
	    fs_give((void **)&mc->command);
	if(mc->testcommand)
	    fs_give((void **)&mc->testcommand);
	if(mc->label)
	    fs_give((void **)&mc->label);
	if(mc->printcommand)
	    fs_give((void **)&mc->printcommand);
	fs_give((void **)&mc);
    }

    mc_is_initialized = 0;
    mc_head = (MailcapEntry *)NULL;
    mc_tail = (MailcapEntry *)NULL;
}



/*
 * END OF MAILCAP ROUTINES, START OF MIME.TYPES ROUTINES
 */


/*
 * Exported function that does the work of sniffing the mime.types
 * files and filling in the body pointer if found.  Returns 1 (TRUE) if
 * extension found, and body pointer filled in, 0 (FALSE) otherwise.
 */
int
set_mime_type_by_extension(body, filename)
    BODY *body;
    char *filename;
{
    char     *extension;
    MT_MAP_T  e2b;

    mt_get_file_ext(filename, &extension);
    dprint(5, (debugfile, "mt_set_new : filename=%s, extension=%s\n",
	       filename, extension));
    if (*extension) {
	e2b.from.ext = extension;
	e2b.to.body = body;
	return (mt_srch_mime_type(mt_srch_by_ext, &e2b));
    }
    return(0);
}


/*
 * Exported function that maps from mime types to file extensions.
 */
int
set_mime_extension_by_type (ext, mtype)
    char *ext;
    char *mtype;
{
    MT_MAP_T t2e;

    t2e.from.mtype = mtype;
    t2e.to.ext = ext;
    return (mt_srch_mime_type (mt_srch_by_type, &t2e));
}
    



/* 
 * Separate and return a pointer to the first character in the 'filename'
 * character buffer that comes after the rightmost '.' character in the
 * filename. (What I mean is a pointer to the filename - extension).
 */
void
mt_get_file_ext(filename, extension)
    char *filename;
    char **extension;
{
    char *index = NULL;

    do
      if(*filename == '.')
	index = filename;
    while (*filename++ != '\0');

    *extension = (index) ? index + 1 : filename - 1;
}


/*
 * Build a list of possible mime.type files.  For each one that exists
 * call the mt_operator function.
 * Loop terminates when mt_operator returns non-zero.  
 */
int
mt_srch_mime_type(mt_operator, mt_map)
    MT_OPERATORPROC	mt_operator;
    MT_MAP_T	       *mt_map;
{
    char *s, *pathcopy, *path;
    int   rv = 0;
  
    dprint(2, (debugfile, "- mt_use_mime_type -\n"));

    /* We specify MIMETYPES as a path override */
    if(ps_global->VAR_MIMETYPE_PATH)
      /* there may need to be an override specific to pine */
      pathcopy = cpystr(ps_global->VAR_MIMETYPE_PATH);
    else if((path = getenv("MIMETYPES")) != (char *)NULL)
      pathcopy = cpystr(path);
    else{
#if defined(DOS) || defined(OS2)
        /*
	 * This gets interesting.  Since we don't have any standard location
	 * for config/data files, look in the same directory as the PINERC
	 * and the same dir as PINE.EXE.  This is similar to the UNIX
	 * situation with personal config info coming before 
	 * potentially shared config data...
	 */
	s = last_cmpnt(ps_global->pinerc);
	if(s != NULL){
	    strncpy(tmp_20k_buf, ps_global->pinerc, s - ps_global->pinerc);
	    tmp_20k_buf[s - ps_global->pinerc] = '\0';
	}
	else
	  strcpy(tmp_20k_buf, ".\\");

	sprintf(tmp_20k_buf + strlen(tmp_20k_buf),
	        "%s%c%s\\%s",
		MT_USER_FILE, MT_PATH_SEPARATOR,
		ps_global->pine_dir, MT_USER_FILE);
#else	/* !DOS */
	build_path(tmp_20k_buf, ps_global->home_dir, MT_STDPATH);
#endif	/* !DOS */
	pathcopy = cpystr(tmp_20k_buf);
    }

    path = pathcopy;			/* overloaded "path" */

    dprint(7, (debugfile, "mime_types: path: %s\n", path));
    while(path){
	if(s = strindex(path, MT_PATH_SEPARATOR))
	  *s++ = '\0';

	if(rv = mt_browse_types_file(mt_operator, mt_map, path))
	  break;

	path = s;
    }
  
    if(pathcopy)
      fs_give((void **)&pathcopy);

    return(rv);
}


/*
 * Try to match a file extension against extensions found in the file
 * ``filename'' if that file exists. return 1 if a match
 * was found and 0 in all other cases.
 */
int
mt_browse_types_file(mt_operator, mt_map, filename)
    MT_OPERATORPROC	mt_operator;
    MT_MAP_T	       *mt_map;
    char *filename;
{
    int   rv = 0;
    FILE *file;

    if(file = fopen(filename, "r")){
	rv = mt_operator(mt_map, file);
	fclose(file);
    }
    else
      dprint(1, (debugfile, "mt_browse: FAILED open(%s) : %s.\n",
		 filename, error_description(errno)));

    return(rv);
}


/*
 * scan each line of the file. Treat each line as a mime type definition.
 * The first word is a type/subtype specification. All following words
 * are file extensions belonging to that type/subtype. Words are separated
 * bij whitespace characters.
 * If a file extension occurs more than once, then the first definition
 * determines the file type and subtype.
 */
int
mt_srch_by_ext(e2b, file)
    MT_MAP_T *e2b;
    FILE     *file;
{
    char buffer[LINE_BUF_SIZE];
#if	defined(DOS) || defined(OS2)
#define	STRCMP	strucmp
#else
#define	STRCMP	strcmp
#endif

    /* construct a loop reading the file line by line. Then check each
     * line for a matching definition.
     */
    while(fgets(buffer,LINE_BUF_SIZE,file) != NULL){
	char *typespec;
	char *try_extension;

	if(buffer[0] == '#')
	  continue;		/* comment */

	/* remove last character from the buffer */
	buffer[strlen(buffer)-1] = '\0';
	/* divide the input buffer into words separated by whitespace.
	 * The first words is the type and subtype. All following words
	 * are file extensions.
	 */
	dprint(5, (debugfile, "traverse: buffer=%s.\n", buffer));
	typespec = strtok(buffer," \t");	/* extract type,subtype  */
	dprint(5, (debugfile, "typespec=%s.\n", typespec));
	while((try_extension = strtok(NULL, " \t")) != NULL){
	    /* compare the extensions, and assign the type if a match
	     * is found.
	     */
	    dprint(5, (debugfile,"traverse: trying ext %s.\n", try_extension));
	    if(STRCMP(try_extension, e2b->from.ext) == 0){
		/* split the 'type/subtype' specification */
		char *type = strtok(typespec,"/");
		char *subtype = strtok(NULL,"/");
		dprint(5, (debugfile, "traverse: type=%s, subtype=%s.\n",
			   type,subtype));
		/* The type is encoded as a small integer. we have to
		 * translate the character string naming the type into
		 * the corresponding number.
		 */
		e2b->to.body->type = mt_translate_type(type);
		e2b->to.body->subtype = cpystr(subtype);
		return 1; /* a match has been found */
	    }
	}
    }

    dprint(5, (debugfile, "traverse: search failed.\n"));
    return 0;
}




/*
 * scan each line of the file. Treat each line as a mime type definition.
 * Here we are looking for a matching type.  When that is found return the
 * first extension that is three chars or less.
 */
int
mt_srch_by_type(t2e, file)
    MT_MAP_T *t2e;
    FILE     *file;
{
    char buffer[LINE_BUF_SIZE];

    /* construct a loop reading the file line by line. Then check each
     * line for a matching definition.
     */
    while(fgets(buffer,LINE_BUF_SIZE,file) != NULL){
	char *typespec;
	char *try_extension;

	if(buffer[0] == '#')
	  continue;		/* comment */

	/* remove last character from the buffer */
	buffer[strlen(buffer)-1] = '\0';
	/* divide the input buffer into words separated by whitespace.
	 * The first words is the type and subtype. All following words
	 * are file extensions.
	 */
	dprint(5, (debugfile, "traverse: buffer=%s.\n", buffer));
	typespec = strtok(buffer," \t");	/* extract type,subtype  */
	dprint(5, (debugfile, "typespec=%s.\n", typespec));
	if (strucmp (typespec, t2e->from.mtype) == 0) {
	    while((try_extension = strtok(NULL, " \t")) != NULL) {
		if (strlen (try_extension) <= MT_MAX_FILE_EXTENSION) {
		    strcpy (t2e->to.ext, try_extension);
		    return (1);
	        }
	    }
        }
    }

    dprint(5, (debugfile, "traverse: search failed.\n"));
    return 0;
}


/*
 * Translate a character string representing a content type into a short 
 * integer number, according to the coding described in c-client/mail.h
 * List of content types taken from rfc1521, September 1993.
 */
int
mt_translate_type(type)
    char *type;
{
    int i;

    for (i=0;(i<=TYPEMAX) && body_types[i] && strucmp(type,body_types[i]);i++)
      ;

    if (i > TYPEMAX)
      i = TYPEOTHER;
    else if (!body_types[i])	/* if empty slot, assign it to this type */
      body_types[i] = cpystr (type);

    return(i);
}
