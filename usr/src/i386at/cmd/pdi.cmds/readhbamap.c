#ident	"@(#)pdi.cmds:readhbamap.c	1.3"

#include	<sys/types.h>
#include	<sys/sdi_edt.h>
#include	<sys/stat.h>
#include	<sys/sysi86.h>
#include	<fcntl.h>

extern int	errno;
extern void	error();

/*
 *  Return the SDI HBA map array
 *	Inputs:  hacnt - pointer to integer to place the number of map entries
 *	Return:  address of the HBA map
 *	         0 if couldn't read the HBA map
 */

int *
readhbamap(int *mapcnt)
{
	int *hba_map;
	int	sdi_fd, map_count;
	char 	*mktemp();
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	dev_t	sdi_dev;

	setuid(0);

	*mapcnt = 0;

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		error(":387:ioctl(SI86SDIDEV) failed.\n");
		return(0);
	}

	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		error(":391:mknod failed for sdi temp device\n");
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
		error(":382:Cannot open sdi device: %s\n", sditempnode);
	}
	unlink(sditempnode);

	/*  Get the Number of EDT entries in the system  */
	map_count = 0;
	if (ioctl(sdi_fd, B_MAP_CNT, &map_count) < 0)  {
		(void)close(sdi_fd);
		error(":484:ioctl(B_MAP_CNT) failed\n"); 
		return(0);
	}

	if (map_count == 0)	{
		(void)close(sdi_fd);
		errno = 0;
		error(":485:Unable to determine the number of HBA map entries.\n");
		return(0);
	}

	*mapcnt = map_count;
	/*  Allocate space for HBA map  */
	if ((hba_map = (int *)calloc(map_count, sizeof(int))) == NULL)	{
		(void)close(sdi_fd);
		errno = 0;
		error(":486:Calloc for HBA map failed\n");
		return(0);
	}

	/*  Read in the HBA map  */
	if (ioctl(sdi_fd, B_GET_MAP, hba_map) < 0)  {
		(void)close(sdi_fd);
		error(":487:ioctl(B_GET_MAP) failed\n"); 
		return(0);
	}

	(void)close(sdi_fd);
	return(hba_map);
}

/*
 *  Update the SDI HBA map array
 *	Inputs:  hba_map - pointer to updated HBA map
 *	         map_count - integer reflecting the number of map entries
 *	Return:  address of the HBA map
 *	         0 if couldn't read the HBA map
 */

int
writehbamap(int *hba_map,int map_count)
{
	int	sdi_fd;
	char 	*mktemp();
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	dev_t	sdi_dev;
	struct putmapargs {
		int	a_cnt;
		int *	a_hbamap;
	} putmapargs;


	if (map_count == 0)
		return(1);

	setuid(0);

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		error(":387:ioctl(SI86SDIDEV) failed.\n");
		return(0);
	}

	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		error(":391:mknod failed for sdi temp device\n");
		return(0);
	}

	errno = 0;
	if ((sdi_fd = open(sditempnode, O_RDONLY)) < 0) {
		unlink(sditempnode);
		error(":382:Cannot open sdi device: %s\n", sditempnode);
	}
	unlink(sditempnode);

	putmapargs.a_cnt = map_count;
	putmapargs.a_hbamap = hba_map;

	/*  Write out the HBA map  */
	if (ioctl(sdi_fd, B_RESERVED, &putmapargs) < 0)  {
		(void)close(sdi_fd);
		error(":487:ioctl(B_GET_MAP) failed\n"); 
		return(0);
	}

	(void)close(sdi_fd);
	return(1);
}
