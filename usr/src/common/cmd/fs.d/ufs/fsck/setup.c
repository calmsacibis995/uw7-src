/*	copyright	"%c%"	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */
#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/setup.c	1.8.9.6"

/*  "errexit()" and "pfatal()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/stat.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include <string.h>
#include "fsck.h"
#include <sys/vfstab.h>
#include <pfmt.h>

char	*calloc();
extern int	mflag;
extern char 	hotroot;

#define altsblock asblk.b_un.b_fs

/*
 * Initialize the super block, link lists, and buffers.
 */
char *
setup(dev)
	char *dev;
{
	dev_t rootdev;
	struct stat statb;
	daddr_t super = bflag ? bflag : SBLOCK;
	int i, j;
	long size;
	BUFAREA asblk;
	static char devstr[MAXPATHLEN];
	char	*raw, *rawname();
	int mntcode;

	if (stat("/", &statb) < 0)
		errexit(":232:Cannot stat root\n");
	rootdev = statb.st_dev;

	strcpy(devstr, dev);
	devname = dev;
restat:
	if (stat(devstr, &statb) < 0) {
		myfprintf(stderr, MM_ERROR, ":233:Cannot stat %s\n", devstr);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK) {
		if (rootdev == statb.st_rdev)
			hotroot++;
		else if ((statb.st_flags & _S_ISMOUNTED) && !nflag) {
			myfprintf(stderr, MM_ERROR, ":316:%s is a mounted file system, ignored\n", dev);
			exit(33);
		}	
	} else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		FILE *vfstab;
		struct vfstab vfsbuf;
		/*
		 * Check vfstab for a mount point with this name
		 */
		if ((vfstab = fopen(VFSTAB, "r")) == NULL) {
			errexit(":210:Cannot open checklist file: %s\n", VFSTAB);
		}
		while (getvfsent(vfstab, &vfsbuf) == NULL) {
			if (vfsbuf.vfs_mountp &&
				strcmp(devstr,vfsbuf.vfs_mountp) == 0) {
				if (vfsbuf.vfs_fstype &&
				   strcmp(vfsbuf.vfs_fstype, MNTTYPE_UFS) != 0) {
					/*
					 * found the entry but it is not a
					 * ufs filesystem, don't check it
					 */
					fclose(vfstab);
					return (0);
				}
				strcpy(devstr, vfsbuf.vfs_special);
				if (rflag) {
					raw =
					    rawname(unrawname(vfsbuf.vfs_special));
					strcpy(devstr, raw);
				}
				goto restat;
			}
		}
		fclose(vfstab);
	} else {
		if (reply(0, gettxt(":282","file is not a block or character device; OK")) == 0)
			return (0);
	}
	if (mounted(devstr))
		if (rawflg)
			mountedfs++;
		else {
			myfprintf(stdout, MM_ERROR, ":283:%s is mounted, fsck on BLOCK device ignored\n",
				devstr);
			exit(33);
		}
	if ((dfile.rfdes = open(devstr, 0)) < 0) {
		myfprintf(stdout, MM_ERROR, ":284:Cannot open %s\n", devstr);
		return (0);
	}
	if (nflag || (dfile.wfdes = open(devstr, 1)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			mypfatal(":285:NO WRITE ACCESS\n");
		if (!bflg) {
			getsem();
			if (Pflag && Parent_notified == B_FALSE) {
				pfmt(print_fp, MM_NOSTD, ":353:** %s: NO WRITE\n",devstr);
               	 		fflush(print_fp);
			} else 
				pfmt(stdout, MM_NOSTD, ":353:** %s: NO WRITE\n", devstr);
			relsem();
		}
	} else if ((preen == 0) && !bflg) {
		getsem();
		if (Pflag && Parent_notified == B_FALSE) {
			pfmt(print_fp, MM_NOSTD|MM_NOGET, "** %s\n",devstr);
               		fflush(print_fp);
		} else 
			pfmt(stdout, MM_NOSTD|MM_NOGET, "** %s\n",devstr);
		relsem();
	}
	dfile.mod = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&fileblk);
	initbarea(&cgblk);
	initbarea(&asblk);
	
	/*
	 * Read in the super block and its summary info.
	 */
	if (bread(&dfile, (char *)&sblock, super, (long)SBSIZE) != 0)
		return (0);
	sblk.b_bno = super;
	sblk.b_size = SBSIZE;
	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock.fs_magic != UFS_MAGIC){
		badsb(gettxt(":286","MAGIC NUMBER WRONG"));
		return (0);
	}
	if (sblock.fs_ncg < 1){
		badsb(gettxt(":287","NCG OUT OF RANGE"));
		return (0);
	}
	if (sblock.fs_cpg < 1 || sblock.fs_cpg > MAXCPG){
		badsb(gettxt(":288","CPG OUT OF RANGE"));
		return (0);
	}
	if (sblock.fs_ncg * sblock.fs_cpg < sblock.fs_ncyl ||
	    (sblock.fs_ncg - 1) * sblock.fs_cpg >= sblock.fs_ncyl){
		badsb(gettxt(":289","NCYL DOES NOT JIVE WITH NCG*CPG"));
		return (0);
	}
	if (sblock.fs_sbsize > SBSIZE){
		badsb(gettxt(":290","SIZE PREPOSTEROUSLY LARGE"));
		return (0);
	}
	if (mflag)
		return (devstr);
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if (sblock.fs_optim != FS_OPTTIME && sblock.fs_optim != FS_OPTSPACE) {
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":291:UNDEFINED OPTIMIZATION IN SUPERBLOCK");
		if (reply(1, gettxt(":292","SET TO DEFAULT")) == 1) {
			sblock.fs_optim = FS_OPTTIME;
			sbdirty();
		}
		relsem();
	}
	if ((sblock.fs_minfree < 0 || sblock.fs_minfree > 99)) {
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":293:IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
			sblock.fs_minfree);
		if (reply(1, gettxt(":292","SET TO DEFAULT")) == 1) {
			sblock.fs_minfree = 10;
			sbdirty();
		}
		relsem();
	}
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block.
	 * When an alternate super-block is specified this check is skipped.
	 */
	if (bflag)
		goto sbok;
	getblk(&asblk, cgsblock(&sblock, sblock.fs_ncg - 1), sblock.fs_sbsize);
	if (asblk.b_errs != NULL)
		return (0);
	altsblock.fs_link = sblock.fs_link;
	altsblock.fs_rlink = sblock.fs_rlink;
	altsblock.fs_time = sblock.fs_time;
	altsblock.fs_cstotal = sblock.fs_cstotal;
	altsblock.fs_cgrotor = sblock.fs_cgrotor;
	altsblock.fs_fmod = sblock.fs_fmod;
	altsblock.fs_clean = sblock.fs_clean;
	altsblock.fs_ronly = sblock.fs_ronly;
	altsblock.fs_flags = sblock.fs_flags;
	altsblock.fs_maxcontig = sblock.fs_maxcontig;
	altsblock.fs_minfree = sblock.fs_minfree;
	altsblock.fs_optim = sblock.fs_optim;
	altsblock.fs_rotdelay = sblock.fs_rotdelay;
	altsblock.fs_maxbpg = sblock.fs_maxbpg;
	altsblock.fs_state = sblock.fs_state;
	memcpy((char *)altsblock.fs_csp, (char *)sblock.fs_csp,
		sizeof sblock.fs_csp);
	memcpy((char *)altsblock.fs_fsmnt, (char *)sblock.fs_fsmnt,
		sizeof sblock.fs_fsmnt);
	if (memcmp((char *)&sblock, (char *)&altsblock, (int)sblock.fs_sbsize)){
		badsb(gettxt(":294","TRASHED VALUES IN SUPER BLOCK"));
		return (0);
	}
