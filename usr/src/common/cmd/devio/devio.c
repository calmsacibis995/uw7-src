#ident	"@(#)devio:devio/devio.c	1.8"
#ident	"$Header$"
/*#copyright	"%c%"*/

#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * Uncomment to turn-on debugging.
#define DEBUG	1
 */

/*
 *  Text Messages
 */
#define USAGE     ":1:devio -[I|O] pathname [-p -v -h 'header key (50 bytes)' ] [ -G file ] [-M message] [-b]\n"
#define EHDR_FMT  ":2:\nHeader Expected:\n\tMedia #: %d\n\tKey: %s\n\tDate: %s\n\n"
#define FHDR_FMT  ":3:\nMedia Header:\n\tMedia #: %d\n\tKey: %s\n\tDate: %s\n\n"
#define IMEDIA    ":4:\aInsert Media #%d: " /*should always say please */
#define MEDIA_KEY ":5:Invalid media record key\n"
#define NO_MEDIA  ":6:No Media present in device\n"
#define RO_MEDIA  ":7:Media is read-only\n"
#define WRG_MEDIA ":8:Wrong media present in device\n"
#define PIMEDIA   ":9:\aPlease, Insert Media #%d: "
#define TWRT_FMT  ":10:\n\nTOTALS:\nData Bytes Read = %d\nData Bytes Written = %d\n\n"
#define TRD_FMT   ":11:\n\nTOTALS:\nData Bytes Read = %d\nData Bytes Written = %d\n\n"

#ifdef DEBUG
#define DFBR_FMT  ":12:\n\nData Bytes Read = %d\nMedia Bytes Read = %d\n\n"
#define DFBW_FMT  ":13:\n\nData Bytes Written = %d\nMedia Bytes Written = %d\n\n"
#define RCNT_FMT  ":14:Record Count: %d\n"
#define MTWRT_FMT ":15:Media Bytes Written = %d\n"
#define MTRD_FMT  ":16:Media Bytes Read = %d\n"
#endif

#define ALONE_FMT  ":17:No prompting allowed in batch mode.\n"

#define SYS_ERR_FMT	"%s:%s\n" /* for perror(3c) analog */

#define SYS_ERR_FLAGS	MM_STD|MM_NOGET|MM_ERROR
#define PROMPT_FLAGS	MM_NOSTD|MM_GET|MM_INFO
#define TRACE_FLAGS	MM_NOSTD|MM_GET
#define PGM_ERR_FLAGS	MM_STD|MM_GET|MM_ERROR

/*
 *  Exit Codes
 */
#define EC_ARGUMENT	1	/* argument check */
#define EC_CNT_CHK	2	/* read/write counter failure */
#define EC_IOFAIL	4	/* read/write/open failure */

/*
 *  Buffer and data sizes
 */
#define DEV_BUFSIZE	4096

#define REC_INFOSIZE	6
#define DATA_BUFSIZE	(DEV_BUFSIZE-REC_INFOSIZE)

#define STR_BUFSIZE	52
#define DATE_BUFSIZE	26
#define HDR_INFOSIZE	(14+STR_BUFSIZE+DATE_BUFSIZE)
#define HDR_BUFSIZE	(DEV_BUFSIZE-HDR_INFOSIZE)

/*
 * Media header check values
 */
#define NOMEDIACHK	0x00
#define MEDIACHK	0x01

#define MEDIA_INSERT_TRIES	6

/*
 *  Record keys
 */ 
#define FIRST_KEY	(unsigned)0x0F0F0F0F
#define HDR_KEY		(unsigned)0x1234ABCD
#define REC_KEY		(unsigned)0x1234FFFF
#define TLR_KEY		(unsigned)0xABCD1234

/* header/trailer record layout */
typedef struct hdr_str {
	ulong	key;
	ulong	random_key;
	ulong	dev_cnt;
	char	str_key[STR_BUFSIZE];
	char	date[DATE_BUFSIZE];
	ushort	size;
	char	buf[HDR_BUFSIZE];
} hdr_t;

