#ident	"@(#)crash:common/cmd/crash/vfs.c	1.2.1.1"

/*
 * This file contains code for the crash functions:  vfs (mount).
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/fstyp.h>
#include "crash.h"

static vaddr_t Rootvfs, Mac_installed;
int mac_installed;
vaddr_t prvfs();

/* get arguments for vfs (mount) function */
int
getvfs()
{
	int c;
	vaddr_t addr;
	char *baseheading = " FSTYP  BSZ MAJ/MIN     FSID VNCOVERED     PDATA   BCOUNT MUTEX FLAGS\n";
	char *secheading  = " FSTYP  BSZ MAJ/MIN     FSID VNCOVERED     PDATA   BCOUNT MUTEX FLAGS MACFLOOR MACCEILING\n";
	char *heading;

	if (!Rootvfs)
		Rootvfs = symfindval("rootvfs");
	if (!Mac_installed)
		Mac_installed = symfindval("mac_installed");
	readmem(Mac_installed,1,-1,
		&mac_installed,sizeof mac_installed,"mac_installed");
	if (mac_installed)
		heading = secheading;
	else	heading = baseheading;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	fprintf(fp, "%s", heading);
	if(args[optind]){
		do {
			addr = (vaddr_t)strcon(args[optind], 'h');
			(void) prvfs(addr);
		}while(args[++optind]);
	} else {
		readmem(Rootvfs, 1, -1, &addr, sizeof addr,"rootvfs");
		while (addr)
			addr = prvfs(addr);
	}
}

/* print vfs list */
vaddr_t
prvfs(addr)
vaddr_t addr;
{
	struct vfs vfsbuf;
	char *fsname;
	int n, width;

	readmem(addr,1,-1,(char *)&vfsbuf,sizeof vfsbuf,"vfs list");
	if ((fsname = getfsname(vfsbuf.vfs_fstype)) == NULL)
		fsname = "    - ";

	n = fprintf(fp,"%6s %4u",
		fsname,
		vfsbuf.vfs_bsize);
	n += fprintf(fp,"%4u,%-4u",
		getemajor(vfsbuf.vfs_dev),
		geteminor(vfsbuf.vfs_dev));
	width = 8;
	if ((n -= 20) > 0)		/* try to recover from "processorfs" */
		if ((width -= n) <= 0)
			width = 1;
	fprintf(fp, "%*x  %8x  %8x %8x",
		width,
		vfsbuf.vfs_fsid.val[0],
		vfsbuf.vfs_vnodecovered,
		vfsbuf.vfs_data,
		vfsbuf.vfs_bcount);
	fprintf(fp," %3d  ",
		vfsbuf.vfs_mutex.sp_lock);
	fprintf(fp,"%s%s%s%s%s%s",
		(vfsbuf.vfs_flag & VFS_RDONLY) ? " rd" : "",
		(vfsbuf.vfs_flag & VFS_NOSUID) ? " nosu" : "",
		(vfsbuf.vfs_flag & VFS_REMOUNT) ? " remnt" : "",
		(vfsbuf.vfs_flag & VFS_NOTRUNC) ? " notr" : "",
		(vfsbuf.vfs_flag & VFS_UNLINKABLE) ? " nolnk" : "",
		(vfsbuf.vfs_flag & VFS_BADBLOCK) ? " bdblk" : "");
	if (mac_installed) {
		fprintf(fp," %ld %ld",
		vfsbuf.vfs_macfloor,
		vfsbuf.vfs_macceiling);
	}
	fprintf(fp,"\n");
	return (vaddr_t)vfsbuf.vfs_next;
}
