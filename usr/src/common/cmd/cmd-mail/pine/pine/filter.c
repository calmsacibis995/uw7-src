#if !defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
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
     filter.c

     This code provides a generalized, flexible way to allow
     piping of data thru filters.  Each filter is passed a structure
     that it will use to hold its static data while it operates on 
     the stream of characters that are passed to it.  After processing
     it will either return or call the next filter in 
     the pipe with any character (or characters) it has ready to go. This
     means some terminal type of filter has to be the last in the 
     chain (i.e., one that writes the passed char someplace, but doesn't
     call another filter).

     See below for more details.

     The motivation is to handle MIME decoding, richtext conversion, 
     iso_code stripping and anything else that may come down the
     pike (e.g., PEM) in an elegant fashion.  mikes (920811)

   TODO:
       reasonable error handling

  ====*/


#include "headers.h"


/*
 * Internal prototypes
 */
int	gf_so_readc PROTO((unsigned char *));
int	gf_so_writec PROTO((int));
int	gf_sreadc PROTO((unsigned char *));
int	gf_swritec PROTO((int));
int	gf_freadc PROTO((unsigned char *));
int	gf_fwritec PROTO((int));
void	gf_terminal PROTO((FILTER_S *, int));
char   *gf_filter_puts PROTO((char *));
void	gf_filter_eod PROTO((void));
void    gf_error PROTO((char *));
void	gf_8bit_put PROTO((FILTER_S *, int));
int	so_reaquire PROTO((STORE_S *));
int	so_cs_writec PROTO((int, STORE_S *));
int	so_pico_writec PROTO((int, STORE_S *));
int	so_file_writec PROTO((int, STORE_S *));
int	so_cs_readc PROTO((unsigned char *, STORE_S *));
int	so_pico_readc PROTO((unsigned char *, STORE_S *));
int	so_file_readc PROTO((unsigned char *, STORE_S *));
int	so_cs_puts PROTO((STORE_S *, char *));
int	so_pico_puts PROTO((STORE_S *, char *));
int	so_file_puts PROTO((STORE_S *, char *));


/*
 * GENERALIZED STORAGE FUNCTIONS.  Idea is to allow creation of
 * storage objects that can be written into and read from without
 * the caller knowing if the storage is core or in a file 
 * or whatever.
 */
#define	MSIZE_INIT	8192
#define	MSIZE_INC	4096

#ifdef	DOS
#define	NO_PIPE
#define	CRLF_NEWLINES
#define	READ_MODE	"rb"
#define	APPEND_MODE	"a+b"
#else
#ifdef OS2
#define CRLF_NEWLINES
#define READ_MODE	"rb"
#define APPEND_MODE	"a+b"
#else
#define	READ_MODE	"r"
#define	APPEND_MODE	"a+"
#endif
#endif


/*
 * allocate resources associated with the specified type of
 * storage.  If requesting a named file object, open it for 
 * appending, else just open a temp file.
 *
 * return the filled in storage object
 */
STORE_S *
so_get(source, name, rtype)
    SourceType  source;			/* requested storage type */
    char       *name;			/* file name 		  */
    int		rtype;			/* file access type	  */
{
    STORE_S *so;
    char    *type = (rtype&WRITE_ACCESS) ? APPEND_MODE : READ_MODE;

    so = (STORE_S *)fs_get(sizeof(STORE_S));
    memset(so, 0, sizeof(STORE_S));
    so->flags |= rtype;
    
    if(name)					/* stash the name */
      so->name = cpystr(name);
#ifdef	DOS
    else if(source == TmpFileStar || source == FileStar){
	/*
	 * Coerce to TmpFileStar.  The MSC library's "tmpfile()"
	 * doesn't observe the "TMP" or "TEMP" environment vars and 
	 * always wants to write "\".  This is problematic in shared,
	 * networked environments.
	 */
	source   = TmpFileStar;
	so->name = temp_nam(NULL, "pi");
    }
#else
    else if(source == TmpFileStar)		/* make one up! */
      so->name = temp_nam(NULL, "pine-tmp");
#endif

    so->src = source;
    if(so->src == FileStar || so->src == TmpFileStar){
	so->writec = so_file_writec;
	so->readc  = so_file_readc;
	so->puts   = so_file_puts;

	/*
	 * The reason for both FileStar and TmpFileStar types is
	 * that, named or unnamed, TmpFileStar's are unlinked
	 * when the object is given back to the system.  This is
	 * useful for keeping us from running out of file pointers as
	 * the pointer associated with the object can be temporarily
	 * returned to the system without destroying the object.
	 * 
	 * The programmer is warned to be careful not to assign the
	 * TmpFileStar type to any files that are expected to remain
	 * after the dust has settled!
	 */
	if(so->name){
	    if(!(so->txt = (void *)fopen(so->name, type))){
		dprint(1, (debugfile, "so_get error: %s : %s", so->name,
			   error_description(errno)));
		fs_give((void **)&so->name);
		fs_give((void **)&so); 		/* so freed & set to NULL */
	    }
	}
	else{
	    if(!(so->txt = (void *)create_tmpfile())){
		dprint(1, (debugfile, "so_get error: tmpfile : %s",
			   error_description(errno)));
		fs_give((void **)&so);		/* so freed & set to NULL */
	    }
	}
    }
    else if(so->src == PicoText){
	so->writec = so_pico_writec;
	so->readc  = so_pico_readc;
	so->puts   = so_pico_puts;
	if(!(so->txt = pico_get())){
	    dprint(1, (debugfile, "so_get error: alloc of pico text space"));
	    if(so->name)
	      fs_give((void **)&so->name);
	    fs_give((void **)&so);		/* so freed & set to NULL */
	}
    }
    else{
	so->writec = so_cs_writec;
	so->readc  = so_cs_readc;
	so->puts   = so_cs_puts;
	so->txt	   = (void *)fs_get((size_t) MSIZE_INIT * sizeof(char));
	so->dp	   = so->eod = (unsigned char *) so->txt;
	so->eot	   = so->dp + MSIZE_INIT;
	memset(so->eod, 0, so->eot - so->eod);
    }

    return(so);
}


/* 
 * so_give - free resources associated with a storage object and then
 *           the object itself.
 */
void
so_give(so)
STORE_S **so;
{
    if(!so)
      return;

    if((*so)->src == FileStar || (*so)->src == TmpFileStar){
        if((*so)->txt)
	  fclose((FILE *)(*so)->txt);	/* disassociate from storage */

	if((*so)->name && (*so)->src == TmpFileStar)
	  unlink((*so)->name);		/* really disassociate! */
    }
    else if((*so)->txt && (*so)->src == PicoText)
      pico_give((*so)->txt);
    else if((*so)->txt)
      fs_give((void **)&((*so)->txt));

    if((*so)->name)
      fs_give((void **)&((*so)->name));	/* blast the name            */

    fs_give((void **)so);		/* release the object        */
}


/*
 * put a character into the specified storage object, 
 * expanding if neccessary 
 *
 * return 1 on success and 0 on failure
 */
int
so_cs_writec(c, so)
    int      c;
    STORE_S *so;
{
    register unsigned char  ch = (unsigned char) c;

    if(so->dp >= so->eot){
	register size_t cur_o  = so->dp - (unsigned char *)so->txt;
	register size_t data_o = so->eod - (unsigned char *)so->txt;
	register size_t size   = (so->eot-(unsigned char *)so->txt)+MSIZE_INC;
	fs_resize(&so->txt, size * sizeof(char));
	so->dp   = (unsigned char *) so->txt + cur_o;
	so->eod  = (unsigned char *) so->txt + data_o;
	so->eot  = (unsigned char *) so->txt + size;
	memset(so->eod, 0, so->eot - so->eod);
    }

    *so->dp++ = ch;
    if(so->dp > so->eod)
      so->eod = so->dp;

    return(1);
}

int
so_pico_writec(c, so)
    int      c;
    STORE_S *so;
{
    unsigned char ch = (unsigned char) c;

    return(pico_writec(so->txt, ch));
}

int
so_file_writec(c, so)
    int      c;
    STORE_S *so;
{
    unsigned char ch = (unsigned char) c;
    int rv = 0;

    if(so->txt || so_reaquire(so))
      do
	rv = fwrite(&ch,sizeof(unsigned char),(size_t)1,(FILE *)so->txt);
      while(!rv && ferror((FILE *)so->txt) && errno == EINTR);

    return(rv);
}


/*
 * get a character from the specified storage object.
 * 
 * return 1 on success and 0 on failure
 */
int
so_cs_readc(c, so)
    unsigned char *c;
    STORE_S       *so;
{
    return((so->dp < so->eod) ? *c = *(so->dp)++, 1 : 0);
}

