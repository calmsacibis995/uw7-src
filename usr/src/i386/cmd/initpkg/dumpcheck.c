#ident	"@(#)initpkg:i386/cmd/initpkg/dumpcheck.c	1.10.2.2"
#ident	"$Header$"

/*
 *	dumpcheck
 *
 *	Checks to see if there is a panic dump on the swap device
 *	and if there is, asks whether the dump should be saved.
 *
 *	If TIME is defined as 'n' in /etc/default/dump, dumpcheck will
 *	wait 'n' seconds before timing out and assuming a 'no' answer.
 *	If TIME is zero, the question will not be asked and no save
 *	will be done.  If TIME is negative or /etc/default/dump does
 *	not exist, dumpcheck will not time out.
 */

/*
 * HACK: define _KMEMUSER here because the makefile doesn't
 * conveniently allow a -D_KMEMUSR to be added
 */

#define _KMEMUSER
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <termio.h>
#include <deflt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/param.h>
#include <sys/kcore.h>
#include <errno.h>
#include <pfmt.h>
#include <locale.h>
#include <unistd.h>


#define	DEFFILE	"/etc/default/dump"
#define BUFSIZE 1024
#define MAX_WAITCNT	10

extern size_t find_size();

static size_t swapsz;
static off_t startoff;
static char pathname[512];
static int swapfd;

main()
{
	int  waitcnt = 0;
	void wakeup();
	int flags, timeout = -1, err;
	FILE *deffd, *fp;
	char *ptr, ans[BUFSIZ], header_buf[DEV_BSIZE];
	char buf[BUFSIZ], slen[BUFSIZ], buf2[BUFSIZ], buf3[BUFSIZ];
	struct termio term_setting;
	kcore_t *header = (kcore_t *)header_buf;
	boolean_t dump_found = B_FALSE;
	size_t dump_size;
	char *yes, *no, *YES, *NO;

	(void)setcat("uxrc");
	(void)setlabel("UX:dumpcheck");
	(void)setlocale(LC_ALL,"");

	if ((fp = fopen("/etc/swaptab", "r")) == NULL) {
		pfmt(stderr, MM_ERROR, 
			":165:cannot open /etc/swaptab: errno = %d\n", errno);
		exit(1);
	}

	while (fgets(buf, BUFSIZE, fp)) {
		if (*buf == '#')
			continue;
		err = sscanf(buf, "%s %u %s", pathname, &startoff,slen);
		if (err < 2)
			break;
		swapfd = open(pathname, O_RDWR);
		if (swapfd == -1) {
			pfmt(stderr, MM_ERROR, 
				":166:open of swap device failed: errno = %d\n",
				errno);
			exit(1);
		}
		if (startoff != 0) { 
			err = lseek(swapfd, startoff * DEV_BSIZE, SEEK_SET);
			if (err == -1) {
				pfmt(stderr, MM_ERROR,
					":167:lseek of swap dev failed: errno = %d\n",
					errno);
				exit(1);
			}
		}

		if (read(swapfd, header, DEV_BSIZE) != DEV_BSIZE) {
			pfmt(stderr, MM_ERROR,
				":168:cannot read swapfd: errno = %d\n", errno);
			exit(1);
		}
		if (header->k_magic == KCORMAG) {
			dump_found = B_TRUE;
			break;
		}
		close(swapfd);
	}
	fclose(fp);

	/* there is no dump in any swap device - exit */
	if (!dump_found)
		exit(0);

	if (slen[0] == '-')
		swapsz = find_size(swapfd);
	else
		swapsz = strtoul(slen, NULL, 10);
	if (swapsz == 0) {
		pfmt(stderr, MM_ERROR,
			":169:can't figure out swap size: errno = %d\n",
			errno); 
		exit(1);
	}

	err = remove_swap(pathname, startoff);
	if (err != 0)
		exit(1);

	pfmt(stdout, MM_INFO,
		":170:\nChecking to see if you have a valid dump ...\n\n");

	sprintf(buf, "/sbin/memsize %s", pathname);	
	fp = popen(buf, "r");
	if (fp == 0) {
		pfmt(stderr, MM_ERROR, ":171:popen failed: errno = %d\n",
			errno);
		goto bye;
	}

	if (fgets(buf, BUFSIZE, fp) == NULL) {
		pfmt(stderr, MM_ERROR,
			":172:memsize invalid return value: errno = %d\n",
			errno);
		goto bye;
	}
	pclose(fp);

	if (sscanf(buf, "%u", &dump_size) != 1) {
		goto bye;
	}

	header->k_size = dump_size;
	err = lseek(swapfd, startoff * DEV_BSIZE, SEEK_SET);
	if (err == -1) {
		perror("lseek back failed for swap dev\n");
		goto bye;
	} else {
		if (write(swapfd, header, DEV_BSIZE) != DEV_BSIZE) {
			perror("dumpcheck: cannot write swapfd");
			goto bye;
		}
	}

	/* attempt to open default file to get TIME value */
	if ((deffd = defopen(DEFFILE)) != 0) {
		/* ignore case */
		flags = defcntl(DC_GETFLAGS, 0);
		TURNOFF(flags, DC_CASE);
		defcntl(DC_SETFLAGS, flags);

		if ((ptr = defread(deffd, "TIME")) != NULL)
			timeout = atoi(ptr);
		defclose(deffd);
	}

	if (timeout == 0) {		/* TIME is 0 - exit immediately*/
		goto bye;
	} else if (timeout > 0) {	/* set timeout for "save" question */
		signal(SIGALRM, wakeup);
		alarm(timeout);
	}

	/* ensure sane console port settings */
	ioctl(fileno(stdin), TCGETA, &term_setting);
	term_setting.c_iflag |= ICRNL;
	term_setting.c_oflag |= (OPOST | ONLCR);
	term_setting.c_lflag |= (ICANON | ECHO);
	ioctl(fileno(stdin), TCSETA, &term_setting);

	yes = gettxt(":183", "y");
	no = gettxt(":184", "n");
	YES = gettxt(":185", "Y");
	NO = gettxt(":186", "N");
	while (1) {

		pfmt(stdout, MM_INFO,
			":173:There is a system dump memory image in a swap device.\n");
		pfmt(stdout, MM_NOSTD, 
			":174:Do you want to save it? (%s/%s)> ", yes, no);
		fflush(stdout);
		scanf("%s", ans);
		waitcnt++;
			
		if (strcmp(ans, yes) == 0 || strcmp(ans, YES) == 0) {
			/* save the dump */
			alarm(0);	/* cancel alarm */
			sprintf(buf, "%d", startoff);
			sprintf(buf2, "%u", dump_size);
			sprintf(buf3, "%u", swapsz);
			execl("/sbin/sh", "sh", "/sbin/dumpsave", 
				pathname, buf, buf2, buf3, NULL);
			pfmt(stderr, MM_ERROR,
				":175:cannot exec /sbin/sh: errno = %d\n",
				errno);
			goto bye;
		}
		else if(waitcnt > MAX_WAITCNT || 
		        strcmp(ans, no) == 0 || 
		        strcmp(ans, NO) == 0) {
			goto bye;
		} else
			pfmt(stdout, MM_NOSTD, ":176:\n???\n");
	}
bye:
	header->k_magic = (unsigned)~KCORMAG;
	err = lseek(swapfd, startoff * DEV_BSIZE, SEEK_SET);
	if (err == -1) {
		pfmt(stderr, MM_ERROR, 
			":177:lseek back failed for swap dev: errno = %d\n",
			errno);
	} else {
		if (write(swapfd, header, DEV_BSIZE) != DEV_BSIZE) {
			pfmt(stderr, MM_ERROR,
				":178:cannot write swapfd: errno = %d\n",
				errno);
		}
	}
	if (reattach_swap(pathname, startoff, swapsz) == -1)
		exit(1);
}

