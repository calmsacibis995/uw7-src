/* @(#)local/uw/gemini/crash/vxfs_inode.c	3.5 09/19/97 01:57:27 -  */
/* #ident "@(#)vxfs:local/uw/gemini/crash/vxfs_inode.c	3.5" */
#ident	"@(#)crash:common/cmd/crash/vxfs_inode.c	1.2.3.1"
/*
 * Copyright(C)1997 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */

#define	_FSKI 2			/* this one seems to get i_size right */
#define _FILE_OFFSET_BITS 64	/* this one seems to get off_t right */

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/acl.h>
#include <sys/privilege.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/list.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <sys/fs/vx_const.h>
#include <sys/fs/vx_gemini.h>
#include <sys/fs/vx_machdep.h>
#include <sys/fs/vx_layout.h>
#include <sys/fs/vx_mlink.h>
#include <sys/fs/dmapi.h>
#include <sys/fs/kdm_vnode.h>
#include <sys/fs/vx_inode.h>
#include <stdlib.h>
#include "crash.h"

#define ATTRFMT  0xf0000000		/* bits used for attr */

static vaddr_t	S_vx_vnodeops;
static vaddr_t	S_vx_attrvnodeops;
static vaddr_t	S_vxfs_icache;
static vaddr_t	S_vxfs_ncachelists;
static vaddr_t	S_vxfs_cur_inodes;
static vaddr_t	S_vxfs_inode_size;

struct vx_icache	*vxfs_icache;
struct vx_icache	*vxfs_icptrs;
struct vx_inode		vxfs_ibuf;
struct vnode		vxfs_vbuf;
int			vxfs_ncachelists;
long			vxfs_cur_inodes;
size_t			vxfs_inode_size;
int			full;
int			all;
int			list;
int			lfree;

void			list_vxfs_inodes();
void			print_vxfs_inodes();
struct vx_inode		*print_vxfs_inode(long);
int			get_vxfs_ipos(long);
void			vxflagprt();
int			vxfs_lock();

struct iflag	{
	char	*name;
	int	value;
};

/*
 * flags in i_flag
 */

struct iflag	iflags[] = {
	"IUPD", 		IUPD,
	"IACC", 		IACC,
	"IMOD", 		IMOD,
	"ICHG", 		ICHG,
	"IATIMEMOD", 		IATIMEMOD,
	"IMTIMEMOD", 		IMTIMEMOD,
	"ITRANLAZYMOD", 	ITRANLAZYMOD,
	"INOCLONE",		INOCLONE,
	"ISYNCWRITES", 		ISYNCWRITES,
	"IINTRANS",		IINTRANS,
	"IDELBUF", 		IDELBUF,
	"IGHOST",		IGHOST,
	"ISHRDATA",		ISHRDATA,
	"INOTIMEMOD",		INOTIMEMOD,
	"ILOGWRITELOCK", 	ILOGWRITELOCK,
	"IBADHOLD",		IBADHOLD,
	0,			0
};

/*
 * flags in i_intrflag
 */

struct iflag intrflags[] = {
	"IDELXWRI", 		IDELXWRI,
	"ILOGWRITE", 		ILOGWRITE,
	"IDELAYEDERR", 		IDELAYEDERR,
	"IASYNCUPD", 		IASYNCUPD,
	0,			0
};

/*
 * flags in i_sflags
 */

struct iflag sflags[] = {
	"IADDRVALID", 		IADDRVALID,
	"IBAD", 		IBAD,
	"IUEREAD", 		IUEREAD,
	"INOBMAPCACHE", 	INOBMAPCACHE,
	"IBADUPD", 		IBADUPD,
	"IPUTTIME", 		IPUTTIME,
	"IATTRREM", 		IATTRREM,
	"INOBMAPCLUSTER", 	INOBMAPCLUSTER,
	"IDEVACC",		IDEVACC,
	"IDEVUPDCHG",		IDEVUPDCHG,
	"IXWRITE",		IXWRITE,
	"IWRITEMAP",		IWRITEMAP,
	"IDEVICE",		IDEVICE,
	"ILASTPAGEZEROED",	ILASTPAGEZEROED,
	0,			0
};

