#ident	"@(#)pcintf:pkg_lmf/lmfgen.c	1.3"
/* SCCSID(@(#)lmfgen.c	7.2	LCC)	/* Modified: 17:01:39 3/9/92 */

/*
 *  LMF message file generator
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include "lmf_int.h"

struct domain {
	struct domain	*d_next;
	struct domain	*d_child;
	char		 d_name[31];
	int		 d_nument;
	int		 d_type;
	int		 d_length;
	long		 d_offset;
};

struct domain *root = NULL;
struct domain *cur = NULL;
struct domain *make_domain();
struct domain *make_domain_node();

struct lmf_file_header hdr;

char quote_char = '\0';

#define BUFSIZE	4096
char buf[BUFSIZE];
char buf2[BUFSIZE];
char buf3[BUFSIZE];

FILE *fp;
int fd;
int flipBytes;		/* non-zero = flip */

long lseek();
char *malloc();
char *realloc();

#ifdef MSDOS
#define	ISSWITCH(c)	((c) == '-' || (c) == '/')
#else
#define	ISSWITCH(c)	((c) == '-')
#endif

main(argc, argv)
int argc;
char **argv;
{
	register char *bp;

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
	if (argc != 3) {
		usage();
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("lmfgen: Couldn't open %s\n", argv[1]);
		exit(2);
	}
	if ((fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666)) < 0) {
		fclose(fp);
		printf("lmfgen: Couldn't create %s\n", argv[2]);
		exit(3);
	}
	cur = make_domain_node(NULL, "", 1);
	write(fd, &hdr, sizeof(hdr));
	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (buf[0] == '$') {
			if (isspace(buf[1]) || buf[1] == '\0')
				continue;
			if (!strncmp(buf, "$domain", 7)) {
				do_domain();
				continue;
			}
			if (!strncmp(buf, "$quote", 6)) {
				do_quote();
				continue;
			}
			printf("lmfgen: Unknown command: %s\n", buf);
			continue;
		}
		for (bp = buf; *bp && isspace(*bp); bp++)
			;
		if (*bp)
			add_message(bp);
	}
	output_all_domains();
	close(fd);
	fclose(fp);
}


do_domain()
{
	register char *bp;
	register char *bp2;

	for (bp = &buf[7]; *bp && isspace(*bp); bp++)
		;
	for (bp2 = bp; *bp2 && !isspace(*bp2); bp2++)
		;
	*bp2 = '\0';
	cur = root;
	cur = make_domain(bp);
}


do_quote()
{
	register char *bp;

	for (bp = &buf[6]; *bp && isspace(*bp); bp++)
		;
	quote_char = *bp;
}


add_message(msg)
register char *msg;
{
	register char *bp, *str;
	struct domain *dom;
	int handling_quote;
	int len, length;
	long offset;

	for (bp = msg; *bp && !isspace(*bp); bp++)
		;
	if (*bp)
		*bp++ = '\0';
	while (*bp && isspace(*bp))
		bp++;
	if (*bp == '\0') {
		printf("lmfgen: Message %s doesn't have any text associated with it\n", msg);
		return;
	}
	remove_trailing_spaces(bp);
	str = buf2;
	length = 0;
	offset = lseek(fd, 0L, 1);
	handling_quote = 0;

	if (*bp && *bp == quote_char) {
		handling_quote++;
		bp++;
	}

	while (*bp) {
		if (handling_quote && *bp == quote_char && *++bp != quote_char)
			break;
		if (*bp == '\\' && (*++bp != quote_char || !handling_quote)) {
			switch (*bp) {
			case 'n': *str++ = '\n'; break;
			case 'r': *str++ = '\r'; break;
			case 't': *str++ = '\t'; break;
			case 'v': *str++ = '\v'; break;
			case 'b': *str++ = '\b'; break;
			case 'f': *str++ = '\f'; break;
			default: *str++ = *bp; break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				*str = *bp - '0';
				while (bp[1] >= '0' && bp[1] <= '7') {
					*str <<= 3;
					*str |= (*++bp - '0');
				}
				str++;
				break;

			case '\0':
				if (fgets(buf3, BUFSIZE, fp) == NULL) {
					printf("lmfgen: message string incomplete\n");
					return;
				}
				bp = buf3;
				while (*bp && isspace(*bp))
					bp++;
				remove_trailing_spaces(bp);
				if ((len = str - buf2) > 0) {
					if (write(fd, buf2, len) != len) {
						printf("lmfgen: message write failed.\n");
						exit(5);
					}
					length += len;
					str = buf2;
				}
				continue;
			}
			bp++;
		} else
			*str++ = *bp++;
	}
	*str = '\0';
	if ((len = str - buf2) > 0) {
		if (write(fd, buf2, len) != len) {
			printf("lmfgen: message write failed.\n");
			exit(5);
		}
		length += len;
		str = buf2;
	}

	for (bp = &msg[strlen(msg)]; bp > msg && *bp != '.'; bp--)
		;
	if (bp != msg) {
		*bp++ = '\0';
		make_message(make_domain(msg), bp, offset, length);
	} else
		make_message(cur, msg, offset, length);
}


remove_trailing_spaces(bp)
register char *bp;
{
	register char *bp2;

	for (bp2 = &bp[strlen(bp)]; bp2 > bp && isspace(bp2[-1]); bp2--)
		;
	*bp2 = '\0';
}


