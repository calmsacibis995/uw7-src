#ident	"@(#)ar:common/ar.c	1.36"
/* ar: UNIX Archive Maintainer */


#include <stdio.h>
#include <sys/param.h>
#include <ar.h>
#include <locale.h>
#include <unistd.h>

#ifndef	UID_NOBODY
#define UID_NOBODY	60001
#endif

#ifndef GID_NOBODY
#define GID_NOBODY      60001
#endif

#ifdef __STDC__
#include <stdlib.h>
#endif

#include "libelf.h"
#include <ccstypes.h>

#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pfmt.h>
#include "sgs.h"

#ifdef __STDC__
#include <time.h>
#include <locale.h>
#endif

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

#define FLAG(ch)	(flg[ch - 'A'])
#define CHUNK		500
#define SYMCHUNK	1000
#define SNAME		16
#define ROUNDUP(x)	(((x) + 1) & ~1)

#define LONGDIRNAME	"//              "
#define SYMDIRNAME	"/               "	/* symbol directory filename */
#define LONGNAMEPREFIX	"/"

#define FORMAT		"%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s"
#define DATESIZE	60

extern 	int optind; 	/* arg list index */

static	struct stat	stbuf;


typedef struct arfile ARFILE;

struct arfile
{
	char	ar_name[SNAME];		/* info from archive member header */
	long	ar_date;
	unsigned long	ar_mode;
	long	ar_size;
	char    *longname;
	long	offset;
	char	*pathname;
	char	*contents;
	ARFILE	*next;
	int	name_len;
	int	ar_uid;
	int	ar_gid;
};

static long	nsyms, *symlist;
static long	sym_tab_size, long_tab_size;
static long	*sym_ptr;
static long	*nextsym = NULL;
static int	syms_left = 0;
static int	errflag = 0;


static ARFILE	*listhead, *listend;

static FILE     *outfile;
static int	fd;

static Elf	*elf, *arf;

static char	flg['z' - 'A'];
static char	**namv;
static char	*arnam;
static char	*ponam;
static char	*gfile;
static char	*str_base,	/* start of string table for names */
		*str_top;	/* pointer to next available location */

static char	*long_base,
		*long_top;
static	int	longnames = 0;

static int      signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
static int      namc;
static int      modified;
static int	Vflag=0;

static	int	m1[] = { 1, ROWN, 'r', '-' };
static	int	m2[] = { 1, WOWN, 'w', '-' };
static	int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
static	int	m4[] = { 1, RGRP, 'r', '-' };
static	int	m5[] = { 1, WGRP, 'w', '-' };
static	int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
static	int	m7[] = { 1, ROTH, 'r', '-' };
static	int	m8[] = { 1, WOTH, 'w', '-' };
static	int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

static	int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

static	int	notfound(),	qcmd(),
		rcmd(),		dcmd(),		xcmd(),
		pcmd(),		mcmd(),		tcmd();

static	void	setcom(),	usage(),	sigexit(),	
		cleanup(),	movefil(),	 mesg(),		
		choose(),	mksymtab(),	getaf(),
		savename(),     writefile(),	search_sym_tab(),
		sputl(),	writesymtab(),	mklong_tab(),
		synopsis();	

static	char	*trim(),	*match(),	*trimslash();

static	ARFILE	*getfile(),	*newfile();

static	FILE	*stats();
static  int     (*comfun)();

extern	char	*tempnam(),	*ctime();
extern	long	time(), lseek();
extern	void	exit(),		free();
extern  int	creat(),	write(),	close(),
		access(),	unlink(),	stat(),
		read();

static char badposnam[] = ":1:Posname, %s, not found\n";
static char nomem[] = ":2:Out of memory\n";
static char malformar[] = ":3:%s: malformed archive (at %ld)\n";
static char badopen[] = ":4:Cannot open %s: %s\n";
static char badread[] = ":5:Cannot read %s: %s\n";
static char badlibelf[] = ":6:libelf error: %s\n";
static char nostring[] = ":7:%s cannot get string table space\n";
static char nogrowstring[] = ":8:%s cannot grow string table\n";
static char creating[] = ":9:Creating %s\n";
static char badcreate[] = ":10:Cannot create %s: %s\n";

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *cp;
	int argv_len=0;
	char label[20];
	char *temp_argv;


	for (i = 0; signum[i]; i++)
		if (signal(signum[i], SIG_IGN) != SIG_IGN)
			(void) signal(signum[i], sigexit);

	(void)setlocale(LC_ALL, "");
	(void)sprintf(label, "UX:%sar", SGS);
	(void) setlabel(label);
	(void) setcat("uxar");

	if (argc < 2){
		pfmt(stderr, MM_ERROR, ":11:Incorrect usage\n");
		usage();
	}

	cp = argv[1];
	argv_len = strlen(argv[1]);

	if (*cp != '-') {
		temp_argv = (char *)malloc(2 + argv_len );
		temp_argv[0]= '-';
		strcpy( temp_argv+1, argv[1] );
		argv[1]=temp_argv;
	}
	synopsis(argc, argv);
		
	modified = FLAG('s');
	getaf(); 

	if ( (fd == -1) && (FLAG('d') || FLAG('t') || FLAG('p')
                              || FLAG('m') || FLAG('x') || 
			      (FLAG('r') && (FLAG('a') || FLAG('i') || 
			       FLAG('b') ) ) )   ) 
		{
                   (void) pfmt(stderr, MM_ERROR,":14:Archive, %s, not found\n"
			, arnam);
                   exit(1);
                }

	(*comfun)();
	if (modified)	/* make archive symbol table */
		writefile();
	(void) close(fd);
	return(errflag + notfound());
}