/* data record layout */
typedef struct rec_str {
	ulong	key;
	ushort	size;
	char	buf[DATA_BUFSIZE];
} rec_t;

/* format of device record */
typedef union rec_un {
	hdr_t	hdr;
	rec_t	rec;
	char	buf[DEV_BUFSIZE];
} dev_rec_t;

/*
 *  Declarations
 */
static void device_writer	(char *, char *);
static void device_reader	(char *, char *);
static int  open_wrtdevice	(char *, dev_rec_t *, int);
static int  open_rddevice	(char *, dev_rec_t *, dev_rec_t *, int);
static int  write_header	(dev_rec_t *, ssize_t *, char *);
static int  write_hdrdata	(int, dev_rec_t *, int *);
static int  compare_header		(dev_rec_t *, dev_rec_t *);
static int  compare_header_for_chg	(dev_rec_t *, dev_rec_t *);
static void usage_exit		();
static void ttyready		();
static void prompt4media	(ulong);
static void header_trace	();
static void record_trace	();

/*
 *  Global Variables
 */
static FILE	*ttyin;			/* TTY input stream */ 
static FILE	*ttyout;		/* TTY output stream */
static char	*ttypath="/dev/tty";	/* TTY pathname */
static int	devfd;			/* device file descriptor */
static int	devtype;		/* device file type */
static int	nfp;			/* no first prompt - TRUEorFALSE, -p */
static int	verbose	= 0; /*FALSE*/	/* tracing mode - TRUEorFALSE, -v */
static char	*pimedia=PIMEDIA;	/* default media prompt message */
static int	prompt_flags 		/* flags for user prompts */
			=PROMPT_FLAGS;
static int	devfd;			/* device file descriptor */
static boolean_t bmode = B_FALSE;	/* batch mode */


main (int argc, char **argv)
{
	extern char	*optarg;

	char	*cmt=NULL;
	char	*devname=NULL;

	int	c;
	int	input=0;
	int	output=0;

	/*
	 *  Set appropriate message catalogs for use.
	 */
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdevio");
	(void)setlabel("UX:devio");

	/*
	 *  Get and validate arguments.
	 */
	if (argc <= 1) {
		usage_exit();
	}

	while ((c = getopt(argc, argv, "I:O:h:pvG:M:b")) != -1) {
		switch (c) {
		case 'I':	/* read from device */
			input++;
			devname = optarg;
			break;
		case 'O':	/* write to device */
			output++;
			devname = optarg;
			break;
		case 'h':	/* header comment */
			cmt = optarg;
			break;
		case 'p':	/* no first media prompt */
			nfp++;
			break;
		case 'v':	/* printing mode */
			verbose++;
			break;
		case 'G':	/* I/F for manual input */
			ttypath = optarg;
			break;
		case 'M':	/* media prompt message */
			pimedia	= optarg;
			prompt_flags = MM_NOSTD|MM_NOGET|MM_INFO;
				/* MM_NOGET,
					user message not in catalog */
			break;
		case 'b':	/* batch mode, disallow prompting */
			bmode = B_TRUE;
			break;
		default:
			usage_exit();
			break;
		}
	}

	if (input && output) {
		usage_exit();
	}


	/*
	 *  Read data from device to standard output.
	 */
	if (input) {
		device_reader(devname, cmt);
	}

	/*
	 *  Write data from standard input to device.
	 */
	if (output) {
		device_writer(devname, cmt);
	}

	exit(EC_ARGUMENT); /*NOTREACHED*/
}

/* usage message handler */
void
usage_exit()
{
	(void)pfmt(stderr, MM_STD|MM_GET|MM_ACTION, USAGE);
	exit(1);
}

/*
 *  Main media reader routine.
 */