int
so_pico_readc(c, so)
    unsigned char *c;
    STORE_S       *so;
{
    return(pico_readc(so->txt, c));
}

int
so_file_readc(c, so)
    unsigned char *c;
    STORE_S       *so;
{
    int rv = 0;

    if(so->txt || so_reaquire(so))
      do
	rv = fread(c, sizeof(char), (size_t)1, (FILE *)so->txt);
      while(!rv && ferror((FILE *)so->txt) && errno == EINTR);

    return(rv);
}


/* 
 * write a string into the specified storage object, 
 * expanding if necessary (and cheating if the object 
 * happens to be a file!)
 *
 * return 1 on success and 0 on failure
 */
int
so_cs_puts(so, s)
    STORE_S *so;
    char    *s;
{
    int slen = strlen(s);

    if(so->dp + slen >= so->eot){
	register size_t cur_o  = so->dp - (unsigned char *) so->txt;
	register size_t data_o = so->eod - (unsigned char *) so->txt;
	register size_t len   = so->eot - (unsigned char *) so->txt;
	while(len <= cur_o + slen + 1)
	  len += MSIZE_INC;		/* need to resize! */

	fs_resize(&so->txt, len * sizeof(char));
	so->dp	 = (unsigned char *)so->txt + cur_o;
	so->eod	 = (unsigned char *)so->txt + data_o;
	so->eot	 = (unsigned char *)so->txt + len;
	memset(so->eod, 0, so->eot - so->eod);
    }

    memcpy(so->dp, s, slen);
    so->dp += slen;
    if(so->dp > so->eod)
      so->eod = so->dp;

    return(1);
}

int
so_pico_puts(so, s)
    STORE_S *so;
    char    *s;
{
    return(pico_puts(so->txt, s));
}

int
so_file_puts(so, s)
    STORE_S *so;
    char    *s;
{
    int rv = *s ? 0 : 1;

    if(!rv && (so->txt || so_reaquire(so)))
      do
	rv = fwrite(s, strlen(s)*sizeof(char), (size_t)1, (FILE *)so->txt);
      while(!rv && ferror((FILE *)so->txt) && errno == EINTR);

    return(rv);
}



/*
 * Position the storage object's pointer to the given offset
 * from the start of the object's data.
 */
int
so_seek(so, pos, orig)
    STORE_S *so;
    long     pos;
    int      orig;
{
    if(so->src == CharStar){
	switch(orig){
	    case 0 :				/* SEEK_SET */
	      return((pos < so->eod - (unsigned char *) so->txt)
		      ? so->dp = (unsigned char *)so->txt + pos, 0 : -1);
	    case 1 :				/* SEEK_CUR */
	      return((pos > 0)
		       ? ((pos < so->eod - so->dp) ? so->dp += pos, 0: -1)
		       : ((pos < 0)
			   ? ((-pos < so->dp - (unsigned char *)so->txt)
			        ? so->dp += pos, 0 : -1)
			   : 0));
	    case 2 :				/* SEEK_END */
	      return((pos < so->eod - (unsigned char *) so->txt)
		      ? so->dp = so->eod - pos, 0 : -1);
	    default :
	      return(-1);
	}
    }
    else if(so->src == PicoText)
      return(pico_seek(so->txt, pos, orig));
    else			/* FileStar or TmpFileStar */
      return((so->txt || so_reaquire(so)) && fseek((FILE *)so->txt,pos,orig));
}


/*
 * Change the given storage object's size to that specified.  If size
 * is less than the current size, the internal pointer is adjusted and
 * all previous data beyond the given size is lost.
 */
int
so_truncate(so, size)
    STORE_S *so;
    long     size;
{
    if(so->src == CharStar){
	if(so->eod < (unsigned char *) so->txt + size){	/* alloc! */
	    unsigned char *newtxt = (unsigned char *) so->txt;
	    register size_t len   = so->eot - (unsigned char *) so->txt;

	    while(len <= size)
	      len += MSIZE_INC;		/* need to resize! */

	    if(len > so->eot - (unsigned char *) newtxt){
		fs_resize((void **) &newtxt, len * sizeof(char));
		so->eot = newtxt + len;
		so->eod = newtxt + (so->eod - (unsigned char *) so->txt);
		memset(so->eod, 0, so->eot - so->eod);
	    }

	    so->eod = newtxt + size;
	    so->dp  = newtxt + (so->dp - (unsigned char *) so->txt);
	    so->txt = newtxt;
	}
	else if(so->eod > (unsigned char *) so->txt + size){
	    if(so->dp > (so->eod = (unsigned char *)so->txt + size))
	      so->dp = so->eod;

	    memset(so->eod, 0, so->eot - so->eod);
	}
    }
    else if(so->src == PicoText)
      fatal("programmer botch: unsupported so_truncate call");
    else			/* FileStar or TmpFileStar */
      return(ftruncate(fileno((FILE *)so->txt), size));
}


/*
 * so_release - a rather misnamed function.  the idea is to release
 *              what system resources we can (e.g., open files).
 *              while maintaining a reference to it.
 *              it's up to the functions that deal with this object
 *              next to re-aquire those resources.
 */
int
so_release(so)
STORE_S *so;
{
    if(so->txt && so->name && (so->src == FileStar || so->src == TmpFileStar)){
	if(fget_pos((FILE *)so->txt, (fpos_t *)&(so->used)) == 0){
	    fclose((FILE *)so->txt);		/* free the handle! */
	    so->txt = NULL;
	}
    }

    return(1);
}


/*
 * so_reaquire - get any previously released system resources we
 *               may need for the given storage object.
 *       NOTE: at the moment, only FILE * types of objects are
 *             effected, so it only needs to be called before
 *             references to them.
 *                     
 */
so_reaquire(so)
STORE_S *so;
{
    int   rv = 1;
    char  *type = ((so->flags)&WRITE_ACCESS) ? APPEND_MODE : READ_MODE;

    if(!so->txt && (so->src == FileStar || so->src == TmpFileStar)){
	if(!(so->txt=(void *)fopen(so->name, type))){
	    q_status_message2(SM_ORDER,3,5, "ERROR reopening %s : %s", so->name,
				error_description(errno));
	    rv = 0;
	}
	else if(fset_pos((FILE *)so->txt, (fpos_t *)&(so->used))){
	    q_status_message2(SM_ORDER, 3, 5, "ERROR positioning in %s : %s", 
				so->name, error_description(errno));
	    rv = 0;
	}
    }

    return(rv);
}


/*
 * so_text - return a pointer to the text the store object passed
 */
void *
so_text(so)
STORE_S *so;
{
    return((so) ? so->txt : NULL);
}


/*
 * END OF GENERALIZE STORAGE FUNCTIONS
 */


/*
 * Start of filters, pipes and various support functions
 */

/*
 * pointer to first function in a pipe, and pointer to last filter
 */
FILTER_S         *gf_master = NULL;
static	gf_io_t   last_filter;
static	char     *gf_error_string;
static	long	  gf_byte_count;
static	jmp_buf   gf_error_state;


/*
 * A list of states used by the various filters.  Reused in many filters.
 */
#define	DFL	0
#define	EQUAL	1
#define	HEX	2
#define	WSPACE	3
#define	CCR	4
#define	CLF	5
#define	CESCP	6
#define	TOKEN	7
#define	LITERAL	8


/*
 * Macros to reduce function call overhead associated with calling
 * each filter for each byte filtered, and to minimize filter structure
 * dereferences.  NOTE: "queuein" has to do with putting chars into the
 * filter structs data queue.  So, writing at the queuein offset is 
 * what a filter does to pass processed data out of itself.  Ditto for
 * queueout.  This explains the FI --> queueout init stuff below.
 */
#define	GF_QUE_START(F)	(&(F)->queue[0])
#define	GF_QUE_END(F)	(&(F)->queue[GF_MAXBUF - 1])

#define	GF_IP_INIT(F)	ip  = (F) ? &(F)->queue[(F)->queuein] : NULL
#define	GF_EIB_INIT(F)	eib = (F) ? GF_QUE_END(F) : NULL
#define	GF_OP_INIT(F)	op  = (F) ? &(F)->queue[(F)->queueout] : NULL
#define	GF_EOB_INIT(F)	eob = (F) ? &(F)->queue[(F)->queuein] : NULL

#define	GF_IP_END(F)	(F)->queuein  = ip - GF_QUE_START(F)
#define	GF_OP_END(F)	(F)->queueout = op - GF_QUE_START(F)