static void
setcom(fun)
	int (*fun)();
{

	if (comfun != 0)
	{
		(void) pfmt(stderr, MM_ERROR, ":15:Only one of [drqtpmx] allowed\n"
			);
		exit(1);
	}
	comfun = fun;
}


static	int
rcmd()
{
	register FILE *f;
	register ARFILE *fileptr;
	register ARFILE	*abifile = NULL;
	register ARFILE	*backptr = NULL;
	ARFILE	*endptr;
	ARFILE	*moved_files;

	for ( fileptr = getfile(); fileptr; fileptr = getfile())
	{
		if ( !abifile && ponam && strcmp( fileptr->longname, ponam ) == 0)
			abifile = fileptr;
		else if ( !abifile )
			backptr = fileptr;


		if (namc == 0 || match(fileptr->longname) != NULL )
		{
			f = stats( gfile );
			if (f == NULL)
			{
				if (namc) {
					(void) pfmt(stderr, MM_ERROR, badopen,
						gfile, strerror(errno));
					errflag++;
				}
				
				mesg('c', gfile);
			}
			else
			{
				if (FLAG('u') && stbuf.st_mtime <= fileptr->ar_date)
				{
					(void) fclose(f);
					continue;
				}
				mesg('r', namc ? gfile:fileptr->longname);
				movefil( fileptr );
				free( fileptr->contents );
				if ((fileptr->contents = (char *)malloc( ROUNDUP( stbuf.st_size ))) == NULL)
				{
					(void) pfmt( stderr, MM_ERROR, nomem);
					exit(1);
				}
				if (fread( fileptr->contents, sizeof(char), stbuf.st_size, f ) != stbuf.st_size)
				{
					(void) pfmt( stderr, MM_ERROR, badread,
					fileptr->longname, strerror(errno));
					exit(1);
				}
				if ( fileptr->pathname != NULL)
					free(fileptr->pathname);
				if ((fileptr->pathname= (char *)malloc( strlen(gfile) * sizeof(char *)  )) == NULL)
                                {
                                        (void) pfmt( stderr, MM_ERROR, nomem);
                                        exit(1);
                                }
				(void) strcpy(fileptr->pathname, gfile);
				fileptr->offset = 0;

				(void) fclose(f);
				modified++;
			}
		}
		else
			mesg( 'c', fileptr->longname);
	}

	endptr = listend;
	cleanup();
	if (ponam && endptr && (moved_files = endptr->next))
	{
		if (!abifile)
		{
			(void) pfmt( stderr, MM_ERROR, badposnam, ponam);
			exit(2);
		}
		endptr->next = NULL;

		if (FLAG('b'))
			abifile = backptr;

		if (abifile)
		{
			listend->next = abifile->next;
			abifile->next = moved_files;
		}
		else
		{
			listend->next = listhead;
			listhead = moved_files;
		}
		listend = endptr;
	}
	else if (ponam && !abifile) {
                (void) pfmt(stderr, MM_ERROR, badposnam, ponam);
		errflag++;
	}
	return(0);
}

static	int
dcmd()
{
	register ARFILE	*fptr;
	register ARFILE *backptr = NULL;

	for( fptr = getfile(); fptr; fptr = getfile())
	{
		if (match( fptr->longname) != NULL)
		{
			mesg('d', gfile);
			if (backptr == NULL)
				listhead = NULL;
			else
			{
				backptr->next = NULL;
				listend = backptr;
			}
			modified = 1;
		}
		else
		{
			mesg('c', fptr->longname);
			backptr = fptr;
		}
	}
	return(0);
}