static char	*heading1 = "  MAJ/MIN     INUMB  RCNT  LINK    UID    GID \
    SIZE    TYPE       MODE\n";
static char	*heading2 = "ACLS DACL  ACLENTRIES\n";

/*
 * Get arguments for vxfs inode.
 */

int
get_vxfs_inode()
{
	struct vx_icache	*icp;
	long			arg1;
	long			arg2;
	int			c;

	S_vx_vnodeops = symfindval("vx_vnodeops");
	S_vx_attrvnodeops = symfindval("vx_attrvnodeops");
	S_vxfs_ncachelists = symfindval("vx_ncachelists");
	S_vxfs_icache = symfindval("vx_icache");
	S_vxfs_cur_inodes = symfindval("vx_cur_inodes");
	S_vxfs_inode_size = symfindval("vxfs_inode_size");
	optind = 1;
	full = all = list = lfree = 0;
	while((c = getopt(argcnt, args, "eflrw:")) != EOF) {
		switch(c) {
		case 'e':	all = 1;
				break;

		case 'f':	full = 1;
				break;

		case 'l':	list = 1;
				break;

		case 'r':	lfree = 1;
				break;

		case 'w':	redirect();
				break;

		default:	longjmp(syn, 0);
		}
	}
	readmem(S_vxfs_ncachelists, 1, -1, &vxfs_ncachelists,
		sizeof (vxfs_ncachelists), "number of vxfs inode caches");
	readmem(S_vxfs_icache, 1, -1, &vxfs_icache,
		sizeof (vxfs_icache), "vxfs inode table pointer");
	readmem(S_vxfs_cur_inodes, 1, -1, &vxfs_cur_inodes,
		sizeof (vxfs_cur_inodes), "number of vxfs inodes");
	readmem(S_vxfs_inode_size, 1, -1, &vxfs_inode_size,
		sizeof (vxfs_inode_size), "size of a vxfs inode");
	vxfs_icptrs = readmem((vaddr_t)vxfs_icache, 1, -1, NULL,
                sizeof (struct vx_icache) * vxfs_ncachelists,
		"vxfs inode table");
	if (list) {
		list_vxfs_inodes();
	} else {
		fprintf(fp, "INODE TABLE SIZE = %d\n", vxfs_cur_inodes);
		fprintf(fp, "INODE SIZE = %d\n", vxfs_inode_size);
		if (!full) {
			fprintf(fp, "%s", heading1);
		}
		if (lfree) {
			all++;
			print_vxfs_inodes();
		} else if (args[optind]) {
			all++;
			do {
				(void)getargs(vxfs_cur_inodes, &arg1, &arg2);
				if (get_vxfs_ipos(arg1)) {
					(void)print_vxfs_inode(arg1);
				}
			} while(args[++optind]);
		} else {
			print_vxfs_inodes();
		}
	}
	return;
}

void
list_vxfs_inodes()
{
	struct vx_icache	*icp, *endicp;
	struct vx_inode		*addr;
	int			slot = 0;
	int			list = 1;

	fprintf(fp, "\nThe following are the existing vxfs inodes:\n");
	for (icp = vxfs_icptrs, endicp = vxfs_icache;
	     icp < vxfs_icptrs + vxfs_ncachelists; icp++, endicp++) {
		fprintf(fp, "\nInode Cache List %3d", list++);
		slot = 0;
		addr = icp->i_cforw;
		while (addr != (struct vx_inode *)endicp) {
			readmem((vaddr_t)addr, 1, -1, &vxfs_ibuf,
				sizeof (struct vx_inode), "vxfs inode");
			if (!(slot % 3)) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%8x %s %s ", addr,
				vxfs_ibuf.av_forw != NULL ? "free" : "    ",
				(vxfs_ibuf.i_mode & IFMT) == IFATT ?
					"attr" : "    ");
			addr = vxfs_ibuf.i_cforw;
			slot++;
		}
	}
	fprintf(fp, "\n");
	return;
}

