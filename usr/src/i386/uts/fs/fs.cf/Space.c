#ident	"@(#)kern-i386:fs/fs.cf/Space.c	1.4.2.1"
#ident	"$Header$"

#include <config.h>
#include <sys/conf.h>
#include <sys/buf.h>

struct hbuf hbuf[NHBUF];

/* pageio_setup overflow list for pageout daemon */
struct buf pgoutbuf[NPGOUTBUF];
int npgoutbuf = NPGOUTBUF;

int rstchown = RSTCHOWN;

char rootfstype[ROOTFS_NAMESZ+1] = ROOTFSTYPE;

/* Directory name lookup cache size */
int     ncsize;
int     nchash_size = NC_HASH_SIZE;

/* Enhanced Application Compatibility Support */
#ifdef ACAD_CMAJOR_0
	int dev_autocad_major = ACAD_CMAJOR_0;
#else
     	int dev_autocad_major = -1;
#endif

int poll_delay_compatibility = SCO_POLL_DELAY;

/* End Enhanced Application Compatibility Support */


