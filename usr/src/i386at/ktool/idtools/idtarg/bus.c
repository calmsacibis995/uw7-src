#ident	"@(#)ktool:i386at/ktool/idtools/idtarg/bus.c	1.1.2.1"
#ident	"$Header:"

#include	<stdio.h>
#include	<malloc.h>
#include	<unistd.h>
#include	<errno.h>
#include	<assert.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/confmgr.h>
#include	<sys/cm_i386at.h>


char *rmdb_file = "/dev/resmgr";

int rmfd;
int valbufsz;
struct rm_ioctl_args r;


void *
malloc_fe(len)
size_t len;
{
	void *p;

	p = malloc(len);

	if (p == NULL)
	{
		fprintf(stderr, "can't malloc %d bytes\n", len);
		exit(1);
	}

	return p;
}


rm_init()
{

	rmfd = open(rmdb_file, O_RDONLY);
	if (rmfd < 0)
	{
		perror(rmdb_file);
		exit(1);
	}

	valbufsz = 1024;
	r.rma.rm_val = malloc_fe(valbufsz);
}


rm_done()
{

	close(rmfd);
	free(r.rma.rm_val);
	valbufsz = 0;
	r.rma.rm_val = NULL;
}


rm_getval(param, n)
char *param;
int n;
{
	int ret;
	int first = 1;			/* for assertion checking */

	r.rma.rm_n = n;
	r.mode= RM_READ;
	if (param)
		strcpy(r.rma.rm_param, param);

	(void) ioctl(rmfd, RMIOC_BEGINTRANS, &r);
try_again:
	r.rma.rm_vallen = valbufsz;
	ret = ioctl(rmfd, RMIOC_GETVAL, &r);

	if (ret != 0)
	{
		if (errno == ENOSPC)
		{
			/*
			 *  Buffer was too small, malloc one big enough
			 */

			assert(valbufsz > 0);
			assert(r.rma.rm_vallen > valbufsz);	/* paranoia */
			assert(first);
			first = 0;

			valbufsz = r.rma.rm_vallen;
			free(r.rma.rm_val);
			r.rma.rm_val = malloc_fe(valbufsz);
			goto try_again;
		}

		if (errno == ENOENT)
			return 0;	/* out of values for this param */

		perror("RMIOC_GETVAL");
		(void) ioctl(rmfd, RMIOC_ABORTTRANS, &r);
		exit(1);
	}

	(void) ioctl(rmfd, RMIOC_ENDTRANS, &r);
	return 1;
}


bus_name()
{
	int ret;

	rm_init();

	r.rma.rm_key = RM_KEY;
	if (!rm_getval(CM_BUSTYPES, 0))
	{
		fprintf(stderr, "can't determine system bus type\n");
		exit(1);
	}

	assert(r.rma.rm_vallen == sizeof(ret));
	ret = *((int *) r.rma.rm_val);

	if (ret & CM_BUS_ISA)
		printf("%s\n", CM_ISA);
	if (ret & CM_BUS_EISA)
		printf("%s\n", CM_EISA);
	if (ret & CM_BUS_PCI)
		printf("%s\n", CM_PCI);
	if (ret & CM_BUS_PCMCIA)
		printf("%s\n", CM_PCCARD);
	if (ret & CM_BUS_PNPISA)
		printf("%s\n", CM_PNPISA);
	if (ret & CM_BUS_MCA)
		printf("%s\n", CM_MCA);

	rm_done();

	exit(0);
}

