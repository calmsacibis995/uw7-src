/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#pragma ident	"@(#)olmisc:textbuff.c	1.34"
#endif

/*
 * textbuff.c
 *
 */

/*#define DEBUG/**/

#include <stdio.h>
#include <string.h>
#include <X11/Xos.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/regexp.h>
#include <X11/memutil.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>

#ifdef I18N
#include <ctype.h>

#if defined(SVR4_0) || defined(SVR4)    /* do we care SVR3?? */

#include <widec.h>
#include <wctype.h>	/* For post-6.0 ccs, include <wchar.h> instead */

#else /* defined(SVR4_0) || defined(SVR4) */

        /* Let Xlib.h take care this if XlibSpecificationRelease
         * is defined (note that, this symbol is introduced in
         * X11R5), otherwise do similar check as in X11R5:Xlib.h
         */
#if !defined(XlibSpecificationRelease)

#if defined(X_WCHAR) || defined(sun)
#include <stddef.h>
#endif

#endif /* !defined(XlibSpecificationRelease) */
#endif /* defined(SVR4_0) || defined(SVR4) */

#endif /* I18N */

#ifdef __STDC__
#include <limits.h>
#else

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 5
#endif

#endif /* __STDC__ */


#ifndef SEEK_SET
#define SEEK_SET 0
#endif


#ifdef I18N

#ifdef __STDC__
#define IS_A_WORD_CHAR(c) (c != L'\0' && (*IsAWordChar)(c))
#define IS_NOT_A_WORD_CHAR(c) (c == L'\0' || !(*IsAWordChar)(c))
#else
#define IS_A_WORD_CHAR(c) (c != (wchar_t)'\0' && (*IsAWordChar)(c))
#define IS_NOT_A_WORD_CHAR(c) (c == (wchar_t)'\0' || !(*IsAWordChar)(c))
#endif

#else
#define IS_A_WORD_CHAR(c) (c != '\0' && (*IsAWordChar)(c))
#define IS_NOT_A_WORD_CHAR(c) (c == '\0' || !(*IsAWordChar)(c))
#endif

#define AppendLineToTextBuffer(text, b)                        \
   InsertLineIntoTextBuffer(text, (text-> lines.used), b)

extern FILE	    *tmpfile();
static TextPage     NewPage();
static TextPage     SplitPage();
static void         SwapIn();
static void         SwapOut();
static TextLine     FirstLineOfPage();
static void         AllocateBlock();
static void         FreeBlock();
static void         InitTextUndoList();
static void         FreeTextUndoList();

static EditResult   InsertBlockIntoTextBuffer();
static EditResult   DeleteBlockInTextBuffer();
static EditResult   DeleteLineInTextBuffer();
static int          is_a_word_char();
#ifdef DEBUG
extern void         _Print_wc_String();
#endif

static TextPosition textblockavailable = 0;

static char * (*Strexp)()  = strexp;
static char * (*Strrexp)() = strrexp;
static int    (*IsAWordChar)() = is_a_word_char;

static BufferElement blank = (BufferElement) '\0';
/*
 * printbuffer
 *
 */

static void printbuffer(text)
TextBuffer * text;
{
register int i;
register int j;

(void) fprintf(stderr, " page  qpos  dpos lines   used blocks...\n");
for (i = 0; i < text-> pages.used; i++)
   {
   (void) fprintf(stderr, "%c[K%5d %5d %5d %5d %5d",
   0x1b /* esc */, i,
   text-> pages.p[i].qpos,
   text-> pages.p[i].dpos,
   text-> pages.p[i].lines,
   text-> pages.p[i].chars);
   for (j = 0; text-> pages.p[i].dpos != NULL &&
               j < text-> pages.p[i].dpos-> used; j++)
   (void) fprintf(stderr, "\t%5d", text-> pages.p[i].dpos-> p[j]);
   (void) fprintf(stderr, "\n");
   }

(void) fprintf(stderr, " qpos  page  time\n");
for (i = 0; i < text-> pagecount; i++)
   (void) fprintf(stderr,"%5d %5d %5d\n",
   i,
   text-> pqueue[i].pageindex,
   text-> pqueue[i].timestamp);

(void) fprintf(stderr, " line  page   len   slen size\n");
for (i = 0; i < text-> lines.used; i++)
   {
   (void) fprintf(stderr,"%5d %5d %5d %5d %5d\n", 
                  i, 
                  text-> lines.p[i].pageindex,
                  text-> lines.p[i].buffer-> used,
#ifdef I18N
                  _StringLength(text-> lines.p[i].buffer-> p),
#else
                  strlen(text-> lines.p[i].buffer-> p),
#endif
                  text-> lines.p[i].buffer-> size);
   }

} /* end of printbuffer */

/*
 * AllocateTextBuffer
 *
 * The \fIAllocateTextBuffer\fR function is used to allocate a new TextBuffer.
 * After it allocates the structure itself, initializes
 * the members of the structure, allocating storage, setting
 * initial values, etc.
 * The routine also registers the update function provided by the caller.
 * This function normally need not be called by an application developer 
 * since the \fIReadFileIntoTextBuffer\fR and
 * \fIReadStringIntoTextBuffer\fR functions call this routine
 * before starting their operation.
 * The routine returns a pointer to the allocated TextBuffer.
 *
 * The \fIFreeTextBuffer\fR function should be used to deallocate
 * the storage allocated by this routine.
 *
 * See also:
 *
 * FreeTextBuffer(3), ReadFileIntoTextBuffer(3), ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer * AllocateTextBuffer(filename, f, d)
char * filename;
TextUpdateFunction f;
caddr_t d;
{
TextBuffer * text = (TextBuffer *) MALLOC(sizeof(TextBuffer));

text-> filename = filename == NULL ?
       NULL : strcpy(MALLOC((unsigned)(strlen(filename) + 1)), filename);
text-> tempfile      = NULL;
text-> blockcnt      = (TextBlock)0;
text-> lines.used    = (TextLine)0;
text-> lines.size    = LTALLOC;
text-> lines.esize   = sizeof(Line);
text-> lines.p       = (Line *) MALLOC((unsigned)(sizeof(Line) * LTALLOC));
text-> pages.used    = (TextPage)0;
text-> pages.size    = PTALLOC;
text-> pages.esize   = sizeof(Page);
text-> pages.p       = (Page *) MALLOC((unsigned)(sizeof(Page) * PTALLOC));
text-> free_list     = (BlockTable *)AllocateBuffer(sizeof(TextBlock), FTALLOC);
text-> pagecount     = (TextPage)0;
text-> pageref       = (TextPage)0;
text-> curpageno     = NewPage(text, -1);
text-> buffer        = NULL; 
                       /* AllocateBuffer(sizeof(text->buffer->p[0]), LNMIN); */
text-> dirty         = 0;
text-> status        = NOTOPEN;
text-> update        = (TextUpdateCallback *)NULL;
text-> refcount      = 0;
RegisterTextBufferUpdate(text, f, d);
InitTextUndoList(text);

return (text);

} /* end of AllocateTextBuffer */
/*
 * FreeTextBuffer
 *
 * The \fIFreeTextBuffer\fR procedure is used to deallocate storage
 * associated with a given TextBuffer.  Note: the storage is not
 * actually freed if the TextBuffer is still associated with other
 * update function/data pairs.
 *
 * See also:
 *
 * AllocateTextBuffer(3), RegisterTextBufferUpdate(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern void FreeTextBuffer(text, f, d)
TextBuffer * text;
TextUpdateFunction f;
caddr_t d;
{
register TextPage i;
register TextLine j;
register TextPage pageindex;

(void) UnregisterTextBufferUpdate(text, f, d);

if (text-> refcount == 0)
   {
   for (i = 0; i < text-> lines.used; i++)
      {
      if (text-> lines.p[i].buffer-> p != NULL)
         FREE(text-> lines.p[i].buffer-> p);
      FREE(text-> lines.p[i].buffer);
      }

   for (i = 0; i < text-> pages.used; i++)
      if (text-> pages.p[i].dpos != NULL)
         FreeBuffer((Buffer *)text-> pages.p[i].dpos);

   FREE(text-> lines.p);
   FREE(text-> pages.p);
   if (text-> free_list);
      FreeBuffer((Buffer *)text-> free_list);

   if (text-> tempfile != NULL)
      (void) fclose(text-> tempfile);
   if (text-> filename != NULL)
      FREE(text-> filename);

   if (text-> insert.string != NULL)
      FREE(text-> insert.string);
   if (text-> deleted.string != NULL)
      FREE(text-> deleted.string);

   FREE(text);
   }

} /* FreeTextBuffer */
/*
 * ReadStringIntoTextBuffer
 *
 * The \fIReadStringIntoTextBuffer\fR function is used to copy the
 * given \fIstring\fR into a newly allocated TextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this TextBuffer.
 *
 * See also:
 *
 * ReadFileIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer * ReadStringIntoTextBuffer(string, f, d)
char * string;
TextUpdateFunction f;
caddr_t d;
{
Buffer * sp;
Buffer * work;
TextBuffer * text = AllocateTextBuffer(NULL, f, d);

#ifdef DEBUG
printf("ReadStringIntoTextBuffer: input string=%s\n",string);
#endif
sp = stropen(string);
if (sp != NULL)
   {
   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

   while(ReadStringIntoBuffer(sp, work) != EOF)
   {  
#ifdef DEBUG
      printf("Next portion of string=");
      _Print_wc_String(work->p);
#endif
      (void) AppendLineToTextBuffer(text, work);
   }
#ifdef DEBUG
   printf("Next portion of string=");
   _Print_wc_String(work->p);
#endif
   (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   strclose(sp);
   (void) wcGetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

#ifdef DEBUG
printf("ReadStringIntoTextBuffer [LEAVE]\n");
#endif
return (text);

} /* end of ReadStringIntoTextBuffer */
/*
 * ReadFileIntoTextBuffer
 *
 * The \fIReadFileIntoTextBuffer\fR function is used to read the
 * given \fIfile\fR into a newly allocated TextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this TextBuffer.
 *
 * See also:
 *
 * ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer * ReadFileIntoTextBuffer(filename, f, d)
char * filename;
TextUpdateFunction f;
caddr_t d;
{
FILE * fp;
TextBuffer * text = NULL;
TextFileStatus status = READWRITE;
Buffer * work;

status = READWRITE;
if ((fp = fopen(filename, "rw")) == NULL)
   {
   status = READONLY;
   if ((fp = fopen(filename, "r")) == NULL)
      status = NEWFILE;
   }

if (fp != NULL)
   {
   text = AllocateTextBuffer(filename, f, d);
   text-> status = status;

   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

#ifdef NOT_DEFINED
   while(ReadFileIntoBuffer(fp, work) != EOF)
      (void) AppendLineToTextBuffer(text, work);
#else
   {
   int i = 0;
   while(ReadFileIntoBuffer(fp, work) != EOF) {
      ++i;
      if (i%100 == 0) {
#ifdef VERBOSE
         fprintf(stderr,
            "at i = %d, pagecount = %d, curpageno = %d, pagesused = %d\n",
            i, text-> pagecount, text-> curpageno, text-> pages.used);
#endif /* VERBOSE */
      }
      (void) AppendLineToTextBuffer(text, work);
   }
   }
