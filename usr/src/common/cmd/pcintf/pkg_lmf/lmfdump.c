#ident	"@(#)pcintf:pkg_lmf/lmfdump.c	1.3"
/* SCCSID(@(#)lmfdump.c	7.2	LCC)	/* Modified: 17:03:49 3/9/92 */

/*
 *  LMF message file dumper
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "lmf_int.h"

struct lmf_file_header hdr;

unsigned char buf[4096];
char name_buf[128];
char *name;
int fd;
int flipBytes;		/* non-zero = flip */

char *malloc();

#ifdef MSDOS
#define	ISSWITCH(c)	((c) == '-' || (c) == '/')
#else
#define	ISSWITCH(c)	((c) == '-')
#endif

main(argc, argv)
int argc;
char **argv;
{
	/* determine machine byte ordering */
	flipBytes = get_byteorder();

	if (argc > 1 && ISSWITCH(argv[1][0])) {
		if (argv[1][1] == '?' || argv[1][1] == 'h' || argv[1][1] == 'H')
			usage();
		else if (ISSWITCH(argv[1][1])) {
			argc--;
			argv++;
		}
	}
	if (argc == 1) {
		usage();
	}
	while (--argc)
		process_file(*++argv);
}


process_file(file)
char *file;
{
	if ((fd = open(file, O_RDONLY|O_BINARY)) < 0) {
		fprintf(stderr, "lmfdump: Can't open file %s\n", file);
		return;
	}
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr) ||
	    strcmp(hdr.lmfh_magic, "LMF")) {
		fprintf(stderr, "lmfdump: invalid file format: %s\n", file);
		close(fd);
		return;
	}
	if (flipBytes) {
	    sflip(hdr.lmfh_base_length);
	    lflip(hdr.lmfh_base_offset);
	}
	name_buf[0] = '\0';
	name = &name_buf[1];
	*name = '\0';
	printf("$domain %s\n\n", hdr.lmfh_base_domain);
	if (hdr.lmfh_base_attr & 0x01)
		process_domain(hdr.lmfh_base_offset, hdr.lmfh_base_length);
	else
		process_message(hdr.lmfh_base_offset, hdr.lmfh_base_length);
	close (fd);
}

process_domain(off, len)
long off;
int len;
{
	char *dom;
	char *namep;
	struct lmf_dirent *de;
	int i, l;

	if ((dom = (char *)malloc(len)) == NULL) {
		fprintf(stderr, "lmfdump: can't allocate domain %s\n", name);
		return;
	}
	namep = &name_buf[strlen(name_buf)];
	lseek(fd, off, 0);
	if (read(fd, dom, len) != len) {
		fprintf(stderr, "lmfdump: read error on domain %s\n", name);
		free(dom);
		return;
	}
	for (i = 0; i < len; i += l) {
		de = (struct lmf_dirent *)&dom[i];
		if (flipBytes) {
		    sflip(de->lmfd_length);
		    lflip(de->lmfd_offset);
		}
		l = de->lmfd_len;
		sprintf(namep, ".%s", de->lmfd_name);
		if (de->lmfd_attr & LMFA_DOMAIN)
			process_domain(de->lmfd_offset, de->lmfd_length);
		else
			process_message(de->lmfd_offset, de->lmfd_length);
	}
	*namep = '\0';
	free(dom);
}


process_message(off, len)
long off;
int len;
{
	register unsigned char *bp, *ebp;

	bp = buf;
	if (len >= 4096 && (bp = (unsigned char *)malloc(len + 1)) == NULL) {
		fprintf(stderr, "lmfdump: Can't allocate space for message %s\n", name);
		return;
	}
	lseek(fd, off, 0);
	if (read(fd, bp, len) != len) {
		fprintf(stderr, "lmfdump: Error reading message %s\n", name);
		if (len >= 4096)
			free(bp);
		return;
	}
	ebp = &bp[len];
	*ebp = '\0';
	printf("%s\t", name);
	while (bp < ebp) {
		switch (*bp) {
		case '\n': printf(((bp+1) != ebp) ? "\\n\\\n\t" : "\\n"); break;
		case '\r': putchar('\\'); putchar('r'); break;
		case '\t': putchar('\\'); putchar('t'); break;
		case '\v': putchar('\\'); putchar('v'); break;
		case '\b': putchar('\\'); putchar('b'); break;
		case '\f': putchar('\\'); putchar('f'); break;
		case '\\': putchar('\\'); putchar('\\'); break;
		default:
			if (*bp < ' ')
				printf("\\%o", *bp);
			else
				putchar(*bp);
			break;
		}
		bp++;
	}
	putchar('\n');
	if (len >= 4096)
		free(bp);
}

usage()
{
	fprintf(stderr, "usage: lmfdump file [file] ...\n");
#ifdef MSDOS
	fprintf(stderr, "       lmfdump /h or /? prints this message\n");
#else
	fprintf(stderr, "       lmfdump -h or -? prints this message\n");
#endif
	exit(1);
}

/*
 *  int get_byteorder()
 *
 *	This function determines the machine byte ordering.
 *	Returns 0 if flipping is not necessary.
 *	Returns 1 if flipping is necessary (i.e. big endian architecture).
 */
int
get_byteorder()
{
	int	ivar = 1;

	if (*(char *)&ivar)
		return 0;
	else 
		return 1;
}
