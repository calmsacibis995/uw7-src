#ident	"@(#)ld:common/util.c	1.21"
/*
 * Utility functions
 */

/****************************************
** Imports
****************************************/

#ifdef	__STDC__
#include	<stdarg.h>
#else	
#include	<varargs.h>
#endif	

#include	<stdio.h>
#include	<unistd.h>
#include	<signal.h>
#include	<pfmt.h>
#include	"sgs.h"
#include	<sys/types.h>
#include	<sys/stat.h>

#include	"globals.h"

#ifndef	__STDC__
extern void	exit();
extern char*	malloc();
extern char*	calloc();
extern int	unlink();
#endif


/****************************************
** Local Constants
****************************************/

#define MALLOC_CHUNK	500

/****************************************
** Local Function Declarations
****************************************/

LPROTO(void ldexit, (void));

/****************************************
** Local Function Definitions
****************************************/

/*
 * Exit after cleaning up
 */

static void
ldexit()
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_IGN);
	(void) signal(SIGHUP, SIG_DFL);

	/* outfile_name will be set to NULL
	 * if the file could not be opened
	 */
	if (outfile_name) {
		struct stat s;
		if (!stat(outfile_name,&s) && ((s.st_mode & S_IFMT) == S_IFREG))
			(void) unlink(outfile_name); /* remove the a.out */
	}
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}


/***************************************
** Global Function Definitions
****************************************/

/*
** init_signals() sets up ld to properly trap signals
*/
void
init_signals()
{
#ifdef	__STDC__
	if (signal(SIGINT, (void (*)(int)) ldexit) == SIG_IGN )
		(void) signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, (void (*)(int)) ldexit) == SIG_IGN )
		(void) signal(SIGHUP, SIG_IGN);
	if (signal(SIGQUIT, (void (*)(int)) ldexit) == SIG_IGN )
		(void) signal(SIGQUIT, SIG_IGN);
#else
	if (signal(SIGINT, (void (*)()) ldexit) == SIG_IGN )
		(void) signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, (void (*)()) ldexit) == SIG_IGN )
		(void) signal(SIGHUP, SIG_IGN);
	if (signal(SIGQUIT, (void (*)()) ldexit) == SIG_IGN )
		(void) signal(SIGQUIT, SIG_IGN);
#endif
}


/*
 * Print a message to stdout
 */
/*VARARGS2*/
#ifdef	__STDC__
void
lderror(Errorlevel level, char* format, ...)
#else	
void
lderror(level, format, va_alist)
	Errorlevel level;		/* Severity of the error */
	CONST char *format;		/* Printf(3)-style format string */
	va_dcl
#endif
{
	int	err;	/* to hold libelf error code */
	va_list	ap;

#ifdef	DEBUG
	if (level != MSG_DEBUG)
#endif

	switch (level) {
#ifdef	DEBUG
	case MSG_DEBUG:
		(void) fputs("debug: ", stderr);
		break;
#endif
	case MSG_NOTICE:
		if (cur_file_name)
			pfmt(stderr,MM_WARNING, ":1202:%s: notice: ", cur_file_name);
		else
			pfmt(stderr,MM_WARNING,":1203: notice: ");
		break;
	case MSG_WARNING:
		if (cur_file_name)
			pfmt(stderr,MM_WARNING, ":1204:%s: warning: ", cur_file_name);
		else
			pfmt(stderr,MM_WARNING,":1205: warning: ");
		break;
	case MSG_FATAL:
		if (cur_file_name)
			pfmt(stderr,MM_ERROR, ":1206:%s: fatal error: ", cur_file_name);
		else
			pfmt(stderr,MM_ERROR,":1207: fatal error: ");
		break;
	case MSG_ELF:
		if (cur_file_name)
			pfmt(stderr,MM_ERROR, ":1208:%s: libelf error: ", cur_file_name);
		else
			pfmt(stderr,MM_ERROR,":1209: libelf error: ");
		if( (err = elf_errno()) != 0)
			fprintf(stderr, " %s ", elf_errmsg(err));
		break;
	case MSG_SYSTEM:
		if (cur_file_name)
			pfmt(stderr,MM_ERROR, ":1210:%s: system error: ", cur_file_name);
		else
			pfmt(stderr,MM_ERROR,":1211: system error: ");
		break;
	default:
		if (cur_file_name)
			pfmt(stderr,MM_ERROR, ":1212:%s: internal error: unknown error level", cur_file_name);
		else
			pfmt(stderr,MM_ERROR,":1213: internal error: unknown error level");
		ldexit();
	}

#ifdef	__STDC__
	va_start(ap, format);
#else
	va_start(ap);
#endif

	(void) vfprintf(stderr, format, ap);
	putc('\n',stderr);
	(void) fflush(stderr);
	va_end(ap);
	if (level >= MSG_FATAL)
		ldexit();
}