/* read from device */ 
void
device_reader(char *devname, char *cmt)
{
	dev_rec_t	hdr, wrt, savehdr;

	int	fdout;

	ssize_t	wrtcnt;
	ssize_t	rdcnt;

	ulong	data_rdcnt;
	ulong	data_wrtcnt;

#ifdef DEBUG
	int	rcnt;
	ulong	data_total;
	ulong	floppy_total;
	ulong	fp_rdcnt;
#endif

	/* insert first device media */
	savehdr.hdr.key		= FIRST_KEY;	/* record type */
	savehdr.hdr.dev_cnt	=1;	/* device count */
	if (cmt) {
		(void)sprintf(savehdr.hdr.str_key, "%.50s", cmt);
	} else {
		savehdr.hdr.str_key[0] = '\0';
	}
	if (!nfp) {
		prompt4media(savehdr.hdr.dev_cnt);
	}

	/* open the device */
	(void)open_rddevice(devname, &hdr, &savehdr, NOMEDIACHK);
	savehdr = hdr;	/* save first header record */

	/* get the file descriptor for stdout */
	fdout = fileno(stdout);

	/* write first data segment to standard output */
	(void)write_hdrdata(fdout, &hdr, &wrtcnt);

	/* udpate counters */
	data_rdcnt = hdr.hdr.size; 
	data_wrtcnt = wrtcnt;
#ifdef DEBUG
	if (verbose) {
		floppy_total = DEV_BUFSIZE;
		fp_rdcnt = DEV_BUFSIZE;
		rcnt=1;			/* record counter */
		data_total = wrtcnt;
	}
#endif

	/* write data to device */
	for (;;) {
		/* read data from device */
		if ((rdcnt = read(devfd, (void *)wrt.buf, DEV_BUFSIZE)) == -1) {
			if (errno != EIO) {
				(void)pfmt(stderr,
					SYS_ERR_FLAGS,
					SYS_ERR_FMT,
					devname,
					strerror(errno));
				exit(EC_IOFAIL);
			}
			rdcnt = 0;
		}

		/* end of data */
		if (rdcnt == 0) {
#ifdef DEBUG
			if (verbose) {
				(void)pfmt(stderr, TRACE_FLAGS, DFBR_FMT,
					(int)data_total, (int)fp_rdcnt);
				(void)pfmt(stderr, TRACE_FLAGS, RCNT_FMT, rcnt);
				data_total=0;
			}
#endif
			prompt4media(++savehdr.hdr.dev_cnt);

			/* open new media */
			(void)open_rddevice(devname, &hdr, &savehdr, NOMEDIACHK);

			/* write data segment to standard output */
			(void)write_hdrdata(fdout, &hdr, &wrtcnt);

			/* udpate counters */
			data_rdcnt	+= hdr.hdr.size; 
			data_wrtcnt	+= wrtcnt; 
#ifdef DEBUG
			if (verbose) {
				data_total = wrtcnt; 
				floppy_total += DEV_BUFSIZE;
				fp_rdcnt = DEV_BUFSIZE;
				rcnt=1;
			}
#endif
			continue;
		}

		if (wrt.hdr.key == TLR_KEY) {
			if (verbose) {
#ifdef DEBUG
				floppy_total += DEV_BUFSIZE;
				fp_rdcnt += DEV_BUFSIZE;
#endif
				header_trace();
			}

			/* terminate loop */
			break;
		}

		if (wrt.rec.key != REC_KEY) {
			if (verbose) {
				(void)pfmt(stderr, PGM_ERR_FLAGS, MEDIA_KEY);
			}
			exit(EC_CNT_CHK);
		}

		/* write data to standard output */
		if ((wrtcnt = write(fdout, (void *)wrt.rec.buf, wrt.rec.size)) == -1) {
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				devname,
				strerror(errno));
			exit(EC_IOFAIL);
		}

		/* counts */
		if (wrtcnt != wrt.rec.size) {
			exit(EC_CNT_CHK);
		}
		data_rdcnt += wrt.rec.size; 
		data_wrtcnt += wrtcnt; 

		if (verbose) {
#ifdef DEBUG
			data_total += wrtcnt; 
			floppy_total += rdcnt;
			fp_rdcnt += rdcnt;

			/* count record written */
			rcnt++;
#endif
			record_trace();

		}
	}

	if (verbose) {
#ifdef DEBUG
		(void)pfmt(stderr, TRACE_FLAGS, DFBR_FMT,
			(int)data_total, (int)fp_rdcnt);
#endif
		(void)pfmt(stderr, TRACE_FLAGS, TRD_FMT,
			(int)data_rdcnt, (int)data_wrtcnt);
#ifdef DEBUG
		(void)pfmt(stderr, TRACE_FLAGS, MTRD_FMT, (int)floppy_total);
#endif
	}

	if (data_rdcnt != data_wrtcnt) {
		exit(EC_CNT_CHK);
	}

	exit(0);
}