#endif
   if (text-> lines.used == 0 && work-> used == 0)
      {
      work-> used = 1;
      work-> p[0] = '\0';
      (void) AppendLineToTextBuffer(text, work);
      }
   else
      if (work-> used != 0)
         (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   (void) fclose(fp);
   (void) wcGetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

return (text);

} /* end of ReadFileIntoTextBuffer */


/*
 * ReadPipeIntoTextBuffer
 *
 * The \fIReadPipeIntoTextBuffer\fR function is used to read the
 * given \fIpipe\fR into a newly allocated TextBuffer.  The
 * supplied TextUpdateFunction and data pointer are associated
 * with this TextBuffer.
 *
 * See also:
 *
 * ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer * ReadPipeIntoTextBuffer(fp, f, d)
FILE * fp;
TextUpdateFunction f;
caddr_t d;
{
TextBuffer * text = NULL;
TextFileStatus status = READONLY;
Buffer * work;

if (fp != NULL)
   {
   text = AllocateTextBuffer(NULL, f, d);
   text-> status = status;

   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

   while(ReadFileIntoBuffer(fp, work) != EOF) 
      (void) AppendLineToTextBuffer(text, work);

   if (text-> lines.used == 0 && work-> used == 0)
      {
      work-> used = 1;
      work-> p[0] = '\0';
      (void) AppendLineToTextBuffer(text, work);
      }
   else
      if (work-> used != 0)
         (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   (void) fclose(fp);
   (void) wcGetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

return (text);

} /* end of ReadPipeIntoTextBuffer */

/*
 * GetTextBufferLocation
 *
 * The \fIGetTextBufferLocation\fR function is used to retrieve
 * the contents of the given line within the TextBuffer.  It
 * returns a pointer to the character string.  If the line
 * number is invalid a NULL pointer is returned.  If a non-NULL TextLocation
 * pointer is supplied in the argument list the contents of this
 * structure are modified to reflect the values corresponding to
 * the given line.
 *
 * See also:
 *
 * GetTextBufferBlock(3), wcGetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char * GetTextBufferLocation(text, line_number, location)
TextBuffer * text;
TextLine line_number;
TextLocation * location;
{
static char * mb_copy;
static int    mb_size;
int           size;

if (line_number < 0 || line_number >= text-> lines.used)
   return (NULL);

if (text-> lines.p[line_number].pageindex != text-> curpageno)
   SwapIn(text, text-> lines.p[line_number].pageindex);

text-> buffer = text-> lines.p[line_number].buffer;

if (location)
   {
   location-> line   = line_number;
   location-> offset = 0;
   location-> buffer = text-> buffer-> p;
   }

#ifdef DEBUG
printf("GetTextBufferLocation:\n");
printf("\tline_number:%d",line_number);
printf("\tstring:");
_Print_wc_String(text-> buffer-> p);
#endif

#ifdef I18N
   /* create a multibyte copy of the buffer for caller */
size = (1 + _StringLength(text-> buffer-> p)) * sizeof(BufferElement); 
if (mb_copy == NULL){
   mb_size = size;
   mb_copy = MALLOC(mb_size);
}
else
   if (mb_size < size){
      mb_size = size;
      mb_copy = REALLOC(mb_copy, mb_size);
   }
(void) wcstombs(mb_copy, text-> buffer-> p, mb_size);
return (mb_copy);

#else
return(text-> buffer-> p);
#endif

} /* GetTextBufferLocation */
/*
 * ForwardScanTextBuffer
 *
 * The \fIForwardScanTextBuffer\fR function is used to scan,
 * towards the end of the buffer,
 * for a given \fIexp\fRression in the TextBuffer starting
 * at \fIlocation\fR.  A \fIScanResult\fR is returned which
 * indicates
 *
 * .so CWstart
 *   SCAN_NOTFOUND  The scan wrapped without finding a match.
 *   SCAN_WRAPPED   A match was found at a location \fIbefore\fP the start location.
 *   SCAN_FOUND     A match was found at a location \fIafter\fP the start location.
 *   SCAN_INVALID   Either the location or the expression was invalid.
 * .so CWend
 *
 * See also:
 *
 * BackwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern ScanResult ForwardScanTextBuffer(text, exp, location)
TextBuffer * text;
char * exp;
TextLocation * location;
{
char * c;
TextLine s = location-> line;
ScanResult retval = SCAN_INVALID;
char *block;
int byte_indx = 0;

if (exp != NULL)
   {
   char *ptr;
   int i, k, num_bytes = 0;

   retval = SCAN_FOUND;
   block = GetTextBufferLocation(text, s, (TextLocation *)NULL);

   /* Be careful - start at location->offset + 1 - THIS IS A CHARACTER
    * OFFSET, NOT A BYTE OFFSET.  So, count through the correct number
    * of bytes in this returned m.b. string to get to the (location->
    * offset + 1)st character.
    * For Strexp:
    * Arg 1: a char. ptr, use block (the whole string);
    * Arg 2: a char ptr - from this char. start looking for the pattern
    */

   ptr = block;
   byte_indx = (int)location->offset + 1;
   for (i=0; i< byte_indx ;i++,ptr+= k) {
	num_bytes += k = mblen(ptr, MB_LEN_MAX);
   }
   
#if !defined(I18N)
   if ((c = Strexp(block,
                  &block[location-> offset + 1], exp)) == NULL)
#else
   if ((c = Strexp(block,
                  &block[num_bytes], exp)) == NULL)
#endif
      {
      for (s = location-> line + 1; s < text-> lines.used; s++)
         {
         block = GetTextBufferLocation(text, s, (TextLocation *)NULL);
         if ((c = Strexp(block, block, exp)) != NULL)
            break;
         }

      if (s == text-> lines.used)
         {
         retval = SCAN_WRAPPED;
         for (s = 0; s <= location-> line; s++)
            {
            block = GetTextBufferLocation(text, s, (TextLocation *)NULL);
            if ((c = Strexp(block,
                            block, exp)) != NULL)
               break;
            }
         if (s > location-> line || (s == location-> line &&
            (c == NULL || c - block >= location-> offset)))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
	/* For I18N:
	 * c, block are char *; they should be multi-byte chars;
	 * we need the offset in wide characters, because the
	 * TextEdit widget stores them that way;  so do the
	 * conversion.
	 */
#if !defined(MAX_CONVERTED_CHARS)
#define MAX_CONVERTED_CHARS 1000
#endif
	wchar_t converted[MAX_CONVERTED_CHARS];
	size_t converted1 = (size_t)0, converted2 = (size_t)0;

      converted1 = mbstowcs(converted, block, MAX_CONVERTED_CHARS);
	if (converted1 != (size_t)-1)
		converted2 = mbstowcs(converted, c, MAX_CONVERTED_CHARS);
	if (converted2 != (size_t)-1) {
		location-> line   = s;
		location-> offset = converted1 - converted2 ;
		location-> buffer = text->buffer->p;
	}
      }
   }

return (retval);
} /* end of ForwardScanTextBuffer */
/*
 * BackwardScanTextBuffer
 *
 * The \fIBackwardScanTextBuffer\fR function is used to scan,
 * towards the beginning of the buffer,
 * for a given \fIexp\fRression in the TextBuffer starting
 * at \fIlocation\fR.  A \fIScanResult\fR is returned which
 * indicates
 *
 * .so CWstart
 *   SCAN_NOTFOUND  The scan wrapped without finding a match.
 *   SCAN_WRAPPED   A match was found at a location \fIafter\fP the start location.
 *   SCAN_FOUND     A match was found at a location \fIbefore\fP the start location.
 *   SCAN_INVALID   Either the location or the expression was invalid.
 * .so CWend
 *
 * See also:
 *
 * ForwardScanTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern ScanResult BackwardScanTextBuffer(text, exp, location)
TextBuffer * text;
char * exp;
TextLocation * location;
{
char * c;
TextLine s = location-> line;
ScanResult retval = SCAN_INVALID;
char *block;
int byte_indx = 0;

if (exp != NULL)
   {
   char *ptr;
   int i, k, num_bytes = 0;

   retval = SCAN_FOUND;
   block = GetTextBufferLocation(text, s, (TextLocation *)NULL);

   ptr = block;
   byte_indx = (int)location->offset - 1;
   for (i=0; i< byte_indx ;i++,ptr+= k) {
	num_bytes += k = mblen(ptr, MB_LEN_MAX);
   }

#if !defined(I18N)
   if ((c = Strrexp(block,
                  &block[location-> offset - 1], exp)) == NULL)
#else
   if ( (byte_indx < 0) || (c = Strrexp(block,
                  &block[num_bytes], exp)) == NULL)
#endif
      {
      for (s = location-> line - 1; s >= (TextLine)0; s--)
         {
         block = GetTextBufferLocation(text, s, (TextLocation *)NULL);
         if ((c = Strrexp(block, NULL, exp)) != NULL)
            break;
         }

      if (s < 0)
         {
         retval = SCAN_WRAPPED;
         for (s = text-> lines.used - 1; s >= location-> line; s--)
            {
            block = GetTextBufferLocation(text, s, (TextLocation *)NULL);
            if ((c = Strrexp(block, NULL, exp)) != NULL)
               break;
            }

         if (s == location-> line &&
            (c == NULL || c - block <= location-> offset))
            retval = SCAN_NOTFOUND;
         }
      }
   if (retval == SCAN_FOUND || retval == SCAN_WRAPPED)
      {
#if !defined(MAX_CONVERTED_CHARS)
#define MAX_CONVERTED_CHARS 1000
#endif
	wchar_t converted[MAX_CONVERTED_CHARS];
	size_t converted1 = (size_t)0, converted2 = (size_t)0;

      converted1 = mbstowcs(converted, block, MAX_CONVERTED_CHARS);
	if (converted1 != (size_t)-1)
		converted2 = mbstowcs(converted, c, MAX_CONVERTED_CHARS);
	if (converted2 != (size_t)-1) {
		location-> line   = s;
		location-> offset = converted1 - converted2 ;
		location-> buffer = text-> buffer-> p;
	}
      }
   }

return (retval);
} /* end of BackwardScanTextBuffer */
/*
 * NewPage
 *
 */

