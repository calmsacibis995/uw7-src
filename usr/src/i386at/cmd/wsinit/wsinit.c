#ident	"@(#)wsinit.c	1.14"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation, and Microsoft Corporation
 *      and should be treated as Confidential.
 */

/*
 *	L000	3Aug97		rodneyh@sco.com
 *      - Change offset used to overwrite 'a' with 'm' in cmdbuf in main,
 *	  /sbin had been added to the path for putdev making the offset wrong.
 *	- Moved three calls to free outside the while loop in mkchan, we were
 *	  freeing the same buffers multiple times and screwing mallocs internal
 *	  information which resulted in a SIGSEGV if there were ever more than
 *	  one line in /etc/default/workstations as is the case if there is a
 *	  multiconsole device attached to the system.
 *
 */ 

#include <stdio.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/stropts.h>
#include	<sys/fcntl.h>
#include	<sys/mkdev.h> 
#include	<sys/genvid.h>
#include	<grp.h>
#include	<malloc.h>
#include	<ftw.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<mac.h>

char	*WSFILEDEFAULT	= "/dev/vt	/dev/kd/kd	9";
char	*WSFILE		= "/etc/default/workstations";
char	*WSDOTFILE	= "/etc/.wsinitdate";
char	*GVIDFILE	= "/dev/vidadm";
#define SYSNCHAN	15
#define	MAXLEN		128
#define	CMUX_MAJOR	5


#ifndef _STYPES
#define SYSMAXMINOR	MAXMIN
#else
#define	SYSMAXMINOR	OMAXMIN
#endif

struct group *getgrnam();

int cmux_minor = 0;
gvid_t Gvid;
dev_t *devbuf;
int devbufsize = 0;
struct group *grp;
int remake_devs = 0;

void
alloc_devbuf(size)
unsigned size;
{
	if (size <= devbufsize || size == 0)
		return;

	if (devbuf == (dev_t *) NULL) 
		devbuf = (dev_t *) malloc(size*sizeof(dev_t));
	else
		devbuf = (dev_t *)realloc((char *)devbuf,size*sizeof(dev_t));

	if (devbuf == (dev_t *) NULL) {
		disaster(10,gettxt(":1","out of space"));
		return;
	}
	devbufsize = size;
	return;
}

int
need_to_update_devs()
{
	struct stat wsfile_time;
	struct stat dotfile_time;

	if (stat(WSFILE,&wsfile_time) < 0) {
		pfmt(stderr, MM_ERROR,
			":2:stat of workstations file: %s\n",strerror(errno));
		exit(1);
	}
	if (stat(WSDOTFILE,&dotfile_time) < 0) 
		return (1); /* no dot file; we need to update devs */

	if (dotfile_time.st_mtime <= wsfile_time.st_mtime)
		return (1); /* ws file later than dot file; update devs */

	return (0); /* don't need to update devs */
}