/*
 *  Main media writer routine.
 */
void
device_writer(char *devname, char *cmt)
{
	char	*bptr;

	dev_rec_t	hdr, rd;

	int	fdin;
	int	move;

	ssize_t	wrtcnt;
	ssize_t	rdcnt;

	time_t	tm;

	ulong	data_rdcnt;
	ulong	data_wrtcnt;

#ifdef DEBUG
	int	rcnt;
	ulong	data_total;
	ulong	fp_wrtcnt;
	ulong	floppy_total;
#endif

	/* initialize header */
	hdr.hdr.key = HDR_KEY;	/* record type */
	hdr.hdr.dev_cnt=1;	/* device count */
	hdr.hdr.random_key=0;	/* key */
	if (cmt) {
		(void)sprintf(hdr.hdr.str_key, "%.50s", cmt);
	} else {
		hdr.hdr.str_key[0] = '\0';
	}
	tm = (time_t)time((time_t *)NULL);
	(void)sprintf(hdr.hdr.date, "%s", (char *)ctime(&tm));
	srand(tm);
	hdr.hdr.random_key = (ulong)rand();

	if (!nfp) {
		prompt4media(hdr.hdr.dev_cnt);
	}

	/* open the device */
	(void)open_wrtdevice(devname, &hdr, NOMEDIACHK);

	/* get the file descriptor for stdin */
	fdin = fileno(stdin);

	/* read first segment to go into header record */
	if (	(rdcnt = read(fdin, (void *)hdr.hdr.buf, HDR_BUFSIZE)) == -1
	|| 	rdcnt == 0
	){
		(void)pfmt(stderr,
			SYS_ERR_FLAGS,
			SYS_ERR_FMT,
			devname,
			strerror(errno));
		exit(EC_IOFAIL);
	}
	hdr.hdr.size = (short)rdcnt;

	/* write header to media */
	(void)write_header(&hdr, &wrtcnt, devname);

	/* update counters */
	data_rdcnt	= hdr.hdr.size;
	data_wrtcnt	= hdr.hdr.size;
#ifdef DEBUG
	if (verbose) {
		data_total = hdr.hdr.size; 
		fp_wrtcnt = wrtcnt; 
		floppy_total = wrtcnt; 
		rcnt=1;			/* record count */
	}
#endif

	/* write data to device */
	for (move=0;;) {
		if (move > 0) {
			bptr = &rd.rec.buf[move];
			rdcnt = read(fdin, (void *)bptr, (DATA_BUFSIZE-move));
			if (rdcnt != -1) {
				/* add 'move data' to read count */
				rdcnt += move;

				/* 'move data' already counted */
				data_rdcnt -= move;
				data_wrtcnt -= move;
				move = 0;
			}
		} else {
			rdcnt = read(fdin, (void *)rd.rec.buf, DATA_BUFSIZE);
		}

		/* read data from standard input */
		if (rdcnt == -1) {
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				devname,
				strerror(errno));
			exit(EC_IOFAIL);
		}

		/* end of data */
		if (rdcnt == 0) {
			/* terminate loop */
			break;
		}

		/* count data read from input */
		data_rdcnt += rdcnt;

		/* set record key and size */
		rd.rec.key = REC_KEY;
		rd.rec.size = (short)rdcnt;

		/* write data to the device */
		if ((wrtcnt = write(devfd, (void *)rd.buf, DEV_BUFSIZE)) == -1){
			switch (errno) {
				case ENXIO :
					break; /*go change media */
				case EFBIG :
				case ENOSPC:
					if (devtype & S_IFREG){
						(void)pfmt(stderr,
							SYS_ERR_FLAGS,
							SYS_ERR_FMT,
							devname,
							strerror(errno));
						/*fall through*/
					} else
						break; /*go change media */
				default:
					(void)pfmt(stderr,
						SYS_ERR_FLAGS,
						SYS_ERR_FMT,
						devname,
						strerror(errno));
					exit(EC_IOFAIL);
			}
			(void)close(devfd);
#ifdef DEBUG
			/* insert next device media */
			if (verbose) {
				(void)pfmt(stderr, TRACE_FLAGS, DFBW_FMT,
					(int)data_total, (int)fp_wrtcnt);
				(void)pfmt(stderr, TRACE_FLAGS, RCNT_FMT, rcnt);
				data_total=0;
			}
#endif
			prompt4media(++hdr.hdr.dev_cnt);

			/* open new media */
			(void)open_wrtdevice(devname, &hdr, MEDIACHK);

			/* move data to hdr buffer */
			if (rdcnt >= HDR_BUFSIZE) {
				hdr.hdr.size = HDR_BUFSIZE;
				move = rdcnt - hdr.hdr.size;
			} else {
				hdr.hdr.size = (short)rdcnt;
				move=0;
			}
			(void)memmove((void *)hdr.hdr.buf, (void *)rd.rec.buf, hdr.hdr.size);

			/* write header to media */
			(void)write_header(&hdr, &wrtcnt, devname);

			/* count data written to device */
			data_wrtcnt += rd.rec.size;

			/* rest of data forward */
			if (move > 0) {
				bptr = &rd.rec.buf[hdr.hdr.size];
				(void)memmove((void *)rd.rec.buf, (void *)bptr, move);
			}

			/* reset and update counters */
#ifdef DEBUG
			if (verbose) {
				data_total += rd.rec.size; 
				fp_wrtcnt = DEV_BUFSIZE; 
				floppy_total += DEV_BUFSIZE; 
				rcnt=1;
			}
#endif

			/* get next record */
			continue;
		}

		/* counts */
		if (wrtcnt != DEV_BUFSIZE) {
			exit(EC_CNT_CHK);
		}

		/* count data written to device */
		data_wrtcnt += rd.rec.size;

		if (verbose) {
#ifdef DEBUG
			/* count record written */
			rcnt++;

			data_total += rd.rec.size; 
			fp_wrtcnt += wrtcnt; 
			floppy_total += wrtcnt; 
#endif
			record_trace();
		}
	}

	hdr.hdr.key = TLR_KEY;
	(void)write_header(&hdr, &wrtcnt, devname);
	(void)close(devfd);

	if (verbose) {
#ifdef DEBUG
		fp_wrtcnt += wrtcnt; 
		floppy_total += wrtcnt; 
		(void)pfmt(stderr, TRACE_FLAGS, DFBW_FMT,
			(int)data_total, (int)fp_wrtcnt);
#endif
		(void)pfmt(stderr, TRACE_FLAGS, TWRT_FMT,
			(int)data_rdcnt, (int)data_wrtcnt);
#ifdef DEBUG
		(void)pfmt(stderr, TRACE_FLAGS, MTWRT_FMT, (int)floppy_total);
#endif
	}

	if (data_rdcnt != data_wrtcnt) {
		exit(EC_CNT_CHK);
	}

	exit(0);
}

