#ident	"@(#)crash:common/cmd/crash/sfs_inode.c	1.8.1.2"

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/privilege.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <stdlib.h>

#include "crash.h"

#define AREAD	0x4				/* read permission */
#define AWRITE	0x2				/* write permission */
#define AEXEC	0x1				/* exec permission */

#define INOHSZ  512
#define Sec	sec.is_secdata

static vaddr_t S_sfs_vnodeops, S_ufs_vnodeops, S_sfs_inode_cnt;
static vaddr_t S_totally_free, S_partially_free, S_sfs_ihead;

struct inode		sfs_ibuf;		/* buffer for SFS inode */
struct vnode		vnode;
struct inode_marker	totally_free;
struct inode_marker	partially_free;
struct inode		*sfs_pfreechain;
struct inode		*sfs_tfreechain;
union ihead		sfs_ihead[INOHSZ];
ulong_t			sfs_ninode, ninode;	/* size of inode table */
long			iptr;

struct listbuf {
	long	addr;
	char	state;
};


/*
 * Get arguments for SFS inode.
 */
int
get_sfs_inode ()
{
	int	slot;
	int	full = 0;
	int	list = 0;
	int	all = 0;
	long	addr;
	long	arg1;
	long	arg2;
	int	lfree = 0;
	long	next;
	int	c;
	int	i,j;
	struct inode *freelist, *ip, *ipx=NULL;
	union ihead	*ih;
	struct listbuf	*listptr;
	struct listbuf	*listbuf;

	char	*heading1 = 
	    "SLOT  MAJ/MIN    INUMB  RCNT  LINK    UID    GID   TYPE \n";
	char	*heading2 = 
	    " ACLS DACL  ABLK ACLENTRIES LID SFLG FLGS SIZE\n"; 

	S_sfs_vnodeops = symfindval("sfs_vnodeops");
	S_ufs_vnodeops = symfindval("ufs_vnodeops");
	S_sfs_inode_cnt = symfindval("sfs_ninode");
	S_partially_free = symfindval("sfs_partially_free");
	S_totally_free = symfindval("sfs_totally_free");
	S_sfs_ihead = symfindval("sfs_ihead");

	optind = 1;
	while((c = getopt(argcnt, args, "efrlw:")) !=EOF) {
		switch(c) {

		case 'e':	all = 1;
				break;

		case 'f':	full =1;
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

	readmem(S_partially_free,1,-1,&partially_free,
		 sizeof(partially_free), "SFS/UFS free inode table");

	readmem(S_totally_free,1,-1,&totally_free,
		 sizeof(totally_free), "SFS/UFS free inode table");

	readmem(S_sfs_ihead,1,-1,&sfs_ihead,
		 sizeof(sfs_ihead), "SFS/UFS inode table head");

	readmem(S_sfs_inode_cnt,1,-1,&sfs_ninode,
		 sizeof(sfs_ninode), "SFS/UFS inode count");

	listbuf = listptr =
	    cr_malloc(sizeof(struct listbuf)*sfs_ninode, "SFS/UFS inode list");

	ninode = 2;
	listptr = &listptr[2];
	/* Get the inode addresses so that they could be referenced directly */
	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++){
		addr = (long)ih->ih_chain[0];
		ipx = (inode_t *)ih->ih_chain[0];
		for (ip = (inode_t *)addr; ; ip = (inode_t *)addr) {
			readmem((long)addr,1,-1,(char *)&sfs_ibuf,
			 	sizeof sfs_ibuf, "SFS/UFS inode");
			if (sfs_ibuf.i_forw == ipx)
				break;
			listptr->addr = addr;
			listptr->state = 'n';		/* unknown state */
			ninode++;
			listptr++;

			/* Structure copy */
			vnode = sfs_ibuf.i_vnode;
			if ((long)vnode.v_op != S_sfs_vnodeops &&
			    (long)vnode.v_op != S_ufs_vnodeops) {
				listptr->state = 'x'; /* not sfs/ufs */
				addr = (long)sfs_ibuf.i_forw;
				continue;
			}
			if (vnode.v_count != 0)
				listptr->state = 'u';		/* in use */
			addr = (long)sfs_ibuf.i_forw;
		}
	}
	ip = totally_free.im_chain[2];
	while (ip != (struct inode *)S_totally_free) {	
		readmem((long)ip,1,-1,(char *)&sfs_ibuf,
                               sizeof sfs_ibuf, "SFS/UFS inode");
		ninode++;
		listptr->addr = (long)ip;
		/* Structure copy */
		vnode = sfs_ibuf.i_vnode;
		if ((long)vnode.v_op != S_sfs_vnodeops &&
		    (long)vnode.v_op != S_ufs_vnodeops) {
			listptr->state = 'x'; /* not sfs/ufs */
			listptr++;
			ip = sfs_ibuf.i_freef;
			continue;
		} else if (vnode.v_count == 0)
			listptr->state = 'f';
		else 
			listptr->state = 'b';
       		ip = sfs_ibuf.i_freef;
		listptr++;
       	}
	ip = partially_free.im_chain[2];
	while (ip != (struct inode *)S_partially_free) {	
		readmem((long)ip,1,-1,(char *)&sfs_ibuf,
                               sizeof sfs_ibuf, "SFS/UFS inode");
		ninode++;
		listptr->addr = (long)ip;
		/* Structure copy */
		vnode = sfs_ibuf.i_vnode;
		if ((long)vnode.v_op != S_sfs_vnodeops &&
	    	    (long)vnode.v_op != S_ufs_vnodeops) {
			listptr->state = 'x'; /* not sfs/ufs */
			listptr++;
			ip = sfs_ibuf.i_freef;
			continue;
		} else if (vnode.v_count == 0)
			listptr->state = 'f';
		else 
			listptr->state = 'b';
		ip = sfs_ibuf.i_freef;
		listptr++;
	}
done:
	if(list)
		list_sfs_inode (listbuf);
	else {
		fprintf(fp, "INODE TABLE SIZE = %d\n", sfs_ninode);
		if(!full)
			(void) fprintf(fp, "%s", heading1);
		if(lfree) {

			ip = totally_free.im_chain[2];
			while (ip != (struct inode *)S_totally_free) {
				print_sfs_inode(1,full,-1,ip,heading1,heading2);
				ip = sfs_ibuf.i_freef;
        		}
			ip = partially_free.im_chain[2];
			while (ip != (struct inode *)S_partially_free) {
				print_sfs_inode(1,full,-1,ip,heading1,heading2);
                		ip = sfs_ibuf.i_freef;
        		}

		} else if(args[optind]) {
			do {
				if (getargs(ninode,&arg1,&arg2)) {
				    for(slot = arg1; slot <= arg2; slot++) {
					addr = listbuf[slot].addr;
					print_sfs_inode(1,full,slot,addr,
						heading1, heading2);
				    }
				} else {
					addr = arg1;
					slot = getsfs_ipos(addr,listbuf,ninode);
					print_sfs_inode(1,full,slot,addr,
						heading1, heading2);
				}
			}while(args[++optind]);
		} else {
			for(slot = 2; slot < ninode; slot++) {
				addr = listbuf[slot].addr;
				print_sfs_inode(all,full,slot,addr,
					heading1, heading2);
			}
		}
	}
}

list_sfs_inode(listbuf)
struct listbuf *listbuf;
{
	int		k, i, j;
	long		next;
	struct inode 	*freelist;

	if (listbuf == NULL)
		return;

	(void) fprintf(fp, "The following SFS inodes are in use:\n");
	for(i = 0, j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are on the freelist:\n");
	for(i = 0, j=0; i < ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0, j=0; i < ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}

	fprintf(fp, "\n\nThe following SFS inodes are in unknown states:\n");
	for(i = 0, j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'n') {
			if(j && (j % 10) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "%3d    ", i);
			j++;
		}
	}
	fprintf(fp, "\n");
}

/*
 * Print SFS inode table.
 */

int
print_sfs_inode (all, full, slot, addr, heading1, heading2)
	int	all, full, slot;
	long	addr;
	char	*heading1;
	char	*heading2;
{
	char		flags[22];
	char		prflag[3] = "  ";
	char		ch;
	char		typechar;
	int		i;
	int		defflag = 0;	/* default ACLs not found yet */
	union		i_secure sec;
	struct vfs	vfst;
	char 		*fsname;

	if (addr == -1)
		return;

	readmem(addr, 1, -1, (char *)&sfs_ibuf,sizeof sfs_ibuf,"SFS/UFS inode");
	/* Structure copy */
	vnode = sfs_ibuf.i_vnode;

	if( (long)vnode.v_op != S_sfs_vnodeops &&
		(long)vnode.v_op != S_ufs_vnodeops) {
		return;	/* not ufs/sfs */
	}

	if(!vnode.v_count && !all)
		return;
	if(full)
		fprintf(fp, "%s", heading1);

	if(slot == -1)
		fprintf(fp, "  - ");
	else
		fprintf(fp, "%4d", slot);

	fprintf(fp, " %4u,%-4u%7u   %3d %4u %7d %7d",
		getemajor(sfs_ibuf.i_dev),
		geteminor(sfs_ibuf.i_dev),
		sfs_ibuf.i_number,
		vnode.v_count,
		sfs_ibuf.i_nlink,
		sfs_ibuf.i_uid,
		sfs_ibuf.i_gid);
	switch(vnode.v_type) {
		case VDIR: ch = 'd'; break;
		case VCHR: ch = 'c'; break;
		case VBLK: ch = 'b'; break;
		case VREG: ch = 'f'; break;
		case VLNK: ch = 'l'; break;
		case VFIFO: ch = 'p'; break;
		default:    ch = '-'; break;
	}
	fprintf(fp, "   %c", ch);
	fprintf(fp, "%s%s%s\n",
		sfs_ibuf.i_mode & ISUID ? "u" : "-",
		sfs_ibuf.i_mode & ISGID ? "g" : "-",
		sfs_ibuf.i_mode & ISVTX ? "v" : "-");

	if (!full)
		return;

	addr = (long)sfs_ibuf.i_vnode.v_vfsp;
	readmem((long)addr,-1,1,(char *)&vfst, sizeof (struct vfs), "VFS");
	fsname = getfsname(vfst.vfs_fstype);

	if (fsname && strcmp(fsname, "sfs") == 0) {
		fprintf(fp, "%s", heading2);
		fprintf(fp, " %4d %4d %5x",
			Sec.isd_aclcnt, Sec.isd_daclcnt, Sec.isd_aclblk);

		/*
		 * print USER_OBJ ACL entry from permission bits.
		 */
		fprintf(fp, " u::%c%c%c   ", 
			(sfs_ibuf.i_mode >> 6) & AREAD ? 'r' : '-',
			(sfs_ibuf.i_mode >> 6) & AWRITE ? 'w' : '-',
			(sfs_ibuf.i_mode >> 6) & AEXEC ? 'x' : '-');
	
		fprintf(fp, "%5d %4x", Sec.isd_lid, Sec.isd_sflags);
	
		fprintf(fp, "%s%s%s%s%s%s%s%s\n",
			sfs_ibuf.i_flag & IUPD ? " up" : "",
			sfs_ibuf.i_flag & IACC ? " ac" : "",
			sfs_ibuf.i_flag & ICHG ? " ch" : "",
			sfs_ibuf.i_flag & ISYNC ? " sy" : "",
			sfs_ibuf.i_flag & ITFREE ? " fr" : "",
			sfs_ibuf.i_flag & INOACC ? " na" : "",
			sfs_ibuf.i_flag & IMODTIME ? " mt" : "",
			sfs_ibuf.i_flag & IMOD ? " md" : "");
		fprintf(fp, "%ld\n",
			sfs_ibuf.i_size);
	
		if ((Sec.isd_aclcnt == 0) || 
			(Sec.isd_aclcnt == Sec.isd_daclcnt)) {
			/*
			 * No non-default ACL entries.  Print GROUP_OBJ entry
			 * from permission bits.
			 */
			fprintf(fp, "%69s g::%c%c%c\n", "", 
				(sfs_ibuf.i_mode >> 3) & AREAD ? 'r' : '-',
				(sfs_ibuf.i_mode >> 3) & AWRITE ? 'w' : '-',
				(sfs_ibuf.i_mode >> 3) & AEXEC ? 'x' : '-');
		} 
		if (Sec.isd_aclcnt > 0) {
			for (i = 0; (i < Sec.isd_aclcnt) && (i < NACLI); i++) {
				if (Sec.isd_acl[i].a_type & ACL_DEFAULT) {
					if (defflag == 0) {
						/*
						 * 1st default ACL entry.  Print
						 * CLASS_OBJ & OTHER_OBJ entries 
						 * from permission bits before 
						 * default entry.
						 */
						fprintf(fp, "%69s c:%c%c%c\n", "", 
						(sfs_ibuf.i_mode >> 3) & AREAD 
							? 'r' : '-',
						(sfs_ibuf.i_mode >> 3) & AWRITE 
							? 'w' : '-',
						(sfs_ibuf.i_mode >> 3) & AEXEC 
							? 'x' : '-');
						fprintf(fp, "%69s o:%c%c%c\n", "", 
						sfs_ibuf.i_mode & AREAD ? 'r' : '-',
						sfs_ibuf.i_mode & AWRITE ? 'w' : '-',
						sfs_ibuf.i_mode & AEXEC ? 'x' : '-');
						defflag++;
					}
				}
				/* print each ACL entry stored in inode */
				fprintf(fp, "%69s %s", "",
				Sec.isd_acl[i].a_type & ACL_DEFAULT ? "d:" : "");
				switch (Sec.isd_acl[i].a_type & ~ACL_DEFAULT) {
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
				}	/* end switch */
				if ((Sec.isd_acl[i].a_type & GROUP) ||
				    (Sec.isd_acl[i].a_type & USER)) 
					fprintf(fp, "%c:%d:%c%c%c\n",
					typechar,
					Sec.isd_acl[i].a_id,
					Sec.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					Sec.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					Sec.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
				else if ((Sec.isd_acl[i].a_type & USER_OBJ) ||
					(Sec.isd_acl[i].a_type & GROUP_OBJ))
					fprintf(fp, "%c::%c%c%c\n", 
					typechar,
					Sec.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					Sec.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					Sec.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
				else
					fprintf(fp, "%c:%c%c%c\n", 
					typechar,
					Sec.isd_acl[i].a_perm & AREAD ? 'r' : '-',
					Sec.isd_acl[i].a_perm & AWRITE ? 'w' : '-',
					Sec.isd_acl[i].a_perm & AEXEC ? 'x' : '-');
			}	/* end for */
		}	/* end if */
		if (defflag == 0) {
			/*
			 * No default ACL entries.  Print CLASS_OBJ & 
			 * OTHER_OBJ entries from permission bits now.
			 */
			fprintf(fp, "%69s c:%c%c%c\n", "", 
				(sfs_ibuf.i_mode >> 3) & AREAD ? 'r' : '-',
				(sfs_ibuf.i_mode >> 3) & AWRITE ? 'w' : '-',
				(sfs_ibuf.i_mode >> 3) & AEXEC ? 'x' : '-');
			fprintf(fp, "%69s o:%c%c%c\n", "", 
				sfs_ibuf.i_mode & AREAD ? 'r' : '-',
				sfs_ibuf.i_mode & AWRITE ? 'w' : '-',
				sfs_ibuf.i_mode & AEXEC ? 'x' : '-');
		}
	}


	fprintf(fp,"\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp,"\t%8x",sfs_ibuf.i_forw);
	fprintf(fp,"\t%8x",sfs_ibuf.i_back);
	fprintf(fp,"\t%8x",sfs_ibuf.i_freef);
	fprintf(fp,"\t%8x\n",sfs_ibuf.i_freeb);

	fprintf(fp,"\t   COUNT    NEXTR   RWLOCK    SWAPCNT   DIROFF    FTIME \n");
	/*fprintf(fp, "\t%8d", sfs_ibuf.i_opencnt);*/
	fprintf(fp, "\t%8d", 0);

	if (sfs_ibuf.i_rwlock.rw_mode & RWS_WRITE) {
		fprintf(fp, " %8ld WRITER      %8ld %8ld %8ld\n",
			sfs_ibuf.i_nextr,
			sfs_ibuf.i_swapcnt,
			sfs_ibuf.i_diroff,
			sfs_ibuf.i_ftime);
	} else if (sfs_ibuf.i_rwlock.rw_mode & RWS_READ) {
		fprintf(fp, " %8ld %6dRD      %8ld %8ld %8ld\n",
			sfs_ibuf.i_nextr,
			sfs_ibuf.i_rwlock.rw_read,
			sfs_ibuf.i_swapcnt,
			sfs_ibuf.i_diroff,
			sfs_ibuf.i_ftime);
	} else {
		fprintf(fp, " %8ld   AVAIL  %8ld %8ld   %8ld\n",
			sfs_ibuf.i_nextr,
			sfs_ibuf.i_swapcnt,
			sfs_ibuf.i_diroff,
			sfs_ibuf.i_ftime);
	}

	if((vnode.v_type == VDIR) || (vnode.v_type == VREG)
		|| (vnode.v_type == VLNK)) {
		for(i = 0; i < NADDR; i++) {
			if(!(i & 3))
				fprintf(fp, "\n\t");
			fprintf(fp, "[%2d]: %-10x", i, sfs_ibuf.i_db[i]);
		}
		fprintf(fp, "\n");
	} else
		fprintf(fp, "\n");

	/* print vnode info */
	fprintf(fp, "\nVNODE :\n");
	fprintf(fp,vnheading);
	cprvnode(&vnode);
	fprintf(fp, "\n");
}


getsfs_ipos(addr, list, max)
long	addr;
struct listbuf *list;
int	max;
{
	int	i;
	int	pos;
	struct listbuf *listptr;

	listptr = list;
	pos = -1;
	for(i = 0; i < max; i++, listptr++) {
		if (listptr->addr == addr) {
			pos = i;
			break;
		}
	}
	return(pos);
}


int sfslck()
{
	union ihead	*ih;
	inode_t	*ip, *ipx, sfs_ibuf;
	int active = 0;
	extern print_lock();

	S_sfs_ihead = symfindval("sfs_ihead");
	readmem(S_sfs_ihead,1,-1,&sfs_ihead,sizeof(sfs_ihead),"sfs_ihead");

	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++){
                ipx = (inode_t *)ih->ih_chain[0];
                for (ip = ipx;;) {
			readmem((long)ip,1,-1, (char *)&sfs_ibuf,
				sizeof sfs_ibuf, "sfs/ufs inode");
                        if (sfs_ibuf.i_forw == ipx)
                                break;
			if(sfs_ibuf.i_mode != 0 && sfs_ibuf.i_vnode.v_filocks) {
				active += print_lock(sfs_ibuf.i_vnode.v_filocks, ip, "sfs/ufs");
			}
			ip = sfs_ibuf.i_forw;
		}
	}
	return (active);
}