#define	GF_INIT(FI, FO)	register unsigned char *GF_OP_INIT(FI);	 \
			register unsigned char *GF_EOB_INIT(FI); \
			register unsigned char *GF_IP_INIT(FO);  \
			register unsigned char *GF_EIB_INIT(FO);

#define	GF_CH_RESET(F)	((int)(op = eob = GF_QUE_START(F), \
					    (F)->queueout = (F)->queuein = 0))

#define	GF_END(FI, FO)	(GF_OP_END(FI), GF_IP_END(FO))

#define	GF_FLUSH(F)	((int)(GF_IP_END(F), (*(F)->f)((F), GF_DATA), \
			       GF_IP_INIT(F), GF_EIB_INIT(F)))

#define	GF_PUTC(F, C)	((int)(*ip++ = (C), (ip >= eib) ? GF_FLUSH(F) : 1))

#define	GF_GETC(F, C)	((op < eob) ? ((int)((C) = *op++), 1) : GF_CH_RESET(F))


/*
 * Generalized getc and putc routines.  provided here so they don't
 * need to be re-done elsewhere to 
 */

/*
 * pointers to objects to be used by the generic getc and putc
 * functions
 */
static struct gf_io_struct {
    FILE          *file;
    char          *txtp;
    unsigned long  n;
} gf_in, gf_out;
static STORE_S *gf_so_in, *gf_so_out;


/*
 * setup to use and return a pointer to the generic
 * getc function
 */
void
gf_set_readc(gc, txt, len, src)
    gf_io_t       *gc;
    void          *txt;
    unsigned long  len;
    SourceType     src;
{
    gf_in.n = len;
    if(src == FileStar){
	gf_in.file = (FILE *)txt;
	fseek(gf_in.file, 0L, 0);
	*gc = gf_freadc;
    }
    else{
	gf_in.txtp = (char *)txt;
	*gc = gf_sreadc;
    }
}


/*
 * setup to use and return a pointer to the generic
 * putc function
 */
void
gf_set_writec(pc, txt, len, src)
    gf_io_t       *pc;
    void          *txt;
    unsigned long  len;
    SourceType     src;
{
    gf_out.n = len;
    if(src == FileStar){
	gf_out.file = (FILE *)txt;
	*pc = gf_fwritec;
    }
    else{
	gf_out.txtp = (char *)txt;
	*pc = gf_swritec;
    }
}


/*
 * setup to use and return a pointer to the generic
 * getc function
 */
void
gf_set_so_readc(gc, so)
    gf_io_t *gc;
    STORE_S *so;
{
    gf_so_in = so;
    *gc      = gf_so_readc;
}


/*
 * setup to use and return a pointer to the generic
 * putc function
 */
void
gf_set_so_writec(pc, so)
    gf_io_t *pc;
    STORE_S *so;
{
    gf_so_out = so;
    *pc       = gf_so_writec;
}


/*
 * put the character to the object previously defined
 */
int
gf_so_writec(c)
int c;
{
    return(so_writec(c, gf_so_out));
}


/*
 * get a character from an object previously defined
 */
int
gf_so_readc(c)
unsigned char *c;
{
    return(so_readc(c, gf_so_in));
}


/* get a character from a file */
/* assumes gf_out struct is filled in */
int
gf_freadc(c)
unsigned char *c;
{
    int rv = 0;

    do
      rv = fread(c, sizeof(unsigned char), (size_t)1, gf_in.file);
    while(!rv && ferror(gf_in.file) && errno == EINTR);

    return(rv);
}


/* put a character to a file */
/* assumes gf_out struct is filled in */
int
gf_fwritec(c)
    int c;
{
    unsigned char ch = (unsigned char)c;
    int rv = 0;

    do
      rv = fwrite(&ch, sizeof(unsigned char), (size_t)1, gf_out.file);
    while(!rv && ferror(gf_out.file) && errno == EINTR);

    return(rv);
}


/* get a character from a string, return nonzero if things OK */
/* assumes gf_out struct is filled in */
int
gf_sreadc(c)
unsigned char *c;
{
    return((gf_in.n) ? *c = *(gf_in.txtp)++, gf_in.n-- : 0);
}


/* put a character into a string, return nonzero if things OK */
/* assumes gf_out struct is filled in */
int
gf_swritec(c)
    int c;
{
    return((gf_out.n) ? *(gf_out.txtp)++ = c, gf_out.n-- : 0);
}


/*
 * output the given string with the given function
 */
int
gf_puts(s, pc)
    register char *s;
    gf_io_t        pc;
{
    while(*s != '\0')
      if(!(*pc)((unsigned char)*s++))
	return(0);		/* ERROR putting char ! */

    return(1);
}


/*
 * Start of generalized filter routines
 */

/* 
 * initializing function to make sure list of filters is empty.
 */
void
gf_filter_init()
{
    FILTER_S *flt, *fltn = gf_master;

    while((flt = fltn) != NULL){	/* free list of old filters */
	fltn = flt->next;
	fs_give((void **)&flt);
    }

    gf_master = NULL;
    gf_error_string = NULL;		/* clear previous errors */
    gf_byte_count = 0L;			/* reset counter */
}



/*
 * link the given filter into the filter chain
 */
gf_link_filter(f)
    filter_t f;
{
    FILTER_S *new, *tail;

    new = (FILTER_S *)fs_get(sizeof(FILTER_S));
    memset(new, 0, sizeof(FILTER_S));

    new->f = f;				/* set the function pointer     */
    (*f)(new, GF_RESET);		/* have it setup initial state  */

    if(tail = gf_master){		/* or add it to end of existing  */
	while(tail->next)		/* list  */
	  tail = tail->next;

	tail->next = new;
    }
    else				/* attach new struct to list    */
      gf_master = new;			/* start a new list */
}


/*
 * terminal filter, doesn't call any other filters, typically just does
 * something with the output
 */
void
gf_terminal(f, flg)
    FILTER_S *f;
    int       flg;
{
    if(flg == GF_DATA){
	GF_INIT(f, f);

	while(op < eob)
	  if((*last_filter)(*op++) <= 0) /* generic terminal filter */
	    gf_error(errno ? error_description(errno) : "Error writing pipe");

	GF_CH_RESET(f);
    }
    else if(flg == GF_RESET)
      errno = 0;			/* prepare for problems */
}


/*
 * set some outside gf_io_t function to the terminal function 
 * for example: a function to write a char to a file or into a buffer
 */
void
gf_set_terminal(f)			/* function to set generic filter */
    gf_io_t f;
{
    last_filter = f;
}


/*
 * common function for filter's to make it known that an error
 * has occurred.  Jumps back to gf_pipe with error message.
 */
void
gf_error(s)
    char *s;
{
    /* let the user know the error passed in s */
    gf_error_string = s;
    longjmp(gf_error_state, 1);
}


/*
 * The routine that shoves each byte through the chain of
 * filters.  It sets up error handling, and the terminal function.
 * Then loops getting bytes with the given function, and passing
 * it on to the first filter in the chain.
 */
char *
gf_pipe(gc, pc)
    gf_io_t gc, pc;			/* how to get a character */
{
    unsigned char c;

#ifdef	DOS
    MoveCursor(0, 1);
    StartInverse();
#endif

    dprint(4, (debugfile, "-- gf_pipe: "));

    /*
     * set up for any errors a filter may encounter
     */
    if(setjmp(gf_error_state)){
#ifdef	DOS
	ibmputc(' ');
	EndInverse();
#endif
	dprint(4, (debugfile, "ERROR: %s\n",
		   gf_error_string ? gf_error_string : "NULL"));
	return(gf_error_string); 	/*  */
    }

    /*
     * set and link in the terminal filter
     */
    gf_set_terminal(pc);
    gf_link_filter(gf_terminal);

    /* 
     * while there are chars to process, send them thru the pipe.
     * NOTE: it's necessary to enclose the loop below in a block
     * as the GF_INIT macro calls some automatic var's into
     * existence.  It can't be placed at the start of gf_pipe
     * because its useful for us to be called without filters loaded
     * when we're just being used to copy bytes between storage
     * objects.
     */
    {
	GF_INIT(gf_master, gf_master);

	while((*gc)(&c)){
	    gf_byte_count++;
#ifdef	DOS
	    if(!(gf_byte_count & 0x3ff))
#ifdef	_WINDOWS
	      /* Under windows we yeild to allow event processing.
	       * Progress display is handled throught the alarm()
	       * mechinism.
	       */
	      mswin_yeild ();
#else
	      /* Poor PC still needs spinning bar */
	      ibmputc("/-\\|"[((int) gf_byte_count >> 10) % 4]);
	      MoveCursor(0, 1);
#endif
#endif

	    GF_PUTC(gf_master, c & 0xff);
	}

	/*
	 * toss an end-of-data marker down the pipe to give filters
	 * that have any buffered data the opportunity to dump it
	 */
	GF_FLUSH(gf_master);
	(*gf_master->f)(gf_master, GF_EOD);
    }

#ifdef	DOS
    ibmputc(' ');
    EndInverse();
#endif

    dprint(1, (debugfile, "done.\n"));
    return(NULL);			/* everything went OK */
}


