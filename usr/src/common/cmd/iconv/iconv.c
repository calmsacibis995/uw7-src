/*	copyright	"%c%"	*/

#ident	"@(#)iconv.c	1.2"

/*
 * iconv.c	code set conversion
 */

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>
#include <iconv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int optind;
extern int opterr;
extern char *optarg;

iconv_t __iconv_open(const char *tcode, const char *fcode, const char *md);

#ifndef LINEMAX
#define LINEMAX 4096
#endif

#define min(a1, a2)	((a1) < (a2) ? (a1) : (a2))

#define ERR_CANNOTOPEN	":18:Cannot open %s: %s\n"
#define ERR_NOTSUPPM	":55:No support for %s to %s: mode %s\n"
#define ERR_NOTSUPP	":56:No support for %s to %s\n"
#define ERR_INITFAIL	":199:Could not initialize conversion: %s\n"
#define ERR_NOTCONV	":200:Could not convert byte %d in file \"%s\"\n"
#define ERR_INCCHAR	":201:Incomplete character at end of file \"%s\"\n"
#define ERR_UNKNOWN	":202:Error at byte %d in file \"%s\": %s\n"
#define ERR_CLOSE	":203:Error in closing: %s\n"
#define ERR_USAGE1	":26:Incorrect usage\n"
#define ERR_USAGE2	":57:Usage: iconv -f fromcode -t tocode [-m mode] [file ...]\n"

char inbuf[LINEMAX];
char outbuf[LINEMAX];

main(argc, argv)
int argc;
char **argv;
{
	int c;
	char *fcode;
	char *tcode;
	char *mode;			/* Optional argument mode */
	int fd, retval = 0;
	iconv_t cd;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxmesg");
	(void)setlabel("UX:iconv");

	fcode = (char*)NULL;
	tcode = (char*)NULL;
	mode  = (char*)NULL;
	c = 0;

	/*
	 * what about files
	 */
	while ((c = getopt(argc, argv, "f:t:m:")) != EOF) {

		switch (c) {

			case 'f':
				fcode = optarg;
				break;	

			case 't':
				tcode = optarg;
				break;

			/* New case to provide ES 3.2 compatability */
			case 'm':
				mode = optarg;
				break;

			default:
				usage_iconv(0);
				exit(1);
		}

	}

	/* required arguments */
	if (!fcode || !tcode) {
		usage_iconv(1);
		exit(1);
	}

	if ((cd = __iconv_open(tcode, fcode, mode)) == (iconv_t) -1) {
		if (errno == EINVAL) {
			if (mode)
				pfmt(stderr, MM_ERROR, ERR_NOTSUPPM,
					fcode, tcode, mode);
			else
				pfmt(stderr, MM_ERROR, ERR_NOTSUPP,
					fcode, tcode);
		} else {
			pfmt(stderr, MM_ERROR, ERR_INITFAIL,
				strerror(errno));
		}
		exit(1);
	}

	if (optind < argc) {
		/*
		 * there are files
		 */
		while (optind < argc) {
			if ((fd = open(argv[optind], O_RDONLY)) == -1) {
				pfmt(stderr, MM_ERROR, ERR_CANNOTOPEN,
					argv[optind], strerror(errno));
				retval++;
			} else {
				retval += process(fd, argv[optind], cd);
				close(fd);
			}
			optind++;
		}
	} else {
		retval += process(0, gettxt(":204", "stdin"), cd);
	}

	if (iconv_close(cd) == -1) {
		pfmt(stderr, MM_ERROR, ERR_CLOSE, strerror(errno));
		if (retval == 0) {
			retval++;
		}
	}
	exit(retval);
}

int
process(int fd, const char *fname, iconv_t cd)
{
	char *inptr, *outptr;
	size_t byte, bytesleft, spaceleft, linebytes;
	int eof = 0;
	char *ct, *cf;
	int cnt;

	byte = 1;
	inptr = inbuf;
	bytesleft = 0;
	while ((linebytes = read(fd, inptr, LINEMAX - bytesleft)) > 0 ||
			bytesleft > 0) {
		inptr = inbuf;
		bytesleft += linebytes;
		eof = linebytes == 0 ? 1 : 0;	/* true if read returned 0 */
		linebytes = bytesleft;
		spaceleft = LINEMAX;
		outptr = outbuf;

		if (iconv(cd, &inptr, &bytesleft, &outptr, &spaceleft) ==
								(size_t) -1) {
			switch (errno) {
			case EILSEQ:
				write(1, outbuf, LINEMAX - spaceleft);
				pfmt(stderr, MM_ERROR, ERR_NOTCONV,
					byte + linebytes - bytesleft, fname);
				return(1);
				break;
			case E2BIG:
				break;		/* write outbuf and continue */

			case EINVAL:
				if (eof) { 	/* if at EOF */
					write(1, outbuf, LINEMAX - spaceleft);
					pfmt(stderr, MM_ERROR,
						ERR_INCCHAR, fname);
					return(1);
				}		/* write outbuf and continue */
				break;
			default:
				write(1, outbuf, LINEMAX - spaceleft);
				pfmt(stderr, MM_ERROR,
					ERR_UNKNOWN,
					byte + linebytes - bytesleft,
					fname, strerror(errno));
				return(1);
				break;
			}
			cf = inptr;
			ct = inbuf;
			cnt = bytesleft;
			while (cnt--) {
				*ct++ = *cf++;
			}
			inptr = ct;
		} else {
			inptr = inbuf;
		}
		write(1, outbuf, LINEMAX - spaceleft);
		byte += linebytes - bytesleft;
	}
		
	return(0);
}


usage_iconv(complain)
int complain;
{
	if (complain) {
		pfmt(stderr, MM_ERROR, ERR_USAGE1);
	}
	pfmt(stderr, MM_ACTION, ERR_USAGE2);
}