/*
 * Print vxfs inode table.
 */

void
print_vxfs_inodes()
{
	struct vx_icache	*icp, *endicp;
	struct vx_inode		*addr;

	for (icp = vxfs_icptrs, endicp = vxfs_icache;
	     icp < vxfs_icptrs + vxfs_ncachelists; icp++, endicp++) {
		addr = icp->i_cforw;
		while (addr != (struct vx_inode *)endicp) {
			if ((addr = print_vxfs_inode((long)addr)) == NULL) {
				break;
			}
		}
	}
	return;
}

struct vx_inode *
print_vxfs_inode(addr)
	long			addr;
{
	char			extbuf[50];
	char			ch;
	char			typechar;
	int			i;
	int			attr_type;
	int			defflag = 0;	/* default ACLs not found yet */
	struct acl		*aclp;
	struct acl		*acl_end;
	struct acl		*acl_start = NULL;
	char			*aclbuf = NULL;
	struct vx_iattr		*ap;
	struct vx_attr_immed	*iap;
	int			aoff;
	struct vx_aclhd		*vxahd;
	int			nacl;

	readmem((vaddr_t)addr, 1, -1, &vxfs_ibuf, sizeof (struct vx_inode),
		"vxfs inode");
	readmem((vaddr_t)vxfs_ibuf.i_vnode, 1, -1, &vxfs_vbuf,
		sizeof (struct vnode), "vxfs vnode");
	if ((long)vxfs_vbuf.v_op != S_vx_vnodeops &&
	    (long)vxfs_vbuf.v_op != S_vx_attrvnodeops) {

		/*
		 * Not a vxfs inode.
		 */

		fprintf(fp, "%8x is not a vxfs inode\n", addr);
		return NULL;
	}
	if (!vxfs_vbuf.v_count && !all) {
		return NULL;
	}
	if (lfree && !vxfs_ibuf.av_forw) {
		return vxfs_ibuf.i_cforw;
	}
	if (full) {
		fprintf(fp, "INODE :\n%s", heading1);
	}
	fprintf(fp, "%4u, %-5u %7u   %3d %5d% 7d%7d %8lld",
		getemajor(vxfs_ibuf.i_dev),
		geteminor(vxfs_ibuf.i_dev),
		vxfs_ibuf.i_number,
		vxfs_vbuf.v_count,
		vxfs_ibuf.i_nlink,
		vxfs_ibuf.i_uid,
		vxfs_ibuf.i_gid,
		vxfs_ibuf.i_size);
	attr_type = vxfs_ibuf.i_mode & ATTRFMT;
	if (attr_type) {

		/*
		 * For attribute inodes we dispense with the
		 * keyletter presentation of the file type.
		 */

		switch (attr_type) {
		case IFFSH:
			fprintf(fp, "   %s", "IFFSH");
			break;

		case IFILT:
			fprintf(fp, "   %s", "IFILT");
			break;

		case IFIAU:
			fprintf(fp, "   %s", "IFIAU");
			break;

		case IFCUT:
			fprintf(fp, "   %s", "IFCUT");
			break;

		case IFATT:
			fprintf(fp, "   %s", "IFATT");
			break;

		case IFLCT:
			fprintf(fp, "   %s", "IFLCT");
			break;

		case IFIAT:
			fprintf(fp, "   %s", "IFIAT");
			break;

		case IFEMR:
			fprintf(fp, "   %s", "IFEMR");
			break;

		default:
			fprintf(fp, "       ");
			break;
		}
	} else {
		switch (vxfs_vbuf.v_type) {
		case VDIR:
			ch = 'd';
			break;

		case VCHR:
			ch = 'c';
			break;

		case VBLK:
			ch = 'b';
			break;

		case VREG:
			ch = 'f';
			break;

		case VLNK:
			ch = 'l';
			break;

		case VFIFO:
			ch = 'p';
			break;

		case VXNAM:
			ch = 'x';
			break;

		default:
			ch = '-';
			break;
		}
		fprintf(fp, "    %c", ch);
		fprintf(fp, "%s%s%s",
			vxfs_ibuf.i_mode & ISUID ? "u" : "-",
			vxfs_ibuf.i_mode & ISGID ? "g" : "-",
			vxfs_ibuf.i_mode & ISVTX ? "v" : "-");
	}
	fprintf(fp, "  %s%s%s%s%s%s%s%s%s",
		vxfs_ibuf.i_mode & IREAD	 ? "r" : "-",
		vxfs_ibuf.i_mode & IWRITE	 ? "w" : "-",
		vxfs_ibuf.i_mode & IEXEC	 ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  3) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 3) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  3) ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  6) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 6) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  6) ? "x" : "-");
	fprintf(fp, "\n");
	if (!full) {
		return vxfs_ibuf.i_cforw;
	}

	/*
	 * ACL stuff borrowed from sfs.  Incomplete in that
	 * we can only deal with attributes in the inode.
	 */

	if (!attr_type && vxfs_ibuf.i_aclcnt != -1) {
		fprintf(fp, "%s", heading2);
		fprintf(fp, "%4d %4d     ",
			vxfs_ibuf.i_aclcnt,
			vxfs_ibuf.i_daclcnt);

		/*
		 * Print USER_OBJ ACL entry from
		 * permission bits.
		 */

		fprintf(fp, " u::%c%c%c\n",
			vxfs_ibuf.i_mode & IREAD ? 'r' : '-',
			vxfs_ibuf.i_mode & IWRITE ? 'w' : '-',
			vxfs_ibuf.i_mode & IEXEC ? 'x' : '-');
		if (vxfs_ibuf.i_aclcnt == vxfs_ibuf.i_daclcnt) {

			/*
			 * No non-default ACL entries.
			 * Print GROUP_OBJ entry from
			 * permission bits.
			 */

			fprintf(fp, "%14s g::%c%c%c\n", "",
				vxfs_ibuf.i_mode & (IREAD >> 3) ? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 3) ? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 3) ? 'x' : '-');
		}
		if (vxfs_ibuf.i_aclcnt == 0 || vxfs_ibuf.i_attr == NULL) {
			goto done_acl;
		}

		/*
		 * Paw through the immediate attribute area
		 * looking for any svr4 acls.
		 */

		aclbuf = readmem((vaddr_t)vxfs_ibuf.i_attr, 1, -1, NULL,
				 vxfs_ibuf.i_attrlen, "vxfs acls");
		ap = (struct vx_iattr *)aclbuf;
		aoff = 0;
		while (aoff < vxfs_ibuf.i_attrlen && ap->a_format) {
			switch (ap->a_format) {
			case VX_ATTR_IMMED:
				iap = (struct vx_attr_immed *)ap->a_data;
				if (iap->aim_class == VX_ACL_CLASS &&
				    iap->aim_subclass == VX_ACL_SVR4_SUBCLASS) {
					acl_start = (struct acl *)iap->aim_data;
					acl_end =
					    (struct acl *)((long)acl_start +
						ap->a_length -
						VX_ATTROVERHEAD -
						VX_ATTR_IMMEDOVER);
				}
				break;

			case VX_ATTR_DIRECT:
				break;
			}
			aoff += (ap->a_length + 0x3) & ~0x3;
			ap = (struct vx_iattr *)((int)aclbuf + aoff);
		}
		if (acl_start == NULL) {
			goto done_acl;
		}
		vxahd = (struct vx_aclhd *)acl_start;
		nacl = vxahd->aclcnt;
		aclp = (struct acl *)((int)acl_start + sizeof(struct vx_aclhd));
		for (i = 0; aclp < acl_end && i < nacl; i++) {
			if (aclp->a_type & ACL_DEFAULT) {
				if (defflag == 0) {

					/*
					 * 1st default ACL entry.  Print
					 * CLASS_OBJ & OTHER_OBJ entries
					 * from permission bits before
					 * default entry.
					 */

					fprintf(fp, "%14s c:%c%c%c\n", "",
					    vxfs_ibuf.i_mode & (IREAD >> 3)
						? 'r' : '-',
					    vxfs_ibuf.i_mode & (IWRITE >> 3)
						? 'w' : '-',
					    vxfs_ibuf.i_mode & (IEXEC >> 3)
						? 'x' : '-');
					fprintf(fp, "%14s o:%c%c%c\n", "",
					    vxfs_ibuf.i_mode & (IREAD >> 6)
						? 'r' : '-',
					    vxfs_ibuf.i_mode & (IWRITE >> 6)
						? 'w' : '-',
					    vxfs_ibuf.i_mode & (IEXEC >> 6)
						? 'x' : '-');
					defflag++;
				}
			}
			fprintf(fp, "%14s %s", "",
				aclp->a_type & ACL_DEFAULT ? "d:" : "");
			switch (aclp->a_type & ~ACL_DEFAULT) {
			case GROUP:
			case GROUP_OBJ:
				typechar = 'g';
				break;

			case USER:
			case USER_OBJ:
				typechar = 'u';
				break;

			case CLASS_OBJ:
				typechar = 'c';
				break;

			case OTHER_OBJ:
				typechar = 'o';
				break;

			default:
				typechar = '?';
				break;
			}
			if ((aclp->a_type & GROUP) || (aclp->a_type & USER)) {
				fprintf(fp, "%c:%d:%c%c%c\n",
				    typechar,
				    aclp->a_id,
				    aclp->a_perm & (IREAD >> 6) ? 'r' : '-',
				    aclp->a_perm & (IWRITE >> 6) ? 'w' : '-',
				    aclp->a_perm & (IEXEC >> 6) ? 'x' : '-');
			} else if ((aclp->a_type & USER_OBJ) ||
				   (aclp->a_type & GROUP_OBJ)) {
				fprintf(fp, "%c::%c%c%c\n",
				    typechar,
				    aclp->a_perm & (IREAD >> 6) ? 'r' : '-',
				    aclp->a_perm & (IWRITE >> 6) ? 'w' : '-',
				    aclp->a_perm & (IEXEC >> 6) ? 'x' : '-');
			} else {
				fprintf(fp, "%c:%c%c%c\n",
				    typechar,
				    aclp->a_perm & (IREAD >> 6) ? 'r' : '-',
				    aclp->a_perm & (IWRITE >> 6) ? 'w' : '-',
				    aclp->a_perm & (IEXEC >> 6) ? 'x' : '-');
			}
		}