/*
 * return the number of bytes piped so far
 */
long
gf_bytes_piped()
{
    return(gf_byte_count);
}


/*
 * filter the given input with the given command
 *
 *  Args: cmd -- command string to execute
 *	prepend -- string to prepend to filtered input
 *	source_so -- storage object containing data to be filtered
 *	pc -- function to write filtered output with
 *	aux_filters -- additional filters to pass data thru after "cmd"
 *
 *  Returns: NULL on sucess, reason for failure (not alloc'd!) on error
 */
char *
gf_filter(cmd, prepend, source_so, pc, aux_filters)
    char     *cmd, *prepend;
    STORE_S  *source_so;
    gf_io_t   pc;
    filter_t *aux_filters;
{
    unsigned char c;
    int	     flags;
    char   *errstr = NULL, buf[MAILTMPLEN], *rfile = NULL;
    PIPE_S *fpipe;

    gf_filter_init();
    while(aux_filters && *aux_filters)
      gf_link_filter(*aux_filters++);

    gf_set_terminal(pc);
    gf_link_filter(gf_terminal);

    /*
     * Spawn filter feeding it data, and reading what it writes.
     */
    so_seek(source_so, 0L, 0);
#ifdef	NO_PIPE
    /*
     * When there're no pipes for IPC, use an output file to collect
     * the result...
     */
    flags = PIPE_WRITE | PIPE_NOSHELL | PIPE_RESET;
    rfile = temp_nam(NULL, "pf");
#else
    flags = PIPE_WRITE | PIPE_READ | PIPE_NOSHELL | PIPE_RESET;
#endif

    if(fpipe = open_system_pipe(cmd, rfile ? &rfile : NULL, NULL, flags)){
#ifdef	NO_PIPE
	if(prepend && (fputs(prepend, fpipe->out.f) == EOF
		       || fputc('\n', fpipe->out.f) == EOF))
	  errstr = error_description(errno);

	/*
	 * Write the output, and deal with the result later...
	 */
	while(!errstr && so_readc(&c, source_so))
	  if(fputc(c, fpipe->out.f) == EOF)
	    errstr = error_description(errno);
#else
#ifdef	NON_BLOCKING_IO
	int     n;

	if(fcntl(fileno(fpipe->in.f), F_SETFL, NON_BLOCKING_IO) == -1)
	  errstr = "Can't set up non-blocking IO";

	if(prepend && (fputs(prepend, fpipe->out.f) == EOF
		       || fputc('\n', fpipe->out.f) == EOF))
	  errstr = error_description(errno);

	while(!errstr){
	    /* if the pipe can't hold a K we're sunk (too bad PIPE_MAX
	     * isn't ubiquitous ;).
	     */
	    for(n = 0; !errstr && fpipe->out.f && n < 1024; n++)
	      if(!so_readc(&c, source_so)){
		  fclose(fpipe->out.f);
		  fpipe->out.f = NULL;
	      }
	      else if(fputc(c, fpipe->out.f) == EOF)
		errstr = error_description(errno);

	    /*
	     * Note: We clear errno here and test below, before ferror,
	     *	     because *some* stdio implementations consider
	     *	     EAGAIN and EWOULDBLOCK equivalent to EOF...
	     */
	    errno = 0;

	    while(!errstr && fgets(buf, MAILTMPLEN, fpipe->in.f))
	      errstr = gf_filter_puts(buf);

	    /* then fgets failed! */
	    if(!errstr && !(errno == EAGAIN || errno == EWOULDBLOCK)){
		if(feof(fpipe->in.f))		/* nothing else interesting! */
		  break;			
		else if(ferror(fpipe->in.f))	/* bummer. */
		  errstr = error_description(errno);
	    }
	}
#else
	if(prepend && (fputs(prepend, fpipe->out.f) == EOF
		       || fputc('\n', fpipe->out.f) == EOF))
	  errstr = error_description(errno);

	/*
	 * Well, do the best we can, and hope the pipe we're writing
	 * doesn't fill up before we start reading...
	 */
	while(!errstr && so_readc(&c, source_so))
	  if(fputc(c, fpipe->out.f) == EOF)
	    errstr = error_description(errno);

	fclose(fpipe->out.f);
	fpipe->out.f = NULL;
	while(!errstr && fgets(buf, MAILTMPLEN, fpipe->in.f))
	  errstr = gf_filter_puts(buf);
#endif /* NON_BLOCKING */
#endif /* NO_PIPE */

	gf_filter_eod();

	if(close_system_pipe(&fpipe) && !errstr)
	  errstr = "Pipe command returned error.";

#ifdef	NO_PIPE
	/*
	 * retrieve filters result...
	 */
	{
	    FILE *fp;
	    if(fp = fopen(rfile, "rb")){
		while(!errstr && fgets(buf, MAILTMPLEN, fp))
		  errstr = gf_filter_puts(buf);

		fclose(fp);
	    }

	    fs_give((void **)&rfile);
	}
#endif
    }

    return(errstr);
}


/*
 * gf_filter_puts - write the given string down the filter's pipe
 */
char *
gf_filter_puts(s)
    register char *s;
{
    GF_INIT(gf_master, gf_master);

    /*
     * set up for any errors a filter may encounter
     */
    if(setjmp(gf_error_state)){
	dprint(4, (debugfile, "ERROR: gf_filter_puts: %s\n",
		   gf_error_string ? gf_error_string : "NULL"));
	return(gf_error_string);
    }

    while(*s)
      GF_PUTC(gf_master, (*s++) & 0xff);

    GF_END(gf_master, gf_master);
    return(NULL);
}


/*
 * gf_filter_eod - flush pending data filter's input queue and deliver
 *		   the GF_EOD marker.
 */
void
gf_filter_eod()
{
    GF_INIT(gf_master, gf_master);
    GF_FLUSH(gf_master);
    (*gf_master->f)(gf_master, GF_EOD);
}




/*
 * END OF PIPE SUPPORT ROUTINES, BEGINNING OF FILTERS
 *
 * Filters MUST use the specified interface (pointer to filter
 * structure, the unsigned character buffer in that struct, and a
 * cmd flag), and pass each resulting octet to the next filter in the
 * chain.  Only the terminal filter need not call another filter.
 * As a result, filters share a pretty general structure.
 * Typically three main conditionals separate initialization from
 * data from end-of-data command processing.
 * 
 * Lastly, being character-at-a-time, they're a little more complex
 * to write than filters operating on buffers because some state
 * must typically be kept between characters.  However, for a
 * little bit of complexity here, much convenience is gained later
 * as they can be arbitrarily chained together at run time and
 * consume few resources (especially memory or disk) as they work.
 * (NOTE 951005: even less cpu now that data between filters is passed
 *  via a vector.)
 *
 * A few notes about implementing filters:
 *
 *  - A generic filter template looks like:
 *
 *    void
 *    gf_xxx_filter(f, flg)
 *        FILTER_S *f;
 *        int       flg;
 *    {
 *	  GF_INIT(f, f->next);		// def's var's to speed queue drain
 *
 *        if(flg == GF_DATA){
 *	      register unsigned char c;
 *
 *	      while(GF_GETC(f, c)){	// macro taking data off input queue
 *	          // operate on c and pass it on here
 *                GF_PUTC(f->next, c);	// macro writing output queue
 *	      }
 *
 *	      GF_END(f, f->next);	// macro to sync pointers/offsets
 *	      //WARNING: DO NOT RETURN BEFORE ALL INCOMING DATA'S PROCESSED
 *        }
 *        else if(flg == GF_EOD){
 *            // process any buffered data here and pass it on
 *	      GF_FLUSH(f->next);	// flush pending data to next filter
 *            (*f->next->f)(f->next, GF_EOD);
 *        }
 *        else if(flg == GF_RESET){
 *            // initialize any data in the struct here
 *        }
 *    }
 *
 *  - Any free storage allocated during initialization (typically tied
 *    to the "line" pointer in FILTER_S) is the filter's responsibility
 *    to clean up when the GF_EOD command comes through.
 *
 *  - Filter's must pass GF_EOD they receive on to the next
 *    filter in the chain so it has the opportunity to flush
 *    any buffered data.
 *
 *  - All filters expect NVT end-of-lines.  The idea is to prepend
 *    or append either the gf_local_nvtnl or gf_nvtnl_local 
 *    os-dependant filters to the data on the appropriate end of the
 *    pipe for the task at hand.
 *
 *  - NOTE: As of 951004, filters no longer take their input as a single
 *    char argument, but rather get data to operate on via a vector
 *    representing the input queue in the FILTER_S structure.
 *
 */



