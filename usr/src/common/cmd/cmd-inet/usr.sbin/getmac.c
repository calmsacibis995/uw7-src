#ident	"@(#)getmac.c	1.4"
#ident	"$Header$"

#include <stdio.h>
#include <fcntl.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/stropts.h>

#define DLGADDR	(('D' << 8) | 5)

/*
 * getmac
 *	Usage: getmac [ device_name ]
 *
 * getmac will generate a file, /etc/inet/macaddr which contains
 * info on the mac address of devices that are in the 
 * /etc/confnet.d/inet/interface file. The purpose of this file is for
 * upgrade of inet pkg to uniquely identify devices of the same kind.
 * Example entries are as follows:
 *
 *	/dev/el16_0	aa:bb:cc:dd
 *	/dev/el16_1	ee:ff:gg:hh
 *
 * if device_name is provided, getmac will print the output to stdout.
 */

static int get_macaddr(char *, FILE *);
static void print_all(void);

main(int argc , char **argv)
{
	if (argc > 2 ) {
		(void)printf("Usage: getmac [ device_name ]\n");
		exit(2);
	}
	if (argc == 2) {
		if (get_macaddr(argv[1], stdout) < 0) {
			perror("get_macaddr failed");
			(void)printf("Usage: getmac [ device_name ]\n");
			exit(1);
		}
		exit(0);
	}
	print_all();
	exit(0);
} 

static union DL_primitives primbuf;
static struct strbuf primptr = { 0, 0, (char *)&primbuf };

static int 
get_macaddr(char *device, FILE *outfp)
{
	int fd;
	struct strioctl strioc;
	unsigned char llc_mc[8];
	int ret, flags = 0;
	register dl_phys_addr_req_t *brp;
	register dl_phys_addr_ack_t *bap;
	dl_error_ack_t *bep;
	unsigned char	buf[80];		/* place for received table */
	unsigned char	*cp;

	if ((fd = open(device, O_RDWR)) == -1) {
		return(-1);
	} 

	brp = (dl_phys_addr_req_t *)primptr.buf;
	memset(brp, 0, sizeof(dl_phys_addr_req_t));
	brp->dl_primitive = DL_PHYS_ADDR_REQ;
	brp->dl_addr_type = DL_CURR_PHYS_ADDR;
	primptr.len = sizeof(dl_phys_addr_req_t);

	if (putmsg(fd, &primptr, (struct strbuf *)0, 0) < 0)
		return(-1);

	primptr.maxlen = primptr.len = sizeof(primbuf) + 6;
	primptr.buf = (char *)buf;
	ret = getmsg(fd, &primptr, (struct strbuf *)0, &flags);

	primptr.buf = (char *)&primbuf;

	if (ret < 0)
		return(-1);

	bap = (dl_phys_addr_ack_t *)buf;
	if (bap->dl_primitive == DL_PHYS_ADDR_ACK) {
		cp = buf + bap->dl_addr_offset;
		goto out;
	}

	strioc.ic_cmd = DLIOCGENADDR;
	strioc.ic_timout = 15;
	strioc.ic_len = LLC_ADDR_LEN;
	strioc.ic_dp = (char *)llc_mc;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		/*
		 * dlpi token driver does not recognize DLIOCGENADDR,
		 * so try DLGADDR
		 */
		strioc.ic_cmd = DLGADDR;
		strioc.ic_timout = 15;
		strioc.ic_len = LLC_ADDR_LEN;
		strioc.ic_dp = (char *)llc_mc;
		if (ioctl(fd, I_STR, &strioc) < 0) {
			return(-1);
		}
	}
	cp = llc_mc;
out:
	(void)fprintf(outfp, "%s\t%02X.%02X.%02X.%02X.%02X.%02X\n",
		device, cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]);
	return(0);
}

static void
print_all(void)
{
	char	*extractor = "/usr/bin/egrep -v \"^#|^lo:\" /etc/confnet.d/inet/interface | /usr/bin/cut -d: -f 4";
	FILE *fp;
	FILE *outfp;
	char devname[256];
	

	if ((outfp = fopen("/etc/inet/macaddr", "w")) == NULL) {
		perror("open /etc/inet/macaddr failed");
		return;
	}
	if ((fp = popen(extractor, "r")) != NULL) {
		while (fgets(devname, 256, fp) != NULL) {
			devname[strlen(devname) - 1] ='\0';
			(void)get_macaddr(devname, outfp);
		}
		(void)pclose(fp);
	}
	else
		perror("popen failed");
}