static TextPage NewPage(text, after)
TextBuffer * text;
TextPage after;
{
TextPage at = after + 1;
register TextPage i = text-> pagecount;
register TextPage j;

#ifdef DEBUG
   (void) fprintf(stderr,"allocating a new page at %d\n", at);
#endif

if (i == PQLIMIT)
   {
   for (i = 0, j = 1; j < PQLIMIT; j++)
      if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
         i = j;

   SwapOut(text, text-> pqueue[i].pageindex);
   }
else
   text-> pagecount++;

if (text-> pages.used == text-> pages.size)
   {
   text-> pages.size *= 2;
   text-> pages.p = (Page *) REALLOC
      ((char *)text-> pages.p, (unsigned)(text-> pages.size * sizeof(Page)));
   }

if (at < text-> pages.used++)
   {
   for (j = FirstLineOfPage(text, at); j < text-> lines.used; j++)
      text-> lines.p[j].pageindex++;

   for (j = 0; j < text-> pagecount; j++)
      if (text-> pqueue[j].pageindex >= at)
         text-> pqueue[j].pageindex++;
   memmove(&text-> pages.p[at + 1], &text-> pages.p[at],
           (text-> pages.used - at  - 1) * sizeof(Page));
   }

text-> pages.p[at].chars = (TextPosition)0;
text-> pages.p[at].lines = (TextLine)0;
text-> pages.p[at].qpos  = i;
text-> pages.p[at].dpos  = (BlockTable *)NULL;

text-> pqueue[i].pageindex = text-> curpageno = at;
text-> pqueue[i].timestamp = text-> pageref++;

#ifdef DEBUG
printbuffer(text);
(void) fprintf(stderr,"after adding a page at %d\n\n", at);
#endif

return (at);

} /* end of NewPage */
/*
 * SplitPage
 *
 */

static TextPage SplitPage(text, after, line)
TextBuffer * text;
TextPage after;
TextLine line;
{
TextPage newpage;
TextLine l;
TextLine s;

/*
if (line == text-> lines.used)
   {
   (void) fprintf(stderr,"adding page after %d\n", after);
   newpage = NewPage(text, after);
   }
else
*/
   {
#ifdef DEBUG
   printbuffer(text);
   (void) fprintf(stderr,"splitting page %d\n", after);
#endif
   if (text-> curpageno != after)
      SwapIn(text, after);
   s = l = FirstLineOfPage(text, after) + MAXLPP / 2;
   newpage = NewPage(text, after);
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> pages.p[after].chars -= text-> lines.p[l].buffer-> used;
      text-> pages.p[after].lines--;
      l++;
      }
#ifdef DEBUG
   (void) fprintf(stderr,"AFTER = %d NEWPAGE = %d   ...", after, newpage);
#endif
   l = s;
   while (l < text-> lines.used && text-> lines.p[l].pageindex == after)
      {
      text-> lines.p[l].pageindex = newpage;
      text-> pages.p[newpage].chars += text-> lines.p[l].buffer-> used;
      text-> pages.p[newpage].lines++;
      l++;
      }
   newpage = (text-> lines.used ==  (TextLine)0 ? (TextPage)0 :
              text-> lines.used == line ? text-> lines.p[line - 1].pageindex :
                                          text-> lines.p[line].pageindex);

#ifdef DEBUG
   (void) fprintf(stderr,"l: %d newpage = %d  curpage = %d\n", 
                  line, newpage, text-> curpageno);
   printbuffer(text);
#endif
   if (text-> curpageno != newpage)
      SwapIn(text, newpage);
   }

return (newpage);

} /* end of SplitPage */
/*
 * SwapIn
 *
 * The algorithm here is:
 *
 * 1. if the page is not in the queue:
 *    a. find the least recently used page and page it out
 *    b. set the qpos to this slot
 *    c. get the block used by this page and free them
 *    d. loop for each line starting at the first line for this page
 *       and allocate space and read in the line (freeing the blocks
 *       consumed).
 * 2. set the page queue location to the pageindex and increment the
 *    timestamp for the queue slot.
 */

static void SwapIn(text, pageindex)
TextBuffer * text;
TextPage pageindex;
{
register TextPage  i = text-> pages.p[pageindex].qpos;
register TextPage  j;
register TextBlock block = 0;
register TextLine  n;
register BufferElement * p;

#ifdef DEBUG
(void) fprintf(stderr, "swapping in %d\n", pageindex);
#endif

if (i == PQLIMIT)
   {
   if (text-> pagecount == PQLIMIT)
      {
      for (i = 0, j = 1; j < PQLIMIT; j++)
         if (text-> pqueue[j].timestamp < text-> pqueue[i].timestamp)
            i = j;
      SwapOut(text, text-> pqueue[i].pageindex);
      }
   else
      {
      i = text-> pagecount;
      text-> pagecount++;
      }

   text-> pages.p[pageindex].qpos = i;

   FreeBlock(text, pageindex, block++);
   for (j = FirstLineOfPage(text, pageindex);
        text-> lines.p[j].pageindex == pageindex && j < text-> lines.used; j++)
      {
      text-> lines.p[j].buffer-> size =
      n = text-> lines.p[j].buffer-> used;
      p = text-> lines.p[j].buffer-> p =
         (BufferElement *) MALLOC((unsigned)(n * sizeof(BufferElement)));
      while (n > textblockavailable)
         {
         (void) fread(p, sizeof(BufferElement), textblockavailable, text-> tempfile);
         n -= textblockavailable;
         p = &p[textblockavailable];
         FreeBlock(text, pageindex, block++);
         }
      if (n > 0)
         {
         (void) fread(p, sizeof(BufferElement), n, text-> tempfile);
         textblockavailable -= n;
         }
      }
   text-> pages.p[pageindex].dpos-> used = 0;
   }

text-> pqueue[i].pageindex = text-> curpageno = pageindex;
text-> pqueue[i].timestamp = text-> pageref++;

} /* end of SwapIn */
/*
 * SwapOut
 *
 * The algorithm here is:
 *
 * 1. if the tempfile is not open, open it and calculate a block size
 *    based on the history accumulated thus far. (NOTE: not implemented)
 * 2. if the dpos BlockTable has not been allocated, allocate one.
 * 3. Allocate a block to page out to.
 * 4. loop through the lines in the page (starting from the first line)
 *    and write out the lines filling the blocks (allocating new ones
 *    as necessary).
 * 5. Set the qpos (queue position) indicator to show that the page is
 *    not in the queue.
 */

static void SwapOut(text, pageindex)
TextBuffer * text;
TextPage pageindex;
{
register TextLine i;
register TextPosition n;
register BufferElement * p;

#ifdef DEBUG
(void) fprintf(stderr, "swapping out %d\n", pageindex);
#endif

if (pageindex == -1)
   return;

if (text-> tempfile == NULL)
   {
   text-> blocksize = BLOCKSIZE;
   text-> tempfile = tmpfile();
   }

if (text-> pages.p[pageindex].dpos == (BlockTable *)NULL)
   text-> pages.p[pageindex].dpos =
      (BlockTable *) AllocateBuffer(sizeof(TextBlock), BTALLOC);

AllocateBlock(text, pageindex);
for (i = FirstLineOfPage(text, pageindex);
     text-> lines.p[i].pageindex == pageindex && i < text-> lines.used; i++)
   {
   p = text-> lines.p[i].buffer-> p;
   n = text-> lines.p[i].buffer-> used;
   while (n > textblockavailable)
      {
      (void) fwrite(p, sizeof(BufferElement), textblockavailable, text-> tempfile);
      n -= textblockavailable;
      p = &p[textblockavailable];
      AllocateBlock(text, pageindex);
      }
   if (n > 0)
      {
      (void) fwrite(p, sizeof(BufferElement), n, text-> tempfile);
      textblockavailable -= n;
      }
   FREE(text-> lines.p[i].buffer-> p);
   text-> lines.p[i].buffer-> p = NULL;
   }

text-> pages.p[pageindex].qpos = PQLIMIT;

} /* end of SwapOut */
static TextLine FirstLineOfPage(text, page)
TextBuffer * text;
TextPage page;
{
register TextPage i;
register TextLine l = 0;

for (i = 0; i < page; i++)
   l += text-> pages.p[i].lines;

return (l);

} /* end of FirstLineOfPage */
/*
 * AllocateBlock
 *
 */

static void AllocateBlock(text, pageindex)
TextBuffer * text;
TextPage pageindex;
{
TextBlock block;

if (BufferEmpty(text-> free_list))
   block = text-> blockcnt++;
else
   block = text-> free_list-> p[--text-> free_list-> used];

(void) fseek(text-> tempfile, 
             (long)(block * text-> blocksize * sizeof(BufferElement)), SEEK_SET);
textblockavailable = text-> blocksize;

if (BufferFilled(text-> pages.p[pageindex].dpos))
   GrowBuffer((Buffer *)text-> pages.p[pageindex].dpos, 66);

text-> pages.p[pageindex].dpos-> p[text-> pages.p[pageindex].dpos-> used++] =
   block;

} /* end of AllocateBlock */
/*
 * FreeBlock
 *
 */

static void FreeBlock(text, pageindex, blockindex)
TextBuffer * text;
TextPage pageindex;
TextBlock blockindex;
{
TextBlock block = text-> pages.p[pageindex].dpos-> p[blockindex];

if (BufferFilled(text-> free_list))
   GrowBuffer((Buffer *)text-> free_list, text-> free_list-> size * 2);
text-> free_list-> p[text-> free_list-> used++] = block;

(void) fseek(text-> tempfile, 
             (long)(block * text-> blocksize * sizeof(BufferElement)), SEEK_SET);
textblockavailable = text-> blocksize;

} /* end of FreeBlock */
/*
 * InsertBlockIntoTextBuffer
 *
 */