/* ----------------------------------
 *  list manipulation routines
 * ----------------------------------
 */

/*
 * Add an item to the indicated list and return a pointer to the
 * list node created
 */
Listnode*
list_append(lst, item)
	List	*lst;		/* The list */
	CONST	VOID	*item;	/* The item */
{
	if(lst == NULL)
		lderror(MSG_FATAL,gettxt(":1145","ld internal error: null list passed to list_append"));

	if (lst->head == NULL)
		lst->head = lst->tail = NEWZERO(Listnode);
	else {
		if(lst->tail == NULL)
			lderror(MSG_FATAL,gettxt(":1146","ld internal error: null list tail passed to list_append"));

		lst->tail->next = NEWZERO(Listnode);
		lst->tail = lst->tail->next;
	}
	lst->tail->data = (VOID *)item;
	lst->tail->next = NULL;

	return lst->tail;
}

/*
 * Add an item to the indicated list after the given node
 * and return a pointer to the list node created
 */
Listnode*
list_insert(after, item)
	Listnode	*after;		/* The node to insert after */
	CONST	VOID		*item;	/* The item */
{
	Listnode	*ln;		/* Temp list node ptr */

	if(after == NULL)
		lderror(MSG_FATAL,gettxt(":1147","internal error: attempt to list insert after a null pointer"));

	ln = NEWZERO(Listnode);
	ln->data = (VOID *)item;
	ln->next = after->next;
	after->next = ln;

	return ln;
}

/*
 * Prepend an item to the indicated list and return a pointer to the
 * list node created
 */
Listnode*
list_prepend(lst, item)
	List	*lst;		/* The list */
	CONST	VOID	*item;	/* The item */
{
	Listnode	*ln;		/* Temp list node ptr */

	if(lst == NULL)
		lderror(MSG_FATAL,gettxt(":1148","internal error: attempt to list prepend to a null list"));

	if (lst->head == NULL)
		lst->head = lst->tail = NEWZERO(Listnode);
	else {
		ln = NEWZERO(Listnode);
		ln->next = lst->head;
		lst->head = ln;
	}
	lst->head->data = (VOID *)item;

	return lst->head;
}


/* ----------------------------------
 *  libelf interface routines
 * ----------------------------------
 */

Elf*
my_elf_begin(fildes, cmd, ref)
	int	fildes;
	Elf_Cmd	cmd;
	Elf	*ref;
{
	Elf	*retval;

	if ( (retval = elf_begin(fildes, cmd, ref)) == NULL)
		lderror(MSG_ELF,gettxt(":1149","elf_begin: "));
	else
		return retval;
	/*NOTREACHED*/
}

void
my_elf_cntl(elf, cntl)
	Elf	*elf;
	Elf_Cmd	cntl;
{
	if ( elf_cntl(elf, cntl) == -1 )
		lderror(MSG_ELF,gettxt(":1150","elf_cntl: "));
	/* toss away return value since not used by ld */
	return;
}

int
my_elf_end(elf)
	Elf	*elf;
{
	int	retval;

	if ( (retval = elf_end(elf)) == NULL)
		lderror(MSG_ELF,gettxt(":1151","elf_end: "));
	else
		return retval;
	/*NOTREACHED*/
}

size_t
my_elf_fsize(type, count, ver)
	Elf_Type	type;
	size_t		count;
	unsigned	ver;
{
	size_t	retval;

	if ( (retval = elf_fsize(type, count, ver)) == 0)
		lderror(MSG_ELF,gettxt(":1152","elf_fsize: "));
	else
		return retval;
	/*NOTREACHED*/
}