/*
 *  Open media for write-only operations, verify it is
 *  not the previous media written out.
 */
int
open_wrtdevice(char *devname, dev_rec_t *rec, int mediachk)
{
	dev_rec_t	prehdr;
	int		err;
	struct stat	statbuf;

	/* Verify that the media has been changed. */
	if (mediachk & MEDIACHK) {
		rec->hdr.dev_cnt--;
		(void)open_rddevice(devname, &prehdr, rec, MEDIACHK);
		rec->hdr.dev_cnt++;
	}

	/* open device */
	for(	err  = 0;
		err <= MEDIA_INSERT_TRIES
	&&	((devfd = open(devname, O_WRONLY|O_TRUNC|O_CREAT, 0666)) == -1);
		err++
	){
		switch(errno) {
		case EROFS:
			(void)pfmt(stderr, PGM_ERR_FLAGS, RO_MEDIA);
			break;
		case EIO:
			(void)pfmt(stderr, PGM_ERR_FLAGS, NO_MEDIA);
			break;
		default:
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				devname,
				strerror(errno));
			exit(EC_IOFAIL);
		}
		prompt4media(rec->hdr.dev_cnt);
	}
	if(devfd == -1)
		exit(EC_IOFAIL);
	if( fstat(devfd, &statbuf) == -1){
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				devname,
				strerror(errno));
			exit(EC_IOFAIL);
	} else {
		devtype = statbuf.st_mode&S_IFMT;
	}
	return 0;
}