struct domain *
make_domain_node(par, name, type)
struct domain *par;
char *name;
int type;
{
	struct domain *dom, *dp;

	if (par && !strcmp(name, par->d_name)) {
		if (par->d_type != type)
			printf("lmfgen: Found %s, but types disagree\n", name);
		return par;
	}
	if (par && par->d_child) {
		for (dp = par->d_child;
		     dp != NULL && strcmp(name, dp->d_name);
		     dp = dp->d_next)
			;
		if (dp != NULL) {
			if (dp->d_type != type)
				printf("lmfgen: Found %s, but types disagree\n", name);
			return dp;
		}
	}
	dom = (struct domain *)malloc(sizeof(struct domain));
	if (dom == NULL) {
		printf("lmfgen: No memory in make_domain_node\n");
		exit(4);
	}
	dom->d_next = NULL;
	dom->d_child = NULL;
	strcpy(dom->d_name, name);
	dom->d_nument = 0;
	dom->d_type = type;
	dom->d_length = 0;
	dom->d_offset = 0L;

	if (par == NULL)
		return root = dom;
	if (par->d_child == NULL)
		return par->d_child = dom;
	if (strcmp(name, par->d_child->d_name) < 0) {
		dom->d_next = par->d_child;
		return par->d_child = dom;
	}
	for (dp = par->d_child; dp->d_next; dp = dp->d_next) {
		if (strcmp(name, dp->d_next->d_name) < 0) {
			dom->d_next = dp->d_next;
			return dp->d_next = dom;
		}
	}
	return dp->d_next = dom;
}


struct domain *
make_domain(dp)
char *dp;
{
	struct domain *dom;
	char *bp;

	dom = cur;
	while (*dp) {
		for (bp = dp; *bp && *bp != '.'; bp++)
			;
		if (*bp)
			*bp++ = '\0';
		dom = make_domain_node(dom, dp, 1);
		dp = bp;
	}
	return dom;
}


make_message(dom, msg, offset, len)
struct domain *dom;
char *msg;
long offset;
int len;
{
	struct domain *me;

	me = make_domain_node(dom, msg, 0);
	if (me->d_type || me->d_offset) {
		printf("lmfgen: Duplicate message %s\n", msg);
		return;
	}
	me->d_length = len;
	me->d_offset = offset;
}


output_all_domains()
{
	struct domain *dp;
	char *bp;
	int len;

	bp = buf;
	*bp = '\0';
	buf[1] = '\0';
	for (dp = root; dp->d_type &&
			dp->d_next == NULL &&
			dp->d_child != NULL; dp = dp->d_child) {
		if (dp != root)
			sprintf(bp, ".%s", dp->d_name);
		if (strlen(buf) >= LMFH_BASE_LEN) {
			*bp = '\0';
			break;
		}
		bp = &buf[strlen(buf)];
	}
	strcpy(hdr.lmfh_magic, "LMF");
	strcpy(hdr.lmfh_base_domain, &buf[1]);
	hdr.lmfh_base_attr = 1;
	output_domain(dp, &len, &hdr.lmfh_base_offset);
	hdr.lmfh_base_length = len;
	if (flipBytes) {
	    sflip(hdr.lmfh_base_length);
	    lflip(hdr.lmfh_base_offset);
	}
	lseek(fd, 0L, 0);
	write(fd, &hdr, sizeof(hdr));
}


output_domain(dom, lenp, offp)
struct domain *dom;
int *lenp;
long *offp;
{
	struct domain *dp;
	struct lmf_dirent *de;
	char *bp;
	static char *bf;
	static int buf_size = 0;
	int len;

	for (dp = dom; dp; dp = dp->d_next) {
		if (dp->d_type && dp->d_child != NULL)
			output_domain(dp->d_child, &dp->d_length,&dp->d_offset);
	}
	if (buf_size == 0 && (bf = (char *)malloc(buf_size = 4096)) == NULL) {
		printf("lmfgen: No memory output_domain\n");
		exit(6);
	}
	bp = bf;
	for (dp = dom; dp; dp = dp->d_next) {
		len = (strlen(dp->d_name) + 9 + 3) & 0xfc;
		if (bp + len >= &bf[buf_size]) {
			len = bp - bf;
			buf_size <<= 1;
			if ((bf = (char *)realloc(bf, buf_size)) == NULL) {
				printf("lmfgen: No memory output_domain\n");
				exit(6);
			}
			bp = &bf[len];
			len = (strlen(dp->d_name) + 9 + 3) & 0xfc;
		}
		de = (struct lmf_dirent *)bp;
		de->lmfd_len = len;
		de->lmfd_attr = dp->d_type;
		de->lmfd_length = dp->d_length;
		de->lmfd_offset = dp->d_offset;
		if (flipBytes) {
		    sflip(de->lmfd_length);
		    lflip(de->lmfd_offset);
		}
		strcpy(de->lmfd_name, dp->d_name);
		bp += len;
	}
	*lenp = bp - bf;
	*offp = lseek(fd, 0L, 1);
	if (write(fd, bf, *lenp) != *lenp) {
		printf("lmfgen: write domain failed\n");
		exit(7);
	}
}

usage()
{
	printf("usage: lmfgen input output\n");
#ifdef MSDOS
	fprintf(stderr, "       lmfgen /h or /? prints this message\n");
#else
	fprintf(stderr, "       lmfgen -h or -? prints this message\n");
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