/*
 * BASE64 TO BINARY encoding and decoding routines below
 */


/*
 * BINARY to BASE64 filter (encoding described in rfc1341)
 */
void
gf_binary_b64(f, flg)
    FILTER_S *f;
    int       flg;
{
    static char *v =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register unsigned char t = f->t;
	register long n = f->n;

	while(GF_GETC(f, c)){

	    switch(n++){
	      case 0 : case 3 : case 6 : case 9 : case 12: case 15: case 18:
	      case 21: case 24: case 27: case 30: case 33: case 36: case 39:
	      case 42: case 45:
		GF_PUTC(f->next, v[c >> 2]);
					/* byte 1: high 6 bits (1) */
		t = c << 4;		/* remember high 2 bits for next */
		break;

	      case 1 : case 4 : case 7 : case 10: case 13: case 16: case 19:
	      case 22: case 25: case 28: case 31: case 34: case 37: case 40:
	      case 43:
		GF_PUTC(f->next, v[(t|(c>>4)) & 0x3f]);
		t = c << 2;
		break;

	      case 2 : case 5 : case 8 : case 11: case 14: case 17: case 20:
	      case 23: case 26: case 29: case 32: case 35: case 38: case 41:
	      case 44:
		GF_PUTC(f->next, v[(t|(c >> 6)) & 0x3f]);
		GF_PUTC(f->next, v[c & 0x3f]);
		break;
	    }

	    if(n == 45){			/* start a new line? */
		GF_PUTC(f->next, '\015');
		GF_PUTC(f->next, '\012');
		n = 0L;
	    }
	}

	f->n = n;
	f->t = t;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){		/* no more data */
	switch (f->n % 3) {		/* handle trailing bytes */
	  case 0:			/* no trailing bytes */
	    break;

	  case 1:
	    GF_PUTC(f->next, v[(f->t) & 0x3f]);
	    GF_PUTC(f->next, '=');	/* byte 3 */
	    GF_PUTC(f->next, '=');	/* byte 4 */
	    break;

	  case 2:
	    GF_PUTC(f->next, v[(f->t) & 0x3f]);
	    GF_PUTC(f->next, '=');	/* byte 4 */
	    break;
	}

	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset binary_b64\n"));
	f->n = 0L;
    }
}



/*
 * BASE64 to BINARY filter (encoding described in rfc1341)
 */
void
gf_b64_binary(f, flg)
    FILTER_S *f;
    int       flg;
{
    static char v[] = {65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
		       65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,65,
		       65,65,65,65,65,65,65,65,65,65,65,62,65,65,65,63,
		       52,53,54,55,56,57,58,59,60,61,62,65,65,64,65,65,
		       65, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
		       15,16,17,18,19,20,21,22,23,24,25,65,65,65,65,65,
		       65,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
		       41,42,43,44,45,46,47,48,49,50,51,65,65,65,65,65};
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register unsigned char t = f->t;
	register int n = (int) f->n;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    if(state){
		state = 0;
		if (c != '=') {
		    gf_error("Illegal '=' in base64 text");
		    /* NO RETURN */
		}
	    }

	    /* in range, and a valid value? */
	    if((c & ~0x7f) || (c = v[c]) > 63){
		if(c == 64){
		    switch (n++) {	/* check quantum position */
		      case 2:
			state++;	/* expect an equal as next char */
			break;

		      case 3:
			n = 0L;		/* restart quantum */
			break;

		      default:		/* impossible quantum position */
			gf_error("Internal base64 decoder error");
			/* NO RETURN */
		    }
		}
	    }
	    else{
		switch (n++) {		/* install based on quantum position */
		  case 0:		/* byte 1: high 6 bits */
		    t = c << 2;
		    break;

		  case 1:		/* byte 1: low 2 bits */
		    GF_PUTC(f->next, (t|(c >> 4)));
		    t = c << 4;		/* byte 2: high 4 bits */
		    break;

		  case 2:		/* byte 2: low 4 bits */
		    GF_PUTC(f->next, (t|(c >> 2)));
		    t = c << 6;		/* byte 3: high 2 bits */
		    break;

		  case 3:
		    GF_PUTC(f->next, t | c);
		    n = 0L;		/* reinitialize mechanism */
		    break;
		}
	    }
	}

	f->f1 = state;
	f->t = t;
	f->n = n;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset b64_binary\n"));
	f->n  = 0L;			/* quantum position */
	f->f1 = 0;			/* state holder: equal seen? */
    }
}




/*
 * QUOTED-PRINTABLE ENCODING AND DECODING filters below.
 * encoding described in rfc1341
 */

#define	GF_MAXLINE	80		/* good buffer size */

/*
 * default action for QUOTED-PRINTABLE to 8BIT decoder
 */
#define	GF_QP_DEFAULT(f, c)	{ \
				    if((c) == ' '){ \
					state = WSPACE; \
						/* reset white space! */ \
					(f)->linep = (f)->line; \
					*((f)->linep)++ = ' '; \
				    } \
				    else if((c) == '='){ \
					state = EQUAL; \
				    } \
				    else \
				      GF_PUTC((f)->next, (c)); \
				}


/*
 * QUOTED-PRINTABLE to 8BIT filter
 */
void
gf_qp_8bit(f, flg)
    FILTER_S *f;
    int       flg;
{
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    switch(state){
	      case DFL :		/* default case */
	      default:
		GF_QP_DEFAULT(f, c);
		break;

	      case CCR    :		/* non-significant space */
		state = DFL;
		if(c == '\012')
		  continue;		/* go on to next char */

		GF_QP_DEFAULT(f, c);
		break;

	      case EQUAL  :
		if(c == '\015'){	/* "=\015" is a soft EOL */
		    state = CCR;
		    break;
		}

		if(c == '='){		/* compatibility clause for old guys */
		    GF_PUTC(f->next, '=');
		    state = DFL;
		    break;
		}

		if(!isxdigit((unsigned char)c)){	/* must be hex! */
		    fs_give((void **)&(f->line));
		    gf_error("Non-hexadecimal character in QP encoding");
		    /* NO RETURN */
		}

		if (isdigit ((unsigned char)c)) 
		  f->t = c - '0';
		else
		  f->t = c - (isupper((unsigned char)c) ? 'A' - 10 : 'a' - 10);

		state = HEX;
		break;

	      case HEX :
		state = DFL;
		if(!isxdigit((unsigned char)c)){	/* must be hex! */
		    fs_give((void **)&(f->line));
		    gf_error("Non-hexadecimal character in QP encoding");
		    /* NO RETURN */
		}

		if (isdigit((unsigned char)c)) 
		  c -= '0';
		else
		  c -= (isupper((unsigned char)c) ? 'A' - 10 : 'a' - 10);

		GF_PUTC(f->next, c + (f->t << 4));
		break;

	      case WSPACE :
		if(c == ' '){		/* toss it in with other spaces */
		    if(f->linep - f->line < GF_MAXLINE)
		      *(f->linep)++ = ' ';
		    break;
		}

		state = DFL;
		if(c == '\015'){	/* not our white space! */
		    f->linep = f->line;	/* reset buffer */
		    GF_PUTC(f->next, '\015');
		    break;
		}

		/* the spaces are ours, write 'em */
		f->n = f->linep - f->line;
		while((f->n)--)
		  GF_PUTC(f->next, ' ');

		GF_QP_DEFAULT(f, c);	/* take care of 'c' in default way */
		break;
	    }
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	fs_give((void **)&(f->line));
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset qp_8bit\n"));
	f->f1 = DFL;
	f->linep = f->line = (char *)fs_get(GF_MAXLINE * sizeof(char));
    }
}



/*
 * USEFUL MACROS TO HELP WITH QP ENCODING
 */

#define	QP_MAXL	75			/* 76th place only for continuation */

/*
 * Macro to test and wrap long quoted printable lines
 */
#define	GF_8BIT_WRAP(f)		{ \
				    GF_PUTC((f)->next, '='); \
				    GF_PUTC((f)->next, '\015'); \
				    GF_PUTC((f)->next, '\012'); \
				}

/*
 * write a quoted octet in QUOTED-PRINTABLE encoding, adding soft
 * line break if needed.
 */