/*
 *  Write header record to media.
 */
int
write_header(dev_rec_t *rec, ssize_t *wrtcnt, char *devname)
{
	/* write header record */
	if ((*wrtcnt = write(devfd, (void *)rec->buf, DEV_BUFSIZE)) == -1) {
		(void)pfmt(stderr,
			SYS_ERR_FLAGS,
			SYS_ERR_FMT,
			devname,
			strerror(errno));
		exit(EC_IOFAIL);
	}

	/* counts */
	if (*wrtcnt != DEV_BUFSIZE) {
		exit(EC_CNT_CHK);
	}

	if (verbose) {
		header_trace();
	}
	return 0;
}

/*
 *  Open media for read-only operations, and verify it
 *  is the media expected next.
 */
int
open_rddevice(char *devname, dev_rec_t *hdr, dev_rec_t *savehdr, int mediachk)
{
	int	err=0;
	int	loop=1;
	ssize_t	rdcnt;

	/* open device */
	while (loop) {
		if ((devfd = open(devname, O_RDONLY)) == -1) {
			if (err++ > MEDIA_INSERT_TRIES) {
				exit(EC_IOFAIL);
			}
			switch(errno) {
			case EIO:
				(void)pfmt(stderr, PGM_ERR_FLAGS, NO_MEDIA);
				break;

			default:
				(void)pfmt(stderr,
					SYS_ERR_FLAGS,
					SYS_ERR_FMT,
					devname,
					strerror(errno));
				exit(EC_IOFAIL);
			}
			prompt4media(savehdr->hdr.dev_cnt);
		} else {
			/* read header record */
			if ((rdcnt = read(devfd, (void *)hdr->buf, DEV_BUFSIZE)) == -1) {
				(void)pfmt(stderr,
					SYS_ERR_FLAGS,
					SYS_ERR_FMT,
					devname,
					strerror(errno));
				exit(EC_IOFAIL);
			}

			/* counts */
			if (rdcnt != DEV_BUFSIZE) {
				exit(EC_CNT_CHK);
			}

			/* compare header information */ 
			if (mediachk & MEDIACHK) {
				if (compare_header_for_chg(hdr, savehdr) == -1) {
					(void)close(devfd);
	
					/* try again */ 
					prompt4media(savehdr->hdr.dev_cnt+1);
					continue;
				}

				/*
				 *  close device, nolonger needed for
				 *  read-only operation.
				 */
				(void)close(devfd);
			} else {
				if (compare_header(hdr, savehdr) == -1) {
					(void)close(devfd);
	
					/* try again */
					prompt4media(savehdr->hdr.dev_cnt);
					continue;
				}
			}
			loop=0;
		}
	}

	if (verbose && mediachk == NOMEDIACHK) {
		header_trace();
	}
	return 0;
}

/*
 *  Verify that the next media has been inserted into the device.
 */ 
int
compare_header(dev_rec_t *hdr, dev_rec_t *savehdr)
{
	/* compare header information */
	if(
	(	hdr->hdr.key != FIRST_KEY
		&&	!
		(	hdr->hdr.key == HDR_KEY
		||	hdr->hdr.random_key == savehdr->hdr.random_key
		||	strcmp(hdr->hdr.date, savehdr->hdr.date) == 0
		)
	)
	||	strcmp(savehdr->hdr.str_key, hdr->hdr.str_key) != 0
	||	savehdr->hdr.dev_cnt != hdr->hdr.dev_cnt
	){
		(void)pfmt(stderr, PGM_ERR_FLAGS, WRG_MEDIA);
		if (hdr->hdr.key == HDR_KEY) {
			(void)pfmt(stderr, TRACE_FLAGS,
				EHDR_FMT,
				(int)savehdr->hdr.dev_cnt,
				savehdr->hdr.str_key,
				savehdr->hdr.date);
			(void)pfmt(stderr, TRACE_FLAGS,
				FHDR_FMT,
				(int)hdr->hdr.dev_cnt,
				hdr->hdr.str_key,
				hdr->hdr.date);
		}
		return -1;
	}
	return 0;
}