main(argc, argv)
int argc;
char **argv;
{
	FILE *fp;
	struct	stat wsstatbuf;
	int dotfilefd;
	int gvidfd;
	int max, i;
	int chanmaj, drvmaj;
	int	macinstalled = 0;
	level_t	level;
	static char chan[MAXLEN],driver[MAXLEN],buf[3*MAXLEN];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxwsinit");
	(void)setlabel("UX:wsinit");
	
	/* parent of wsinit should be init */

	if ( getppid() != 1 ) {
		pfmt(stderr, MM_ERROR, ":24:Only init should be calling wsinit\n");
		exit(1);
	}

	if (lvlin(SYS_PUBLIC, &level) == 0) {
		macinstalled = 1;
	}

	devbuf = (dev_t *) NULL;

	grp = getgrnam("tty");
	if (grp == (struct group *) NULL)
		disaster(8,"tty");

	alloc_devbuf(256); 

	if (devbuf != (dev_t *) NULL)
		for (i=0; i<devbufsize; i++)
			*(devbuf +i) = NODEV;

	chanmaj = CMUX_MAJOR; 

	if ( stat(WSFILE, &wsstatbuf) == -1 ) {
		int	fd;
		
		pfmt(stderr, MM_ERROR, ":3:Cannot stat %s file: %s\n",
			WSFILE, strerror(errno));
		mkdir("/etc/default", 0x755);
 		fd = creat(WSFILE, 0444);
		if(fd < 0) {
			pfmt(stderr, MM_ERROR,
				":4:Cannot create %s file: %s\n",
				WSFILE, strerror(errno));
			exit(1);
		}
		else {
			write(fd, WSFILEDEFAULT, strlen(WSFILEDEFAULT));
			write(fd, "\n", 1);
			close(fd);
		}
		stat(WSFILE, &wsstatbuf);
		remake_devs = 1;
	}

	if ( (fp=fopen(WSFILE, "r")) == NULL ) {
		pfmt(stderr, MM_ERROR,
			":5:fopen: %s\n", strerror(errno));
		disaster(3, WSFILE);
		exit(1);
	}

	if (wsstatbuf.st_size == 0) {
		disaster(12, WSFILE);
		exit(1);
	}
		
	if(!remake_devs)
		remake_devs = need_to_update_devs();

	while ( fgets(buf,3*MAXLEN -1,fp) != (char *) NULL) {
		if (buf[0] == '#')
			continue;
		if(sscanf(buf,"%s %s %d",chan, driver, &max) != 3)
			continue;
		if (cmux_minor % SYSNCHAN)
			cmux_minor += SYSNCHAN - (cmux_minor % SYSNCHAN);
		if (cmux_minor >= devbufsize)
			alloc_devbuf(devbufsize*2);
		if (cmux_minor > SYSMAXMINOR) {
			disaster(7,chan);
			exit(7);
		}

		/* constrain max to values <= SYSNCHAN */
		max = (max > SYSNCHAN) ? SYSNCHAN : max;

		mkchan(chan,chanmaj,max,driver);
	}

	if (remake_devs) {
	   char cmdbuf[1024] = "/sbin/putdev -a console ";
           char vtbuf[16];
	   int  vt;

	   if (macinstalled) {
		strcat(cmdbuf, "range=SYS_RANGE_MAX-SYS_RANGE_MIN state=pub_priv mode=static startup=no ual_enable=yes other=\">y\" ");
	   }
	   strcat(cmdbuf, "cdevice=/dev/console cdevlist=\"");
           for(vt=0; vt< max; vt++) {
                sprintf(vtbuf, "/dev/vt%02d,", vt);
                strcat(cmdbuf, vtbuf);
           }
           strcat(cmdbuf, "/dev/vtmon,/dev/syscon,/dev/sysconreal,/dev/systty,/dev/video\"");

 
	   /* create/overwrite dot file and set mod time on dot file */
	   dotfilefd = creat(WSDOTFILE, 0644);
	   close (dotfilefd);
           if(system(cmdbuf) != 0) {
	   	cmdbuf[14] = 'm';			/* L000 */
		system(cmdbuf);
		}
	}

	if ( (gvidfd=open(GVIDFILE, O_RDWR)) == -1) {
		pfmt(stderr, MM_ERROR,
			":6:open: %s\n", strerror(errno));
		disaster(3,GVIDFILE);
		exit(3);
	}
	
	if (devbuf != (dev_t *) NULL) {
		Gvid.gvid_num = cmux_minor;
		Gvid.gvid_maj = chanmaj;
		Gvid.gvid_buf = &devbuf[0];
		if (ioctl(gvidfd,GVID_SETTABLE,&Gvid) < 0) {
			pfmt(stderr, MM_ERROR,
				":7:ioctl: %s\n", strerror(errno));
			disaster(8,GVIDFILE);
			exit(8);
		}
	}

	exit (0);
}

