/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olmisc:buffutil.c	1.12"
#endif

/*
 * buffutil.c
 *
 */

#include <stdio.h>
#include <string.h>
#include <buffutil.h>
#include <X11/memutil.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>	/* includes Error.h */


#ifdef I18N
static wchar_t _empty_wchar = (wchar_t) 0;
#endif


/*
 * AllocateBuffer
 *
 * The \fIAllocateBuffer\fR function allocates a Buffer for elements
 * of the given \fIelement_size\fR.
 * The used member of the Buffer is set to zero and
 * the size member is set to the value of \fIinitial_size\fR.
 * If \fIinitial_size\fR is zero the pointer p is set to NULL, otherwise
 * the amount of space required (\fIinitial_size\fR * \fIelement_size\fR)
 * is allocated and the pointer p is set to point to this space.
 * The function returns the pointer to the allocated Buffer.
 *
 * Private:
 *
 * This function is used to allocate a new Buffer.  The algorithm is -
 *
 *  1. Allocate a buffer structure.
 *  2. Set the element size to the size specified by the caller.
 *  3. Set the size and used elements in this structure to zero(0).
 *  4. Set the pointer to NULL.
 *  5. If the caller specified an initial size (which is a number of
 *     elements) then call GrowBuffer to expand the buffer to the
 *     specified extent.
 *
 * See also:
 *
 * FreeBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern Buffer * AllocateBuffer(element_size, initial_size)
int element_size;
int initial_size;
{
Buffer * b = (Buffer *) MALLOC(sizeof(Buffer));

#ifdef DEBUG
printf("Allocating buffer with Element Size = %d\n",element_size);
#endif

b-> esize = element_size;
b-> size = b-> used = 0;
b-> p = NULL;

if (initial_size != 0)
   GrowBuffer(b, initial_size);

return (b);

} /* end of AllocateBuffer */
/*
 * GrowBuffer
 *
 * The \fIGrowBuffer\fR procedure is used to expand (or compress) a 
 * given \fIbuffer\fR size by \fIincrement\fR elements.  If the increment
 * is negative the operation results in a reduction in the size
 * of the Buffer.
 *
 * Private:
 *
 * This procedure is used to extend a given buffer by a given number
 * of elements.  It assumes that the element size is stored in the
 * buffer and uses this value to calculate the amount of space required.
 * The algorithm is -
 *
 *  1. Increment the buffer size by the specified value.
 *  2. Allocate (or reallocate) storage for the buffer pointer.
 *
 * See also:
 *
 * AllocateBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void GrowBuffer(b, increment)
Buffer * b;
int increment;
{
b-> size += increment;

if (b-> size == increment)
   b-> p = (BufferElement *)MALLOC((unsigned)(b-> size * b-> esize));
else
   b-> p = (BufferElement *)REALLOC(b-> p, (unsigned)(b-> size * b-> esize));

} /* end of GrowBuffer */
/*
 * CopyBuffer
 *
 * The \fICopyBuffer\fR function is used to allocate a new
 * Buffer with the same attributes as the given \fIbuffer\fR and to copy
 * the data associated with the given \fIbuffer\fR into the new Buffer.
 * A pointer to the newly allocated and initialized Buffer is returned.
 *
 * Private:
 *
 * This function creates a copy of a given buffer and returns the
 * pointer to the new buffer to the caller.  That is, this routine
 * is used to "clone" a buffer.
 *
 * See also:
 *
 * AllocateBuffer(3), FreeBuffer(3), InsertIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern Buffer * CopyBuffer(buffer)
Buffer * buffer;
{
Buffer * newbuffer = AllocateBuffer(buffer-> esize, buffer-> size);

#ifdef DEBUG
printf("CopyBuffer [ENTER]: used = %d\tstring=",buffer->used);
_Print_wc_String(buffer->p);
#endif

newbuffer-> used = buffer-> used;
(void) memcpy(newbuffer-> p, buffer-> p, buffer-> used * buffer-> esize);

#ifdef DEBUG
printf("CopyBuffer: copy = ");
_Print_wc_String(newbuffer->p);
printf("CopyBuffer [LEAVE]\n");
#endif

return (newbuffer);

} /* end of CopyBuffer */
/*
 * FreeBuffer
 *
 * The \fIFreeBuffer\fR procedure is used to deallocate (free)
 * storage associated with the given \fIbuffer\fR pointer.
 *
 * Private:
 *
 * This procedure is used to free the storage associate with a Buffer.
 * It simply frees the storage in the buffer and the buffer itself.
 * It checks the pointer in the buffer (since AllocateBuffer sets
 * it to NULL and the caller may never have called GrowBuffer to
 * allocate space within the buffer).  It presumes that the given
 * buffer pointer itself is valid, however.
 *
 * See also:
 *
 * AllocateBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void FreeBuffer(buffer)
Buffer * buffer;
{

if (buffer-> p)
   FREE(buffer-> p);
FREE(buffer);

} /* end of FreeBuffer */
/*
 * InsertIntoBuffer
 *
 * The \fIInsertIntoBuffer\fR function is used to insert the
 * elements stored in the \fIsource\fR buffer into the \fItarget\fR
 * buffer \fIbefore\fR the element stored at \fIoffset\fR.
 * If the \fIoffset\fR is invalid or if the \fIsource\fR buffer is
 * empty the function returns zero otherwise it returns one after
 * completing the insertion.
 *
 * Private:
 *
 * This function is used to insert a source buffer into a target buffer
 * at a specified offset.  The algorithm used is -
 *
 *  1. Check the offset for range in the target and is source used for
 *     greater than zero(0).  If the chaeck fails return failure.
 *  2. Calculate the amount of space needed above what is available
 *     in the target.  If more space is needed grow the target buffer
 *     by this amount PLUS the LNINCRE amount (to avoid trashing).
 *  3. Check if the operation is a simple append (and if so append the
 *     source to the target, otherwise shift the target (so that the
 *     source will fit) and then copy the source into the target.
 *  4. Return success.
 *
 * See also:
 *
 * ReadStringIntoBuffer(3), ReadFileIntoBuffer(3), BufferMacros(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int InsertIntoBuffer(target, source, offset)
Buffer * target;
Buffer * source;
int offset;
{
if (offset < 0 || offset > target-> used || source-> used <= 0)
   return (0);
else
   {
   int space_needed = source-> used - (target-> size - target-> used);
#ifdef DEBUG
fprintf(stderr, "SRC = %d(%d) '%s' DST = %d(%d) '%s'\n",
source-> used, source-> size, source-> p,
target-> used, target-> size, target-> p);
#endif
   if (space_needed > 0)
      GrowBuffer(target, space_needed + LNINCRE);

   if (offset != target-> used)
      memmove((char *)target->p + (offset + source-> used - 1) * target->esize,
              (char *)target->p + offset * target->esize,
              (target-> used - offset) * target-> esize);

   memmove((char *)target->p + offset * target->esize,
	   source-> p,
           (source-> used - 1) * target-> esize);

   target-> used += (source-> used - 1);
#ifdef DEBUG
fprintf(stderr, "SRC = %d(%d) '%s' DST = %d(%d) '%s'\n",
source-> used, source-> size, source-> p,
target-> used, target-> size, target-> p);
#endif

   return (1);
   }
} /* end of InsertIntoBuffer */
/*
 * stropen
 *
 * The \fIstropen\fR function copies the character \fIstring\fR into a
 * newly allocated Buffer.  This string buffer can be \fIread\fR using the
 * \fIstrgetc\fR function and \fIclosed\fR using the \fIstrclose\fR procedure.
 *
 * Private:
 *
 * This function "opens" a string for "reading".  It basically
 * "bufferizes" a given string.  This buffer can be intelligently
 * accessed using the strgetc function and "closed" using the strclose
 * procedure.  The algorithm is -
 *
 *  1. If the string pointer passed in is NULL then set it to a NULL STRING.
 *  2. Allocate a buffer large enough to accommodate the string.
 *  3. Copy the string into the buffer.
 *  4. Decrement size (to avoid permitting reading past the end
 *     of the string).
 *  5. Return the buffer pointer.
 *
 * See also:
 *
 * strclose(3), strgetc(3), wcstropen(3)
 *
 * Synopsis:
 *
 *#include <buffuti.h>
 *  ...
 */