static EditResult InsertBlockIntoTextBuffer(text, location, string)
TextBuffer * text;
TextLocation * location;
BufferElement * string;
{
Buffer * sp;
Buffer * buffer;
static Buffer * work;
TextLine lineindex = location-> line;
TextPage pageindex;

if (work == NULL)
   work = AllocateBuffer(sizeof(work->p[0]), LNMIN);

if (string == (BufferElement *) NULL || *string == (BufferElement) NULL)
   return (EDIT_SUCCESS);

#ifdef DEBUG
printf("InsertBlockIntoTextBuffer:");
_Print_wc_String(string);
#endif

if (location-> line >= (TextLine)0 && location-> line < text-> lines.used &&
    location-> offset >= (TextLine)0 &&
    location-> offset < text-> lines.p[lineindex].buffer-> used)
   {
   sp = wcstropen(string);
   while (ReadStringIntoBuffer(sp, work) != EOF)
      {
      if (location->offset == 0)
         {
#ifdef VERBOSE
         (void) fprintf(stderr,"inserting a line at %d\n", location-> line);
#endif
         text-> insert.hint |= TEXT_BUFFER_INSERT_LINE;
         InsertLineIntoTextBuffer(text, location-> line, work);
         location-> line++;
         location-> offset = 0;
         }
      else
         {
         Buffer * work2;

#ifdef VERBOSE
         (void) fprintf(stderr,"splitting a line at %d\n", location-> line);
#endif
         text-> insert.hint |= TEXT_BUFFER_INSERT_SPLIT_LINE;

         (void) wcGetTextBufferLocation(text, location-> line, (TextLocation *)NULL);
         buffer = text-> lines.p[location-> line].buffer;
         work2 = CopyBuffer(buffer);
         text-> pages.p[text-> lines.p[location-> line].pageindex].chars -=
            (buffer-> used - location-> offset - 1);
         buffer-> used = location-> offset + 1;
         buffer-> p[location-> offset] = '\0';

         InsertIntoBuffer(buffer, work, location-> offset);
         text-> pages.p[text-> lines.p[location-> line].pageindex].chars +=
            (work-> used - 1);

         location-> line++;

         work2-> used -= location-> offset;
         memmove(work2-> p, &work2-> p[location-> offset], 
            work2-> used * sizeof(BufferElement));
         InsertLineIntoTextBuffer(text, location-> line, work2);
         FreeBuffer(work2);

         location-> offset = 0;
         }
      }
   if (work-> used > 1)
      {
#ifdef VERBOSE
      (void) fprintf(stderr,"inserting '%s' at %d.%d\n",
                     work-> p, location-> line, location-> offset);
#endif
      text-> insert.hint |= TEXT_BUFFER_INSERT_CHARS;
      (void) wcGetTextBufferLocation(text, location-> line, (TextLocation *)NULL);
      buffer = text-> lines.p[location-> line].buffer;
      InsertIntoBuffer(buffer, work, location-> offset);
      location-> offset += (work-> used - 1);
      text-> pages.p[text-> lines.p[location-> line].pageindex].chars +=
         (work-> used - 1);
      }
   strclose(sp);
   return (EDIT_SUCCESS);
   }
else
   return (EDIT_FAILURE);

} /* end of InsertBlockIntoTextBuffer */
/*
 * InsertLineIntoTextBuffer
 *
 * This function insert a given Buffer into a given TextBuffer at
 * a given line.  The algorithm used is:	
 *
 *  1. Check for line within range, if not return failure else continue.
 *  2. Determine if affected page has room for another line, if not
 *     allocate a new page.
 *  3. If page is not the current page then SwapIn the page.
 *  4. Increase the LineTable and shift down if necessary.
 *  5. Copy the Buffer into a newly allocated buffer.
 *  6. Store this buffer pointer and the pageindex in the line table.
 *  7. Increment the Page chars and lines.
 *  8. Return success.
 */

extern EditResult InsertLineIntoTextBuffer(text, at, buffer)
TextBuffer * text;
TextLine at;
Buffer * buffer;
{
TextPage page;
int savesize;

#ifdef DEBUG
printf("InsertLineIntoTextBuffer [ENTER]:");
_Print_wc_String(buffer->p);
#endif

if (at < (TextLine)0 || at > text-> lines.used)
   {
#ifdef DEBUG
   printf("InsertLineIntoTextBuffer [LEAVE/FAIL]:");
#endif
   return (EDIT_FAILURE);
   }
else
   {
   page = (text-> lines.used ==  (TextLine)0 ? (TextPage)0 :
           text-> lines.used == at ? text-> lines.p[at - 1].pageindex :
                                     text-> lines.p[at].pageindex);

   if (text-> pages.p[page].lines == MAXLPP)
      page = SplitPage(text, page, at);
   else
      if (text-> curpageno != page)
         SwapIn(text, page);

   if (text-> lines.used == text-> lines.size)
      {
      text-> lines.size *= 2;
      text-> lines.p = (Line *) REALLOC
         ((char *)text-> lines.p, (unsigned)(text-> lines.size * sizeof(Line)));
      }

   if (at < text-> lines.used++)
      memmove(&text-> lines.p[at + 1], &text-> lines.p[at],
              (text-> lines.used - at - 1) * sizeof(Line));

   savesize = buffer-> size; 
   buffer-> size = buffer-> used;
   text-> lines.p[at].buffer    = CopyBuffer(buffer);
   buffer-> size = savesize;
   text-> lines.p[at].pageindex = page;
   text-> lines.p[at].userData  = 0L;

   text-> pages.p[page].chars += text-> lines.p[at].buffer-> used;
   text-> pages.p[page].lines++;

#ifdef DEBUG
   printf("InsertLineIntoTextBuffer [LEAVE/SUCCESS]\n");
#endif 

   return (EDIT_SUCCESS);

   }

} /* end of InsertLineIntoTextBuffer */
/**************************************************************/
/**************************************************************/
/**************************************************************/
/*
 * DeleteBlockInTextBuffer
 *
 */

static EditResult DeleteBlockInTextBuffer(text, startloc, endloc)
TextBuffer * text;
TextLocation * startloc;
TextLocation * endloc;
{
Buffer *        work;
BufferElement * p;
register int    len;
register int    i;
register int    j;
register int    k;

if (startloc-> line == endloc-> line)
   {
#ifdef VERBOSE
   (void) fprintf(stderr," deleteing in line\n");
#endif
   len = endloc-> offset - startloc-> offset;
   if (len > 0)
      if (len == text-> lines.p[endloc-> line].buffer-> used)
         {
#ifdef VERBOSE
(void) fprintf(stderr,"delete a line!\n");
#endif
         }
      else
         {
#ifdef VERBOSE
         (void) fprintf(stderr,"simple delete of chars in line\n");
#endif
         text-> deleted.hint = TEXT_BUFFER_DELETE_SIMPLE;
         p = wcGetTextBufferLocation(text, startloc-> line, (TextLocation *)NULL);
         memmove(&p[startloc-> offset],
                 &p[endloc-> offset],
                 (sizeof(BufferElement) * 
                 (text-> lines.p[endloc-> line].buffer-> used - endloc-> offset)));
         text-> lines.p[endloc-> line].buffer-> used -= len;
         text-> pages.p[text-> lines.p[endloc-> line].pageindex].chars -= len;
         }
   }
else
   {
   if (startloc-> offset == 0)
      {
      i = startloc-> line;
      text-> deleted.hint |= TEXT_BUFFER_DELETE_START_LINE;
      }
   else
      {
#ifdef VERBOSE
      (void) fprintf(stderr," deleting at start\n");
#endif
      text-> deleted.hint |= TEXT_BUFFER_DELETE_START_CHARS;
      p = wcGetTextBufferLocation(text, startloc-> line, (TextLocation *)NULL);
      len = text-> lines.p[startloc-> line].buffer-> used - startloc-> offset;
      text-> lines.p[startloc-> line].buffer-> used -= (len - 1);
      text-> pages.p[text-> lines.p[startloc-> line].pageindex].chars -= (len - 1);
      p[startloc-> offset] = '\0';
      i = startloc-> line + 1;
      }

   if (endloc-> offset == text-> lines.p[endloc-> line].buffer-> used)
      {
      k = endloc-> line;
      text-> deleted.hint |= TEXT_BUFFER_DELETE_END_LINE;
      }
   else
      {
      if (endloc-> offset != 0)
         {
#ifdef VERBOSE
         (void) fprintf(stderr," deleting on end\n");
#endif
         text-> deleted.hint |= TEXT_BUFFER_DELETE_END_CHARS;
         p = wcGetTextBufferLocation(text, endloc-> line, (TextLocation *)NULL);
         len = endloc-> offset;
         text-> lines.p[endloc-> line].buffer-> used -= len;
         text-> pages.p[text-> lines.p[endloc-> line].pageindex].chars -= len;
         memmove(p, &p[len], sizeof(BufferElement) * 
                (text-> lines.p[endloc-> line].buffer-> used));
         }
      k = endloc-> line - 1;
      }

   for (j = i; j <= k; j++)
      DeleteLineInTextBuffer(text, i);

   if (i != startloc-> line && k != endloc-> line)
      {
#ifdef VERBOSE
      (void) fprintf(stderr," joining lines\n");
#endif
      text-> deleted.hint |= TEXT_BUFFER_DELETE_JOIN_LINE;
      j = startloc-> line;
      (void) wcGetTextBufferLocation(text, j, (TextLocation *)NULL);
      work = CopyBuffer(text-> buffer);
      DeleteLineInTextBuffer(text, j);

      (void) wcGetTextBufferLocation(text, j, (TextLocation *)NULL);
      InsertIntoBuffer(text-> buffer, work, 0);
      text-> pages.p[text-> lines.p[j].pageindex].chars += (work-> used - 1);

      FreeBuffer(work);
      }
   }

#ifdef DEBUG
(void) fprintf(stderr,"end of delete\n\n");
#endif

return (EDIT_SUCCESS);

} /* end of DeleteBlockInTextBuffer */
/*
 * DeleteLineInTextBuffer
 *
 */

