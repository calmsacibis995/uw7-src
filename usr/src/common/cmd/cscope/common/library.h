#ident	"@(#)cscope:common/library.h	1.3"
/* library function return value declarations */

#if BSD
#define	strchr	index
#define strrchr	rindex
#undef	tolower		/* BSD toupper and tolower don't test the character */
#undef	toupper
#define	tolower(c)	(islower(c) ? (c) : (c) - 'A' + 'a')	
#define	toupper(c)	(isupper(c) ? (c) : (c) - 'a' + 'A')	
#endif

/* private library */
char	*basename(), *compath(), *egrepinit(), *logdir();
char	*mycalloc(), *mymalloc(), *myrealloc(), *stralloc();
FILE	*mypopen(), *vpfopen();
void	egrepcaseless();

/* standard C library */
char	*ctime(), *getcwd(), *getenv(), *mktemp();
char	*strcat(), *strcpy(), *strncpy(), *strpbrk(), *strchr(), *strrchr();
char	*strtok();
long	lseek(), time();
unsigned sleep();
void	exit(), free(), qsort();
#if BSD
FILE	*popen();	/* not in stdio.h */
#endif

/* Programmer's Workbench library (-lPW) */
char	*regcmp(), *regex();