extern Buffer * stropen(string)
char * string;
{
Buffer *     sp = NULL;
register int l  = 0;


if (string == NULL)
   string = "";

#ifdef I18N
l = _mbstrlen(string) + 1;
if (l == 0)
   l = 1;
#else
l = strlen(string) + 1;
#endif

sp = AllocateBuffer(sizeof(sp->p[0]), l);

#ifdef I18N
      /* convert string from multibyte to wide character format */
if (mbstowcs(sp->p, string, l) == -1)
   {
	OlVaDisplayWarningMsg((Display *)NULL, OleNfileBuffutil,
		OleTmsg1, OleCOlToolkitWarning,
		OleMfileBuffutil_msg1,
		string);
     /*
      * an illegal multibyte string was found, place a null character
      * in the buffer
      */
   sp->p[0] = (BufferElement) NULL;
   }
#else

for (--l; l >= 0; l--)
   sp-> p[l] = string[l];
#endif

sp-> size--;

#ifdef DEBUG
printf("stropen:%s",string);
#endif

return (sp);
} /* end of stropen */
/*
 * strgetc
 *
 * The \fIstrgetc\fR function is used to read the next character
 * stored in the string \fIbuffer\fR.
 * The function returns the next character in the Buffer.
 * When no characters remain the routine returns EOF.
 *
 * Private:
 *
 * This function returns the next character "read" from a string
 * buffer "opened" using stropen.  It returns EOF when the string is
 * exhaused (and it is an error to continue reading once the end is
 * reached).  The algorithm is -
 *
 *  1. If the used counter has reached the size (BufferFilled) then
 *     set c to EOF else set c to the next character in the buffer
 *     and increment the used counter.
 *  2. Return c.
 *
 * See also:
 *
 * stropen(3), strclose(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int strgetc(sp)
Buffer * sp;
{
int c;

if (BufferFilled(sp))
   c = EOF;
else
   c = sp-> p[sp-> used++];

#ifdef NO_DEBUG
printf("strgetc:");
if (c == EOF)
   printf("EOF\n");
else
   printf("%c\n",c & 0x7f);
#endif
return (c);

} /* end of strgetc */
/*
 * strclose
 *
 * The \fIstrclose\fR procedure is used to close a string Buffer
 * which was opened using \fIstropen\fR or \fIwcstropen\fR.
 *
 * Private:
 *
 * This procedure "closes" a string "opened" using stropen.  Note: it
 * simply calls FreeBuffer to perform the necessary frees and can
 * be repalced by a macro definition.
 *
 * See also:
 *
 * stropen(3), strgetc(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern void strclose(sp)
Buffer * sp;
{

FreeBuffer(sp);

} /* end of strclose */
/*
 * ReadStringIntoBuffer
 *
 * The \fIReadStringIntoBuffer\fR function reads the buffer associated with
 * \fIsp\fR and inserts the characters read into \fIbuffer\fR.
 * The read operation terminates when either EOF is returned when
 * reading the buffer or when a NEWLINE is encountered.  The function
 * returns the last character read to the caller (either EOF or NEWLINE).
 *
 * Private:
 *
 * This function "reads" an "opened" buffer into a given buffer.  It
 * copies characters from the input into the buffer until either EOF
 * or a NEWLINE is reached.  It performs any necessary expansion of the
 * output buffer during processing and returns the character which
 * caused the operation to cease (EOF or '\n').  This return value
 * can be used by the caller to determine when to stop reading the
 * input (and close the buffer).
 *
 * See also:
 *
 * ReadFileIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int ReadStringIntoBuffer(sp, buffer)
Buffer * sp;
Buffer * buffer;
{
int c;

buffer-> used = 0;

#ifdef DEBUG
printf("ReadStringIntoBuffer:");
_Print_wc_String(sp->p);
#endif

for(;;)
   {
   c = strgetc(sp);
   if (BufferFilled(buffer))
      GrowBuffer(buffer, LNINCRE);
   if (c == (BufferElement) '\n' || c == EOF)
      {
      buffer-> p[buffer-> used++] = (BufferElement) '\0';
      break;
      }
   else
      buffer-> p[buffer-> used++] = (BufferElement) c;
   }


return (c);

} /* end of ReadStringIntoBuffer */
/*
 * ReadFileIntoBuffer
 *
 * The \fIReadFileIntoBuffer\fR function reads the file associated with
 * \fIfp\fR and inserts the characters read into the \fIbuffer\fR.
 * The read operation terminates when either EOF is returned when
 * reading the file or when a NEWLINE is encountered.  The function
 * returns the last character read to the caller (either EOF or NEWLINE).
 *
 * Private:
 *
 * This function reads an opened file into a given buffer.  It
 * copies characters from the input into the buffer until either EOF
 * or a NEWLINE is reached.  It performs any necessary expansion of the
 * output buffer during processing and returns the character which
 * caused the operation to cease (EOF or '\n').  This return value
 * can be used by the caller to determine when to stop reading the
 * input (and close the file).  Note: the logic here differs from the
 * ReadStringIntoBuffer function since it is assumed that the last line
 * in the file is NEWLINE terminated and the string (in the ...String...
 * function) is not.
 *
 * See also:
 *
 * ReadStringIntoBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *  ...
 */

