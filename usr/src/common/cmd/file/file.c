/*		copyright	"%c%" 	*/

#ident	"@(#)file:common/cmd/file/file.c	1.17.2.19"
#ident "$Header$"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
#include	<ctype.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<stdio.h>
#include        <stdlib.h>
#include        <limits.h>
#include        <locale.h>
#include        <archives.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<pfmt.h>
#include	<string.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/core.h>

#include	 "proc.h"

/*
**	File Types
*/

#define	C_TEXT		0
#define FORTRAN_TEXT	1
#define AS_TEXT		2
#define NROFF_TEXT	3
#define CMD_TEXT	4
#define ENGLISH_TEXT	5
#define ASCII_TEXT	6
#define TEXT		7
#define PS_TEXT		8

/*
**	Misc
*/

#define BUFSZ	128
#define	FBSZ	4096	/* justify by the fact that standard block size
			 * for UFS is 8K and S5 is 2K bytes
			 */
#define	reg	register
#define NBLOCK  20

/* Assembly language comment char */
#define ASCOMCHAR '#'
char	fbuf[FBSZ];
char	*mfile = "/etc/magic";
						/* Fortran */
char	*fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char	*asc[] = {
	"sys","mov","tst","clr","jmp",0};
						/* C Language */
char	*c[] = {
	"int","char","float","double","short","long","unsigned","register",
	"static","struct","extern", 0};
						/* Assembly Language */
char	*as[] = {
	"globl","byte","even","text","data","bss","comm",0};

char 	cfunc[] = { '(', ')', '{', 0 };		/* state of C function */
char	*strchr();
char	*strcpy();

/* start for MB env */
wchar_t wchar;
int     length;
int     IS_ascii;
int     Max;
/* end for MB env */
int     i = 0;
int	fbsz;
int	ifd;
int	tret;
int	hflg = 0;
extern	char	**flaglist;
static int posix;
void	exit();
extern int stat(), lstat();
void	elfcore(int);

const	char	posix_var[] = "POSIX2";
const	char	badopen[]  = ":92:cannot open %s: %s\n";

#define	pfmte(s,f,c,a,b)	{putchar('\n'); fflush(stdout); pfmt((s),(f),(c),(a),(b));}

static prf(x)
char *x;
{
	if (pfmt(stdout, MM_NOSTD, "uxlibc:84:%s:\t", x) < 9)
		printf("\t");
}

main(argc, argv)
int  argc;
char **argv;
{
	reg	char	*p;
	reg	int	ch;
	reg	FILE	*fl;
	reg	int	cflg = 0, eflg = 0, fflg = 0;
	auto	char	ap[BUFSZ];
	extern	int	optind;
	extern	char	*optarg;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:file");

	if (getenv(posix_var)) 
		posix = 1;
	else
		posix = 0;

	while((ch = getopt(argc, argv, "chf:m:")) != EOF)
	switch(ch) {
	case 'c':
		cflg++;
		break;

	case 'f':
		fflg++;
		if ((fl = fopen(optarg, "r")) == NULL) {
			pfmte(stderr, MM_ERROR, badopen, optarg, strerror(errno));
			goto use;
		}
		break;

	case 'm':
		mfile = optarg;
		break;

	case 'h':
		hflg++;
		break;

	case '?':
		eflg++;
		break;
	}
	if(!cflg && !fflg && (eflg || optind == argc)) {
		if (!eflg)
			pfmt(stderr, MM_ERROR, ":194:Incorrect usage:\n");
use:
		pfmt(stderr, MM_INFO, 
			":195:Usage: file [-c] [-h] [-f ffile] [-m mfile] file...\n");
		exit(2);
	}
	if(mkmtab(mfile, cflg) == -1)
		exit(2);
	if(cflg) {
		prtmtab();
		exit(0);
	}
	for(; fflg || optind < argc; optind += !fflg) {
		reg	int	l;

		if(fflg) {
			if((p = fgets(ap, BUFSZ, fl)) == NULL) {
				fflg = 0;
				optind--;
				continue;
			}
			l = strlen(p);
			if(l > 0)
				p[l - 1] = '\0';
		} else
			p = argv[optind];
		prf(p);				/* print "file_name:<tab>" */

		tret += type(p);
		if(ifd)
			close(ifd);
	}
	if (tret != 0) {
		exit(tret);
	} else {
		exit(0);	/*NOTREACHED*/
	}
}