static	int
xcmd()
{
	register int f;
	register ARFILE *next;
	int name_max = 0;
	int flags;

	/* POSIX 1003.2 */
	if (FLAG('C'))
		flags = O_WRONLY|O_CREAT|O_TRUNC|O_EXCL;
	else
		flags = O_WRONLY|O_CREAT|O_TRUNC;


	errno = 0;
	name_max=pathconf(".",_PC_NAME_MAX);
	if (name_max == -1 && errno != 0) {
		(void) pfmt(stderr, MM_ERROR, ":41:pathconf failed: %s\n",
			strerror(errno));
		exit(1);
	}

	for( next = getfile(); next; next = getfile())
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			char *file = next->longname;
			char *tr_file = NULL;

			/* POSIX 1003.2 */
			if (name_max > -1 && (unsigned)name_max<strlen(next->longname)){
				if (FLAG('T')) {
					if ((tr_file=malloc(name_max+1))==NULL){
						(void) pfmt( stderr, MM_ERROR, 
							nomem);
						exit(1);
					}
					strncpy(tr_file,file,name_max);
					tr_file[name_max] = '\0';
					file = tr_file;
				} else {
					(void) pfmt(stderr, MM_ERROR, 
						":42:%s: file name too long\n",
						next->longname);
					mesg('c', next->longname);
					errflag++;
					continue;
				}
					
			}

			f=open(file,flags,(mode_t)next->ar_mode&0777);

			if (f < 0)
			{
				(void) pfmt(stderr, MM_ERROR, badcreate,
					file, strerror(errno));
				errflag++;
				mesg('c', next->longname);
			}
			else
			{
				mesg( 'x', namc ? gfile : next->longname);
				if (write( f, next->contents, (unsigned)next->ar_size ) != next->ar_size)
				{
					(void) pfmt( stderr, MM_ERROR, 
					":16:Cannot write %s: %s\n",
					file,strerror(errno));
					exit(1);
				}
				(void) close(f);
			}
			if (tr_file != NULL)
				free(tr_file);
		}
	}
	return(0);
}

static	int
pcmd()
{
	register ARFILE	*next;

	for( next = getfile(); next; next = getfile())
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			if (FLAG('v'))
			{
				(void) fprintf(stdout, "\n<%s>\n\n",
					namc ? gfile : next->longname);
				(void) fflush(stdout);
			}
			(void) fwrite( next->contents, sizeof(char), next->ar_size, stdout );
		}
	}
	return(0);
}

static	int
mcmd()
{
	register ARFILE	*fileptr;
	register ARFILE	*abifile = NULL;
	register ARFILE	*tmphead = NULL;
	register ARFILE	*tmpend = NULL;
	ARFILE	*backptr1 = NULL;
	ARFILE	*backptr2 = NULL;

	for( fileptr = getfile(); fileptr; fileptr = getfile() )
	{
		if (match( fileptr->longname) != NULL)
		{
			mesg( 'm', gfile);
			if ( tmphead )
				tmpend->next = fileptr;
			else
				tmphead = fileptr;
			tmpend = fileptr;

			if (backptr1)
			{
				listend = backptr1;
				listend->next = NULL;
			}
			else
				listhead = NULL;
			continue;
		}

		mesg( 'c', fileptr->longname);
		backptr1 = fileptr;
		if ( ponam && !abifile )
		{
			if ( strcmp( fileptr->longname, ponam ) == 0)
				abifile = fileptr;
			else
				backptr2 = fileptr;
		}
	}

	if ( !tmphead )
		return(1);

	if ( !ponam )
		listend->next = tmphead;
	else
	{
		if ( !abifile )
		{
			(void) pfmt( stderr, MM_ERROR, badposnam, ponam);
			exit(2);
		}
		if (FLAG('b'))
			abifile = backptr2;

		if (abifile)
		{
			tmpend->next = abifile->next;
			abifile->next = tmphead;
		}
		else
		{
			tmphead->next = listhead;
			listhead = tmphead;
		}
	}

	modified++;
	return(0);
}

static	int
tcmd()
{
	register ARFILE	*next;
	register int	**mp;
#ifdef __STDC__
	char   buf[DATESIZE];
#else
	register char	*cp;
#endif

	for( next = getfile(); next; next = getfile() )
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			if (FLAG('v'))
			{
				for (mp = &m[0]; mp < &m[9];)
					choose(*mp++, next->ar_mode);

				(void)fprintf(stdout, " %6u/%u",/*POSIX 1003.2*/
					next->ar_uid, next->ar_gid);
				(void) fprintf(stdout, " %7lu",
						next->ar_size);
#ifdef __STDC__
				(void)setlocale(LC_TIME, "");
				if ( (strftime(buf,DATESIZE,
				     gettxt(":46", "%b %e %H:%M %Y"),
					localtime( &(next->ar_date) )) ) == 0 ) 
				{
				    (void) pfmt(stderr, MM_ERROR, 
			":18:Not enough space to store the date\n");
					exit(1);
				}
			
				(void) fprintf(stdout, " %s ", buf);	
#else
				cp = ctime( &(next->ar_date));
                                (void) fprintf(stdout, " %-12.12s %-4.4s ", cp+4, cp + 20);
#endif


			}
			(void) fprintf(stdout, "%s\n",
				namc ? gfile : next->longname);
		}
	}
	return(0);
}

static	int
qcmd()
{
	register ARFILE *fptr;

	if (FLAG('a') || FLAG('b'))
	{
		(void) pfmt(stderr, MM_ERROR, ":19:abi not allowed with q\n");
		exit(1);
	}

	for ( fptr = getfile(); fptr; fptr = getfile())
		;
	cleanup();
	return(0);
}



