#ident	"@(#)dcu:dcusilent.c	1.6.2.1"

#include	<fcntl.h>
#include	<sys/resmgr.h>
#include	<sys/cm_i386at.h>

main()
{
	struct rm_ioctl_args	r;
	cm_num_t	dcu_mode;
	int		rm_fd;
	int ret;

	if((rm_fd = open(DEV_RESMGR, O_RDONLY)) < 0) {
		printf("open(DEV_RESMGR) failed.\n");
		exit(0);
	}

	r.rma.rm_key = 0;
	r.mode = RM_READ;
	if((ret = ioctl(rm_fd, RMIOC_BEGINTRANS, &r)) < 0) {
		printf("ioctl() failed for BEGIN_TRANS %d.\n", ret);
		exit(0);
	}

	strcpy(r.rma.rm_param, CM_DCU_MODE);
	r.rma.rm_key = RM_KEY;
	r.rma.rm_val = &dcu_mode;
	r.rma.rm_vallen = sizeof(dcu_mode);
	r.rma.rm_n = 0;

	if(ioctl(rm_fd, RMIOC_GETVAL, &r) < 0) {
		printf("ioctl() failed for CM_DCU_MODE.\n");
		exit(0);
	}

	if((ret=ioctl(rm_fd, RMIOC_ENDTRANS, &r)) < 0) {
		printf("ioctl() failed for ENDTRANS %d.\n", ret);
		exit(0);
	}

	exit(dcu_mode);
}