static EditResult DeleteLineInTextBuffer(text, at)
TextBuffer * text;
TextLine at;
{
TextPage pageindex;
TextLine j;

if (at < 0 || at >= text-> lines.used)
   return (EDIT_FAILURE);
else
   {
   pageindex = text-> lines.p[at].pageindex;

   if (text-> lines.p[at].pageindex != text-> curpageno)
      SwapIn(text, pageindex);

   text-> pages.p[pageindex].chars -= text-> lines.p[at].buffer-> used;
   if (--text-> pages.p[pageindex].lines == (TextLine)0)
      {
      int qpos = text-> pages.p[pageindex].qpos;
      text-> curpageno = -1;
      if (qpos != --text-> pagecount)
         {
         text-> pages.p[text-> pqueue[text-> pagecount].pageindex].qpos = qpos;
         text-> pqueue[qpos] = text-> pqueue[text-> pagecount];
         }
      if (pageindex != --text-> pages.used)
         {
#ifdef DEBUG
         printbuffer(text);
         fprintf(stderr, "free page\n");
#endif

         memmove(&text-> pages.p[pageindex],
                 &text-> pages.p[pageindex + 1],
                 (text-> pages.used - pageindex) * sizeof(Page));
         for (j = FirstLineOfPage(text, pageindex); j < text-> lines.used; j++)
            text-> lines.p[j].pageindex--;
         for (j = 0; j < text-> pagecount; j++)
            if (text-> pqueue[j].pageindex > pageindex)
               text-> pqueue[j].pageindex--;
#ifdef DEBUG
         printbuffer(text);
#endif
         }
      }

   FreeBuffer((Buffer *)text-> lines.p[at].buffer);

   if (at != --text-> lines.used)
      memmove(&text-> lines.p[at], &text-> lines.p[at + 1],
              (text-> lines.used - at) * sizeof(Line));

   return (EDIT_SUCCESS);
   }

} /* DeleteLineInTextBuffer */
/*
 * ReplaceCharInTextBuffer
 *
 * The \fIReplaceCharInTextBuffer\fR function is used to replace
 * the character in the TextBuffer \fItext\fR at \fIlocation\fR
 * with the character \fIc\fR.
 *
 * See also:
 *
 * ReplaceBlockInTextBuffer(3), wcReplaceBlockInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern EditResult ReplaceCharInTextBuffer(text, location, c, f, d)
TextBuffer *       text;
TextLocation *     location;
int                c;
TextUpdateFunction f;
caddr_t            d;
{
BufferElement buffer[2];
TextLocation nextloc;

buffer[0] = (BufferElement) c;
buffer[1] = (BufferElement) '\0';

nextloc = NextLocation(text, *location);
if (nextloc.line == 0 && nextloc.offset == 0)
   nextloc = *location;

return (wcReplaceBlockInTextBuffer(text, location, &nextloc, buffer, f, d));

} /* end of ReplaceCharInTextBuffer */
/*
 * ReplaceBlockInTextBuffer
 *
 * The \fIReplaceBlockInTextBuffer\fR function is used to update the contents
 * of the TextBuffer associated with \fItext\fR.  The characters stored
 * between \fIstartloc\fR and \fIendloc\fR are deleted and the \fIstring\fR
 * is inserted after \fIstartloc\fR.  On systems where the storage
 * is wchar_t, the string is converted from multibyte (char *)
 * form to wide character (wchar_t *) form before insertion.
 * If the edit succeeds the 
 * TextUpdateFunction \fIf\fR is called with the parameters: \fId\fR, 
 * \fItext\fR, and 1; then any other \fBdifferent\fR update functions
 * associated with the TextBuffer are called with their associated data
 * pointer, \fItext\fR, and 0.
 *
 * This function records the operation performed in TextUndoItem structures.
 * The contents of these structures can be used to implement an undo function.
 * The contents can also be used to determine the type of operation performed.
 * A structure is allocated for both the delete and insert information.
 *
 * The hints provided in these structures is the inclusive or of:~
 *
 *    TEXT_BUFFER_NOP
 *    TEXT_BUFFER_INSERT_LINE
 *    TEXT_BUFFER_INSERT_SPLIT_LINE
 *    TEXT_BUFFER_INSERT_CHARS
 *    TEXT_BUFFER_DELETE_START_LINE
 *    TEXT_BUFFER_DELETE_END_LINE
 *    TEXT_BUFFER_DELETE_START_CHARS
 *    TEXT_BUFFER_DELETE_END_CHARS
 *    TEXT_BUFFER_DELETE_JOIN_LINE
 *    TEXT_BUFFER_DELETE_SIMPLE
 *
 * See also:
 *
 * ReplaceCharInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern EditResult ReplaceBlockInTextBuffer(text, startloc, endloc, string, f, d)
TextBuffer *   text;
TextLocation * startloc;
TextLocation * endloc;
char *         string;
TextUpdateFunction f;
caddr_t d;
{
register int i;
EditResult   retval;
#ifdef I18N
static BufferElement * wc_copy;
static int wc_size;
BufferElement *wptr1,*wptr2;
#endif

FreeTextUndoList(text);

if (startloc-> line != endloc-> line || startloc-> offset != endloc-> offset)
   text-> deleted.string = wcGetTextBufferBlock
      (text, *startloc, PreviousLocation(text, *endloc));
if (string != NULL && *string != '\0'){
   if (text-> insert.string)
      FREE(text-> insert.string);
#ifdef I18N
   i = _mbstrlen(string) + 1;
   text-> insert.string = (BufferElement *) MALLOC(sizeof(BufferElement)* i);
      /*
       *  convert string, including NULL terminator, into wide character format
       */
   if (mbstowcs(text-> insert.string, string, i) == -1)
	OlVaDisplayWarningMsg((Display *)NULL, OleNfileTextbuff,
		OleTmsg1, OleCOlToolkitWarning,
		OleMfileTextbuff_msg1,
		string);
#else
   text-> insert.string = strcpy(MALLOC(strlen(string) + 1), string);
#endif
}

text-> insert.hint =
text-> deleted.hint = 0;

text-> deleted.start =
text-> insert.start =
text-> insert.end   = *startloc;
text-> deleted.end   = *endloc;

retval = DeleteBlockInTextBuffer(text, startloc, endloc);

if (retval == EDIT_SUCCESS)
   {
#ifdef I18N
   if (!wc_copy){	
			/* _mbstrlen returns -1 for illegal string */
      wc_size = _mbstrlen(string) + 2;
      wc_copy = (BufferElement *) MALLOC(sizeof(BufferElement) * wc_size);
   }
   else{
			/* _mbstrlen returns -1 for illegal string */
      int size = _mbstrlen(string) + 2;

      if (size > wc_size){
         wc_size = size;
         wc_copy = (BufferElement *) REALLOC(wc_copy, sizeof(BufferElement) * wc_size);
      }
   }
	if ((wptr1 = text->insert.string) != (BufferElement *)NULL){
			wptr2 = wc_copy;
			while(*wptr1)
				*wptr2++ = *wptr1++;
			*wptr2 = NULL;				/* null terminate the string */
		}
	else{
			wc_copy[0] = NULL;
		}
	InsertBlockIntoTextBuffer(text, startloc, wc_copy);
#else
   retval = InsertBlockIntoTextBuffer(text, startloc, string);
#endif
   if (retval == EDIT_SUCCESS)
      {
      text-> dirty = 1;
      text-> insert.end = *startloc;
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f == f && text-> update[i].d == d)
            {
            (*text-> update[i].f) (text-> update[i].d, text, 1);
            break;
            }
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f != f || text-> update[i].d != d)
            (*text-> update[i].f) (text-> update[i].d, text, 0);
      }
   }
else
   retval = EDIT_FAILURE;

return (retval);

} /* end of ReplaceBlockInTextBuffer */
/*
 * LocationOfPosition
 *
 * The \fILocationOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR TextBuffer to a TextLocation.
 * It returns the translated TextLocation.  If the \fIposition\fR is invalid
 * the Buffer pointer \fIbuffer\fP is set to NULL and the line and 
 * offset members are set the the last valid location in 
 * the TextBuffer; otherwise \fIbuffer\fP is set to a non-NULL 
 * (though useless) value.
 *
 * See also:
 *
 * LineOfPosition(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation LocationOfPosition(text, position)
TextBuffer * text;
TextPosition position;
{
TextLocation location;
register TextPage i;
register TextPosition c = (TextPosition)0;
register TextLine l = (TextLine)0;

if (position < 0)
    position = LastTextBufferPosition(text) + 1;

for (i = (TextPage)0; c < position && i < text-> pages.used; i++)
   {
   c += text-> pages.p[i].chars;
   l += text-> pages.p[i].lines;
   }

if (position == (TextPosition)0)
   {
   location.line = (TextLine)0;
   location.offset = (TextPosition)0;
   location.buffer = &blank;
   }
else
   if (c <= position && i == text-> pages.used)
      {
      location.line = l - 1;
      location.offset = c - 1;
      location.buffer = NULL;
      }
   else
      {
      i--;
      c -= text-> pages.p[i].chars;
      l -= text-> pages.p[i].lines;
      while (c <= position)
         c += text-> lines.p[l++].buffer-> used;
      c -= text-> lines.p[--l].buffer-> used;
      location.line   = l;
      location.offset = position - c;
      location.buffer = &blank;
      }

return (location);

} /* end of LocationOfPosition */
/*
 * LineOfPosition
 *
 * The \fILineOfPosition\fR function is used to translate a 
 * \fIposition\fR in the \fItext\fR TextBuffer to a line index.
 * It returns the translated line index or EOF if the \fIposition\fR
 * is invalid.
 *
 * See also:
 *
 * LineOfPositon(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern int LineOfPosition(text, position)
TextBuffer * text;
TextPosition position;
{
TextLocation location;

location = LocationOfPosition(text, position);
if (location.buffer == NULL)
   location.line = EOF;

return (location.line);

} /* end of LineOfPosition */
/*
 * PositionOfLine
 *
 * The \fIPositionOfLine\fR function is used to translate a 
 * \fIlineindex\fR in the \fItext\fR TextBuffer to a TextPosition.
 * It returns the translated TextPosition or EOF if the \fIlineindex\fR
 * is invalid.
 *
 * See also:
 *
 * LineOfPositon(3), PositionOfLocation(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition PositionOfLine(text, lineindex)
TextBuffer * text;
TextLine lineindex;
{
TextPage i;
TextLine j;
TextPosition position = (TextPosition)EOF;

if (lineindex >= (TextLine)0 && lineindex < text-> lines.used)
   {
   position = (TextPosition)0;
   for (i = (TextPage)0; i < text-> lines.p[lineindex].pageindex; i++)
      position += text-> pages.p[i].chars;
   for (j = FirstLineOfPage(text, i); j < lineindex; j++)
      position += text-> lines.p[j].buffer-> used;
   }

return (position);

} /* end of PositionOfLine */
/*
 * LastTextBufferLocation
 *
 * The \fILastTextBufferLocation\fR function returns the last valid
 * TextLocation in the TextBuffer associated with \fItext\fR.
 *
 * See also:
 *
 * LastTextBufferPosition(3), FirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation LastTextBufferLocation(text)
TextBuffer * text;
{
TextLocation last;

last.line = LastTextBufferLine(text);
last.offset = LastCharacterInTextBufferLine(text, last.line);

return (last);

} /* end of LastTextBufferLocation */
/*
 * LastTextBufferPosition
 *
 * The \fILastTextBufferPosition\fR function returns the last valid
 * TextPositon in the TextBuffer associated with \fItext\fR.
 *
 * See also:
 *
 * LastTextBufferLocation(3), FirstTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition LastTextBufferPosition(text)
TextBuffer * text;
{

return (PositionOfLocation(text, LastTextBufferLocation(text)));

} /* end of LastTextBufferPosition */
/*
 * PositionOfLocation
 *
 * The \fIPositionOfLocation\fR function is used to translate a 
 * \fIlocation\fR in the \fItext\fR TextBuffer to a TextPosition.
 * The function returns the translated TextPosition or EOF if 
 * the \fIlocation\fR is invalid.
 *
 * See also:
 *
 * PositionOfLine(3), LocationOfPosition(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextPosition PositionOfLocation(text, location)
TextBuffer * text;
TextLocation location;
{
TextPosition position = PositionOfLine(text, location.line);

if (position != (TextPosition)EOF)
   if (location.offset >= text-> lines.p[location.line].buffer-> used)
      position = (TextPosition)EOF;
   else
      position += location.offset;

return (position);

} /* end of PostionOfLocation */
/*
 * GetTextBufferLine
 *
 * The \fIGetTextBufferLine\fR function is used to retrieve the contents of
 * \fIline\fR from the \fItext\fR TextBuffer.  It returns a pointer to a 
 * string containing the copy of the contents of the line or NULL if the 
 * \fIline\fR is outside the range of valid lines in \fItext\fR.
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferBlock(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferBlock(3),
 * wcGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char * 
GetTextBufferLine(text, lineindex)
TextBuffer * text;
TextLine lineindex;
{
register char * p = GetTextBufferLocation(text, lineindex, (TextLocation *)NULL);

if (p != NULL)
   p = strcpy(MALLOC((unsigned)(strlen(p) + 1)), p);
return (p);

} /* end of GetTextBufferLine */
/*
 * GetTextBufferChar
 *
 * The \fIGetTextBufferChar\fR function is used to retrieve a character
 * stored in the \fItext\fR TextBuffer at \fIlocation\fR.  It returns
 * either the character itself or EOF if location is outside the range
 * of valid locations within the TextBuffer.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferBlock(3), GetTextBufferLine(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferBlock(3),
 * wcGetTextBufferBlock(3), wcGetTextBufferLine(3) 
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

int GetTextBufferChar(text, location)
TextBuffer * text;
TextLocation location;
{
BufferElement * p = wcGetTextBufferLocation(text, location.line, (TextLocation *)NULL);
register int c = EOF;

if (p != NULL)
   if (location.offset < (int)_StringLength(p))   /* ANSI_C */
      c = p[location.offset];