/*
 *  Verify that the previous media has been removed from the device.
 */ 
int
compare_header_for_chg(dev_rec_t *hdr, dev_rec_t *savehdr)
{
	/* compare header information */
	if(	hdr->hdr.key == HDR_KEY
	&&	hdr->hdr.random_key == savehdr->hdr.random_key
	&&	strcmp(hdr->hdr.date, savehdr->hdr.date) == 0
	&&	strcmp(savehdr->hdr.str_key, hdr->hdr.str_key) == 0
	&&	savehdr->hdr.dev_cnt == hdr->hdr.dev_cnt
	){
		(void)pfmt(stderr, PGM_ERR_FLAGS, WRG_MEDIA);
		if (hdr->hdr.key == HDR_KEY) {
			(void)pfmt(stderr, TRACE_FLAGS,
				EHDR_FMT,
				(int)savehdr->hdr.dev_cnt+1,
				     savehdr->hdr.str_key,
				     savehdr->hdr.date);
			(void)pfmt(stderr, TRACE_FLAGS,
				FHDR_FMT,
				(int)hdr->hdr.dev_cnt,
				     hdr->hdr.str_key,
				     hdr->hdr.date);
		}
		return -1;
	}
	return 0;
}

/*
 *  Write header data extracted from the header
 *  record to standard output.
 */
int
write_hdrdata(int fdout, dev_rec_t *hdr, int *wrtcnt)
{
	/* write data to standard output */
	if ((*wrtcnt = write(fdout, (void *)hdr->hdr.buf, hdr->hdr.size)) == -1) {
		exit(EC_IOFAIL);
	}

	/* counts */
	if (*wrtcnt != hdr->hdr.size) {
		exit(EC_CNT_CHK);
	}
	return 0;
}
static void
ttyready()
{
	static boolean_t been_here_b4 = B_FALSE;
	
	if(!been_here_b4){
		been_here_b4 = B_TRUE;
		/*
		 *  Open tty device for input and output.
		 */
		if ((ttyout = (FILE *)fopen(ttypath, "w")) == (FILE *)NULL) {
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				ttypath,
				strerror(errno));
			exit(EC_IOFAIL);
		}
		if ((ttyin  = (FILE *)fopen(ttypath, "r")) == (FILE *)NULL) {
			(void)pfmt(stderr,
				SYS_ERR_FLAGS,
				SYS_ERR_FMT,
				ttypath,
				strerror(errno));
			exit(EC_IOFAIL);
		}
	}
}
void
prompt4media(ulong n)
{
	if(bmode == B_TRUE){
		(void)pfmt(stderr, PGM_ERR_FLAGS, ALONE_FMT);
		exit(EC_ARGUMENT);
	}
	ttyready();
	if (verbose) {
		(void)fputc('\n', ttyout);
	}
	(void)pfmt(ttyout, prompt_flags, pimedia, (int)n);
	(void)fflush(ttyout);
	if( fgetc(ttyin) == EOF)
		exit(EC_ARGUMENT);
	if (verbose) {
		(void)fputc('\n', ttyout);
		(void)fflush(ttyout);
	}
}
void
header_trace()
{
	(void)fputc('+', stderr);
	(void)fflush(stderr);
}
void
record_trace() /* use for output XOR input, just one counter */
{
	static	int j = 0;

	(void)fputc((int)'.', stderr);
	j++;
	if (j == 50) {
		j=0;
		(void)fputc('\n', stderr);
	}
	(void)fflush(stderr);
}