extern int ReadFileIntoBuffer(fp, buffer)
FILE * fp;
Buffer * buffer;
{
int c;

#ifdef I18N
wchar_t wc;

buffer-> used = 0;
while ((wc = getwc(fp)) != (wchar_t) EOF)
   {
   if (BufferFilled(buffer))
      GrowBuffer(buffer, LNINCRE);
   if (wc == (BufferElement) '\n')
      {
      buffer-> p[buffer-> used++] = (BufferElement) '\0';
      break;
      }
   else
      buffer-> p[buffer-> used++] = wc;
   }
c = wc;
#else

buffer-> used = 0;

while ((c = fgetc(fp)) != EOF)
   {
   if (BufferFilled(buffer))
      GrowBuffer(buffer, LNINCRE);
   if (c == '\n')
      {
      buffer-> p[buffer-> used++] = '\0';
      break;
      }
   else
      buffer-> p[buffer-> used++] = c;
   }
#endif
return (c);

} /* end of ReadFileIntoBuffer */

/*
 *	_mbstrlen
 *
 * Private: 
 *
 *	This function is used to determine the number of characters in a
 * multibyte string, where each character may take up more than
 * a single byte.  If an invalid multibyte character is encountered
 * the function returns -1.  The string must be null-terminated.  The
 * null-terminator is not included in the character count.
 */