static void
getaf()
{
        Elf_Cmd cmd;

        if (elf_version(EV_CURRENT) == EV_NONE){
                (void) pfmt(stderr, MM_ERROR, ":20:libelf.a out of date\n");
		exit(1);
		}

        if ( (fd  = open(arnam, O_RDONLY)) == -1) {

		if (errno != ENOENT) {
			(void) pfmt(stderr, MM_ERROR, badopen,
						arnam, strerror(errno));
			exit(1);
		} else /* archive does not exist yet, may have to create one*/
                	return; 
	}

	
	cmd = ELF_C_READ;
	arf = elf_begin(fd, cmd, (Elf *)0);

	if (elf_kind(arf) != ELF_K_AR){
		(void) pfmt(stderr, MM_ERROR, ":21:%s not in archive format\n"
			, arnam);
		if (FLAG('a') || FLAG('b'))
		    (void) pfmt(stderr, MM_ERROR,
		    ":22:%s taken as mandatory 'posname' with keys 'abi'\n"
				,ponam);
		exit(1);
	}
	
}

static	ARFILE *
getfile()
{
	Elf_Arhdr *mem_header;
	register ARFILE	*file;

	if ( fd == -1 )
		return( NULL ); /* the archive doesn't exist */

	if ( (elf = elf_begin(fd, ELF_C_READ, arf) ) == 0 )
		return( NULL );  /* the archive is empty or have hit the end */

 	if ( ( mem_header = elf_getarhdr(elf) ) == NULL) {
                (void) pfmt( stderr, MM_ERROR, malformar,
                         arnam, elf_getbase(elf) );
                exit(1);
        }


	/* zip past special members like the symbol and string table members */
	while ( strncmp(mem_header->ar_name,"/",1) == 0 ||
	     	strncmp(mem_header->ar_name,"//",2) == 0 ) 
	{
			(void) elf_next(elf);
			(void) elf_end(elf);
			if ((elf = elf_begin(fd, ELF_C_READ, arf)) == 0)
                		return( NULL );  /* the archive is empty or have hit the end */
                        if ( (mem_header = elf_getarhdr(elf) ) == NULL) 
			{
                                (void) pfmt( stderr, MM_ERROR, malformar,
					arnam, elf_getbase(elf) );
                                exit(1);
			}
       }

	file = newfile();

	file->name_len = strlen(mem_header->ar_name);
	if ((file->longname = (char *)malloc(file->name_len + 1)) == NULL) {
		(void) pfmt( stderr, MM_ERROR, nomem);
		exit(1);
	}
	(void)strcpy(file->longname, mem_header->ar_name);

	file->ar_date = mem_header->ar_date;
	file->ar_uid  = mem_header->ar_uid;
	file->ar_gid  = mem_header->ar_gid;
	file->ar_mode = (unsigned long) mem_header->ar_mode;
	file->ar_size = mem_header->ar_size;
	file->offset = elf_getbase(elf); 

	/* reverse logic */
	if ( !(FLAG('t') && !FLAG('s')) ){
		if ((file->contents = (char *)malloc( ROUNDUP( file->ar_size ))) == NULL) {
			(void) pfmt( stderr, MM_ERROR, nomem);
			exit(1);
		}

		if ( lseek(fd, file->offset, 0) != file->offset ){
			(void) pfmt( stderr, MM_ERROR, ":23:lseek() failed: %s\n",
				strerror(errno));
			exit(1);
		}

		if ( read(fd, file->contents, (unsigned) ROUNDUP(file->ar_size ) ) == -1 ){
			(void) pfmt( stderr, MM_ERROR, badread, arnam,
				strerror(errno));
			exit(1);
		}
	}

	(void) elf_next(elf);
	(void) elf_end(elf);
	return (file);
}

static	ARFILE *
newfile()
{
	static ARFILE	*buffer =  NULL;
	static int	count = 0;
	register ARFILE	*fileptr;

	if (count == 0)
	{
		if ((buffer = (ARFILE *) calloc( CHUNK, sizeof( ARFILE ))) == NULL)
		{
			(void) pfmt( stderr, MM_ERROR, nomem);
			exit(1);
		}
		count = CHUNK;
	}
	count--;
	fileptr = buffer++;

	if (listhead)
		listend->next = fileptr;
	else
		listhead = fileptr;
	listend = fileptr;
	return( fileptr );
}

static void
usage()
{
 	(void) pfmt(stderr, MM_ACTION, ":43:Usage: ar [-V?] -key[arg] [posname] afile [name ...]\n\t where key[arg] is one of the following:\n\t r[uabi], m[abi], d, q, t, p, x[CT]\n" );

	if (errflag)
	{
         	(void) pfmt(stderr, MM_NOSTD, 
			":45:\n\
A key may be one of the following:\n\
		[-d       delete file(s) from archive]\n\
		[-m[abi]  move file(s) to the end of the archive]\n\
		[-p	  print file(s) in the archive]\n\
		[-q	  quickly append file(s) to the end of the archive]\n\
		[-r[abiu] replace file in archive]\n\
		[-t	  print a table of contents of the archive]\n\
		[-x[CT]	  extract file]\n\
		\nThe following arguments can modify any key:\n\
		[c	suppress message when a file is created]\n\
		[s	force regeneration of archive symbol table]\n\
		[v	verbose description of the creation of the archive]\n\
		\nThe following arguments can modify only the key(s) noted above\n\
		[a	new files are placed after posname]\n\
		[b	new files are placed before posname]\n\
		[i	new files are inserted before posname]\n\
		[u	files newer than the archive file are replaced]\n\
		[C	do not overwrite existing files]\n\
		[T	allow truncation of extracted filenames]\n");

	}
	exit(1);

}


