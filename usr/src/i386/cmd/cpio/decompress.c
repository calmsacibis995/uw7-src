#ident	"@(#)cpio:i386/cmd/cpio/decompress.c	1.4"

#include "decompress.h"
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <limits.h>
#include "cpio.h"

#define	min(a, b)	((a>b) ? b : a)
#define ALRM_CLK 15

extern char *IOfil_p;
extern int errno;

caddr_t global_mem;

static long header_size;		/* number of blocks in file */
static HEADER_INFO *header_ptr;
static unsigned char magic_str[] = { 0x19, 0x9e, 'T', 'G' };
static unsigned char *loutbuf;
static unsigned char *global_buf;
static int sh_sb_fd, sh_buf_fd;
static SB *sb;

#ifdef __STDC__
extern void msg(...);
#else
extern void msg();
#endif

/* Locally-defined functions. */
int d_read(char *, int);
pid_t spawn_reader(void);
static int decompress1(char *, int);
static int mread(void *, int);
static int buffer_fill(int);
static int lread(int, char *, int);
static void handler(int);

int
d_read(char *buf, int cnt)
{
	static int first_time = 0;
	unsigned long blk_size;
	int x;

	if (first_time == 0) {
		char mag[4];
		if (mread(mag, 4) == -1)
			return -1;
		if (memcmp(mag, magic_str, 4) != 0) 
			msg(EXT, ":1218", "File is not in compressed format");
		if (mread(&header_size, 4) == -1)
			return -1;
		if (mread(&blk_size, 4) == -1)
			return -1;
		header_ptr = (HEADER_INFO *) malloc(header_size *
		    sizeof(HEADER_INFO));
		if (header_ptr == NULL) 
			msg(EXT, ":1219",
			    "Cannot allocate memory for decompression");
		if (mread(header_ptr, header_size*sizeof(HEADER_INFO)) == -1)
			return -1;
		loutbuf=malloc(blk_size+16);
		global_buf = malloc(blk_size+512);
		first_time = 1;
	}
	x = decompress1(buf, cnt);
#ifdef DEBUG
	fprintf(stderr, "Reading %d bytes, read %d bytes \n", cnt, x);
	(void) fflush(stderr);
#endif
	return x;
}

/*
 * Decompress block by block from the input media.
 */
static int
decompress1(char *buf, int cnt)
{
	static int i = -1;
	static int cp = 0;
	static int size;
	static unsigned char *membuf;
	int al, ac;
	
	al = ac = cnt;
#ifdef DEBUG
	fprintf(stderr, "Want %d bytes\n", cnt);
#endif
	while(al > 0) {
		if (cp == 0) {
			i++;
			if (i >= header_size)
				return 0;
			size = header_ptr[i].cmp_size;
			if (size <= 0)
				break;
			if (mread(global_buf, size) == -1)
				return -1;
			membuf=loutbuf;
			mem_decompress((char *)global_buf, (char *)membuf,
			    size, header_ptr[i].ucmp_size);
			size = header_ptr[i].ucmp_size;
			membuf=loutbuf;
		}
		if (al+cp >= size) {
			ac = size-cp;
			cp = 0;
#ifdef DEBUG
			fprintf(stderr, "Exhausted buffer\n");
#endif
		}
		else cp+=ac;
#ifdef DEBUG
		fprintf(stderr, "Copying %d bytes to %x, cp = %d\n",
		    ac, buf, cp);
#endif
		(void) memcpy(buf, membuf, ac);
		buf += ac;
		membuf += ac;
		al -= ac;
		ac = al;
	}
	return cnt;
}

/*
 * Read data from the input buffers.  Always skip the first 512 bytes from
 * the first block of data.  On the last block of data, return only the
 * actual (partial block) amount of data present
 */
static int
mread(void *dest, int amount)
{
	int i = 0;
	static int sq = 0;
	static ulong lb = 0xffffffff;
	static ulong last_block;
	static int cp = 0;
	int amount_left, copy_amount, offs;
	long offs1;

	amount_left = copy_amount = amount;
	if (amount <= 0)
		return amount;
	offs = 0;
	for(;;) {
		for(i = 0; i < MBUFFERS; i++) {
			if (sb->sequence[i] == sq) {
				if (sq == 0 && lb == 0xffffffff) {
					/*
					 * global_mem is guaranteed to be
					 * word-aligned, so the cast below
					 * is safe.
					 */
					last_block = *(ulong *)(global_mem+4);
					lb = last_block / ZREAD_SIZE;
					last_block %= ZREAD_SIZE;
					if (last_block) 
						lb++;
					cp=512;
				}
				offs1 = i * ZREAD_SIZE;
				if (cp+copy_amount >= sb->count[i]) {
					copy_amount=sb->count[i]-cp;
				}
				(void) memcpy((char *)dest+offs,
				    global_mem+offs1+cp, copy_amount);
				amount_left -= copy_amount;
				cp+=copy_amount;
				if (cp >= sb->count[i]) {
					cp = 0;
					sb->sequence[i] = -1;
					sq++;
					offs+=copy_amount;
					copy_amount = amount_left;
				}
				if (amount_left == 0)
					return amount;
			}
			else {
				if (sb->sequence[i] == -2) {
					errno = EIO;
					return -1;	/* error on input */
				}
			}
		}
	}
}

/*
 * Fill in the buffers with data from the input media
 */