#ifdef I18N
#if OlNeedFunctionPrototypes
extern int 
_mbstrlen(
	char *	mbstring
)
#else
extern int 
_mbstrlen(mbstring)
char *mbstring;
#endif
{
	char *cp = mbstring;
	int retval = 0;
	int byte_index = 0;
	int len;

	while (*cp != NULL){
		len = mblen(cp, sizeof(wchar_t));
		if (len < 0){
			retval = -1;
			break;
		}
		else{
			retval++;
			byte_index += len;
			cp = &mbstring[byte_index];
		}
	}
	return(retval);
} /* end of _mbstrlen */
#endif




/*
 * wcstropen
 *
 * The \fIwcstropen\fR function copies the \fIstring\fR of BufferElements into a
 * newly allocated Buffer.  This string buffer can be \fIread\fR using the
 * \fIstrgetc\fR function and \fIclosed\fR using the \fIstrclose\fR procedure.
 *
 * Private:
 *
 * This function "opens" a string for "reading".  It basically
 * "bufferizes" a given string.  This buffer can be intelligently
 * accessed using the strgetc function and "closed" using the strclose
 * procedure.  The algorithm is -
 *
 *  1. If the string pointer passed in is NULL then set it to a NULL STRING.
 *  2. Allocate a buffer large enough to accommodate the string.
 *  3. Copy the string into the buffer.
 *  4. Decrement size (to avoid permitting reading past the end
 *     of the string).
 *  5. Return the buffer pointer.
 *
 * See also:
 *
 * strclose(3), strgetc(3), stropen(3)
 *
 * Synopsis:
 *
 *#include <buffuti.h>
 *  ...
 */

extern Buffer * wcstropen(string)
BufferElement * string;
{
Buffer *     sp = NULL;
register int l  = 0;

if (string == NULL)
#ifdef I18N
   string = (BufferElement *) &_empty_wchar;
#else
   string = (BufferElement *)"";
#endif

while (string[l] != (BufferElement) NULL)
   l++;
l++;
sp = AllocateBuffer(sizeof(sp->p[0]), l);
for (--l; l >= 0; l--)
   sp-> p[l] = string[l];
sp-> size--;

#ifdef DEBUG
printf("wcstropen:");
_Print_wc_String(string);
#endif

return (sp);

} /* end of wcstropen */
