#ident "@(#)Nonconform-file.c	22.1"
#ident "$Header$"

/*
 *	Nonconform-file.c, media support module file support.
 *      adapted from usr/src/i386at/uts/io/odi/msm/msmfile.c
 *      We can't include ddi.h as that will change uio_offset into 
 *      uio structure.
 */

#include <proc/cred.h>
#include <io/sad/sad.h>
#include <io/uio.h>
#include <io/nd/sys/mdi.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <fs/file.h>
#include <fs/vnode.h>

#define	BLOCK_SIZE	512
#define	MAX_BUF_SIZE	(64 * 1024)

extern	cred_t  *sys_cred;
extern	int	lookupname();
extern void *dlpibase_zalloc(ulong_t, int, int, int);


/*
 * mdi_open_file_i(char *pathname, uchar_t **filebuffer, int *bufferlength)
 *
 * Calling/exit status:
 *	This must be called in a single lwp, so that the file metadata
 *	is accessed only in one lwp. No file locking is done here.
 *
 * Description:
 *	This routine opens the file named by pathname, allocates
 *	a kernel buffer, and reads the entire contents of the file into the
 *	contigous kernel buffer. It then closes the file, and returns the
 *	buffer and its length. This is used by ODI routines to read their 
 *	config (firmware, message, etc) files from within the kernel.
 *
 *	Buffer allocated is the responsibility of the caller.
 */
int
mdi_open_file(char *pathname, uchar_t **filebuffer, int *bufferlength)
{
	return(mdi_open_file_i(pathname, filebuffer, bufferlength,
		DLPIBASE_MEM_DMA_BELOW16, KM_NOSLEEP, DLPIBASE_KMEM));
}

/* 
 * mdi_open_file_i
 *
 * Internal version where you get to supply the memory allocation 
 * scheme.  Not normally exposed to vendors but available as a back door
 * if required.
 *
 * context:  must be called from blockable user context, as lookupname calls
 *           pn_get->pn_alloc->kmem_alloc(MAXPATHLEN, KM_SLEEP)
 *           (and I'm sure there are others too)
 */
int
mdi_open_file_i(char *pathname, uchar_t **filebuffer, int *bufferlength,
		int memtype, int kmaflags, int miscflags)
{
	vnode_t	*filevnode;
	uio_t	ioargs;
	iovec_t	iovector;
	vattr_t	nodeattr;
	int	ret = 0;

	if (pathname == NULL) {
		cmn_err(CE_NOTE,"mdi_open_file_i: null pathanme");
		return(ENOENT);
	}

	if (*pathname != '/') {
		cmn_err(CE_NOTE,"mdi_open_file_i: %s: not absolute",pathname); 
		return(ENOENT);
	}

	/*
	 * have lookupname(3K) translate 'pathname` to a filevnode
	 * (ie a VNODE), so we can use the filevnode internal name to open 
	 * the file for reading.
	 */
	if (lookupname(pathname, UIO_SYSSPACE, FOLLOW, NULLVPP,
			&filevnode) != 0) {
		/*
		 * no file by pathname can be found.
		 */
		cmn_err(CE_CONT, "mdi_open_file_i: The file %s cannot be "
				"found.\n\r", pathname);
		return(ENOENT);
	}

	if (filevnode->v_type != VREG ) {
		/*
		 * pathname is not a regular file.
		 */
		VN_RELE(filevnode);
		cmn_err(CE_CONT, "mdi_open_file_i: The file %s is not a "
				"regular file.\n\r", pathname);
		return(ENOENT);
	}

	/*
	 * open the file via the VFS/VNODE, emulate the generic file system
	 * here. use sys_cred.
	 */
	if (VOP_OPEN(&filevnode, FREAD, sys_cred) != 0 ) {
		/*
		 * Can't open file?
		 */
		VN_RELE(filevnode);
		cmn_err(CE_CONT, "mdi_open_file_i: The file %s cannot "
				"be opened.\n\r", pathname);
		return(EACCES);
	}

	/*
	 * allocate a kernel buffer the same size as the data space in the
	 * configuration file.
	 */
	nodeattr.va_mask = AT_SIZE;
	if (VOP_GETATTR(filevnode, &nodeattr, 0, sys_cred) != 0) {
		/*
		 * Can't stat file?
		 */
		VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
		VN_RELE(filevnode);
		cmn_err(CE_CONT, "mdi_open_file_i: The file %s cannot "
				"be stat'ed.\n\r", pathname);
		return(EACCES);
	}

	if ((nodeattr.va_size == 0) || (nodeattr.va_size > MAX_BUF_SIZE)) {
		/*
		 * file has no size or too big.
		 */
		VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
		VN_RELE(filevnode);
		cmn_err(CE_CONT, "mdi_open_file_i: The file %s is zero "
				"length or is too big(>%d bytes).\n\r",
				pathname, MAX_BUF_SIZE);
		return(EFBIG);
	}

	if (!(*filebuffer = (uchar_t *)dlpibase_zalloc(nodeattr.va_size,
		memtype, kmaflags, miscflags))) {
		/*
		 * buffer could not be allocated.
		 */
		VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
		VN_RELE(filevnode);
		cmn_err(CE_CONT, "mdi_open_file_i: Cannot allocate memory "
				"for file %s.\n\r", pathname);
		return(ENOMEM);
	}

	/*
	 * obtain a read lock necessary for VOP_READ call below.
	 */
	ret = VOP_RWRDLOCK(filevnode, 0, nodeattr.va_size, FNDELAY);

	if (ret) {
		/* 
		 * lock could not be obtained
		 */
		VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
		VN_RELE(filevnode);
		dlpibase_free(*filebuffer, memtype, miscflags);
		cmn_err(CE_CONT, "mdi_open_file_i: Could not obtain rdlock "
				"for %s.\n\r", pathname);
		return(ret);
	}

	/*
	 * read the file via the VFS/VNODE, emulate the generic fs here.
	 */
	iovector.iov_base = (caddr_t)*filebuffer;
	ioargs.uio_iov = &iovector;
	ioargs.uio_iovcnt = 1;
	ioargs.uio_offset = 0;
	ioargs.uio_segflg = UIO_SYSSPACE;
	ioargs.uio_fmode = FREAD;
	ioargs.uio_resid = iovector.iov_len = nodeattr.va_size;
	ioargs.uio_limit = MAX_BUF_SIZE/BLOCK_SIZE;

	do {
		if ((ret = VOP_READ(filevnode, &ioargs, 0, sys_cred)) != 0 ) {
			/*
			 * Can't read file.
			 */
			VOP_RWUNLOCK(filevnode, 0, nodeattr.va_size);
			VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
			VN_RELE(filevnode);
			dlpibase_free(*filebuffer, memtype, miscflags);
			cmn_err(CE_CONT, "mdi_open_file_i: The file %s cannot "
				"be read(ret=%d).\n\r", pathname, ret);
			return(ret);
		}

		if (ioargs.uio_resid) {
			iovector.iov_base = (void *)((int)iovector.iov_base +
					(iovector.iov_len - ioargs.uio_resid));
			iovector.iov_len = ioargs.uio_resid;
		}
	} while (ioargs.uio_resid);

	/* 
	 * release our lock
	 */
	VOP_RWUNLOCK(filevnode, 0, nodeattr.va_size);

	/*
	 * close this instance of the file and release our usage of it.
	 */
	VOP_CLOSE(filevnode, FREAD, 1, 0, sys_cred);
	VN_RELE(filevnode);

	*bufferlength = nodeattr.va_size;
	
	return(0);
}