Elf_Arhdr*
my_elf_getarhdr(elf)
	Elf	*elf;
{
	Elf_Arhdr	*retval;

	if ( (retval = elf_getarhdr(elf)) == NULL)
		lderror(MSG_ELF,gettxt(":1153","elf_getarhdr: "));
	else
		return retval;
	/*NOTREACHED*/
}

Elf_Arsym*
my_elf_getarsym(elf, ptr)
	Elf	*elf;
	size_t	*ptr;
{
	Elf_Arsym	*retval;

	if ( (retval = elf_getarsym(elf, ptr)) == NULL)
		lderror(MSG_ELF,gettxt(":1154","elf_getarsym: "));
	else
		return retval;
	/*NOTREACHED*/
}

Elf_Data*
my_elf_getdata(scn,data)
	Elf_Scn		*scn;
	Elf_Data	*data;
{
	Elf_Data	*retval;

	if ( (retval = elf_getdata(scn,data)) == NULL)
		lderror(MSG_ELF,gettxt(":1155","elf_getdata: "));
	else
		return retval;
	/*NOTREACHED*/
}

Elf_Data*
my_elf_newdata(scn)
	Elf_Scn		*scn;
{
	Elf_Data	*retval;

	if ( (retval = elf_newdata(scn)) == NULL)
		lderror(MSG_ELF,gettxt(":1156","elf_newdata: "));
	else
		return retval;
	/*NOTREACHED*/
}

Ehdr*
my_elf_getehdr(elf)
	Elf	*elf;
{
	Ehdr	*retval;

	if ( (retval = elf_getehdr(elf)) == NULL)
		lderror(MSG_ELF,gettxt(":1074","elf_getehdr: "));
	else
		return retval;
	/*NOTREACHED*/
}

char*
my_elf_getident(elf, ptr)
	Elf	*elf;
	size_t	*ptr;
{
	char	*retval;

	if ( (retval = elf_getident(elf, ptr)) == NULL)
		lderror(MSG_ELF,gettxt(":1157","elf_getident: "));
	else
		return retval;
	/*NOTREACHED*/
}

Phdr*
my_elf_newphdr(elf, count)
	Elf	*elf;
	size_t	count;
{
	Phdr	*retval;

	if ( (retval = elf_newphdr(elf, count)) == NULL)
		lderror(MSG_ELF,gettxt(":1158","elf_newphdr: "));
	else
		return retval;
	/*NOTREACHED*/
}


Elf_Scn*
my_elf_getscn(elf, index)
	Elf	*elf;
	size_t	index;
{
	Elf_Scn	*retval;

	if ( (retval = elf_getscn(elf, index)) == NULL)
		lderror(MSG_ELF,gettxt(":1159","elf_getscn: "));
	else
		return retval;
	/*NOTREACHED*/
}

size_t
my_elf_ndxscn(scn)
	Elf_Scn	*scn;
{
	size_t	retval;

	if ( (retval = elf_ndxscn(scn)) == SHN_UNDEF )
		lderror(MSG_ELF,gettxt(":1160","elf_ndxscn: "));
	else
		return retval;
	/*NOTREACHED*/
}

Elf_Scn*
my_elf_newscn(elf)
	Elf	*elf;
{
	Elf_Scn	*retval;

	if ( (retval = elf_newscn(elf)) == NULL)
		lderror(MSG_ELF,gettxt(":1161","elf_newscn: "));
	else
		return retval;
	/*NOTREACHED*/
}

Shdr*
my_elf_getshdr(scn)
	Elf_Scn	*scn;
{
	Shdr	*retval;

	if ( (retval = elf_getshdr(scn)) == NULL)
		lderror(MSG_ELF,gettxt(":1162","elf_getshdr: "));
	else
		return retval;
	/*NOTREACHED*/
}

size_t
my_elf_rand(elf, offset)
	Elf	*elf;
	size_t	offset;
{
	size_t	retval;

	if ( (retval = elf_rand(elf, offset)) == 0 )
		lderror(MSG_ELF,gettxt(":1163","elf_rand: "));
	else
		return retval;
	/*NOTREACHED*/
}

char*
my_elf_strptr(elf, section, offset)
	Elf	*elf;
	size_t	section;
	size_t	offset;
{
	char	*retval;

	if ( (retval = elf_strptr(elf, section, offset)) == NULL)
		lderror(MSG_ELF,gettxt(":1164","elf_strptr: "));
	else
		return retval;
	/*NOTREACHED*/
}

