#ident	"@(#)crash:i386/cmd/crash/cdfs_inode.c	1.3.2.1"

/*
 * This file contains code for the crash functions:  cdfs_inode.
 */
#include <sys/param.h>
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
#include <sys/fs/cdfs_inode.h>
#include <sys/fs/xnamnode.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <stdlib.h>

#include "crash.h"

static vaddr_t S_cdfs_vnodeops, S_cdfs_InodeFree;
static vaddr_t S_cdfs_InodeCache, S_cdfs_InodeCnt;

struct cdfs_inode	cdfs_ibuf;		/* buffer for cdfs_inodes */
struct cdfs_inode	*cdfs_FreeInode;	/* buffer for free inodes */
struct vnode 		vnode;
long 			cdfs_ifree;		/* inode free list */
uint 			ninode;
int 			freeinodes;		/* free inodes */
long 			iptr;

struct listbuf {
        long    addr;
        char    state;
};

/* get arguments for cdfs inode function */
int
get_cdfs_inode()
{
	int slot;
	int full = 0;
	int all = 0;
	long addr;
	long arg1;
	long arg2;
	int lfree = 0;
	long next;
	int list = 0;
	int i, c;
	struct inode *freelist;
	struct listbuf  *listptr;
        struct listbuf  *listbuf;

	char *heading = 
	    "SLOT  Sect#/Offset  RCNT  LINK     UID     GID     SIZE TYPE  MODE   FLAGS\n";

	S_cdfs_vnodeops = symfindval("cdfs_vnodeops");
	S_cdfs_InodeFree = symfindval("cdfs_InodeFree");
	S_cdfs_InodeCnt = symfindval("cdfs_InodeCnt");
	S_cdfs_InodeCache = symfindval("cdfs_InodeCache");

	optind = 1;
	while((c = getopt(argcnt,args,"efrlw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'r' :	lfree = 1;
					break;
			case 'l' :	list = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	readmem(S_cdfs_InodeCnt,1,-1,
		&ninode, sizeof (ninode), "CDFS Inode");
	readmem(S_cdfs_InodeFree,1,-1,
		&cdfs_ifree, sizeof (cdfs_ifree), "CDFS Free Inode");
	readmem(S_cdfs_InodeCache,1,-1,
		&iptr, sizeof iptr, "CDFS inode");

	listbuf = listptr =
		cr_malloc(sizeof(struct listbuf)*ninode, "CDFS inode list");

	addr = iptr;
	for (i = 0; i < ninode; i++, listptr++) {
		readmem((long)(iptr + i * sizeof cdfs_ibuf), 1,-1,
			 (char *)&cdfs_ibuf, sizeof cdfs_ibuf, "CDFS inode table");
		listptr->addr = iptr;
                listptr->state = 'n';           	/* unknown state */
		listptr->addr += i * (sizeof cdfs_ibuf);
		vnode = cdfs_ibuf.i_VnodeStorage;
                if ((long)vnode.v_op != S_cdfs_vnodeops) {
                        listptr->state = 'x'; 		/* not cdfs */
                        continue;
                }
                if (cdfs_ibuf.i_VnodeStorage.v_count != 0)
                        listptr->state = 'u';           /* in use */
        }
	if(list)
		list_cdfs_inode(listbuf);
	else {
		fprintf(fp,"INODE TABLE SIZE = %d\n", ninode);
		if(!full)
			fprintf(fp,"%s",heading);
		if(lfree) {
			freelist = (struct inode *)cdfs_ifree;
			next = (long)freelist;
			for (i=0; i< freeinodes; i++) {	
				pr_cdfs_inode(1,full,-1,next,heading);
				next = (long)cdfs_ibuf.i_FreeFwd;
			}
		} else if(args[optind]) {
			do {
				if (getargs(ninode,&arg1,&arg2)) {
				    for (slot = arg1; slot <= arg2; slot++) {
					addr = listbuf[slot].addr;
					pr_cdfs_inode(1,full,slot,addr,heading);
				    }
				} else {
					addr = arg1;
					slot = get_cdfs_ipos(addr,listbuf,ninode);
					pr_cdfs_inode(1,full,slot,addr,heading);
				}
			}while(args[++optind]);
		} else {
			for(slot = 1; slot < ninode; slot++) {
				addr = listbuf[slot].addr;
				pr_cdfs_inode(all,full,slot,addr,heading);
			}
		}
	}
}

int
list_cdfs_inode(listbuf)
struct listbuf *listbuf;
{
	struct listbuf  *listptr;
	int i,j;
	long next;
	struct cdfs_inode *cdfs_freelist;

	cdfs_freelist = (struct cdfs_inode *)cdfs_ifree;
        next = (long)cdfs_freelist;
        for (i = 0; i < freeinodes; i++) {
                i = get_cdfs_ipos((long)next,listbuf,ninode);
                readmem((long)next,1,-1,(char *)&cdfs_ibuf,sizeof cdfs_ibuf,"CDFS inode");
                if( listbuf[i].state == 'u' )
                        listbuf[i].state = 'b';
                else
                        if( listbuf[i].state  != 'x' )
                                listbuf[i].state = 'f';
                next = (long)cdfs_ibuf.i_FreeFwd;
        }
	fprintf(fp,"The following cdfs inodes are in use:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following cdfs inodes are on the freelist:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following cdfs inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}

	fprintf(fp,"\n\nThe following cdfs inodes are in unknown states:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'n') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n");
}


/* print inode table */
int
pr_cdfs_inode(all,full,slot,addr,heading)
int all,full,slot;
long addr;
char *heading;
{
	char ch;
	int i;
	cdfs_drec_t	drec;

	if (addr == -1)
		return;
	readmem(addr, 1,-1,(char *)&cdfs_ibuf,sizeof cdfs_ibuf,"cdfs inode table");
	vnode = cdfs_ibuf.i_VnodeStorage;

	if (!vnode.v_count && !all) 
		return;
	if ((long)vnode.v_op != S_cdfs_vnodeops)
		return;				/* not cdfs */

	if (full)
		fprintf(fp,"%s",heading);

	if (slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp," %4u,%-4u   %4u      %3d   %5d %7d  %7d",
		cdfs_ibuf.i_Fid.fid_SectNum,
		cdfs_ibuf.i_Fid.fid_Offset,
		cdfs_ibuf.i_VnodeStorage.v_count,
		cdfs_ibuf.i_LinkCnt,
		cdfs_ibuf.i_UserID,
		cdfs_ibuf.i_GroupID,
		cdfs_ibuf.i_Size);
	switch(vnode.v_type) {
		case VDIR: ch = ' d'; break;
		case VCHR: ch = ' c'; break;
		case VBLK: ch = ' b'; break;
		case VREG: ch = ' f'; break;
		case VLNK: ch = ' l'; break;
		case VFIFO: ch = ' p'; break;
		case VXNAM:
			 switch(cdfs_ibuf.i_DevNum) {
				case XNAM_SEM: ch = ' s'; break;
				case XNAM_SD: ch = ' m'; break;
				default: ch = ' -'; break;
			}
		default:    ch = ' -'; break;
	}
	fprintf(fp,"   %c  ",ch);
	fprintf(fp,"  %s%s%s",
		cdfs_ibuf.i_Mode & ISUID ? "u" : "-",
		cdfs_ibuf.i_Mode & ISGID ? "g" : "-",
		cdfs_ibuf.i_Mode & ISVTX ? "v" : "-");

	fprintf(fp,"  %s%s%s%s%s%s%s%s%s\n",
		cdfs_ibuf.i_Flags & IUPD ? "   up" : "",
		cdfs_ibuf.i_Flags & IACC ? "   ac" : "",
		cdfs_ibuf.i_Flags & ICHG ? "   ch" : "",
		cdfs_ibuf.i_Flags & IFREE ? "   fr" : "",
		cdfs_ibuf.i_Flags & INOACC ? "   na" : "",
		cdfs_ibuf.i_Flags & ISYNC ? "   sc" : "",
		cdfs_ibuf.i_Flags & IMODTIME ? "   mt" : "",
		cdfs_ibuf.i_Flags & IMOD ? "   md" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_HIDDEN ? "   hd" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_ASSOC ? "   as" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_RRIP_REL ? "   rr" : "");
	if(!full)
		return;

	drec = cdfs_ibuf.i_DirRecStorage;
	fprintf(fp,"\t    FREEFORW\t    FREEBCK\t    HASHFORW\t    HASHBCK\n");
	fprintf(fp,"\t%8x",cdfs_ibuf.i_FreeFwd);
	fprintf(fp,"\t   %8x",cdfs_ibuf.i_FreeBack);
	fprintf(fp,"\t   %8x",cdfs_ibuf.i_HashFwd);
	fprintf(fp,"\t   %8x\n",cdfs_ibuf.i_HashBack);

	fprintf(fp,"\t   NEXTBYTE   I_SLEEPLOCK   I_SPINLOCK MAPSZ  \n");
	fprintf(fp,"\t%8x   %8x   %8s   %8x\n", cdfs_ibuf.i_NextByte,
		 cdfs_ibuf.i_splock.sl_avail, cdfs_ibuf.i_mutex,
		 cdfs_ibuf.i_mapsz);

	if((vnode.v_type == VDIR) || (vnode.v_type == VREG)
		|| (vnode.v_type == VLNK)) {
		fprintf(fp, "\t NextDrec   PrevDrec   Loc    Offset\n");
		fprintf(fp,"\t%8x    %8x%8ld%8ld%8ld\n",drec.drec_NextDR,
	 drec.drec_PrevDR, drec.drec_Loc, drec.drec_Offset, drec.drec_Len); 
		fprintf(fp, "\t  Xarlen    ExtLoc    Datalen   UnitSz  Interleave \n");
		fprintf(fp,"\t%8d  %8d  %8d  %8d  %8d\n",
		drec.drec_XarLen, drec.drec_ExtLoc, drec.drec_DataLen,
		drec.drec_UnitSz, drec.drec_Interleave);
	}
	else
		fprintf(fp,"\n");

	/* print vnode info */
	fprintf(fp,"\nVNODE :\n");
	fprintf(fp,vnheading);
	cprvnode(&cdfs_ibuf.i_VnodeStorage);
	fprintf(fp,"\n");
}


get_cdfs_ipos(addr, list, max)
long    addr;
struct listbuf *list;
int     max;
{

        int     i;
        int     pos;
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
