#ident	"@(#)pdi.cmds:hbacompat.c	1.1"
#ident	"$Header$"

#include <fcntl.h>
#include <sys/ksym.h>

#define KMEM "/dev/kmem"
unsigned long hbacnt;
struct mioc_rksym rks = {"sdi_phystokv_hbacnt", 
			  &hbacnt, 
			  sizeof(unsigned long)
			};
char *prog;

main(int argc, char *argv[])
{
	int kmemfd;

	prog = argv[0];
	if ((kmemfd = open (KMEM, O_RDONLY)) < 0 ) {
		exit (2);
	}

	if (ioctl (kmemfd, MIOC_READKSYM, &rks) < 0) {
		exit (2);
	}

	exit (hbacnt ? 1:0);
}