void
my_elf_update(elf, cmd)
	Elf	*elf;
	Elf_Cmd	cmd;
{
	if ( elf_update(elf, cmd) == -1 )
		lderror(MSG_ELF,gettxt(":1165","elf_update: "));
	/* toss away return value since not used by ld */
	return;
}

/* ----------------------------------
 *  memory allocation interface routines
 * ----------------------------------
 */


/*
 * Call malloc(3) to allocate memory and exit gracefully
 * if none can be found
 */
VOID*
mymalloc(nBytes)
	unsigned int	nBytes;	/* Number of bytes to allocate */
{
	register VOID	*mem;	/* Pointer to memory to be allocated */

	if ((mem = malloc(nBytes)) == NULL) {
		lderror(MSG_SYSTEM,gettxt(":1166", "not enough memory to allocate %d bytes"),
		    nBytes);
	}

	DPRINTF(DBG_UTIL, (MSG_DEBUG, "malloc of %x bytes returns %x", nBytes, mem));

	return mem;
}

/*
 * Call calloc(3) to allocate ZEROED-OUT memory and exit gracefully
 * if none can be found
 */
VOID*
mycalloc(nBytes)
	unsigned int	nBytes;	/* Number of bytes to allocate */
{
	register VOID	*mem;	/* Pointer to memory to be allocated */

	if ((mem = calloc(1, nBytes)) == NULL) {
		lderror(MSG_SYSTEM,gettxt(":1166", "not enough memory to allocate %d bytes"),
		    nBytes);
	}

	DPRINTF(DBG_UTIL, (MSG_DEBUG, "calloc of %x bytes returns %x", nBytes, mem));

	return mem;
}

/*
 * Call realloc(3) to resize memory and exit gracefully
 * if none can be found
 */
VOID*
myrealloc(oldmem, nBytes)
	VOID		*oldmem;
	unsigned int	nBytes;	/* Number of bytes to allocate */
{
	register VOID	*mem;	/* Pointer to memory to be allocated */

	if ((mem = realloc(oldmem, nBytes)) == NULL) {
		lderror(MSG_SYSTEM,gettxt(":1166", "not enough memory to allocate %d bytes"),
		    nBytes);
	}

	DPRINTF(DBG_UTIL, (MSG_DEBUG, "realloc of %x bytes returns %x", nBytes, mem));

	return mem;
}
/*
 * Allocate lots of structs in one chunk to save on calls to malloc.
 */
VOID*
new_calloc(size)
	unsigned int size;
{
	static Listnode	*cache0 = NULL;
	static int	count0 = 0;
	static Insect	*cache1 = NULL;
	static int	count1 = 0;
	static Ldsym	*cache2 = NULL;
	static int	count2 = 0;
	register VOID* temp;

	if ( size == sizeof(Listnode)){
		if (count0 == 0) {
			DPRINTF(DBG_UTIL, (MSG_DEBUG, "mallocing Listnodes"));
			cache0 = (Listnode *)mycalloc(sizeof(Listnode) * MALLOC_CHUNK);
			count0 = MALLOC_CHUNK;
		}
		count0--;
		temp = (VOID *)cache0;
		cache0++;
		return temp;
	}
	else if (size == sizeof(Insect)){
		
		if (count1 == 0) {
			DPRINTF(DBG_UTIL, (MSG_DEBUG, "mallocing Insects"));
			cache1 = (Insect *)mycalloc(sizeof(Insect) * MALLOC_CHUNK);
			count1 = MALLOC_CHUNK;
		}
		count1--;
		temp = (VOID *)cache1;
		cache1++;
		return temp;
	}
	else if (size == sizeof(Ldsym)){
		if (count2 == 0) {
			DPRINTF(DBG_UTIL, (MSG_DEBUG, "mallocing Ldsyms"));
			cache2 = (Ldsym *)mycalloc(sizeof(Ldsym) * MALLOC_CHUNK);
			count2 = MALLOC_CHUNK;
		}
		count2--;
		temp = (VOID *)cache2;
		cache2++;
		return temp;
	}
	else
		return mycalloc(size);
}