#define	GF_8BIT_PUT_QUOTE(f, c)	{ \
				    if(((f)->n += 3) > QP_MAXL){ \
					GF_8BIT_WRAP(f); \
					(f)->n = 3;	/* set line count */ \
				    } \
				    GF_PUTC((f)->next, '='); \
						/* high order 4 bits */ \
				    GF_PUTC((f)->next, hex[(c) >> 4]); \
						/* low order 4 bits */ \
				    GF_PUTC((f)->next, hex[(c) & 0xf]); \
				}

/*
 * just write an ordinary octet in QUOTED-PRINTABLE, wrapping line
 * if needed.
 */
#define	GF_8BIT_PUT(f, c)	{ \
				     if((++(f->n)) > QP_MAXL){ \
					 GF_8BIT_WRAP(f); \
					 f->n = 1L; \
				     } \
				     if(f->n == 1L && c == '.'){ \
					 GF_8BIT_PUT_QUOTE(f, c); \
					 f->n = 3; \
				     } \
				     else \
				       GF_PUTC(f->next, c); \
				}


/*
 * default action for 8bit to quoted printable encoder
 */
#define	GF_8BIT_DEFAULT(f, c)	if((c) == ' '){ \
				    state = WSPACE; \
				} \
				else if(c == '\015'){ \
				    state = CCR; \
				} \
				else if(iscntrl(c & 0x7f) || (c == 0x7f) \
					|| (c & 0x80) || (c == '=')){ \
				    GF_8BIT_PUT_QUOTE(f, c); \
				} \
				else{ \
				  GF_8BIT_PUT(f, c); \
				}


/*
 * 8BIT to QUOTED-PRINTABLE filter
 */
void
gf_8bit_qp(f, flg)
    FILTER_S *f;
    int       flg;
{
    static char *hex = "0123456789ABCDEF";
    short dummy_dots = 0, dummy_dmap = 1;
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    /* keep track of "^JFrom " */
	    Find_Froms(f->t, dummy_dots, f->f2, dummy_dmap, c);

	    switch(state){
	      case DFL :		/* handle ordinary case */
		GF_8BIT_DEFAULT(f, c);
		break;

	      case CCR :		/* true line break? */
		state = DFL;
		if(c == '\012'){
		    GF_PUTC(f->next, '\015');
		    GF_PUTC(f->next, '\012');
		    f->n = 0L;
		}
		else{			/* nope, quote the CR */
		    GF_8BIT_PUT_QUOTE(f, '\015');
		    GF_8BIT_DEFAULT(f, c); /* and don't forget about c! */
		}
		break;

	      case WSPACE:
		state = DFL;
		if(c == '\015' || f->t){ /* handle the space */
		    GF_8BIT_PUT_QUOTE(f, ' ');
		    f->t = 0;		/* reset From flag */
		}
		else
		  GF_8BIT_PUT(f, ' ');

		GF_8BIT_DEFAULT(f, c);	/* handle 'c' in the default way */
		break;
	    }
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	switch(f->f1){
	  case CCR :
	    GF_8BIT_PUT_QUOTE(f, '\015'); /* write the last cr */
	    break;

	  case WSPACE :
	    GF_8BIT_PUT_QUOTE(f, ' ');	/* write the last space */
	    break;
	}

	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset 8bit_qp\n"));
	f->f1 = DFL;			/* state from last character        */
	f->f2 = 1;			/* state of "^NFrom " bitmap        */
	f->t  = 0;
	f->n  = 0L;			/* number of chars in current line  */
    }
}



/*
 * RICHTEXT-TO-PLAINTEXT filter
 */

/*
 * option to be used by rich2plain (NOTE: if this filter is ever 
 * used more than once in a pipe, all instances will have the same
 * option value)
 */
static int gf_rich_plain = 0;


/*----------------------------------------------------------------------
      richtext to plaintext filter
    
 Args: f -- 
       flg  --

  This basically removes all richtext formatting. A cute hack is used 
  to get bold and underlining to work.
  Further work could be done to handle things like centering and right
  and left flush, but then it could no longer be done in place. This
  operates on text *with* CRLF's.

  WARNING: does not wrap lines!
 ----*/
void
gf_rich2plain(f, flg)
    FILTER_S *f;
    int       flg;
{
/* BUG: qoute incoming \255 values */
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    switch(state){
	      case TOKEN :		/* collect a richtext token */
		if(c == '>'){		/* what should we do with it? */
		    state       = DFL;	/* return to default next time */
		    *(f->linep) = '\0';	/* cap off token */
		    if(f->line[0] == 'l' && f->line[1] == 't'){
			GF_PUTC(f->next, '<'); /* literal '<' */
		    }
		    else if(f->line[0] == 'n' && f->line[1] == 'l'){
			GF_PUTC(f->next, '\015');/* newline! */
			GF_PUTC(f->next, '\012');
		    }
		    else if(!strcmp("comment", f->line)){
			(f->f2)++;
		    }
		    else if(!strcmp("/comment", f->line)){
			f->f2 = 0;
		    }
		    else if(!strcmp("/paragraph", f->line)) {
			GF_PUTC(f->next, '\r');
			GF_PUTC(f->next, '\n');
			GF_PUTC(f->next, '\r');
			GF_PUTC(f->next, '\n');
		    }
		    else if(!gf_rich_plain){
			if(!strcmp(f->line, "bold")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_BOLDON);
			} else if(!strcmp(f->line, "/bold")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_BOLDOFF);
			} else if(!strcmp(f->line, "italic")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEON);
			} else if(!strcmp(f->line, "/italic")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEOFF);
			} else if(!strcmp(f->line, "underline")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEON);
			} else if(!strcmp(f->line, "/underline")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEOFF);
			} 
		    }
		    /* else we just ignore the token! */

		    f->linep = f->line;	/* reset token buffer */
		}
		else{			/* add char to token */
		    if(f->linep - f->line > 40){
			/* What? rfc1341 says 40 char tokens MAX! */
			fs_give((void **)&(f->line));
			gf_error("Richtext token over 40 characters");
			/* NO RETURN */
		    }
		
		    *(f->linep)++ = isupper((unsigned char)c) ? c-'A'+'a' : c;
		}
		break;

	      case CCR   :
		state = DFL;		/* back to default next time */
		if(c == '\012'){	/* treat as single space?    */
		    GF_PUTC(f->next, ' ');
		    break;
		}
		/* fall thru to process c */

	      case DFL   :
	      default:
		if(c == '<')
		  state = TOKEN;
		else if(c == '\015')
		  state = CCR;
		else if(!f->f2)		/* not in comment! */
		  GF_PUTC(f->next, c);

		break;
	    }
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	if(f->f1 = (f->linep != f->line)){
	    /* incomplete token!! */
	    gf_error("Incomplete token in richtext");
	    /* NO RETURN */
	}

	fs_give((void **)&(f->line));
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset rich2plain\n"));
	f->f1 = DFL;			/* state */
	f->f2 = 0;			/* set means we're in a comment */
	f->linep = f->line = (char *)fs_get(45 * sizeof(char));
    }
}


/*
 * function called from the outside to set
 * richtext filter's options
 */
void
gf_rich2plain_opt(plain)
    int plain;
{
    gf_rich_plain = plain;
}



/*
 * ENRICHED-TO-PLAIN text filter
 */

static int gf_enriched_plain = 0;


/*----------------------------------------------------------------------
      enriched text to plain text filter (ala rfc1523)
    
 Args: f -- state and input data
       flg -- 

  This basically removes all enriched formatting. A cute hack is used 
  to get bold and underlining to work.

  Further work could be done to handle things like centering and right
  and left flush, but then it could no longer be done in place. This
  operates on text *with* CRLF's.

  WARNING: does not wrap lines!
 ----*/