done_acl:
		cr_free(aclbuf);
		aclbuf = NULL;
		if (defflag == 0) {

			/*
			 * No default ACL entries.  Print CLASS_OBJ &
			 * OTHER_OBJ entries from permission bits now.
			 */

			fprintf(fp, "%14s c:%c%c%c\n", "",
				vxfs_ibuf.i_mode & (IREAD >> 3) ? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 3) ? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 3) ? 'x' : '-');
			fprintf(fp, "%14s o:%c%c%c\n", "",
				vxfs_ibuf.i_mode & (IREAD >> 6) ? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 6) ? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 6) ? 'x' : '-');
		}
	}
	vxflagprt(&vxfs_ibuf);
	fprintf(fp, "\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp, "\t%8x" , vxfs_ibuf.i_cforw);
	fprintf(fp, "\t%8x", vxfs_ibuf.i_cback);
	fprintf(fp, "\t%8x", vxfs_ibuf.av_forw);
	fprintf(fp, "\t%8x\n", vxfs_ibuf.av_back);
	if (vxfs_ibuf.i_orgtype == IORG_IMMED) {
		fprintf(fp, "\n    (immediate)\n");
	} else if (vxfs_ibuf.i_orgtype == IORG_EXT4) {
		for (i = 0; i < VX_NDADDR; i++) {
			if (!(i % 3)) {
				fprintf(fp, "\n    ");
			} else {
				fprintf(fp, "  ");
			}
			sprintf(extbuf, hexmode? "[%x, %x]": "[%d, %d]",
				vxfs_ibuf.i_dext[i].ic_de,
				vxfs_ibuf.i_dext[i].ic_des);
			fprintf(fp, "e%d: %-19s", i, extbuf);
		}
		fprintf(fp, "\n    ie0: %-8d      ie1: %-8d      ies: %-8d\n",
			vxfs_ibuf.i_ie[0], vxfs_ibuf.i_ie[1],
			vxfs_ibuf.i_ies);
	}

	/*
	 * Print vnode info.
	 */

	fprintf(fp, "VNODE :\n");
	fprintf(fp, vnheading);
	cprvnode(&vxfs_vbuf);
	fprintf(fp, "\n");
	return vxfs_ibuf.i_cforw;
}