sbok:
	fmax = sblock.fs_size;
	imax = sblock.fs_ncg * sblock.fs_ipg;
	/*
	 * read in the summary info.
	 */
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize;
		sblock.fs_csp[j] = (struct csum *)calloc(1, (unsigned)size);
		if (bread(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    size) != 0)
			return (0);
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsz = roundup(howmany(fmax, NBBY), sizeof(short));
	blockmap = calloc((unsigned)bmapsz, sizeof (char));
	if (blockmap == NULL) {
		myfprintf(stdout, MM_ERROR, ":295:Cannot alloc %d bytes for blockmap\n", bmapsz);
		goto badsb;
	}
	lncntp = (short *)calloc((unsigned)(imax + 1), sizeof(short));
	if (lncntp == NULL) {
		myfprintf(stdout, MM_ERROR, ":297:Cannot alloc %d bytes for lncntp\n", 
		    (imax + 1) * sizeof(short));
		goto badsb;
	}
	statemap.nstate = (struct stmap *)calloc((unsigned)(imax + 1), sizeof(struct stmap));
	if (statemap.nstate != NULL) {
		MEM = 1;
		/*
	 	* Initialize the inode buffer.
	 	* bb_strino set to -1 for each cylinder group in pass1().
	 	*/
		inobuf.bb_size = 65536;		/* 64k */
		inobuf.bb_iperb = inobuf.bb_size / sizeof (DINODE);
		inobuf.bb_dbpbuf = inobuf.bb_size / DEV_BSIZE; 
		inobuf.bb_blk = -1;
		inobuf.bb_dirty = (char *)malloc(dirtycnt);
		if (inobuf.bb_dirty == NULL)
			return;
		for (i = 0; i < dirtycnt; i++)
			*(inobuf.bb_dirty + i) = 0;
	} else {
		MEM = 0;
		statemap.ostate = calloc((unsigned)(imax + 1), sizeof(char));
		if (statemap.ostate == NULL)
			myfprintf(stdout, MM_ERROR, ":296:Cannot alloc %d bytes for statemap\n", imax + 1);
	}

	return (devstr);

badsb:
	ckfini();
	return (0);
}

badsb(s)
	char *s;
{
	WYPFLG(wflg, yflag, preen);
	if (preen)
		myprintf(":346:%s: ", devname);
	pfatal(":299:BAD SUPER BLOCK: %s\nUSE -b OPTION TO FSCK TO SPECIFY LOCATION OF AN ALTERNATE\nSUPER-BLOCK TO SUPPLY NEEDED INFORMATION; SEE fsck(1M).\n", s);
}