/*ARGSUSED0*/
static void
sigexit(i)
	int i;
{
	if (outfile)
		(void) unlink( arnam );
	exit(100);
}

/* tells the user which of the listed files were not found in the archive */

static int
notfound()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
		{
			(void) pfmt(stderr, MM_ERROR, 
				":25:%s not found\n", namv[i]);
			n++;
		}
	return (n);
}


/* puts the file which was in the list in the linked list */

static void
cleanup()
{
	register int i;
	register FILE	*f;
	register ARFILE	*fileptr;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
		mesg('a', namv[i] );
		f = stats( namv[i] );
		if (f == NULL) {
			(void) pfmt(stderr, MM_ERROR, badopen, namv[i],
				strerror(errno));
			errflag++;
		} else
		{
			char	*trim_name;
			fileptr = newfile();
			trim_name = trim(namv[i]);

			fileptr->name_len = strlen(trim_name);
			if ((fileptr->longname = (char *)malloc(fileptr->name_len + 1) ) == NULL)
                        {
                              	 (void) pfmt( stderr, MM_ERROR, nomem);
                              	 exit(1);
                       	 }	

			(void)strcpy(fileptr->longname,  trim_name);

			if ((fileptr->pathname = (char *)malloc( strlen(namv[i]) + 1) ) == NULL) {
                                (void) pfmt( stderr, MM_ERROR, nomem);
                                exit(1);
                        }

                        (void) strcpy( fileptr->pathname,  namv[i] );


			
			movefil( fileptr );
			if ((fileptr->contents = (char *)malloc( ROUNDUP( stbuf.st_size ))) == NULL)
			{
				(void) pfmt( stderr, MM_ERROR, nomem);
				exit(1);
			}
			if (fread( fileptr->contents, sizeof(char), stbuf.st_size, f ) != stbuf.st_size )
			{
				(void) pfmt( stderr, MM_ERROR, badread,
					fileptr->longname, strerror(errno));
				exit(1);
			} 

			(void) fclose(f);
			modified++;
		        namv[i] = 0;
		}
	}
}

/*
* insert the file 'file' into the temporary file
*/

static void
movefil( fileptr )
	register ARFILE *fileptr;
{
	fileptr->ar_size = stbuf.st_size;
	fileptr->ar_date = stbuf.st_mtime;

	if (stbuf.st_uid > 60000 )
		fileptr->ar_uid = UID_NOBODY;
	else
		fileptr->ar_uid = stbuf.st_uid;

	if (stbuf.st_gid > 60000 )
		fileptr->ar_gid = GID_NOBODY;
	else
		fileptr->ar_gid = stbuf.st_gid;

	fileptr->ar_mode = stbuf.st_mode;
}


static FILE *
stats( file )
	register char *file;
{
	register FILE *f;

	f = fopen(file, "r");
	if (f == NULL)
		return(f);
	if (fstat(fileno(f), &stbuf) < 0)
	{
		(void) fclose(f);
		return(NULL);
	}
	return (f);
}


static char *
match( file )
	register char	*file;
{
	register int i;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
		if (strcmp(trim(namv[i]), file) == 0)
		{
			gfile = namv[i];
			file = namv[i];
			namv[i] = 0;
			return (file);
		}
	}
	return (NULL);
}



static void
mesg(c, file)
	int	c;
	char	*file;
{
	if (FLAG('v'))
		if (c != 'c' || FLAG('v') > 1)
			(void) fprintf(stdout, "%c - %s\n", c, file);
}



static char *
trimslash(s)
	char *s;
{
	static char buf[SNAME];

	/* we already verified that strlen(s) <= SNAME-2 */
	strcpy(buf, s);
	return (strcat(buf, "/"));
}


static char *
trim(s)
	char *s;
{
	register char *p1, *p2;

	for (p1 = s; *p1; p1++)
		;
	while (p1 > s)
	{
		if (*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for (p1 = s; *p1; p1++)
		if (*p1 == '/')
			p2 = p1 + 1;
	return (p2);
}

static void
choose(pairp, mode)
	int	*pairp;
	unsigned long	mode;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n >= 0 && (mode & *ap++) == 0)
		ap++;
	(void) putchar(*ap);
}