int
get_vxfs_ipos(addr)
	long			addr;
{
	struct vx_icache	*icp, *endicp;
	struct vx_inode		*ip;

	for (icp = vxfs_icptrs, endicp = vxfs_icache;
	     icp < vxfs_icptrs + vxfs_ncachelists; icp++, endicp++) {
		ip = icp->i_cforw;
		while (ip != (struct vx_inode *)endicp) {
			readmem((vaddr_t)ip, 1, -1, &vxfs_ibuf,
				sizeof (struct vx_inode), "vxfs inode");
			if (ip == (struct vx_inode *)addr) {
				return 1;
			}
			ip = vxfs_ibuf.i_cforw;
		}
	}
	return 0;
}

void
vxflagprt(ip)
	struct vx_inode	*ip;
{
	int		i;

	fprintf(fp, "FLAGS:");
	for (i = 0; iflags[i].value; i++) {
		if (ip->i_flag & iflags[i].value) {
			fprintf(fp, " %s", iflags[i].name);
		}
	}
	for (i = 0; intrflags[i].value; i++) {
		if (ip->i_intrflag & intrflags[i].value) {
			fprintf(fp, " %s", intrflags[i].name);
		}
	}
	for (i = 0; sflags[i].value; i++) {
		if (ip->i_sflag & sflags[i].value) {
			fprintf(fp, " %s", sflags[i].name);
		}
	}
	fprintf(fp, "\n");
	return;
}

