#ifndef NOIDENT
#ident	"@(#)memutil:memutil.c	1.9"
#endif

/*
 * memutil.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
      
typedef void (*VoidFunctionPointer)();
/*
 * Note: can't include memutil.h here
 */

/*#define DEBUG/**/

#define UseMemUtil

#define MemutilLeaks    1
#define MemutilRuntime  2
#define MemutilOverflow 6

#define GUARD           0xffffffaa

#ifdef UseMemUtil

#   define QPRINTF(x)  if (debugging & MemutilLeaks) (void) fprintf x
#   define REMOVE(x,y,z) if (debugging & MemutilRuntime) Remove(x,y,z)
#   define ADD(w,x,y,z) if (debugging & MemutilRuntime) Add(w,x,y,z)

static void   Add();
static void   Remove();

#else                                                                       

#   define QPRINTF(x)
#   define ADD(w,x,y,z)
#   define REMOVE(x,y,z)

#endif

typedef struct _MemoryPointer
   {
   char *       p;
   unsigned int size;
   char *       file;
   unsigned int line;
   } MemoryPointer;                                                         

static MemoryPointer * memory       = (MemoryPointer *)0;
static int             memory_index = 0;
static int             memory_size  = 0;

static const char * M_format = "%x (M) %5d             at %5d in %s\n";
static const char * F_format = "%x (F)                   at %5d in %s\n";
static const char * C_format = "%x (C) %5d %5d %5d at %5d in %s\n";
static const char * R_format = "%x (R) %5d             at %5d in %s\n";
static const char * f_format = "%x <F>                   at %5d in %s\n";
static const char * m_format = "%x <M> %5d             at %5d in %s\n";

static char   debugging = 0;


/*
 * CheckMemutil
 *
 */

extern void
CheckMemutil()
{
   int i;
   int flag = 0;

   for (i = 0; i < memory_index; i++)
      if ((char)memory[i].p[memory[i].size] != GUARD)
      {
         flag = 1;
         fprintf(stderr, "memutil: error %x overwritten (%s: %d)\n", 
         memory[i].p, memory[i].file, memory[i].line);
      }

} /* end of CheckMemutil */
/*                                                                           
 * _GetMemutilDebug
 *
 */

extern int
_GetMemutilDebug()
{

   return (debugging);

} /* end of _GetMemutilDebug */                                             
/*
 * _SetMemutilDebug
 *
 * The \fI_SetMemutilDebug\fR procedure is used to turn the debugging
 * mode of the memutil(3) function set to the logical state of \fIflag\fR.
 * When the debugging mode is on, debugging information is sent to stderr
 * indicating the address which is being freed or allocated, the file
 * and line which initiated the memory request.  This output can be analyzed
 * using the checkmem utility) to find memory leaks and inappropriate
 * free requests.
 *
 * See also:
 *
 * _MUFree(3), _MUCalloc(3), _MUMalloc(3), _MURealloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void
_SetMemutilDebug(flag)
int flag;
{

   debugging = (char)flag;

   if (debugging & MemutilOverflow)
      signal(SIGUSR2, (VoidFunctionPointer) CheckMemutil);

} /* end of _SetMemutilDebug */
/*
 *
 * InitializeMemutil()
 *
 */

extern void
InitializeMemutil()
{
   char * memutil = (char *)getenv("MEMUTIL");

   _SetMemutilDebug(memutil ? atoi(memutil) : 0);

} /* end of InitializeMemutil */
/*                                                                           
 * _MURegisterMalloc
 *
 * The \fI_MURegisterMalloc\fR procedure is used with the rest of the
 * memory allocation tracking utilities to record (in the standard format)
 * memory allocations performed by other c library routines (such as         
 * getcwd()).
 *
 * Note: A utility \fIcheckmem\fR can be used to analyze the memory
 *       management activity reported by the memutil function set.
 *
 * See also:
 *                                                                           
 * _MUCalloc(3), _MUMalloc(3), _MURealloc(3)
 *
 * Synopsis:                                                                 
 *
 *#include <memutil.h>
 * ...
 */