static int
buffer_fill(int fd)
{
	int i, sq;
	int cnt, found_free;
	int x;
	int amount, cp;
	int lb, lba, lb1;
	int done = 0;

	sq = 0;
	lb = 1; /* Just to make sure we go through this loop at least once. */
	for(x = 0; x < lb; x++) {
		found_free = 0;
		while(found_free < 10) {
			for(i = 0, found_free = 0; i < MBUFFERS; i++) {
				if (sb->sequence[i] == -1)
					found_free++;
			}
		}
		for(i = 0, found_free = 0; i < MBUFFERS && done == 0; i++) {
			if (sb->sequence[i] == -1) {
				found_free++;
				if (x == 0) {
					(void) lread(fd,
					    (global_mem+(ZREAD_SIZE*i)), 512);
					(void) memcpy(&lb1,
					    (global_mem+(ZREAD_SIZE*i)+4),
					    sizeof(long));
					lb1+=512;
					lb = lb1 / ZREAD_SIZE;
					if (lb == 0)
						lb1 -= 512;
					lba = lb1 % ZREAD_SIZE;
					lb += (lba == 0) ? 0 : 1;
					lba = (lba / 512) +
					    ((lba % 512) ? 1 : 0);
					lba *= 512;
					if (lb == 1)
						amount = lba;
					else
						amount =  ZREAD_SIZE-512;
					cp = 512;
				}
				else {
					if (x != lb-1) 
						amount = ZREAD_SIZE;
					else {
						amount = lba;
						done = 1;
					}
					cp = 0;
				}
				cnt = lread(fd, (global_mem+(ZREAD_SIZE*i)+cp),
				    amount);
				if (cnt < amount) {
					sb->sequence[i] = -2;
					sb->count[i] = 0;
					return -1;
				}
				sb->count[i] = cnt+cp;
				sb->sequence[i] = sq++;
				break;
			}
		}
	}
	return 0;
}


static int
lread(int fd, char *addr, int cnt)
{
	static int read_size = 0;
	int tcnt, rdsz, rdcnt = 0;

	if (read_size == 0) {
		struct stat sbuf;
		if (fstat(fd, &sbuf) < 0)
			return -1;
		if (S_ISFIFO(sbuf.st_mode)) {
#ifdef	DEBUG
			fprintf(stderr, "cpio: reading from a pipe\n");
#endif
			read_size = PIPE_BUF;
		}
		else
			read_size = ZREAD_SIZE;
	}
	if (cnt < read_size)
		rdsz = cnt;
	else
		rdsz = read_size;

#ifdef DEBUG
	fprintf(stderr, "cpio: cnt = %d, addr = %x, fd = %d\n", cnt, addr, fd);
#endif
	do {
		tcnt = read(fd, addr, rdsz);
#ifdef DEBUG
		fprintf(stderr, "cpio: tcnt = %d\n", tcnt);
#endif
		if (tcnt < 0)
			return -1;		/* error on read */
		if (tcnt == 0)
			break;
		cnt -= tcnt;
		rdcnt += tcnt;
		addr += tcnt;
		if (cnt < read_size)
			rdsz = cnt;
		else
			rdsz = read_size;
	} while(cnt > 0);
	return rdcnt;
}

pid_t
spawn_reader(void)
{
	pid_t pid;
	int i, tape_fd;
	struct sigaction act;

	sh_sb_fd = open("/dev/zero",O_RDWR);
	if (sh_sb_fd < 0)
		msg(EXTN, ":65", "Cannot open \"%s\"", "/dev/zero");
	sb = (SB *)mmap(NULL, sizeof(SB), PROT_READ|PROT_WRITE, MAP_SHARED, sh_sb_fd, 0);
	if (sb == (SB*)((caddr_t)-1)) {
		msg(EXTN,":1215", "Cannot use -Z option, failed to attach to /dev/zero");
	}
	for(i = 0; i < MBUFFERS; i++) {	/* init the pointers to the buffers */
		sb->count[i]   = -1;
		sb->sequence[i] = -1;
	}
	sh_buf_fd = open("/dev/zero", O_RDWR, 0666);
	if (sh_buf_fd < 0)
		msg(EXTN, ":65", "Cannot open \"%s\"", "/dev/zero");
	global_mem = mmap(NULL, ZBUFFER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, sh_buf_fd, 0);
	if (global_mem == (caddr_t)-1)
		msg(EXTN,":1215", "Cannot use -Z option, failed to attach to /dev/zero");
	if ((pid=fork()) == 0) {
		act.sa_handler = handler;
		act.sa_flags = SA_RESTART;
		(void) sigemptyset(&act.sa_mask);
		(void) sigaction(SIGALRM, &act, (struct sigaction *)0);
		(void) alarm(ALRM_CLK);
		if (IOfil_p)
			tape_fd = open(IOfil_p, O_RDONLY);
		else
			tape_fd = 0;
		if (buffer_fill(tape_fd) < 0) {
			sb->sequence[0] = -2;
			errno = ENOENT;
		}
		exit(0);
	}
	return pid;
}

static void
handler(int sig)
{
	switch (sig) {
	case SIGALRM:
		/*
		 * Every time the alarm rings, check my parent process ID.
		 * If it is 1, then I'm an orphan, so exit.
		 */
		if (getppid() == 1)
			exit(1);
		(void) alarm(ALRM_CLK);
		break;
	default:
		/*
		 * This case should never be reached.  If it is, it probably
		 * means that someone set this function to be the signal
		 * handler for some signal, but forgot to add a case to this
		 * switch.
		 */
		msg(EXT, ":1220", "Unexpected signal: %d", sig);
		/*NOTREACHED*/
	}
}