return (c == (BufferElement) '\0' ? '\n' : c);

} /* end of GetTextBufferChar */
/*
 * CopyTextBufferBlock
 *
 * The \fICopyTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_position\fR and \fIend_position\fR inclusive.  It
 * returns the number of characters copied; if the parameters are invalid
 * the return value is zero (0).  
 *
 * Note:
 *
 * The storage for the copy is allocated by the caller.  It is the 
 * responsibility of the caller to ensure that enough storage is allocated
 * to copy end_position - start_position + 1 BufferElements.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern int CopyTextBufferBlock(text, buffer, start_position, end_position)
TextBuffer * text;
char * buffer;
TextPosition start_position;
TextPosition end_position;
{
int retval;
TextLocation start_location;
TextLocation end_location;
char * p = buffer;

start_location = LocationOfPosition(text, start_position);
end_location = LocationOfPosition(text, end_position);

if (end_position < start_position ||
    start_location.buffer == NULL || end_location.buffer == NULL)
   retval = 0;
else
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   TextPosition count = 0;
   BufferElement *t = wcGetTextBufferLocation(text, start_location.line,
                                    (TextLocation *)NULL);

   while (count++ < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = wcGetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            outpos+= wctomb(&p[outpos], t[inpos++]);
            break;
         }
      }
   retval = size;
   p[outpos] = '\0';
   }

return (retval);

} /* end of CopyTextBufferBlock */
/*
 * GetTextBufferBlock
 *
 * The \fIGetTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_location\fR and \fIend_location\fR inclusive.  It
 * returns a pointer to a string containing the copy.  If the parameters
 * are invalid NULL is returned.  
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern char * 
GetTextBufferBlock(text, start_location, end_location)
TextBuffer * text;
TextLocation start_location;
TextLocation end_location;
{
char * p  = NULL;
TextPosition start_position = PositionOfLocation(text, start_location);
TextPosition end_position   = SameTextLocation(start_location, end_location) ?
   start_position : PositionOfLocation(text, end_location);

if (start_position != (TextPosition)EOF && end_position != (TextPosition)EOF &&
    start_position <= end_position)
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   TextPosition count = 0;
   BufferElement *t = wcGetTextBufferLocation(text, start_location.line,
                                             (TextLocation *)NULL);

   p = MALLOC(((unsigned)size + 1) * sizeof(BufferElement));

   while (count++ < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = wcGetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            outpos+= wctomb(&p[outpos], t[inpos++]);
            break;
         }
      }
   p[outpos] = '\0';
   }

return (p);

} /* end of GetTextBufferBlock */
/*
 * WriteTextBuffer
 *
 * The \fIWriteTextBuffer\fR function is used to write the contents
 * of the \fItext\fR TextBuffer to the file pointer to by \fIfp\fR.  
 * The contents are written in multibyte form.  The funtion returns
 * a WriteResult which can be:~
 *
 * .so CWstart
 *    WRITE_FAILURE
 *    WRITE_SUCCESS
 * .so CWend
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern WriteResult WriteTextBuffer(text, fp)
TextBuffer * text;
FILE *       fp;
{
int i;

if (!TextBufferEmpty(text))
   {
   for (i = (TextLine)0; i < text-> lines.used; i++)
      (void) fprintf(fp, "%s\n",
                     GetTextBufferLocation(text, i, (TextLocation *)NULL));
   return (WRITE_SUCCESS);
   }
else
   return (WRITE_FAILURE);

} /* end of WriteTextBuffer */
/*
 * SaveTextBuffer
 *
 * The \fISaveTextBuffer\fR function is used to write the contents
 * of the \fItext\fR TextBuffer to the file \fIfilename\fR.  
 * The contents are written in multibyte form.  The funtion returns
 * a SaveResult which can be:~
 *
 * .so CWstart
 *    SAVE_FAILURE
 *    SAVE_SUCCESS
 * .so CWend
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern SaveResult SaveTextBuffer(text, filename)
TextBuffer * text;
char * filename;
{
FILE * fp;
register TextLine i;
BufferElement *p;

/* NOTE: NEED TO CHECK IF FILE CAN BE WRITTEN, ETC */

if (filename == NULL)
   filename = text-> filename;
else
   {
   if (text-> filename != NULL)
      FREE(text-> filename);
   text-> filename = strcpy(MALLOC((unsigned)(strlen(filename) + 1)), filename);
   }

if (filename != NULL && (fp = fopen(filename, "w")) != NULL)
   {
   if (!TextBufferEmpty(text))
      for (i = (TextLine)0; i < text-> lines.used; i++)
         (void) fprintf(fp, "%s\n",
                        GetTextBufferLocation(text, i, (TextLocation *)NULL));

   (void) fclose(fp);
   text-> dirty = 0;
   return (SAVE_SUCCESS);
   }
else
   return (SAVE_FAILURE);

} /* end of SaveTextBuffer */
/*
 * GetTextBufferBuffer
 *
 * The \fIGetTextBufferBuffer\fR function is used to retrieve a pointer to the
 * Buffer stored in TextBuffer \fItext\fR for \fIline\fR.  This pointer
 * is volatile; subsequent calls to any TextBuffer routine may make it
 * invalid.  If a more permanent copy of this Buffer is required the Buffer
 * Utility CopyBuffer can be used to create a private copy of it.
 *
 * See also:
 *
 * GetTextBufferBlock(3), GetTextBufferLocation(3)
 * wcGetTextBufferBlock(3), wcGetTextBufferLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern Buffer * GetTextBufferBuffer(text, line)
TextBuffer * text;
TextLine     line;
{
BufferElement * p = wcGetTextBufferLocation(text, line, (TextLocation *)NULL);

return (p == NULL ? (Buffer *)NULL : text-> buffer);

} /* end of GetTextBufferBuffer */
/*
 * IncrementTextBufferLocation
 *
 * The \fIIncrementTextBufferLocation\fR function is used to increment
 * a \fIlocation\fR by a either \fIline\fR lines and/or \fIoffset\fR characters.
 * It returns the new location.  Note: if \fIline\fR or \fIoffset\fR are
 * negative the function performs a decrement operation.  If the starting
 * location or the resulting location is invalid the starting location is 
 * returned without modification; otherwise the new location is returned.
 *
 * See also:
 *
 * NextLocation(3), PreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation IncrementTextBufferLocation(text, location, line, offset)
TextBuffer * text;
TextLocation location;
TextLine line;
TextPosition offset;
{
TextLocation newlocation;

if (location.line >= 0 && location.line < text-> lines.used &&
   location.offset >= 0 &&
   location.offset < text-> lines.p[location.line].buffer-> used)
   {
   if (line == 0 && offset == 0)
      newlocation = location;
   else
      if (line == 0 && offset != 0)
         {
         TextPosition new_pos = PositionOfLocation(text, location) + offset;
         if (new_pos >= 0)
            {
            newlocation = LocationOfPosition(text, new_pos);
            if (newlocation.buffer == NULL)
               newlocation = location;
            }
         else
            newlocation = location;
         }
      else
         {
         newlocation.line = location.line + line;
         if (newlocation.line >= 0 && newlocation.line < text-> lines.used)
            {
            newlocation.offset = offset == 0 ?
               location.offset : offset;
            if (newlocation.offset >=
                text-> lines.p[newlocation.line].buffer-> used)
               newlocation.offset =
                  text-> lines.p[newlocation.line].buffer-> used - 1;
            }
         else
            newlocation = location;
         }
   }
else
   newlocation = location;

return (newlocation);

} /* end of IncrementTextBufferLocation */
/*
 * PreviousLocation
 *
 * The \fIPreviousLocation\fR function returns the Location which precedes
 * the given \fIcurrent\fR location in a TextBuffer.  If the current
 * location points to the beginning of the TextBuffer this function wraps.
 *
 * See also:
 *
 * NextLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation PreviousLocation(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{

if (--current.offset < 0)
   {
   if (--current.line < 0)
      current.line = LastTextBufferLine(textBuffer);
   current.offset = LastCharacterInTextBufferLine(textBuffer, current.line);
   }

return (current);

} /* end of PreviousLocation */
/*
 * NextLocation
 *
 * The \fINextLocation\fR function returns the TextLocation which follows
 * the given \fIcurrent\fR location in a TextBuffer.  If the current
 * location points to the end of the TextBuffer this function wraps.
 *
 * See also:
 *
 * PreviousLocation(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation NextLocation(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{

if (++current.offset > LastCharacterInTextBufferLine(textBuffer, current.line))
   {
   if (++current.line > LastTextBufferLine(textBuffer))
      current.line = 0;
   current.offset = 0;
   }

return (current);

} /* end of NextLocation */
/*
 * StartCurrentTextBufferWord
 *
 * The \fIStartCurrentTextBufferWord\fR function is used to locate the beginning
 * of a word in the TextBuffer relative to a given \fIcurrent\fR location.
 * The function returns the location of the beginning of the current
 * word.  Note: this return value will equal the given current value
 * if the current location is the beginning of a word.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), NextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation StartCurrentTextBufferWord(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{
BufferElement * p = wcGetTextBufferLocation(textBuffer, current.line, NULL);

if (IS_A_WORD_CHAR(p[current.offset]))
   {
   while(current.offset >= 0 && IS_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   }
else
   {
   while(current.offset >= 0 && IS_NOT_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   while(current.offset >= 0 && IS_A_WORD_CHAR(p[current.offset]))
      current.offset--;
   }

current.offset++;

return (current);


#ifdef DEBUG
current = NextLocation(textBuffer, current);
(void) fprintf(stderr, "in a CurrentText...%d.%d\n", current.line, current.offset);

return (PreviousTextBufferWord(textBuffer, current));
#endif

} /* end of StartCurrentTextBufferWord */
/*
 * EndCurrentTextBufferWord
 *
 * The \fIEndCurrentTextBufferWord\fR function is used to locate the end
 * of a word in the TextBuffer relative to a given \fIcurrent\fR location.
 * The function returns the location of the end of the current
 * word.  Note: this return value will equal the given currrent value
 * if the current location is already at the end of a word.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), NextTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation EndCurrentTextBufferWord(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{
BufferElement * p = wcGetTextBufferLocation(textBuffer, current.line, NULL);
int end = LastCharacterInTextBufferLine(textBuffer,current.line);

if (IS_A_WORD_CHAR(p[current.offset]))
   {
   while(current.offset < end && IS_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   }
else
   {
   while(current.offset < end && IS_NOT_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   while(current.offset < end && IS_A_WORD_CHAR(p[current.offset]))
      current.offset++;
   }

return (current);

} /* end of EndCurrentTextBufferWord */
/*
 * PreviousTextBufferWord
 *
 * The \fIPreviousTextBufferWord\fR function is used to locate the
 * beginning of a word in a TextBuffer relative to a given \fIcurrent\fR
 * location.  It returns the location of the beginning of the word 
 * which precedes the given current location.  If the current location
 * is within a word this function will skip over the current word.
 * If the current word is the first word in the TextBuffer the function
 * wraps to the end of the buffer.
 *
 * See also:
 *
 * PreviousTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation PreviousTextBufferWord(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{
TextLocation new;
TextLocation save;
register int i;
int done;
BufferElement * p;

save = current;
new = PreviousLocation(textBuffer, current);

p = wcGetTextBufferLocation(textBuffer, new.line, NULL);

if (IS_NOT_A_WORD_CHAR(p[new.offset]))
   {
   current = PreviousLocation(textBuffer, new);
   if (new.line != current.line)
      p = wcGetTextBufferLocation(textBuffer, current.line, NULL);

   for (done = 0; done == 0; current = new)
      {

      /* avoid endless loop */
      if (save.line == current.line && save.offset == current.offset)
         return(current);
      if (IS_A_WORD_CHAR(p[current.offset]))
         done = 1;
      new = PreviousLocation(textBuffer, current);
      if (new.line != current.line)
         p = wcGetTextBufferLocation(textBuffer, new.line, NULL);
      }
   }

