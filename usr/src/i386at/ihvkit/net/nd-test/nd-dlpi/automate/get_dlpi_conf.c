#include <stdio.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/stropts.h>

#define DLGADDR	(('D' << 8) | 5)
/*
 * getdlpiconf
 *	Usage: getdlpiconf 
 *
 * getdlpiconf will hostname, device node and mac address of all the 
 * network devices configured in the system
 */

static int get_macaddr(char *, char *);
static int get_uname(char *);

main()
{
	char	*getinfo = "/usr/bin/egrep -v \"^#|^lo:\" /etc/confnet.d/inet/interface | /usr/bin/cut -d: -f 3,4";
	char 	*hostname, *devname, macaddr[18];
	char	line[256], *p;
	FILE *fp;
	
	if ((fp = popen(getinfo, "r")) != NULL) {
		while (fgets(line, 256, fp) != NULL) {
			hostname = p = line;
			while (*p != ':')
				p++;
			*p++ = '\0';
			devname = p;
			devname[strlen(devname) - 1] ='\0';
			if (get_macaddr(devname,macaddr) < 0) {
		/*
		** there can be many reasons for this to happen:
		** 	- a bogus entry in the 'interface' file
		**	- a device which does not support this IOCTL e.g. PPP
		** so, ignore it.
		*/
				continue;
			}
			if (*hostname == '\0') {
				hostname = devname + strlen(devname) + 1;
				if (get_uname(hostname) < 0) {
					perror("uname failed");
					exit(1);
				}
			}
			printf("%s\t%s\t%s\n",hostname, devname, macaddr);
		}
		(void)pclose(fp);
	}
	else {
		perror("popen failed");
		exit(1);
	}
	exit(0);
}

static int 
get_macaddr(char *device, char *eaddr)
{
	int fd;
	struct strioctl strioc;
	unsigned char llc_mc[8];

	if ((fd = open(device, O_RDONLY|O_NONBLOCK)) == -1) {
		return(-1);
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
	(void)sprintf(eaddr, "%02X.%02X.%02X.%02X.%02X.%02X", llc_mc[0], 
			llc_mc[1], llc_mc[2], llc_mc[3], llc_mc[4], llc_mc[5] );
	return(0);
}

static int
get_uname(host)
char	host[];  
{
	struct utsname name;

	if (uname(&name) < 0)
		return(-1);
	else
		strcpy(host, name.nodename);
	return(0);
}