/*
 *	This routine executes when the "save the dump" question has
 *	timed out.  Print "timeout." to the console and exit.
 */
void
wakeup()
{
	int err;
	kcore_t header;

	pfmt(stdout, MM_INFO, ":179:timeout.\n\n");
	pfmt(stdout, MM_INFO, ":180:No system dump image will be saved.\n");
	header.k_magic = (unsigned)~KCORMAG;
	err = lseek(swapfd, startoff * DEV_BSIZE, SEEK_SET);
	if (err == -1) {
		pfmt(stderr, MM_ERROR, 
			":177:lseek back failed for swap dev: errno = %d\n",
			errno);
	} else {
		if (write(swapfd, &header, sizeof(header)) != sizeof(header)) {
			pfmt(stderr, MM_ERROR,
				":178:cannot write swapfd: errno = %d\n",
				errno);
		}
	}
	reattach_swap(pathname, startoff, swapsz);
	exit(2);
}

int
remove_swap(char *pathname, ulong_t startoff)
{
	swapres_t	swpi;
	extern int	errno;

	swpi.sr_name = pathname;
	swpi.sr_start = startoff;

	if (swapctl(SC_REMOVE, &swpi) < 0) {
		if (errno == ENOMEM) 
			return -1;
	}
	return 0;
}
	
int
reattach_swap(char *pathname, ulong_t startoff, ulong_t swapsz)
{
	swapres_t swpi;

	swpi.sr_name = pathname;
	swpi.sr_start = startoff;
	swpi.sr_length = swapsz;

	if (swapctl(SC_ADD, &swpi) < 0){
		pfmt(stderr, MM_ERROR, ":181:SC_ADD failed: errno = %d\n",
			errno);
		return(-1);
	}
	return 0;
}

size_t
find_size(int fd)
{
	struct stat     statb;

	if (fstat(fd, &statb) < 0) {
		pfmt(stderr, MM_ERROR, 
			":182:cannot stat swap device: errno = %d\n", errno);
		return 0;
	}
	return statb.st_size / 512;
}