for (current = new, done = 0;
     done == 0;
     current = new)
   {

   /* avoid endless loop */
   if (save.line == current.line && save.offset == current.offset)
      return(current);

#ifdef DEBUG
   (void) fprintf(stderr,"trying %d.%d %d\n",
           current.line, current.offset, p[current.offset]);
#endif
   if (IS_NOT_A_WORD_CHAR(p[current.offset]))
      done = 1;
   else
      {
      new = PreviousLocation(textBuffer, current);
      if (new.line != current.line)
         p = wcGetTextBufferLocation(textBuffer, new.line, NULL);
      }
   }

return (NextLocation(textBuffer, current));

} /* end of PreviousTextBufferWord */
/*
 * NextTextBufferWord
 *
 * The \fINextTextBufferWord\fR function is used to locate the beginning
 * of the next word from a given \fIcurrent\fR location in a TextBuffer.
 * If the current location is within the last word in the TextBuffer
 * the function wraps to the beginning of the TextBuffer.
 *
 * See also:
 *
 * PreviousTextBufferWord(3), StartCurrentTextBufferWord(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextLocation NextTextBufferWord(textBuffer, current)
TextBuffer * textBuffer;
TextLocation current;
{
TextLocation new;
register int i;
int phase;
int l;
BufferElement * p;

new = current;
phase = 0;
do
   {
   if (!(p = wcGetTextBufferLocation(textBuffer, new.line, NULL)))
      break; /* @ error! */
   else
      {
      l = _StringLength(p);
      for (i = new.offset; i < l && phase != 2; i++)
         if (IS_NOT_A_WORD_CHAR(p[i]))
            phase = 1;
         else
            if (phase == 1)
               phase = 2;
      if (phase == 2)
         new.offset = i - 1;
      else
         {
         phase = 1;
         if (++new.line > LastTextBufferLine(textBuffer))
            new.line = 0;
         new.offset = 0;
         }
      }
   } while (phase != 2 && new.line != current.line);

return (new);

} /* end of NextTextBufferWord */
/*
 * RegisterTextBufferUpdate
 *
 * The \fIRegisterTextBufferUpdate\fR procedure associates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given TextBuffer \fItext\fR.  
 * This update function will be called whenever an update operation
 * is performed on the TextBuffer.  See ReplaceBlockInTextBuffer
 * for more details.
 *
 * Note:
 *
 * Calling this function increments a reference count mechanism 
 * used to determine when to actually free the TextBuffer.  Calling 
 * the function with a NULL value for the function circumvents 
 * this mechanism.
 *
 * See also:
 * 
 * UnregisterTextBufferUpdate(3), ReadStringIntoTextBuffer(3),
 * ReadFileIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include<textbuff.h>
 *  ...
 */

extern void RegisterTextBufferUpdate(text, f, d)
TextBuffer * text;
TextUpdateFunction f;
caddr_t d;
{
register int i;

if (f != NULL)
   {
   i = text-> refcount++;

   text-> update = (TextUpdateCallback *)
      REALLOC(text-> update, text-> refcount * sizeof(TextUpdateCallback));

   text-> update[i].f = f;
   text-> update[i].d = d;
   }

} /* end of RegisterTextBufferUpdate */
/*
 * UnregisterTextBufferUpdate
 *
 * The \fIUnregisterTextBufferUpdate\fR function disassociates the 
 * TextUpdateFunction \fIf\fR and data pointer \fId\fR with the
 * given TextBuffer \fItext\fR.  If the function/data pointer pair
 * is not associated with the given TextBuffer zero is returned
 * otherwise the association is dissolved and one is returned.
 *
 * See also:
 * 
 * RegisterTextBufferUpdate(3), FreeTextBuffer(3)
 *
 * Synopsis:
 *
 * #include<textbuff.h>
 *  ...
 */

extern int UnregisterTextBufferUpdate(text, f, d)
TextBuffer * text;
TextUpdateFunction f;
caddr_t d;
{
register int i;
int retval;

for (i = 0; i < text-> refcount; i++)
   if (text-> update[i].f == f && text-> update[i].d == d)
      break;

if (i == text-> refcount)
   retval = 0;
else
   {
   text-> refcount--;
   if (text-> refcount == 0)
      {
      FREE(text-> update);
      text-> update = (TextUpdateCallback *)NULL;
      }
   else
      {
      if (i < text-> refcount)
         memmove(&text-> update[i],
                 &text-> update[i+1],
                 (text-> refcount - i) * sizeof(TextUpdateCallback));
      text-> update = (TextUpdateCallback *)
         REALLOC(text-> update, text-> refcount * sizeof(TextUpdateCallback));
      }
   retval = 1;
   }

return (retval);

} /* end of UnregisterTextBufferUpdate */
/*
 * InitTextUndoList
 *
 */

static void InitTextUndoList(text)
TextBuffer * text;
{

text-> insert.string =
text-> deleted.string = (BufferElement *) NULL;

} /* end of InitTextUndoList */
/*
 * FreeTextUndoList
 *
 */

static void FreeTextUndoList(text)
TextBuffer * text;
{

if (text-> insert.string != (BufferElement *) NULL)
   FREE(text-> insert.string);

if (text-> deleted.string != (BufferElement *) NULL)
   FREE(text-> deleted.string);

InitTextUndoList(text);

} /* end of FreeTextUndoList */
/*
 * RegisterTextBufferScanFunctions
 *
 * The \fIRegisterTextBufferScanFunctions\fR procedure provides the capability
 * to replace the scan functions used by the ForwardScanTextBuffer and
 * BackwardScanTextBuffer functions.  These functions are called as:~
 *
 * .so CWstart
 * 	(*forward)(string, curp, expression);
 * 	(*backward)(string, curp, expression);
 * .so CWend
 *
 * and are responsible for returning either a pointer to the begining
 * of a match for the expression or NULL.
 *
 * Calling this procedure with NULL function pointers reinstates the
 * default regular expression facility.
 * 
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern void RegisterTextBufferScanFunctions(forward, backward)
char * (*forward)();
char * (*backward)();
{

Strexp  = forward  ? forward  : strexp;
Strrexp = backward ? backward : strrexp;

} /* end of RegisterTextBufferScanFunctions */
/*
 * RegisterTextBufferWordDefinition
 *
 * The \fIRegisterTextBufferWordDefinition\fR procedure provides the 
 * capability to replace the word definition function used by the 
 * TextBuffer Utilities.  This function is called as:~
 *
 * .so CWstart
 * 	(*word_definition)(c);
 * .so CWend
 *
 * The function is responsible for returning non-zero if the character
 * c is considered a character that can occur in a word and zero
 * otherwise.
 *
 * Calling this function with NULL reinstates the default word definition
 * which allows the following set of characters: a-zA-Z0-9_
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern void RegisterTextBufferWordDefinition(word_definition)
int (*word_definition)();
{

IsAWordChar = word_definition ? word_definition : is_a_word_char;

} /* end of RegisterTextBufferWordDefinition */
/*
 * is_a_word_char
 *
 */

static int is_a_word_char(c)
int c;
{
int retval;

#ifdef I18N

retval = iswalnum(c) || c == '_' || isphonogram(c) || isideogram(c);

#else
retval = 
   ('a' <= c && c <= 'z') || 
   ('A' <= c && c <= 'Z') || 
   ('0' <= c && c <= '9') ||
   (c == '_');

#endif
return (retval);
} /* end of is_a_word_char */


