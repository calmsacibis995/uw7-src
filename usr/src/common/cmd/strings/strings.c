/*		copyright	"%c%" 	*/

#ident	"@(#)strings:common/cmd/strings/strings.c	1.3.7.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft 	*/
/*	Corporation and should be treated as Confidential.	*/


/*
 *	@(#) strings.c 1.3 88/05/09 strings:strings.c
 */
/* Copyright (c) 1979 Regents of the University of California */
#include <stdio.h>
#include "x.out.h"
#include <ctype.h>
#include <libelf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

# define NOTOUT		0
# define AOUT		1
# define BOUT		2
# define XOUT		3
# define ELF		4

long	ftell();
#define dirt(c) (!isprint(c))
/*
 * Strings - extract strings from an object file for whatever
 *
 * Bill Joy UCB 
 * April 22, 1978
 *
 * The algorithm is to look for sequences of "non-junk" characters
 * The variable "minlen" is the minimum length string printed.
 * This helps get rid of garbage.
 * Default minimum string length is 4 characters.
 *
 *	MODIFICATION HISTORY:
 *	M000	05 Dec 82	andyp
 *	- Made handle x.out, b.out formats; previously only handled a.out.
 *	  Moos are only approximate, there was a fair amount of reorganizing.
 */

/*struct	exec header;*/						/*M000*/
union uexec {								/*M000*/
	struct xexec	u_xhdr;	/* x.out */				/*M000*/
	struct aexec	u_ahdr;	/* a.out */				/*M000*/
	struct bexec	u_bhdr;	/* b.out */				/*M000*/
} header;								/*M000*/
struct xexec	*xhdrp	= &(header.u_xhdr);				/*M000*/
struct aexec	*ahdrp	= &(header.u_ahdr);				/*M000*/
struct bexec	*bhdrp	= &(header.u_bhdr);				/*M000*/

void	usage();
char	*infile = "Standard input";
int	dflg, oflg, xflg;
int	asdata;
long	offset;
int	minlength = 4;

main(argc, argv)
	int argc;
	char *argv[];
{
	int  hsize, htype;						/*M000*/
	int fd;
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	char *scn_name;
	int errflag = 0;

	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:strings");
	(void)setcat("uxue");

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
	    register int i;
	    char *opt;
	    if (argv[0][1] == 0)
		asdata++;
	    else if (strcmp(argv[0], "--") == 0)	{
		argc--; argv++;
		break;
	    }
	    else
		for (i = 1; argv[0][i] != 0; i++)
		    switch (argv[0][i]) {

		    case 'a':
			asdata++;
			break;

		    case 'n':
			if (argv[0][i+1] != 0)
			    minlength = getnum(&(argv[0][i+1]));
			else {
			    argc--; argv++;
			    if (argc > 0)
			    	minlength = getnum(argv[0]);
			    else {
				(void)pfmt(stderr, MM_ERROR,
				  ":339:-%c option requires an argument\n", 'n');
				usage();
			    }
			}
			goto while_end;

		    case 'o':
			if (oflg || xflg) {
			    (void)pfmt(stderr, MM_ERROR, 
				":340:More than one offset format specified.\n");
			    usage();
			}
			dflg++;
			break;

		    case 't':
			if (argv[0][i+1] != 0)
				opt = &(argv[0][i+1]);
			else {
				argc--; argv++;
			    if (argc > 0)
				opt = argv[0];
			    else {
				(void)pfmt(stderr, MM_ERROR,
				  ":339:-%c option requires an argument\n", 't');
				usage();
			    }
			}
			if ((strlen(opt) == 1) &&
			    (strchr("dox", *opt) != NULL)) {
			    switch (*opt) {
			    case 'd':
				dflg++;
				break;
			    case 'o':
				oflg++;
				break;
			    case 'x':
				xflg++;
				break;
			    }
			    if ((dflg && oflg) || (dflg && xflg) ||
				(oflg && xflg))	{
			    	(void)pfmt(stderr, MM_ERROR,
				":340:More than one offset format specified.\n");
				usage();
			   }
			} else {
			    (void)pfmt(stderr, MM_ERROR,
			    ":341:Invalid format specified with -t option: %s\n",
				opt);
			    (void)pfmt(stderr, MM_ACTION,
				":342:Valid formats are d, o and x\n");
			    exit(1);
			}
			goto while_end;

		    default:
			if (!isdigit(argv[0][i])) {
				pfmt(stderr, MM_ERROR,
				    ":343:Invalid option: %c\n", argv[0][i]);
				usage();
			}
			minlength = getnum(&(argv[0][i]));
			goto while_end;
		    }
while_end:
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			infile = argv[0];
			argc--, argv++;
			if (freopen(infile, "r", stdin) == NULL) {
				(void)pfmt(stderr, MM_ERROR, ":344:%s: %s\n",
					argv[0], strerror(errno));
				errflag++;
				continue;
			}
		}
		/* M000 begin */
		if (asdata) 
			htype =  NOTOUT;
		else {
			hsize = fread ((char *) &header, sizeof (char),
					sizeof (header), stdin);
			htype = ismagic (hsize, &header, stdin);
		}
		switch (htype) {
			case AOUT:
				fseek (stdin, (long) ADATAPOS (ahdrp), 0);
				find ((long) ahdrp->xa_data);
				continue;
			case BOUT:
				fseek (stdin, (long) BDATAPOS (bhdrp), 0);
				find ((long) bhdrp->xb_data);
				continue;
			case XOUT:
				fseek (stdin, (long) XDATAPOS (xhdrp), 0);
				find ((long) xhdrp->x_data);
				continue;
			case ELF:
				fd = fileno(stdin);
				lseek(fd, 0L, 0);
				elf = elf_begin(fd, ELF_C_READ, NULL);
				ehdr = elf32_getehdr(elf);
				scn = 0;
				while ((scn = elf_nextscn(elf, scn)) != 0)
				{
			 		if ((shdr = elf32_getshdr(scn)) != 0)
			       			scn_name = elf_strptr(elf, ehdr->e_shstrndx, (size_t)shdr->sh_name);
					/* There is more than one */
					/* .data section */

					if ((strcmp(scn_name, ".rodata") == 0) ||
						(strcmp(scn_name, ".rodata1") == 0) ||
						(strcmp(scn_name, ".data") == 0) ||
						(strcmp(scn_name, ".data1") == 0))
					{
						fseek(stdin, (long) shdr->sh_offset, 0);
						find((long) shdr->sh_size);
					}
		 		}
				continue;
			case NOTOUT:
			default:
				fseek(stdin, (long) 0, 0);
				find((long) 100000000L);
				continue;
		}
		/* M000 end */
	} while (argc > 0);
	exit(errflag);
}