static void
mksymtab()
{
	register ARFILE	*fptr;
	long	mem_offset = 0;

	Elf_Scn	*scn;
	Elf32_Shdr *shdr;
	int newfd;

	for( fptr = listhead; fptr; fptr = fptr->next ) {
		newfd = -1;
	
		/* determine if file is coming from the archive or not */

		if ( (fptr->offset > 0) && (fptr->pathname == NULL) ) {

			if (elf_rand(arf, fptr->offset - sizeof(struct ar_hdr) )
				!= fptr->offset - sizeof(struct ar_hdr) ) {
				(void) pfmt(stderr, MM_ERROR, badlibelf,
					elf_errmsg(-1));
				exit(1);
			}

			if ((elf = elf_begin(fd, ELF_C_READ, arf)) == 0) {
				(void) pfmt(stderr, MM_ERROR,
					":26:Hit end of archive\n");
				errflag++;
				break;
				}
			}
		else
		 if ( (fptr->offset == 0) && (fptr->pathname != NULL) ){
			if ( (newfd  = open(fptr->pathname, O_RDONLY)) == -1)
			{
				(void) pfmt(stderr, MM_ERROR, badopen,
					fptr->pathname, strerror(errno));
				exit(1);
			}

			if ((elf = elf_begin(newfd, ELF_C_READ, (Elf *)0)) == 0)
			{
				(void) pfmt(stderr, MM_ERROR, badlibelf,
                                        elf_errmsg(-1));
                                exit(1);
			}
			if (elf_kind(elf) == ELF_K_AR){
				(void) pfmt(stderr, MM_ERROR, 
	":27:%s is in archive format - embedded archives are not allowed\n"
				,fptr->pathname);
				exit(1);
				}
			}
		else{
			(void) pfmt(stderr, MM_ERROR, ":28:Internal error - cannot tell whether file is included in archive or not\n");
			exit(1);
			}
			

		if (elf_kind(elf) == ELF_K_COFF) {
			if (elf_update(elf, ELF_C_NULL) == -1) {
				(void) pfmt(stderr, MM_ERROR, 
				":29:Trouble translating COFF file %s: %s\n"
				,fptr->pathname, elf_errmsg(-1));
				exit(1);
			}
		}

		if ((elf32_getehdr(elf)) != NULL)
		{
			/* loop through sections to find symbol table */
			scn = 0;
			while( (scn = elf_nextscn(elf,scn)) != 0)
			{
				if ((shdr = elf32_getshdr(scn)) == NULL)
				{
        			        (void) pfmt(stderr, MM_ERROR, badlibelf,
					elf_errmsg(-1));
			                break;
        			}
				if ( shdr->sh_type == SHT_SYMTAB)
				{
					search_sym_tab(elf,shdr,scn,mem_offset); 
				}
			}
		}

		mem_offset += sizeof( struct ar_hdr ) + ROUNDUP( fptr->ar_size);
		(void) elf_end(elf);
		if (newfd >= 0)
			(void) close(newfd);
			
	} /* for */
}

static void
writesymtab( tf )
	register FILE	*tf;
{
	long	offset;
	char	buf1[sizeof( struct ar_hdr ) + 1];
	register char	*buf2, *bptr;
	int	i, j;
	long	*ptr;

	/*
	* patch up archive pointers and write the symbol entries
	*/
	while ((str_top - str_base) & 0x3)	/* round up string table */
		*str_top++ = '\0';

	sym_tab_size = ((nsyms + 1) * 4) + (sizeof(char) * (str_top - str_base));

	offset = sym_tab_size + sizeof(struct ar_hdr) + SARMAG;

	(void) sprintf(buf1, FORMAT, SYMDIRNAME, time(0), (unsigned)0, (unsigned)0, (unsigned)0, (long)sym_tab_size, ARFMAG);

	if (longnames)
		offset += long_tab_size + sizeof(struct ar_hdr);

	if (strlen(buf1) != sizeof(struct ar_hdr))
	{
		(void) pfmt(stderr, MM_ERROR,	
			":30:Internal header generation error\n");
		exit(1);
	}

	if ((buf2 = (char *)malloc(4 * (nsyms + 1))) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, 
			":31:Cannot get space for number of symbols\n");
		exit(1);
	}
	sputl(nsyms, buf2);
	bptr = buf2 + 4;

	for (i = 0, j = SYMCHUNK, ptr = symlist; i < nsyms; i++, j--, ptr++)
	{
		if ( !j ) {
			j = SYMCHUNK;
			ptr = (long *) *ptr;
			}
		*ptr += offset;
		sputl( *ptr, bptr );
		bptr += 4;
	}


	(void) fwrite( buf1, 1, sizeof(struct ar_hdr), tf );
	(void) fwrite( buf2, 1, (nsyms  + 1) * 4, tf );
	(void) fwrite( str_base, 1,  sizeof(char) * (str_top - str_base), tf );
}


static void
savename(symbol)
	char    *symbol;
{
	static int str_length = BUFSIZ * 5;
	int len;

	if (str_base == (char *)0)	/* no space allocated yet */
	{
		if ((str_base = (char *)malloc((unsigned)str_length)) == NULL)
		{
			(void) pfmt(stderr, MM_ERROR, nostring, arnam);
			exit(1);
		}
		str_top = str_base;
	}

	len = strlen(symbol) + 1;

	if ((str_top + len) > (str_base + str_length))
	{
		char *old_base = str_base;

		str_length += BUFSIZ * 2;
		if ((str_base = (char *)realloc(str_base, str_length)) == NULL)
		{
			(void) pfmt(stderr, MM_ERROR, nogrowstring, arnam);
				exit(1);
		}
		str_top = str_base + (str_top - old_base);
	}
	strcpy(str_top, symbol);
	str_top += len;
}