extern void
_MURegisterMalloc(p, file, line)
void * p;
char * file;                                                                
int    line;
{
   int size = 1;

   ADD(p, size, file, line);

   QPRINTF((stderr, M_format, p, size, line, file));

} /* end of _MURegisterMalloc */
/*
 * _MUFree
 *
 * The \fI_MUFree\fR procedure is an enhanced version of free(3).  It 
 * is normally used with the FREE macro.  It ensures that any attempts 
 * to free storage using a NULL pointer are reported.  To accomodate
 * versions of free that allow freeing of NULL (e.g. XtFree), a flag 
 * (reportNull) determines whether that condition (freeing NULL) is printed.
 * \fI_MUFree\fR can also be used as a debugging aid to report any FREE
 * requests (including the file and line where the request originated).  
 
 *
 * Note: A utility \fIcheckmem\fR can be used to analyze the memory
 *       management activity reported by the memutil function set.
 *
 * See also:
 *
 * _MUCalloc(3), _MUMalloc(3), _MURealloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void
_MUFree(ptr, file, line, reportNullFree)
void *  ptr;
char *  file;
int     line;
char    reportNullFree;
{

   if (ptr)
   {
      REMOVE(ptr, file, line);
      QPRINTF((stderr, F_format, ptr, line, file));
      free(ptr);
   }
   else
   {
      if (reportNullFree)
         (void) fprintf(stderr, "freeing storage-> NULL in %s at %d!!!\n", 
                         file, line);
   }

} /* end of _MUFree */
/*
 * _MUCalloc
 *
 * The \fI_MUCalloc\fR function is an enhanced version of calloc(3).  It 
 * is normally used with the macro CALLOC and is used to verify the success of
 * allocating storage; reporting the file and line that results in
 * a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocated.
 *
 * Return Value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _MUFree(3), _MUMalloc(3), _MURealloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void *
_MUCalloc(n, size, file, line)
unsigned n;
unsigned size;
char *   file;
int      line;
{
   char * p = (char *) calloc(n * size + 1, 1);

   if (p == NULL)
   {
        (void) fprintf
           (stderr,"couldn't calloc %d elements of size %d in %s at %d!!!\n",
            n, size, file, line);
        abort();
      /* NOTREACHED */
   } 
   else
   {
      ADD(p, n * size, file, line);
      QPRINTF((stderr, C_format, p, n * size, n, size, line, file));
      p[n * size] = GUARD;
   }

   return ((void *)p);
   
} /* end of _MUCalloc */
/*
 * _MUMalloc
 *
 * The \fI_MUMalloc\fR function is an enhanced version of malloc(3).  
 * It is normally used with the macro MALLOC and is used to verify the 
 * success of allocating storage; reporting the file and line that results in
 * a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocated.
 *
 * Return value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _MUFree(3), _MUCalloc(3), _MURealloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void *
_MUMalloc(size, file, line)
unsigned size;
char *   file;
int      line;
{
   char * p = (char *) malloc(size + 1 /* for GUARD */);

   if (p == NULL)
   {
        (void) fprintf
           (stderr, "couldn't malloc %d bytes in %s at %d!!!\n", 
                     size, file, line);
        abort();
      /* NOTREACHED */
   } 
   else
   {
      ADD(p, size, file, line);
      QPRINTF((stderr, M_format, p, size, line, file));
      p[size] = GUARD;
   }

   return ((void *)p);
   
} /* end of _MUMalloc */
/*
 * _MURealloc
 *
 * The \fI_MURealloc\fR function is an enhanced version of realloc(3).  
 * It is normally used with the macro REALLOC and is used to verify 
 * the success of allocating storage; reporting the file and line that 
 * results in a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocate.
 *
 * Return value:
 *
 * Pointer to the allocated space.
 *
 * See also:
 *
 * _MUFree(3), _MUCalloc(3), _MUMalloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern void *
_MURealloc(ptr, size, file, line)
void *   ptr;
unsigned size;
char *   file;
int      line;
{

   char * p;

   if (ptr == NULL)
      p = (char *) _MUMalloc(size, file, line);
   else
   {
      p = (char *) realloc(ptr, size + 1 /* for GUARD (may not be necessary) */);
      if (p == NULL)
      {
         (void)fprintf(stderr, "couldn't realloc %d bytes in %s at %d!!!\n", 
                       size, file, line);
         abort();
         /* NOTREACHED */
      } 
      else
      {
         if (p == ptr)
         {
            ADD(p, size, file, line);
            QPRINTF((stderr, R_format, p, size, line, file));
         }
         else
         {
            REMOVE(ptr, file, line);
            ADD(p, size, file, line);
            QPRINTF((stderr, f_format, ptr, line, file));
            QPRINTF((stderr, m_format, p, size, line, file));
         }
         p[size] = GUARD;
      }
   }

   return ((void *) p);
      
} /* end of _MURealloc */
/*
 * _MUStrdup
 *
 * The \fI_MUStrdup\fR function is an enhanced version of strdup(3).  
 * It is normally used with the macro STRDUP and is used to verify the 
 * success of allocating storage for the string copy; reporting the file and
 * line that results in a failure to allocate a given size.
 * 
 * It can also be used to debug code to show when and how much space is
 * being allocated.
 *
 * Return value:
 *
 * Pointer to the allocated and copied string or
 * Pointer to allocated empty string ("") if passed NULL.
 *
 * See also:
 *
 * _MUFree(3), _MUCalloc(3), _MURealloc(3), _MUMalloc(3)
 *
 * Synopsis:
 *
 *#include <memutil.h>
 * ...
 */