void
gf_enriched2plain(f, flg)
    FILTER_S *f;
    int       flg;
{
/* BUG: qoute incoming \255 values */
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    switch(state){
	      case TOKEN :		/* collect a richtext token */
		if(c == '>'){		/* what should we do with it? */
		    state       = DFL;	/* return to default next time */
		    *(f->linep) = '\0';	/* cap off token */
		    if(!strcmp("param", f->line)){
			(f->f2)++;
		    }
		    else if(!strcmp("/param", f->line)){
			f->f2 = 0;
		    }
		    else if(!gf_enriched_plain){
			/* Following is a cute hack or two to get 
			   bold and underline on the screen. 
			   See Putline0n() where these codes are
			   interpreted */
			if(!strcmp(f->line, "bold")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_BOLDON);
			} else if(!strcmp(f->line, "/bold")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_BOLDOFF);
			} else if(!strcmp(f->line, "italic")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEON);
			} else if(!strcmp(f->line, "/italic")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEOFF);
			} else if(!strcmp(f->line, "underline")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEON);
			} else if(!strcmp(f->line, "/underline")) {
			    GF_PUTC(f->next, TAG_EMBED);
			    GF_PUTC(f->next, TAG_ULINEOFF);
			} 
		    }
		    /* else we just ignore the token! */

		    f->linep = f->line;	/* reset token buffer */
		}
		else if(c == '<'){		/* literal '<'? */
		    if(f->linep == f->line){
			GF_PUTC(f->next, '<');
			state = DFL;
		    }
		    else{
			fs_give((void **)&(f->line));
			gf_error("Malformed Enriched text: unexpected '<'");
			/* NO RETURN */
		    }
		}
		else{			/* add char to token */
		    if(f->linep - f->line > 60){ /* rfc1523 says 60 MAX! */
			fs_give((void **)&(f->line));
			gf_error("Malformed Enriched text: token too long");
			/* NO RETURN */
		    }
		
		    *(f->linep)++ = isupper((unsigned char)c) ? c-'A'+'a' : c;
		}
		break;

	      case CCR   :
		if(c != '\012'){	/* treat as single space?    */
		    state = DFL;	/* lone cr? */
		    f->f2 = 0;
		    GF_PUTC(f->next, '\015');
		    goto df;
		}

		state = CLF;
		break;

	      case CLF   :
		if(c == '\015'){	/* treat as single space?    */
		    state = CCR;	/* repeat crlf's mean real newlines */
		    f->f2 = 1;
		    GF_PUTC(f->next, '\r');
		    GF_PUTC(f->next, '\n');
		    break;
		}
		else{
		    state = DFL;
		    if(!f->f2)
		      GF_PUTC(f->next, ' ');

		    f->f2 = 0;
		}

		/* fall thru to take care of 'c' */

	      case DFL   :
	      default :
	      df : 
		if(c == '<')
		  state = TOKEN;
		else if(c == '\015')
		  state = CCR;
		else if(!f->f2)		/* not in param! */
		  GF_PUTC(f->next, c);

		break;
	    }
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	if(f->f1 = (f->linep != f->line)){
	    /* incomplete token!! */
	    gf_error("Incomplete token in richtext");
	    /* NO RETURN */
	}

	fs_give((void **)&(f->line));

	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset enriched2plain\n"));
	f->f1 = DFL;			/* state */
	f->f2 = 0;			/* set means we're in a comment */
	f->linep = f->line = (char *)fs_get(65 * sizeof(char));
    }
}


/*
 * function called from the outside to set
 * richtext filter's options
 */
void
gf_enriched2plain_opt(plain)
    int plain;
{
    gf_enriched_plain = plain;
}



/*
 * HTML-TO-PLAIN text filter
 */

/*----------------------------------------------------------------------
  HTML text to plain text filter (ala HTML 2.0)
    

  This basically tries to do the best it can with HTML 2.0, level 1
  text formatting.

 ----*/
void
gf_html2plain(f, flg)
    FILTER_S *f;
    int       flg;
{
    /* place holder for filter under development */
}


/*
 * ESCAPE CODE FILTER - remove unknown and possibly dangerous escape codes
 * from the text stream.
 */

#define	MAX_ESC_LEN	5

/*
 * the simple filter, removes unknown escape codes from the stream
 */
void
gf_escape_filter(f, flg)
    FILTER_S *f;
    int       flg;
{
    register char *p;
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    if(state){
		if(c == '\033' || f->n == MAX_ESC_LEN){
		    f->line[f->n] = '\0';
		    f->n = 0L;
		    if(!match_escapes(f->line)){
			GF_PUTC(f->next, '^');
			GF_PUTC(f->next, '[');
		    }
		    else
		      GF_PUTC(f->next, '\033');

		    p = f->line;
		    while(*p)
		      GF_PUTC(f->next, *p++);

		    if(c == '\033')
		      continue;
		    else
		      state = 0;			/* fall thru */
		}
		else{
		    f->line[f->n++] = c;		/* collect */
		    continue;
		}
	    }

	    if(c == '\033')
	      state = 1;
	    else
	      GF_PUTC(f->next, c);
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	if(f->f1){
	    if(!match_escapes(f->line)){
		GF_PUTC(f->next, '^');
		GF_PUTC(f->next, '[');
	    }
	    else
	      GF_PUTC(f->next, '\033');
	}

	for(p = f->line; f->n; f->n--, p++)
	  GF_PUTC(f->next, *p);

	fs_give((void **)&(f->line));	/* free temp line buffer */
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset escape\n"));
	f->f1    = 0;
	f->n     = 0L;
	f->linep = f->line = (char *)fs_get((MAX_ESC_LEN + 1) * sizeof(char));
    }
}



/*
 * CONTROL CHARACTER FILTER - transmogrify control characters into their
 * corresponding string representations (you know, ^blah and such)...
 */

/*
 * the simple filter transforms unknown control characters in the stream
 * into harmless strings.
 */
void
gf_control_filter(f, flg)
    FILTER_S *f;
    int       flg;
{
    register char *p;
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;

	while(GF_GETC(f, c)){

	    if(iscntrl(c & 0x7f)
	       && !(isspace((unsigned char)c)
		    || c == '\016' || c == '\017' || c == '\033')){
		GF_PUTC(f->next, '^');
		GF_PUTC(f->next, c + '@');
	    }
	    else
	      GF_PUTC(f->next, c);
	}

	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
}



/*
 * LINEWRAP FILTER - insert CRLF's at end of nearest whitespace before
 * specified line width
 */

/*
 * option to be used by gf_wrap_filter (NOTE: if this filter is ever 
 * used more than once in a pipe, all instances will have the same
 * option value)
 */
static int gf_wrap_width = 75;


/*
 * the simple filter, breaks lines at end of white space nearest
 * to global "gf_wrap_width" in length
 */
void
gf_wrap(f, flg)
    FILTER_S *f;
    int       flg;
{
    register long i;
    register char *breakp, *tp;
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    switch(state){
	      case CCR:
		state = DFL;
		if(c == '\012'){
		    for(tp = f->line; tp < f->linep; tp++)
		      GF_PUTC(f->next, *tp);

		    GF_PUTC(f->next, '\015');
		    GF_PUTC(f->next, '\012');
		    f->n = 0L;
		    f->linep = f->line;
		    break;
		}
		else{
		    *(f->linep)++ = '\015';	/* shouldn't happen often! */
		    (f->n)++;
		}
		/* else fall thru to take care of c */

	      case DFL:
		if(c == '\015'){		/* already has newline? */
		    state = CCR;
		    break;
		}
		else if(c == '\011'){	/* account for tabs too! */
		    i = f->n;
		    while((++i)&0x07)
		      ;

		    i -= f->n;
		}
		else
		  i = 1;

		if(f->n + i > (long)gf_wrap_width){ /* wrap? */
		    for(breakp = &f->linep[-1]; breakp >= f->line; breakp--)
		      if(isspace((unsigned char)*breakp))
			break;

		    f->n = i = 0;
		    for(tp = f->line; tp < f->linep; tp++){
			if(breakp < f->line || tp <= breakp)
		      GF_PUTC(f->next, *tp); /* write the line */
			else{		/* shift it back */
			    i = tp - breakp - 1;
			    if((f->line[i++] = *tp) == '\011')
			      while((++(f->n))&0x07);
			    else
			      (f->n)++;
			}
		    }

		    GF_PUTC(f->next, '\015');
		    GF_PUTC(f->next, '\012');
		    f->linep = &f->line[i];	/* reset f->linep */
		}

		if((*(f->linep)++ = c) == '\011')
		  while((++(f->n))&0x07);
		else
		  (f->n)++;

		break;
	    }
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	for(i = 0; i < f->n; i++)	/* flush the remaining line */
	  GF_PUTC(f->next, f->line[i]);

	fs_give((void **)&(f->line));	/* free temp line buffer */
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset wrap\n"));
	f->f1    = DFL;
	f->n     = 0L;
	f->linep = f->line = (char *)fs_get((gf_wrap_width+10)*sizeof(char));
    }
}


/*
 * function called from the outside to set
 * wrap filter's width option
 */
void
gf_wrap_filter_opt(width)
    int width;
{
    gf_wrap_width = width;
}

/*
 * LINE PREFIX FILTER - insert given text at beginning of each
 * line
 */

/*
 * option to be used by gf_prefix
 */
static char *gf_prefix_prefix = NULL;

#define	GF_PREFIX_WRITE(s)	{ \
				    register char *p; \
				    if(p = (s)) \
				      while(*p) \
					GF_PUTC(f->next, *p++); \
				}