static void
savelongname(fptr)
ARFILE	*fptr;
{
        static int	str_length = BUFSIZ * 2;
	static int	bytes_used;

        if (long_base == (char *)0)      /* no space allocated yet */
        {
                if ((long_base = (char *)malloc((unsigned)str_length)) == NULL)
                {
                        (void) pfmt(stderr, MM_ERROR, nostring, arnam);
                        exit(1);
                }
                long_top = long_base;
        }

	(void)sprintf(fptr->ar_name, "%s%d", LONGNAMEPREFIX, bytes_used);

	bytes_used += fptr->name_len + 2;
        if (bytes_used > str_length)
        {
		char	*old_base = long_base;
                str_length += BUFSIZ * 2;
                if ((long_base = (char *)realloc(long_base, str_length)) == NULL)
                {
                        (void) pfmt(stderr, MM_ERROR, nogrowstring, arnam);
                                exit(1);
                }
                long_top = long_base + (long_top - old_base);
        }

	strcpy(long_top, fptr->longname);
	long_top += fptr->name_len;
	*long_top++ = '/';
	*long_top++ = '\n';
}

static void
writefile()
{
	register ARFILE	*fptr;
	char		buf[ sizeof( struct ar_hdr ) + 1 ];
	char	       	buf11[sizeof( struct ar_hdr) + 1 ];
	register int i;

	mklong_tab();
	mksymtab();

	for (i = 0; signum[i]; i++) /* started writing, cannot interrupt */
                (void) signal(signum[i], SIG_IGN);
	
	/* Unless c flag, print message when archive is produced */
        if (!FLAG('c') && (access (arnam,(mode_t)00) != 0))
                (void) pfmt(stderr, MM_INFO, creating, arnam);

	if ((outfile = fopen( arnam, "w" )) == NULL)
	{
		(void) pfmt( stderr, MM_ERROR, badcreate, arnam, strerror(errno));
		exit(1);
	}

	(void) fwrite( ARMAG, sizeof(char), SARMAG, outfile );

	if ( nsyms )
		writesymtab( outfile ); 

	if (longnames)
	{
		(void) sprintf(buf11, FORMAT, LONGDIRNAME, time(0), (unsigned)0, (unsigned)0, (unsigned)0, (long)long_tab_size, ARFMAG);
		(void) fwrite(buf11, 1, sizeof(struct ar_hdr), outfile);
		(void) fwrite(long_base, 1, sizeof(char) * (long_top - long_base), outfile);
	}

	for ( fptr = listhead; fptr; fptr = fptr->next )
	{
	   if (fptr->name_len <= (unsigned)SNAME-2)
		(void) sprintf( buf, FORMAT, trimslash( fptr->longname), fptr->ar_date, (unsigned)fptr->ar_uid, (unsigned)fptr->ar_gid, (unsigned)fptr->ar_mode, fptr->ar_size, ARFMAG );

	   else
		(void) sprintf( buf, FORMAT, fptr->ar_name, fptr->ar_date, (unsigned)fptr->ar_uid, (unsigned)fptr->ar_gid, (unsigned)fptr->ar_mode, fptr->ar_size, ARFMAG );


		if (fptr->ar_size & 0x1)
			fptr->contents[ fptr->ar_size ] = '\n';

		(void) fwrite( buf, sizeof( struct ar_hdr ), 1, outfile );
		(void) fwrite( fptr->contents, ROUNDUP( fptr->ar_size ), 1, outfile );
	}

	if ( ferror( outfile ))
	{
		(void) pfmt( stderr, MM_ERROR, ":32:Cannot write archive: %s\n",
			strerror(errno));
		(void) unlink( arnam );
		exit(2);
	}

	(void) fclose( outfile );
}

static void
mklong_tab()
{
	ARFILE  *fptr;
	for ( fptr = listhead; fptr; fptr = fptr->next )
        {
		if (fptr->name_len >= (unsigned)SNAME-1)
		{
			longnames++;
			savelongname(fptr);
		}
	}
	if (longnames)
	{
		/* round up table that keeps the long filenames */
		/* while (not a multiple of 4) 			*/
		int	mod =  (long_top - long_base) & 0x3;
		int	diff;
		for(diff = 4 - mod; diff; diff--)
			*long_top++ = '\n';

		long_tab_size = sizeof(char) * (long_top - long_base);
	}
	return;

}

static
void sputl(n,cp)
	long n;
	char *cp;
{
	*cp++ =n/(256*256*256);
	*cp++ =n/(256*256);
	*cp++ =n/(256);
	*cp++ =n&255;
}