int
vxfs_lck()
{
	struct vx_icache	*icp, *endicp;
	struct vx_inode		*addr;
	int			active = 0;
	extern			print_lock();

	S_vx_vnodeops = symfindval("vx_vnodeops");
	S_vxfs_ncachelists = symfindval("vx_ncachelists");
	S_vxfs_icache = symfindval("vxfs_icachep");
	readmem(S_vxfs_ncachelists, 1, -1, &vxfs_ncachelists,
		sizeof (vxfs_ncachelists), "number of vxfs inode caches");
	readmem(S_vxfs_icache, 1, -1, &vxfs_icache,
		sizeof (vxfs_icache), "vxfs inode table head");
	vxfs_icptrs = readmem((vaddr_t)vxfs_icache, 1, -1, NULL,
			      sizeof (struct vx_icache) * vxfs_ncachelists,
			      "vxfs inode table");
	for (icp = vxfs_icptrs, endicp = vxfs_icache;
	     icp < vxfs_icptrs + vxfs_ncachelists; icp++, endicp++) {
		addr = icp->i_cforw;
		while (addr != (struct vx_inode *)endicp) {
			readmem((vaddr_t)addr, 1, -1, &vxfs_ibuf,
				sizeof (struct vx_inode), "vxfs inode");
			readmem((vaddr_t)vxfs_ibuf.i_vnode, 1, -1, &vxfs_vbuf,
				sizeof (struct vnode), "vxfs vnode");
			if ((long)vxfs_vbuf.v_op != S_vx_vnodeops) {

				/*
				 * Not a vxfs inode - or at least not an inode
				 * that could have locks.
				 */

				continue;
			}
			if (vxfs_ibuf.i_mode == 0 || !vxfs_vbuf.v_count ||
			    vxfs_ibuf.av_forw != NULL) {
				continue;
			}
			if (vxfs_vbuf.v_filocks) {
				active += print_lock(vxfs_vbuf.v_filocks,
						     (long)addr, "vxfs");
			}
		}
	}
	cr_free(vxfs_icptrs);
	return active;
}

#undef _KERNEL