void
usage()
{
	(void)pfmt(stderr, MM_ACTION,
		":345:Usage:\nstrings [ -a | - ] [ -t format | -o ] [-n number | -number ] [file ...]\n");
	exit(1);
}

int
getnum(char *numstring)
{
	char *leftover;
	int num;
	
	errno = 0;
	num = (int)strtol(numstring, &leftover, 10);
/*
printf("num = %d, leftover = %s\n", num, leftover);
perror("errno");
*/
	if (strlen(leftover) > 0)	{
    		(void)pfmt(stderr, MM_ERROR,
			":346:Invalid number: %s\n", numstring);
		usage();
	}
	else if (( num <= 0 ) || (errno == ERANGE)) {
    		(void)pfmt(stderr, MM_ERROR,
			":347:Number out of range: %s\n", numstring);
		exit(1);
	}
	return(num);
}



find(cnt)
	long cnt;
{
	static char *buf;
	static int bsz;
	register int c, cc;

	if (buf == 0) {
		if ((buf = malloc(BUFSIZ)) == NULL) {
			(void)pfmt(stderr, MM_ERROR,
				  ":60:could not malloc enough memory\n");
			exit(1);
		}
		bsz = BUFSIZ;
	}

	cc = 0;
	for (c = !EOF; (cnt > 0) && (c != EOF); cnt--) {
		c = getc(stdin);
		if (dirt(c)) {
			if (cc >= minlength) {
				if (dflg)
					printf("%7ld ", ftell(stdin) - cc - 1);
				else if (oflg)
					printf("%7lo ", ftell(stdin) - cc - 1);
				else if (xflg)
					printf("%7lx ", ftell(stdin) - cc - 1);
				buf[cc] = '\0';
				puts(buf);
			}
			cc = 0;
		} else {
			if (cc >= bsz-2) {	/* -2 ensures room for null */
				if ((buf = realloc(buf, 2*bsz)) == NULL) {
					(void)pfmt(stderr, MM_ERROR,
				  		":60:could not malloc enough memory\n");
					exit(1);
				}
				bsz *= 2;
			}
			buf[cc] = c;
			++cc;
		}
	}
}

/* M000 begin */
ismagic(hsize, hdr, fp)
	int hsize;
	union uexec *hdr;
	FILE *fp;
{
	switch ((int) (hdr->u_bhdr.xb_magic)) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
		case A_MAGIC4:
			if (hsize < sizeof (struct bexec))
				return (NOTOUT);
			else
				return (BOUT);
		default:
			break;
	}
	switch (hdr->u_xhdr.x_magic) {
		case X_MAGIC:
			if (hsize < sizeof (struct xexec))
				return (NOTOUT);
			else
				return (XOUT);
		default:
			break;
	}
	switch (hdr->u_ahdr.xa_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
		case A_MAGIC4:
			if (hsize < sizeof (struct aexec))
				return (NOTOUT);
			else
				return (AOUT);
		default:
			break;
	}
	return (tryelf(fp));
}
/* M000 end */


tryelf(fp)
FILE *fp;
{
	int fd;
	Elf *elf;
	Elf32_Ehdr *ehdr;

	fd = fileno(fp);

	if ((elf_version(EV_CURRENT)) == EV_NONE) {
		fprintf(stderr, "%s\n", elf_errmsg(-1));
		return(NOTOUT);
	}

	lseek(fd, 0L, 0);

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		fprintf(stderr, "%s\n", elf_errmsg(-1));
		return(NOTOUT);
	}

	if ((elf_kind(elf) == ELF_K_NONE) ||
	    (elf_kind(elf) == ELF_K_AR)) {
		elf_end(elf);
		return(NOTOUT);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "%s\n", elf_errmsg(-1));
		elf_end(elf);
		return(NOTOUT);
	}

	if ((ehdr->e_type == ET_CORE) || (ehdr->e_type == ET_NONE)) {
		elf_end(elf);
		return(NOTOUT);
	}

	elf_end(elf);

	return(ELF);

}