/*
 * wcReadStringIntoTextBuffer
 *
 * The \fIwcReadStringIntoTextBuffer\fR function is used to copy the
 * given (wide character) \fIstring\fR into a newly allocated 
 * TextBuffer.  The supplied TextUpdateFunction and data pointer 
 * are associated with this TextBuffer.
 *
 * See also:
 *
 * ReadFileIntoTextBuffer(3), ReadStringIntoTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern TextBuffer * wcReadStringIntoTextBuffer(string, f, d)
BufferElement * string;
TextUpdateFunction f;
caddr_t d;
{
Buffer * sp;
Buffer * work;
TextBuffer * text = AllocateTextBuffer(NULL, f, d);

#ifdef DEBUG
printf("wcReadStringIntoTextBuffer: input string=");
_Print_wc_String(string);
#endif
sp = wcstropen(string);
if (sp != NULL)
   {
   work = AllocateBuffer(sizeof(work-> p[0]), LNMIN); 

   while(ReadStringIntoBuffer(sp, work) != EOF)
   {  
#ifdef DEBUG
      printf("Next portion of string=");
      _Print_wc_String(work->p);
#endif
      (void) AppendLineToTextBuffer(text, work);
   }
#ifdef DEBUG
   printf("Next portion of string=");
   _Print_wc_String(work->p);
#endif
   (void) AppendLineToTextBuffer(text, work);

   FreeBuffer(work);
   strclose(sp);
   (void) wcGetTextBufferLocation(text, 0, (TextLocation *)NULL);
   }

#ifdef DEBUG
printf("wcReadStringIntoTextBuffer [LEAVE]\n");
#endif
return (text);

} /* end of wcReadStringIntoTextBuffer */
/*
 * wcGetTextBufferLocation
 *
 * The \fIwcGetTextBufferLocation\fR function is used to retrieve
 * the contents of the given line within the TextBuffer.  It
 * returns a pointer to the character string.  If the line
 * number is invalid a NULL pointer is returned.  If a non-NULL TextLocation
 * pointer is supplied in the argument list the contents of this
 * structure are modified to reflect the values corresponding to
 * the given line.
 *
 * See also:
 *
 * GetTextBufferBlock(3), wcGetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern BufferElement * wcGetTextBufferLocation(text, line_number, location)
TextBuffer * text;
TextLine line_number;
TextLocation * location;
{
if (line_number < 0 || line_number >= text-> lines.used)
   return (NULL);

if (text-> lines.p[line_number].pageindex != text-> curpageno)
   SwapIn(text, text-> lines.p[line_number].pageindex);

text-> buffer = text-> lines.p[line_number].buffer;

if (location)
   {
   location-> line   = line_number;
   location-> offset = 0;
   location-> buffer = text-> buffer-> p;
   }

#ifdef DEBUG
printf("wcGetTextBufferLocation:\n");
printf("\tline_number:%d",line_number);
printf("\tstring:");
_Print_wc_String(text-> buffer-> p);
#endif
return (text-> buffer-> p);

} /* wcGetTextBufferLocation */
/*
 * wcReplaceBlockInTextBuffer
 *
 * The \fIwcReplaceBlockInTextBuffer\fR function is used to update the contents
 * of the TextBuffer associated with \fItext\fR.  The characters stored
 * between \fIstartloc\fR and \fIendloc\fR are deleted and the \fIstring\fR
 * is inserted after \fIstartloc\fR.  If the edit succeeds the 
 * TextUpdateFunction \fIf\fR is called with the parameters: \fId\fR, 
 * \fItext\fR, and 1; then any other \fBdifferent\fR update functions
 * associated with the TextBuffer are called with their associated data
 * pointer, \fItext\fR, and 0.
 *
 * This function records the operation performed in TextUndoItem structures.
 * The contents of these structures can be used to implement an undo function.
 * The contents can also be used to determine the type of operation performed.
 * A structure is allocated for both the delete and insert information.
 *
 * The hints provided in these structures is the inclusive or of:~
 *
 *    TEXT_BUFFER_NOP
 *    TEXT_BUFFER_INSERT_LINE
 *    TEXT_BUFFER_INSERT_SPLIT_LINE
 *    TEXT_BUFFER_INSERT_CHARS
 *    TEXT_BUFFER_DELETE_START_LINE
 *    TEXT_BUFFER_DELETE_END_LINE
 *    TEXT_BUFFER_DELETE_START_CHARS
 *    TEXT_BUFFER_DELETE_END_CHARS
 *    TEXT_BUFFER_DELETE_JOIN_LINE
 *    TEXT_BUFFER_DELETE_SIMPLE
 *
 * See also:
 *
 * ReplaceCharInTextBuffer(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern EditResult wcReplaceBlockInTextBuffer(text, startloc, endloc, string, f, d)
TextBuffer * text;
TextLocation * startloc;
TextLocation * endloc;
BufferElement * string;
TextUpdateFunction f;
caddr_t d;
{
register int i;
EditResult retval;

FreeTextUndoList(text);

if (startloc-> line != endloc-> line || startloc-> offset != endloc-> offset)
   text-> deleted.string = wcGetTextBufferBlock
      (text, *startloc, PreviousLocation(text, *endloc));
if (string != (BufferElement *) NULL && *string != (BufferElement) '\0')
   text-> insert.string = 
              _StringCopy(string, 
              (BufferElement *) MALLOC(sizeof(BufferElement) 
              * (_StringLength(string) + 1)));

text-> insert.hint =
text-> deleted.hint = 0;

text-> deleted.start =
text-> insert.start =
text-> insert.end   = *startloc;
text-> deleted.end   = *endloc;

retval = DeleteBlockInTextBuffer(text, startloc, endloc);

if (retval == EDIT_SUCCESS)
   {
   retval = InsertBlockIntoTextBuffer(text, startloc, string);
   if (retval == EDIT_SUCCESS)
      {
      text-> dirty = 1;
      text-> insert.end = *startloc;
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f == f && text-> update[i].d == d)
            {
            (*text-> update[i].f) (text-> update[i].d, text, 1);
            break;
            }
      for (i = 0; i < text-> refcount; i++)
         if (text-> update[i].f != f || text-> update[i].d != d)
            (*text-> update[i].f) (text-> update[i].d, text, 0);
      }
   }
else
   retval = EDIT_FAILURE;

return (retval);

} /* end of wcReplaceBlockInTextBuffer */
/*
 * wcGetTextBufferLine
 *
 * The \fIwcGetTextBufferLine\fR function is used to retrieve the contents of
 * \fIline\fR from the \fItext\fR TextBuffer.  It returns a pointer to a 
 * string containing the copy of the contents of the line or NULL if the 
 * \fIline\fR is outside the range of valid lines in \fItext\fR.
 * On systems with wide character (wchar_t) support, the string is
 * a wide character string.
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferBlock(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferBlock(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern BufferElement * 
wcGetTextBufferLine(text, lineindex)
TextBuffer * text;
TextLine lineindex;
{
register BufferElement * p = wcGetTextBufferLocation(text, 
                                                   lineindex, 
                                                   (TextLocation *)NULL);

if (p != (BufferElement *) NULL)
   p = _StringCopy(p, (BufferElement *) MALLOC((unsigned)
          (sizeof(BufferElement) * (_StringLength(p) + 1))));

return (p);

} /* end of wcGetTextBufferLine */
/*
 * wcCopyTextBufferBlock
 *
 * The \fIwcCopyTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_position\fR and \fIend_position\fR inclusive.  It
 * returns the number of BufferElements copied; if the parameters are invalid
 * the return value is zero (0).  
 *
 * Note:
 *
 * The storage for the copy is allocated by the caller.  It is the 
 * responsibility of the caller to ensure that enough storage is allocated
 * to copy end_position - start_position + 1 BufferElements.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern int wcCopyTextBufferBlock(text, buffer, start_position, end_position)
TextBuffer * text;
BufferElement * buffer;
TextPosition start_position;
TextPosition end_position;
{
int retval;
TextLocation start_location;
TextLocation end_location;
BufferElement * p = buffer;

start_location = LocationOfPosition(text, start_position);
end_location = LocationOfPosition(text, end_position);

if (end_position < start_position ||
    start_location.buffer == NULL || end_location.buffer == NULL)
   retval = 0;
else
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   BufferElement * t = wcGetTextBufferLocation(text, start_location.line,
                                            (TextLocation *)NULL);
   while (outpos < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = wcGetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            p[outpos++] = t[inpos++];
            break;
         }
      }
   retval = size;
   p[outpos] = '\0';
   }

return (retval);

} /* end of wcCopyTextBufferBlock */
/*
 * wcGetTextBufferBlock
 *
 * The \fIwcGetTextBufferBlock\fR function is used to retrieve a text block
 * from the \fItext\fR TextBuffer.  The block is defined as the characters
 * between \fIstart_location\fR and \fIend_location\fR inclusive.  It
 * returns a pointer to a string containing the copy.  On systems with 
 * wide character (wchar_t) support, the copy is a wide character string.
 * If the parameters are invalid NULL is returned.  
 *
 * Note:
 *
 * The storage for the copy is allocated by this routine.  It is the 
 * responsibility of the caller to free this storage when it becomes
 * dispensible.
 *
 * See also:
 *
 * GetTextBufferLocation(3), GetTextBufferChar(3), GetTextBufferLine(3)
 * wcGetTextBufferLocation(3), wcGetTextBufferLine(3)
 *
 * Synopsis:
 *
 * #include <textbuff.h>
 *  ...
 */

extern BufferElement * 
wcGetTextBufferBlock(text, start_location, end_location)
TextBuffer * text;
TextLocation start_location;
TextLocation end_location;
{
BufferElement * p  = NULL;
TextPosition start_position = PositionOfLocation(text, start_location);
TextPosition end_position   = SameTextLocation(start_location, end_location) ?
   start_position : PositionOfLocation(text, end_location);

if (start_position != (TextPosition)EOF && end_position != (TextPosition)EOF &&
    start_position <= end_position)
   {
   TextPosition size      = end_position - start_position + 1;
   TextLine     lineindex = start_location.line;
   TextPosition inpos     = start_location.offset;
   TextPosition outpos    = 0;
   BufferElement * t = wcGetTextBufferLocation(text, start_location.line,
                                             (TextLocation *)NULL);

   p = (BufferElement *) CALLOC((unsigned)size + 1, sizeof(BufferElement));
   while (outpos < size)
      {
      switch(t[inpos])
         {
         case '\0':
            p[outpos++] = '\n';
            t = wcGetTextBufferLocation(text, ++lineindex, (TextLocation *)NULL);
            inpos = (TextPosition)0;
            break;
         default:
            p[outpos++] = t[inpos++];
            break;
         }
      }
   p[outpos] = '\0';
   }

return (p);

} /* end of wcGetTextBufferBlock */

/*
 *
 *  _StringLength: 
 *
 *  Calculate the length, in BufferElements, of a string of BufferElements.
 *  The string is assumed to be terminated with a zero value BufferElement.
 *  The terminating BufferElement is not included in the count.
 */
#if NeedFunctionPrototypes
extern int
_StringLength(
   BufferElement *string
)
#else
extern int
_StringLength(string)
BufferElement *string;
#endif
{
register BufferElement *ptr = string;

while (*ptr != (BufferElement) '\0')
	ptr++;
return((int)(ptr - string));
} /* end of _StringLength */


/*
 *
 *  _StringCopy:
 *
 *  Copy a string of BufferElements to another memory location.
 *  The NULL terminator is also copied.
 *
 */
  
#if NeedFunctionPrototypes
extern BufferElement *
_StringCopy(
   BufferElement *from,
   BufferElement *to
)
#else
extern BufferElement *
_StringCopy(from,to)
BufferElement *from;
BufferElement *to;
#endif
{
register BufferElement *dest = to;
register BufferElement *src  = from;

while (*src)
	*dest++ = *src++;
*dest = NULL;

return(to);
} /* end of _StringCopy */

#ifdef DEBUG
#if NeedFunctionPrototypes
extern void
_Print_wc_String(
BufferElement *wc_string
)
#else
extern void
_Print_wc_String(wc_string)
BufferElement *wc_string;
#endif
{

#ifdef I18N
char c[4];
wchar_t wc;
int i;

if (!wc_string)
   printf("NULL");
else
   for (i=0; wc_string[i] != (BufferElement) 0; i++){
      wc = wc_string[i];
      c[0] = c[1] = c[2] = c[3] = '\0';
      wctomb(c,wc);
      printf("%s",c);
   }
printf("\n");

#else
if (!wc_string)
   printf("NULL\n");
else
   printf("%s\n",wc_string);
#endif
}
#endif
