#ident	"@(#)prtconf:i386at/cmd/prtconf/prtconf.c	1.7.7.3"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/cram.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/vtoc.h>
#include <sys/sdi_edt.h>
#include <devmgmt.h>
#include <limits.h>
#include <unistd.h>

#define KILOBYTE 0x400

extern int errno;
uint_t getmemsz();
void printpdi();

main()
{
	unsigned char x;
	struct scsi_xedt	*xedtptr;
	uint_t memsize;
	int fd, drivea, driveb;
	int disk_cnt = 0;
	char	*sp;
	long size;
	unsigned char buf[2];
	int found_disk, nedt, edtcnt;

	uint_t diskparms();
	if ((fd = open("/dev/cram",O_RDONLY)) == -1 ) {
		printf("can't open cram, errno %d\n",errno);
		exit(-1);
	}
	printf ("\n    SYSTEM CONFIGURATION:\n\n");

	memsize = getmemsz();
	printf("    Memory Size: %d Megabytes\n", memsize);
	printf ("    System Peripherals:\n\n");

	/* check for floppy drive */
	buf[0] = DDTB;
	crio(fd,buf);
	drivea = ((buf[1] >> 4) & 0x0f);
	(void) printflp(1, drivea);
	driveb = (buf[1] & 0x0f);
	(void) printflp(2, driveb);

	printpdi();

	/* check 80387 chip */
	buf[0] = EB;
	crio(fd,buf);
	if (buf[1] & 0x02)
		printf("	80387 Math Processor\n");

	/* last but not least add a blank line for asthetic appeal */

	printf("\n\n");
	exit(0);
}

/* get size of the disk */
uint_t 
diskparms(cid)
char *cid;
{
int hd;
struct disk_parms dp;
uint_t size;

	if (((hd = open(cid, 2)) != -1) && 
	   (ioctl(hd, V_GETPARMS, &dp) != -1)) 
		/*
		 * The following line of code is being replaced
		 * with a less explicit line to avoid overrunning uint_t:
		 * size = ((uint_t)(dp.dp_cyls*dp.dp_sectors*512*dp.dp_heads))
				/(uint_t)(1024*1024);
		 * since the compiler may not optimize out the 512 factor.
		 */
		 size = ((uint_t)(dp.dp_cyls*dp.dp_sectors*dp.dp_heads))
				/(uint_t)(2*1024);
	else
		size = (uint_t)-1;
	close(hd);
	return (size);
}

printflp(drivenum, drivetype)
int drivenum;
int drivetype;
{
	char	*desc, buf[BUFSIZ];

	sprintf(buf,"diskette%d",drivenum);
	desc = devattr(buf, "desc");

	switch (drivetype) {
		case 1:
			printf("        %s - 360 KB 5.25\n", desc);
			break;
		case 2:
			printf("        %s - 1.2 MB 5.25\n", desc);
			break;
		case 3:
			printf("        %s - 720 KB 3.5\n", desc);
			break;
		case 4:
			printf("        %s - 1.44 MB 3.5\n", desc);
			break;
		case 5:
		case 6:
			printf("        %s - 2.88 MB 3.5\n", desc);
			break;
		default:
			break;
	}
	return;
}

crio(fd,buf)
int fd;
char *buf;
{
	if(ioctl(fd,CMOSREAD, buf) < 0)
	{
		printf("can't open iocntl, cmd=%x, errno %d\n",buf[0], errno);
		exit(-1);
	}
}

/*
 * Returns memsize in megabytes
 */
uint_t
getmemsz()
{

	long npages;
	long kb_per_page;
	
	npages = sysconf(_SC_TOTAL_MEMORY);
	kb_per_page = sysconf(_SC_PAGE_SIZE) / KILOBYTE;

	return ((npages * kb_per_page) + KILOBYTE - 1) / KILOBYTE;
}

void
printpdi()
{
	FILE	*gpipe;
	char	*desc, *inquiry, *revision, *cdevice, *type, buf[BUFSIZ];
	int size;

	if ((gpipe = popen("/usr/bin/getdev pdimkdtab=true","r")) == NULL)
		return;
	while (fgets(buf, BUFSIZ, gpipe) != NULL) {
		buf[strlen(buf) - 1] = NULL;
		desc = devattr(buf, "desc");
		type = devattr(buf, "type");
		if ((inquiry = devattr(buf, "inquiry")) != NULL) {
			if (!strcmp(type,"disk")) {
				if (!strncmp(inquiry,"(athd)",6)) {
					printf("        %s - ATHD     "
					       "- %16.16s",desc,&inquiry[8]);
				} else
					printf("        %s - %8.8s "
					       "- %16.16s",
					       desc,inquiry,&inquiry[8]);
				cdevice = devattr(buf, "cdevice");
				if ((size = diskparms(cdevice)) != -1)
					printf(" - %ld MB\n", size);
				else
					printf("\n");
			} else if (strcmp(type,"hba")) {
				printf("        %s - %8.8s "
				       "- %16.16s\n",desc,inquiry,&inquiry[8]);
			}
		} else {
			if (!strcmp(type,"disk")) {
				printf("        %s",desc);
				cdevice = devattr(buf, "cdevice");
				if ((size = diskparms(cdevice)) != -1)
					printf(" - %ld MB\n", size);
				else
					printf("\n");
			} else if (strcmp(type,"hba")) {
				printf("        %s\n",desc);
			}
		}
	}
	pclose(gpipe);
	return;
}