static void
search_sym_tab(elf, shdr, scn, mem_offset)
Elf *elf;
Elf32_Shdr *shdr;
Elf_Scn *scn;
long mem_offset;
{
	Elf_Data 	*str_data, *sym_data; /* string table, symbol table */
	Elf_Scn  	*str_scn;
	Elf32_Shdr	*str_shdr;
	int 		no_of_symbols, counter;
	char 		*symname;
	Elf32_Sym 	*p;
	int 		symbol_bind;

	if ((str_scn = elf_getscn(elf,shdr->sh_link)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
		errflag++;
		return;
	}
	if ((str_shdr = elf32_getshdr(str_scn)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
		errflag++;
                return;
        }

	if (str_shdr->sh_type != SHT_STRTAB) 
	{
		(void) pfmt(stderr, MM_ERROR, ":33:No string table for symbol names\n");
		errflag++;
		return;	
       	}

	str_data = 0;
	if (((str_data = elf_getdata(str_scn,str_data)) == 0 ) ||
		( str_data->d_size == 0))
	{
		(void) pfmt(stderr, MM_ERROR,":34:No data in string table\n");
		errflag++;
		return;
	}
	
	if (shdr->sh_entsize)
		no_of_symbols = shdr->sh_size/shdr->sh_entsize;
	else 
	{
		(void) pfmt(stderr, MM_ERROR,
		":36:A symbol table entry size of zero is invalid!\n");
		errflag++;

		return;
	}

	sym_data = 0;
	if ((sym_data = elf_getdata(scn, sym_data)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
		errflag++;
                return;
        }

	p = (Elf32_Sym *)sym_data->d_buf;
	p++; /* the first symbol table entry must be skipped */

	for (counter = 1; counter<no_of_symbols; counter++, p++) {

		symbol_bind = ELF32_ST_BIND(p->st_info);

		if ( ((symbol_bind == STB_GLOBAL) || (symbol_bind == STB_WEAK)) && ( p->st_shndx != SHN_UNDEF)  ) {
			symname = (char *)(str_data->d_buf) + p->st_name;
           		if ( !syms_left ) {
		                sym_ptr = (long *) malloc((SYMCHUNK + 1) * sizeof(long));
			         syms_left = SYMCHUNK;
		                if ( nextsym )
               			         *nextsym = (long) sym_ptr;
		                else
               			         symlist = sym_ptr;
		                nextsym = sym_ptr;
           		}

		        sym_ptr = nextsym;
		        nextsym++;
		        syms_left--;
		        nsyms++;
		        *sym_ptr = mem_offset;
			savename(symname); /* put name in the archiver's 
					      symbol table string table */
		}
	}
}

static void
synopsis(argc, argv)
int argc;
char *argv[];
{
	register int c;
	register minarg	= 1;
	register int key;

	while((c = getopt(argc, argv, "CTlvuabcisrdxtpmqV?")) != EOF)
		switch (c)
		{
		case 'l':
			(void) pfmt(stderr, MM_WARNING,
		":12:The l option will be removed in the next release\n");
			break;
		case 'u':
		case 's':
		case 'c':
		case 'a':
		case 'b':
		case 'i':
		case 'T':
		case 'C':
			FLAG(c)=1;
			break;
		case 'v':
			FLAG(c)++;
			break;
		case 'r':
			setcom(rcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'd':
			setcom(dcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'x':
			setcom(xcmd);
			FLAG(c)++;
			key = c;
			break;
		case 't':
			setcom(tcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'p':
			setcom(pcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'm':
			setcom(mcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'q':
			setcom(qcmd);
			FLAG(c)++;
			key = c;
			break;
		case 'V':
			(void) pfmt(stderr, MM_INFO|MM_NOGET,
					"%s %s\n", CPL_PKG, CPL_REL);
			break;
		case '?':
			errflag += 1; usage(); break;
		default:
			usage();
		}

	if (comfun == 0) {
		(void) pfmt(stderr, MM_ERROR,
			":13:One of [drqtpmx] must be specified\n");
		usage();
	}

	if ((FLAG('C') || FLAG('T')) && !FLAG('x')) {
		(void) pfmt(stderr, MM_ERROR,
				":44:-C|-T not allowed with -%c: only valid with -x\n",key);
			usage();
	}


	if ((FLAG('a') + FLAG('b') + FLAG('i')) > 1){
		(void) pfmt(stderr, MM_ERROR,
			":38:Only one of -a|-b|-i allowed\n");
		usage();
	}
	if (FLAG('i'))
		FLAG('b')++;
		
	if (!(FLAG('p') || FLAG('t') || FLAG('x')))
		minarg += 1;
	if (FLAG('a') || FLAG('b')){
		if (FLAG('r') || FLAG('m'))
			minarg += 1;
		else {
			(void) pfmt(stderr, MM_ERROR,
			     ":37:-a|-b|-i not allowed with -%c\n",key);
			usage();
		}
	}

	if ((argc - optind < minarg) 
			|| (FLAG('c') && !(FLAG('r') || FLAG('q')))
			|| (FLAG('u') && !FLAG('r'))) {
		pfmt(stderr, MM_ERROR, ":11:Incorrect usage\n");
		usage();
	}

	if (FLAG('a') || FLAG('b'))
	{
		ponam = trim(argv[optind]);
		optind++;
	}

	arnam = argv[optind];
	optind++;
	namv = &argv[optind];
	namc = argc - optind;
}