type(file)
char	*file;
{
	int	j,nl;
        int     cc,tar;
	char	ch;
	char	buf[BUFSIZ];
	struct	stat	mbuf;
        union   tblock  *tarbuf;                /* for tar archive file */
	int	(*statf)() = hflg ? lstat : stat;
        int     notdecl;
	int	cur_type = -1;
	ushort	is_c_flag;

	int skip_line();
	int c_comment();
	int c_define();
	int c_function();
	
	ifd = -1;
	if ((*statf)(file, &mbuf) < 0) {
		if (statf == lstat || lstat(file, &mbuf) < 0) {
			if (posix) {
				pfmt(stdout, MM_ERROR, badopen, file,
					strerror(errno));
				return 0;
			} else {
				pfmte(stderr, MM_ERROR, badopen, file, 
					strerror(errno));
				return(1);
			}
		}
	}
	switch (mbuf.st_mode & S_IFMT) {
	case S_IFCHR:
					/* major and minor, see sys/mkdev.h */
		pfmt(stdout, MM_NOSTD, ":196:character special (%d/%d)\n",
			major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return(0);

	case S_IFDIR:
		pfmt(stdout, MM_NOSTD, ":197:directory\n");
		return(0);

	case S_IFIFO:
		pfmt(stdout, MM_NOSTD, ":198:fifo\n");
		return(0);

 	case S_IFNAM:
			switch (mbuf.st_rdev) {
			case S_INSEM:
       	                 	pfmt(stdout, MM_NOSTD, ":199:Xenix semaphore\n");
       	                 	return(0);
			case S_INSHD:
       	                 	pfmt(stdout, MM_NOSTD, 
       	                 		":200:Xenix shared memory handle\n");
       	                 	return(0);
			default:
       	              	   	pfmt(stdout, MM_NOSTD, 
       	              	   		":201:unknown Xenix name special file\n");
       	               	  	return(0);
			}

	case S_IFLNK:
		if ((cc = readlink(file, buf, BUFSIZ)) < 0) {
			if (posix) {
				pfmt(stdout, MM_ERROR, badopen, file,
					strerror(errno));
				return 0;
			} else {
				pfmte(stderr, MM_ERROR,
					":202:Cannot read symbolic link %s: %s\n", 
					file, strerror(errno));
				return(1);
			}
		}
		buf[cc] = '\0';
		pfmt(stdout, MM_NOSTD, ":203:symbolic link to %s\n", buf);
		return(0); 

	case S_IFBLK:
					/* major and minor, see sys/mkdev.h */
		pfmt(stdout, MM_NOSTD, ":204:block special (%d/%d)\n",
			major(mbuf.st_rdev), minor(mbuf.st_rdev));
		return(0);
	}
        if ((tarbuf = (union tblock *) calloc(sizeof(union tblock) * NBLOCK, sizeof(char))) == (union tblock *) NULL) {
		pfmt(stderr, MM_ERROR, ":643:Cannot allocate physio buffer\n");
                return(1);
        }

	ifd = open(file, O_RDONLY);
	if(ifd < 0) {
		free(tarbuf);
		if (posix) {
			pfmt(stdout, MM_ERROR, badopen, file, strerror(errno));
			return 0;
		} else {
			pfmte(stderr, MM_ERROR, badopen, file, strerror(errno));
			return(1);
		}
	}
        /* read tar header */
        if ( (tar = read(ifd, tarbuf, TBLOCK)) < 0 ) {
		pfmte(stderr, MM_ERROR, ":205:Cannot read %s: %s\n",
			file, strerror(errno));
		free(tarbuf);
		return(1);
	}
        if(tar == 0) {
		pfmt(stdout, MM_NOSTD, ":644:Empty file\n");
		goto out;
	}
        if ( tar = (strncmp(tarbuf->tbuf.t_magic, "ustar", 5)) == 0) {
		pfmt(stdout, MM_NOSTD, ":645:tar \n");
                goto out;
        }
        if (lseek(ifd, 0, 0) == -1L) {
		pfmt(stderr, MM_ERROR, ":520:Device seek error: %s\n",
			strerror(errno));
		free(tarbuf);
                return(1);
        }
        if ((fbsz = read(ifd, fbuf, FBSZ)) == -1) {
		pfmte(stderr, MM_ERROR, ":205:Cannot read %s: %s\n",
			file, strerror(errno));
		free(tarbuf);
                return(1);
        }
	if(fbsz >= 6 && sccs()) {	/* look for "1hddddd" where d is a digit */
		pfmt(stdout, MM_NOSTD, ":207:sccs\n");
		goto out;
	}
	switch(ckmtab(fbuf,fbsz,0)){ /* Check against Magic Table entries */
		case -1:             /* Error */
			exit(2);
		case 0:              /* Not magic */
			break;
		default:             /* Switch is magic index */
			if (flaglist)
				for (i=0; flaglist[i]; ++i)
					if (!strcmp(flaglist[i],"elfcoreflag"))
						elfcore(ifd);
			goto out;
	}

	/* look at each line for either C 
    	 * definition/declaration statement or C function. 
	 * Skip comment, pre-processor and irrelevant lines.
	 * until the buffer is exhausted or C program is found.
	 * Avoid using goto in this C checking section.
	 */
	is_c_flag = i = 0;	/* reset buffer index pointer and is_c_flag */
	while (i < fbsz && !is_c_flag) {
		switch(fbuf[i++]) {
		case '/':
			if(fbuf[i++] != '*')
				skip_line();
			else
				c_comment();	/* C comment style */
			continue;
		case '#':	/* C preprocessor, skip it */
			skip_line();
			continue;
		case ' ':
		case '\t':
			skip_line();
			continue;
		case '\n':
			continue;
		default:
			i--;  	/* back track 1 character for lookup() */
			if (isalpha(fbuf[i])) {
				if (lookup(c) == 1) {
					if (c_define()) 
						is_c_flag = 1;
				} else {
					if (c_function())
						is_c_flag = 1;
				} /* braces here for clarity only */
				continue;
			}

			/* Not start with character, skip the line */
			skip_line();
			continue;
		} /* end switch */
	} 
	/* If is_c_flag is set, then we find C program,
	 * otherwise we already exhausted the buffer. 
         * Next step is to reset an index pointer and check 
	 * for fortran text.
    	 */
	if (is_c_flag) {
		cur_type = C_TEXT;
		goto outa;
	}
notc:
	i = 0;
	while(fbuf[i] == 'c' || fbuf[i] == '#') {
		while(fbuf[i++] != '\n')
			if(i >= fbsz)
				goto notfort;
	}
	if(lookup(fort) == 1){
		cur_type = FORTRAN_TEXT;
		goto outa;
	}
notfort:
	i = 0;
	if(ascom() == 0)
		goto notas;
	j = i-1;
	if(fbuf[i] == '.') {
		i++;
		if(lookup(as) == 1){
			goto isas;
		}
		else if(j != -1 && fbuf[j] == '\n' && isalpha(fbuf[j+2])){
			goto isroff;
		}
	}
	while(lookup(asc) == 0) {
		if(ascom() == 0)
			goto notas;
		while(fbuf[i] != '\n' && fbuf[i++] != ':')
			if(i >= fbsz)
				goto notas;
		while(fbuf[i] == '\n' || fbuf[i] == ' ' || fbuf[i] == '\t')
			if(i++ >= fbsz)
				goto notas;
		j = i - 1;
		if(fbuf[i] == '.'){
			i++;
			if(lookup(as) == 1) {
				goto isas;
			}
			else if(fbuf[j] == '\n' && isalpha(fbuf[j+2])) {
isroff:				cur_type = NROFF_TEXT;
				goto outa;
			}
		}
	}
isas:	cur_type = AS_TEXT;
	goto outa;
notas:
	/* start modification for multibyte env */	
	IS_ascii = 1;
        if (fbsz < FBSZ)
                Max = fbsz;
        else
                Max = FBSZ - MB_LEN_MAX; /* prevent cut of wchar read */
        /* end modification for multibyte env */
	for(i=0; i < Max; )
		if(fbuf[i]&0200) {
			IS_ascii = 0;
			if (fbuf[0]=='\100' && fbuf[1]=='\357') {
				pfmt(stdout, MM_NOSTD, ":209:troff output\n");
				goto out;
			}
		/* start modification for multibyte env */
			if ((length=mbtowc(&wchar, &fbuf[i],MB_LEN_MAX)) <= 0
			    || !wisprint(wchar)){
				pfmt(stdout, MM_NOSTD, ":208:data\n");
				goto out; 
			}
			i += length;
		}
		else
			i++;
	i = fbsz;
		/* end modification for multibyte env */
	if (mbuf.st_mode&(S_IXUSR|S_IXGRP|S_IXOTH))
		cur_type = CMD_TEXT;
	else if(postscript(fbuf, fbsz))
		cur_type = PS_TEXT;
	else if(english(fbuf, fbsz))
		cur_type = ENGLISH_TEXT;
	else if(IS_ascii)
		cur_type = ASCII_TEXT;
	else 
		cur_type = TEXT;
		/* for multibyte env */
outa:
	/* 
	 * This code is to make sure that no MB char is cut in half
	 * while still being used.
	 */
	fbsz = (fbsz < FBSZ ? fbsz : fbsz - MB_CUR_MAX + 1);
	while(i < fbsz){
		if (isascii(fbuf[i])){
			i++;
			continue;
		}
		else {
			if ((length=mbtowc(&wchar, &fbuf[i],MB_LEN_MAX)) <= 0
		        	|| !wisprint(wchar)){
		        switch(cur_type){
		        case C_TEXT:
		        	pfmt(stdout, MM_NOSTD,
		        		":210:c program text with garbage\n");
		        	break;
		        case FORTRAN_TEXT:
		        	pfmt(stdout, MM_NOSTD,
		        		":211:fortran program text with garbage\n");
				break;
			case AS_TEXT:
				pfmt(stdout, MM_NOSTD,
					":212:assembler program text with garbage\n");
				break;
			case NROFF_TEXT:
				pfmt(stdout, MM_NOSTD,
					":213:[nt]roff, tbl, or eqn input text with garbage\n");
				break;
			case CMD_TEXT:
				pfmt(stdout, MM_NOSTD,
					":214:commands text with garbage\n");
				break;
			case ENGLISH_TEXT:
				pfmt(stdout, MM_NOSTD,
					":215:English text with garbage\n");
				break;
			case ASCII_TEXT:
				pfmt(stdout, MM_NOSTD,
					":216:ascii text with garbage\n");
				break;
			case TEXT:
				pfmt(stdout, MM_NOSTD,
					":217:text with garbage\n");
				break;
			case PS_TEXT:
				pfmt(stdout, MM_NOSTD,
					":646:postscript program text with garbage\n");
				break;
			}
			goto out;
			}
			i = i + length;
		}
	}
	switch(cur_type){
	case C_TEXT:
		pfmt(stdout, MM_NOSTD, ":218:c program text\n");
		break;
	case FORTRAN_TEXT:
        	pfmt(stdout, MM_NOSTD, ":219:fortran program text\n");
        	break;
	case AS_TEXT:
		pfmt(stdout, MM_NOSTD, ":220:assembler program text\n");
		break;
	case NROFF_TEXT:
		pfmt(stdout, MM_NOSTD, ":221:[nt]roff, tbl, or eqn input text\n");
		break;
	case CMD_TEXT:
		pfmt(stdout, MM_NOSTD, ":222:commands text\n");
		break;
	case ENGLISH_TEXT:
		pfmt(stdout, MM_NOSTD, ":223:English text\n");
		break;
	case ASCII_TEXT:
		pfmt(stdout, MM_NOSTD, ":224:ascii text\n");
		break;
	case TEXT:
		pfmt(stdout, MM_NOSTD, ":225:text\n");
		break;
	case PS_TEXT:
		pfmt(stdout, MM_NOSTD, ":647:postscript program text\n");
		break;
	}
out:
	if(tarbuf != NULL)
		free(tarbuf);
	return(0);
}

lookup(tab)
reg	char **tab;
{
	reg	char	r;
	reg	int	k,j,l;

	while(fbuf[i] == ' ' || fbuf[i] == '\t' || fbuf[i] == '\n')
		i++;
	for(j=0; tab[j] != 0; j++) {
		l = 0;
		for(k=i; ((r=tab[j][l++]) == fbuf[k] && r != '\0');k++);
		if(r == '\0')
			if(fbuf[k] == ' ' || fbuf[k] == '\n' || fbuf[k] == '\t'
			    || fbuf[k] == '{' || fbuf[k] == '/') {
				i=k;
				return(1);
			}
	}
	return(0);
}
/*
 * Non-recursive check routine for C comment
 * If it fails return 0, otherwise return 1
 * Use global variable i and fbuf[]
 */
int
c_comment()
{
	reg	char	cc;

	while(fbuf[i] != '*' || fbuf[i+1] != '/') {
		if(fbuf[i] == '\\')
			i += 2;
		else
			i++;
		if(i >= fbsz)
			return(0);
	}
	if((i += 2) >= fbsz)
		return(0);
	return(1);
}
/* 
 * Skip the whole line in fbuf[], also
 * update global index (i).
 * Return 0 if go over limit, otherwise return 1
 */
int
skip_line()
{
 
	while (fbuf[i++] != '\n') {
		if (i >= fbsz )
			return(0);
	}
	return(1);
}
/*
 * Check if the line is likely to be a
 * C definition/declaration statement
 * If true return 1, otherwise return 0
 */
int
c_define()
{
	reg char  a_char;

	while ((a_char = fbuf[i++]) != ';' && a_char != '{' ) {
		if (i >= fbsz || a_char == '\n')
			return(0);
	}
	return(1);
}
/* 
 * Check if this line and the following lines
 * form a C function, normally in the format of
 * func_name(....) {
 * Also detect a line that can't possible be a
 * C function, i.e. page_t xxx; or STATIC ino_t y();
 * Update global array index (i) 
 * If true return 1, otherwise return 0
 */
int
c_function()
{
	reg int state = 0;

	while (cfunc[state] != 0) {
		if (fbuf[i++] == cfunc[state]) {
			if (state == 1 && (fbuf[i] == ';' || fbuf[i] == ','))
				return(0);
			state++;
		}
		if ((state == 0 && (fbuf[i] == ';' || fbuf[i] == '\n'))
			|| (i >= fbsz))
			return(0);
	}
	return(1);
}

ascom()
{
	while(fbuf[i] == ASCOMCHAR) {
		i++;
		while(fbuf[i++] != '\n')
			if(i >= fbsz)
				return(0);
		while(fbuf[i] == '\n')
			if(i++ >= fbsz)
				return(0);
	}
	return(1);
}

sccs() 
{				/* look for "1hddddd" where d is a digit */
	reg int j;

	if(fbuf[0] == 1 && fbuf[1] == 'h') {
		for(j=2; j<=6; j++) {
			if(isdigit(fbuf[j]))  
				continue;
			else  
				return(0);
		}
	} else {
		return(0);
	}
	return(1);
}

english (bp, n)
char *bp;
int  n;
{
#	define NASC 128		/* number of ascii char ?? */
	reg	int	j, vow, freq, rare;
	reg	int	badpun = 0, punct = 0;
	auto	int	ct[NASC];

	if (n<50)
		return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if (0 <= bp[j] && bp[j] < NASC)
			ct[bp[j]|040]++;
		switch (bp[j])
		{
		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if(j < n-1 && bp[j+1] != ' ' && bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if(2*ct[';'] > ct['e'])
		return(0);
	if((ct['>']+ct['<']+ct['/'])>ct['e'])
		return(0);	/* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}

postscript(bp, n)
char *bp;
int n;
{
	if (strncmp(bp, "%!PS", 4) == 0)
		return(1);
	else
		return(0);
}

Core_info	*core;

static void
setup_core_fd(int ifd)
{
	if ((lseek(ifd, 0, SEEK_SET)) >= 0) {
		Elf_Ehdr	ehdr;
		if (read(ifd, (char *)&ehdr, sizeof(Elf_Ehdr)) ==
		    sizeof(Elf_Ehdr)) {
			if ((strncmp((char *)ehdr.e_ident, ELFMAG,
			    SELFMAG) == 0) && (ehdr.e_type == ET_CORE)) {
				core = (Core_info *)malloc(sizeof(Core_info));
				if (!core)
					return;
				core->fd = ifd;
				core->ehdr = (Elf_Ehdr *)malloc(sizeof(Elf_Ehdr));
				if (!core->ehdr)
					return;
				memcpy((char *)core->ehdr, 
					(char *)&ehdr, sizeof(Elf_Ehdr));
				core->phdr = 0;
				core->pstatus = 0;
				core->psinfo = 0;
				return;
			}
		}
	}
	return;
}

static void
process_note(register char *p, int size)
{
	int		namesz, descsz, type;

	while (size > 0) {
		namesz = *(int *)p;	p += sizeof(int);
		descsz = *(int *)p;	p += sizeof(int);
		type   = *(int *)p;	p += sizeof(int);
		size -= 3 * sizeof(int) + namesz + descsz;
		p += namesz;
		switch(type) {
		default:
			break;
		case CF_T_PRSTATUS:
			core->pstatus = (pstatus_t *)p;
			break;
		case CF_T_PRPSINFO:
			core->psinfo = (psinfo_t *)p;
			break;
		}
		p += descsz;
	}
}

static void
setup_core_file()
{
	Elf_Off		phoff;
	Elf_Half	phentsz;
	Elf_Half	phnum;
	Elf_Phdr	*phdrp;
	int		i;

	phoff = core->ehdr->e_phoff;
	phentsz = core->ehdr->e_phentsize;
	phnum = core->ehdr->e_phnum;

	if (!phoff || !phentsz || !phnum) 
		return;
	core->phdr = (Elf_Phdr *)malloc(phnum * phentsz);
	if (!core->phdr)	/* if malloc failed psinfo will be 0 */
		return;
	if ((lseek(core->fd, phoff, SEEK_SET) == -1) ||
	    (read(core->fd, (char *)core->phdr, phnum * phentsz) !=
			(phnum * phentsz)))
		return;
	phdrp = core->phdr;
	for(i = 0; i < (int)phnum; i++, phdrp++) {
		if ( phdrp->p_type == PT_NOTE ) {
			register char	*p;
			int		filesz = phdrp->p_filesz;

			if (!filesz)
				continue;

			p = (char *)malloc(filesz);
			if (!p)		/* if malloc failed psinfo will be 0 */
				return;
			if ((lseek(core->fd, phdrp->p_offset, 0) < 0) ||
			    (read(core->fd, p, filesz) != filesz))
				return;
			process_note(p, filesz);
		}
	}
}

void
elfcore(int ifd)
{
	setup_core_fd(ifd);
	if (core) {
		setup_core_file(ifd);
		if (core->pstatus && core->psinfo)
			pfmt(stdout, MM_NOSTD, ":1130:\tArguments = '%s'\n", core->psinfo->pr_psargs);
	}
	return;
}