extern char *
_MUStrdup(str, file, line)
char *   str;
char *   file;
int      line;
{

   if (str == NULL) str = "";

   return((char *) strcpy ((char *) _MUMalloc(strlen(str)+1, file, line), str));

} /* end of _MUStrdup */
/*
 * Add
 *
 */

static void
Add(p, size, file, line)
char * p;
int    size;
char * file;
int    line;
{

   if (memory_index == memory_size)
   {
      if (memory_size == 0)
      {
         memory_size = 50;
         memory = (MemoryPointer *)
            malloc(memory_size * sizeof(MemoryPointer));
		if (memory == NULL)
      	{
         	(void)fprintf(stderr,
			"Couldn't malloc %d bytes in %s at %d!!!\n", 
                       	memory_size * sizeof(MemoryPointer), 
						"MemUtil: Add", __LINE__);
         	abort();
      	} 
      }
      else
      {
         memory_size *= 2;
         memory = (MemoryPointer *)
            realloc(memory, memory_size * sizeof(MemoryPointer));
		if (memory == NULL)
      	{
         	(void)fprintf(stderr,
			"Couldn't realloc %d bytes in %s at %d!!!\n", 
                       	memory_size * sizeof(MemoryPointer), 
						"MemUtil: Add", __LINE__);
         	abort();
      	} 
      }
   }

   memory[memory_index].p = p;
   memory[memory_index].size = size;
   memory[memory_index].file = file;
   memory[memory_index].line = line;
   memory_index++;

} /* end of Add */
/*
 * Remove
 *
 */

static void
Remove(p, file, line)
char * p;
char * file;
int    line;
{
   register int i;

   for (i = 0; i < memory_index && memory[i].p != p; i++)
    ;

   if (i == memory_index)
   {
      (void)fprintf(stderr, "freeing unregistered memory at %x line:%d in %s\n",
       p, line, file);

      /*
       * exit(abort());
       */
   }
   else
   {
      memory_index--;
      if (memory_index)
/*         memory[i].p = memory[memory_index].p; */
         memory[i] = memory[memory_index];
   }

} /* end of Remove */