/*
 * the simple filter, prepends each line with the requested prefix.
 * if prefix is null, does nothing, and as with all filters, assumes
 * NVT end of lines.
 */
void
gf_prefix(f, flg)
    FILTER_S *f;
    int       flg;
{
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;
	register int first = f->f2;

	while(GF_GETC(f, c)){

	    if(first){			/* write initial prefix!! */
		first = 0;		/* but just once */
		GF_PREFIX_WRITE((char *) f->data);
	    }
	    else if(state){
		state = 0;
		GF_PUTC(f->next, '\015');
		if(c == '\012'){
		    GF_PUTC(f->next, '\012');
		    GF_PREFIX_WRITE((char *) f->data);
		    continue;
		}
		/* else fall thru to handle 'c' */
	    }

	    if(c == '\015')		/* already has newline? */
	      state = 1;
	    else
	      GF_PUTC(f->next, c);
	}

	f->f1 = state;
	f->f2 = first;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset prefix\n"));
	f->f1   = 0;
	f->f2   = 1;			/* nothing written yet */
	f->data = gf_prefix_prefix;
    }
}


/*
 * function called from the outside to set
 * prefix filter's prefix string
 */
void
gf_prefix_opt(prefix)
    char *prefix;
{
    gf_prefix_prefix = prefix;
}


/*
 * LINE TEST FILTER - accumulate lines and offer each to the provided
 * test function.
 */


/*
 * option to be used by gf_line_test
 */
static int (*gf_line_test_f) PROTO((long, char *));

/* accumulator growth increment */
#define	LINE_TEST_BLOCK	1024

#define	GF_LINE_TEST_EOB(f)	((f)->line + ((f)->f2 - 1))

#define	GF_LINE_TEST_FLUSH(f)	{ \
				    register char *op; \
				    for(op = (f)->line; op < p; op++) \
				      GF_PUTC((f)->next, *op); \
				    p = (f)->line; \
				}


#define	GF_LINE_TEST_ADD(f, c)	{ \
				    if(p >= eobuf){ \
					f->f2 += LINE_TEST_BLOCK; \
					fs_resize((void **)&f->line, \
					      (size_t) f->f2 * sizeof(char)); \
					eobuf = GF_LINE_TEST_EOB(f); \
					p = eobuf - LINE_TEST_BLOCK; \
				    } \
				    *p++ = c; \
				}


/*
 * this simple filter accumulates characters until a newline, offers it
 * to the provided test function, and then passes it on.  It assumes
 * NVT EOLs.
 */
void
gf_line_test(f, flg)
    FILTER_S *f;
    int	      flg;
{
    register char *p = f->linep;
    register char *eobuf = GF_LINE_TEST_EOB(f);
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

	    if(state){
		state = 0;
		if(c == '\012'){
		    int done;

		    *p = '\0';
		    done = (*((int (*)()) f->data))(f->n++, f->line);
		    GF_LINE_TEST_FLUSH(f);
		    GF_PUTC(f->next, '\015');
		    GF_PUTC(f->next, '\012');
		    /*
		     * if the line tester returns TRUE, it's
		     * telling us its seen enough and doesn't
		     * want to see any more.  Remove ourself 
		     * from the pipeline...
		     */
		    if(done){
			if(gf_master == f){
			    gf_master = f->next;
			}
			else{
			    FILTER_S *fprev;

			    for(fprev = gf_master;
				fprev && fprev->next != f;
				fprev = fprev->next)
			      ;

			    if(fprev)		/* wha??? */
			      fprev->next = f->next;
			    else
			      continue;
			}

			while(GF_GETC(f, c))	/* pass input */
			  GF_PUTC(f->next, c);

			GF_FLUSH(f->next);	/* and drain queue */
			fs_give((void **)&f->line);
			fs_give((void **)&f);	/* wax our data */
			return;
		    }
		    else
		      continue;
		}
		else			/* add CR to buffer */
		  GF_LINE_TEST_ADD(f, '\015');
	    } /* fall thru to handle 'c' */

	    if(c == '\015')		/* newline? */
	      state = 1;
	    else
	      GF_LINE_TEST_ADD(f, c);
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_LINE_TEST_FLUSH(f);		/* BUG: should pass to test func? */
	fs_give((void **)&f->line);
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset line_test\n"));
	f->f1 = 0;			/* state */
	f->n  = 0L;			/* line number */
	f->f2 = LINE_TEST_BLOCK;	/* size of alloc'd line */
	f->line = p = (char *) fs_get(f->f2 * sizeof(char));
	f->data = (void *)gf_line_test_f;
    }

    f->linep = p;
}


/*
 * function called from the outside to operate on accumulated line.
 */
void
gf_line_test_opt(test_f)
    int  (*test_f) PROTO((long, char *));
{
    gf_line_test_f = test_f;
}


/*
 * Network virtual terminal to local newline convention filter
 */
void
gf_nvtnl_local(f, flg)
    FILTER_S *f;
    int       flg;
{
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;
	register int state = f->f1;

	while(GF_GETC(f, c)){

#ifdef	CRLF_NEWLINES
	    /*
	     * NOOP!
	     */
	    GF_PUTC(f->next, c);
#else
	    if(state){
		state = 0;
		if(c == '\012'){
		    GF_PUTC(f->next, '\012');
		    continue;
		}
		else
		  GF_PUTC(f->next, '\015');
		/* fall thru to deal with 'c' */
	    }

	    if(c == '\015')
	      state = 1;
	    else
	      GF_PUTC(f->next, c);
#endif
	}

	f->f1 = state;
	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset nvtnl_local\n"));
	f->f1 = 0;
    }
}


/*
 * local to network newline convention filter
 */
void
gf_local_nvtnl(f, flg)
    FILTER_S *f;
    int       flg;
{
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;

	while(GF_GETC(f, c)){

#ifdef	CRLF_NEWLINES
	    /*
	     * NOOP!
	     */
	    GF_PUTC(f->next, c);
#else
	    if(c == '\012'){
		GF_PUTC(f->next, '\015');
		GF_PUTC(f->next, '\012');
	    }
	    else
	      GF_PUTC(f->next, c);
#endif
	}

	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(GF_RESET){
	dprint(9, (debugfile, "-- gf_reset local_nvtnl\n"));
	/* no op */
    }

}

#if defined(DOS) || defined(OS2)
/*
 * DOS CodePage to Character Set Translation (and back) filters
 */

/*
 * Charset and CodePage mapping table pointer and length
 */
static unsigned char *gf_xlate_tab;
static unsigned gf_xlate_tab_len;

/*
 * the simple filter takes DOS Code Page values and maps them into
 * the indicated external CharSet mapping or vice-versa.
 */
void
gf_translate(f, flg)
    FILTER_S *f;
    int       flg;
{
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;

	while(GF_GETC(f, c))
	  if((unsigned) c < gf_xlate_tab_len)
	    GF_PUTC(f->next, (int)gf_xlate_tab[c]);

	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(GF_RESET){
	dprint(9, (debugfile, "-- gf_reset translate\n"));
	/* no op */
    }
}


/*
 * function called from the outside to set
 * prefix filter's prefix string
 */
void
gf_translate_opt(xlatetab, xlatetablen)
    unsigned char *xlatetab;
    unsigned       xlatetablen;
{
    gf_xlate_tab     = xlatetab;
    gf_xlate_tab_len = xlatetablen;
}
#endif

/*
 * display something indicating we're chewing on something
 *
 * NOTE : IF ANY OTHER FILTERS WRITE THE DISPLAY, THIS WILL NEED FIXING
 */
void
gf_busy(f, flg)
    FILTER_S *f;
    int       flg;
{
    static short x = 0;
    GF_INIT(f, f->next);

    if(flg == GF_DATA){
	register unsigned char c;

	while(GF_GETC(f, c)){

	    if(!((++(f->f1))&0x7ff)){ 	/* ding the bell every 2K chars */
		MoveCursor(0, 1);
		f->f1 = 0;
		if((++x)&0x04) x = 0;
		Writechar((x == 0) ? '/' : 	/* CHEATING! */
			  (x == 1) ? '-' : 
			  (x == 2) ? '\\' : '|', 0);
	    }

	    GF_PUTC(f->next, c);
	}

	GF_END(f, f->next);
    }
    else if(flg == GF_EOD){
	MoveCursor(0, 1);
	Writechar(' ', 0);
	EndInverse();
	GF_FLUSH(f->next);
	(*f->next->f)(f->next, GF_EOD);
    }
    else if(flg == GF_RESET){
	dprint(9, (debugfile, "-- gf_reset busy\n"));
	f->f1 = 0;
        x = 0;
	StartInverse();
    }

    fflush(stdout);
}
