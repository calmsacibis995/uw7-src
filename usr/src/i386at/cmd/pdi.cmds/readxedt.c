#ident	"@(#)pdi.cmds:readxedt.c	1.4.1.1"

#include	<sys/types.h>
#include	<sys/sdi_edt.h>
#include	<sys/stat.h>
#include	<sys/sysi86.h>
#include	<fcntl.h>
#include	<pfmt.h>
#include	<stdio.h>
#include	"readxedt.h"

extern int	errno;

/*
 *  Return the SCSI Extended Equipped Device Table
 *	Inputs:  hacnt - pointer to integer to place the number of HA's.
 *	Return:  address of the XEDT
 *	         0 if couldn't read the XEDT
 */

struct scsi_xedt *
readxedt(int *edtcnt)
{
	struct	scsi_xedt *xedt;
	int	sdi_fd, edt_count;
	char 	*mktemp();
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	dev_t	sdi_dev;

	setuid(0);

	*edtcnt = 0;

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		pfmt(stderr, MM_ERROR, ":387:ioctl(SI86SDIDEV) failed.\n");
		return(0);
	}

	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		pfmt(stderr, MM_ERROR, ":391:mknod failed for sdi temp device\n");
		return(0);
	}

/*
 *	This open will no longer fail because we are using a
 *	special pass_thru major which is only for issuing sdi_ioctls.
 *	This open does not require exclusive use of the pass_thru
 *	to an HBA so there is no problem with it being in use.
 */
	errno = 0;
	if ((sdi_fd = open(sditempnode, O_RDONLY)) < 0) {
		unlink(sditempnode);
		pfmt(stderr, MM_ERROR,":382:Cannot open sdi device: %s\n", sditempnode);
	}

	/*  Get the Number of EDT entries in the system  */
	edt_count = 0;
	if (ioctl(sdi_fd, B_EDT_CNT, &edt_count) < 0)  {
		(void)close(sdi_fd);
		unlink(sditempnode);
		pfmt(stderr, MM_ERROR, ":388:ioctl(B_EDT_CNT) failed\n"); 
		return(0);
	}

	if (edt_count == 0)	{
		(void)close(sdi_fd);
		unlink(sditempnode);
		errno = 0;
		pfmt(stderr, MM_ERROR, ":383:Unable to determine the number of EDT entries.\n");
		return(0);
	}

	*edtcnt = edt_count;
	/*  Allocate space for SCSI XEDT  */
	if ((xedt = (struct scsi_xedt *) calloc(1, sizeof(struct scsi_xedt) * edt_count)) == NULL)	{
		(void)close(sdi_fd);
		unlink(sditempnode);
		errno = 0;
		pfmt(stderr, MM_ERROR, ":379:Calloc for XEDT structure failed\n");
		return(0);
	}

	/*  Read in the SCSI XEDT  */
	if (ioctl(sdi_fd, B_RXEDT, xedt) < 0)  {
		(void)close(sdi_fd);
		unlink(sditempnode);
		pfmt(stderr, MM_ERROR, ":389:ioctl(B_RXEDT) failed\n"); 
		return(0);
	}

	(void)close(sdi_fd);
	unlink(sditempnode);
	return(xedt);
}