int
mkchan(chan,maj,max,drv)
char *chan, *drv;
int maj,max;
{
	register int i;		
	register int dev;		
	char *chantmp,*drvtmp,*vidtmp;
	int chanlen=strlen(chan);
	int drvlen=strlen(drv);
	int muxfd, devfd;
	struct stat statbuf;
	int mode,gid;
	int noderemake;

	chantmp = (char *)malloc(chanlen+3);
	drvtmp = (char *)malloc(drvlen+3);
	vidtmp = (char *)malloc(drvlen+5);

	mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH;

	if (grp == (struct group *) NULL)
		gid = 0;
	else
		gid = grp->gr_gid;

	if (remake_devs) {
	   for (i=0; i < SYSNCHAN; i++) {
		sprintf(chantmp,"%s%.2d",chan,i);
		if (access(chantmp,0) != -1) {
			if ( unlink(chantmp) == -1 ) 
				disaster(1,chantmp);
		}
	   }
	}


	for (i=0;i < max;i++) {
		sprintf(chantmp,"%s%.2d",chan,i);
		sprintf(vidtmp,"%svm%.2d",drv,i);
		sprintf(drvtmp,"%s%.2d",drv,i);

		noderemake = remake_devs;
		dev = makedev(maj, cmux_minor);
		if (!remake_devs )
		   if (stat(chantmp, &statbuf) == -1 ||
		     (statbuf.st_mode&S_IFCHR) == 0 ||
		     statbuf.st_rdev != dev) {
			unlink(chantmp);
			noderemake = 1;
		   }
		if (noderemake && (mknod(chantmp,mode, dev) == -1) )
			disaster(2,chantmp);

		if ( noderemake && (chown(chantmp,0,gid) == -1))
			disaster(9,chantmp);

		if ( (muxfd=open(chantmp,O_RDWR)) == -1 ) {
			pfmt(stderr, MM_ERROR,
				":8:mux open: %s\n", strerror(errno));
			disaster(2,chantmp);
			return; 
		}


		if ( (devfd=open(drvtmp, O_RDWR)) == -1 ) {
			pfmt(stderr, MM_ERROR,
				":9:driver open: %s\n", strerror(errno));
			disaster(3,drvtmp);
			close(muxfd);
			return;
		}

		if ( ioctl(muxfd, I_PLINK, devfd) == -1 ) {
			pfmt(stderr, MM_ERROR,
				":7:ioctl: %s\n", strerror(errno));
			disaster(4,chantmp);
			close(muxfd);
			close(devfd);
			return;
		}

		if ( stat(vidtmp, &statbuf) == -1 ) {
			pfmt(stderr, MM_ERROR,
				":10:stat: %s\n", strerror(errno));
			disaster(5,vidtmp);
			close(muxfd);
			close(devfd);

			continue;
		}

		if (! (statbuf.st_mode&S_IFCHR)) {
			disaster(6,vidtmp);
			close(muxfd);
			close(devfd);

			continue;
		}

		if (devbuf != (dev_t *) NULL)
			*(devbuf + cmux_minor++) = statbuf.st_rdev;
		
		close(muxfd);
		close(devfd);

	}

	/* L000
	 *
	 * Moved the three free's outside the while loop so we don't screw up
	 * the malloc headers.
	 */
	free(chantmp);
	free(vidtmp);
	free(drvtmp);
}

disaster(typ,file)
int typ;
char *file;
{
	switch(typ) {
	case 1:
		pfmt(stderr, MM_WARNING,
			":11:Cannot unlink %s\n",file);
		break;
	case 2:
		pfmt(stderr, MM_WARNING,
			":12:Cannot make %s\n",file);
		break;
	case 3:
		pfmt(stderr, MM_WARNING,
			":13:Cannot open %s\n",file);
		break;
	case 4:
		pfmt(stderr, MM_WARNING,
			":14:Cannot I_LINK %s\n",file);
		break;
	case 5:
		pfmt(stderr, MM_WARNING,
			":15:Cannot stat(2) %s\n",file);
		break;
	case 6:
		pfmt(stderr, MM_WARNING,
			":16:file %s is not character special\n",file);
		break;
	case 7:
		pfmt(stderr, MM_WARNING,
			":17:out of minor numbers for %s\n",file);
		break;
	case 8:
		pfmt(stderr, MM_WARNING,
			":18:could not locate group information for group %s\n",file);
		break;
	case 9:
		pfmt(stderr, MM_WARNING,
			":19:could not change ownership of %s\n",file);
		break;
	case 10:
		pfmt(stderr, MM_WARNING,
			":20:ran out of space for video device buffer. Graphics will not work\n",file);
		break;
	case 11:
		pfmt(stderr, MM_WARNING,
			":21:%s failed. Some special files may not have been removed\n",file);
		break;
	case 12:
		pfmt(stderr, MM_WARNING,
			":22:file %s is truncated to zero length\n", file);
		break;
	default:
		pfmt(stderr, MM_WARNING,
			":23:Unknown wsinit error type%d\n",typ);
		break;
	}
}
