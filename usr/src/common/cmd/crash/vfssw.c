#ident	"@(#)crash:common/cmd/crash/vfssw.c	1.1.1.1"

/*
 * This file contains code for the crash function vfssw.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/fstyp.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include "crash.h"

static vaddr_t Nfstype, Vfssw; /* from namelist */

/* get arguments for vfssw function */
int
getvfssw()
{
	int slot;
	long arg1;
	long arg2;
	int c;
	int nfstype;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (!Vfssw)
		Vfssw = symfindval("vfssw");
	if (!Nfstype)
		Nfstype = symfindval("nfstype");
	readmem(Nfstype,1,-1,&nfstype,sizeof (int),"nfstype");

	fprintf(fp,"FILE SYSTEM SWITCH TABLE SIZE = %d\n",nfstype);
	fprintf(fp,"SLOT   NAME             FLAGS\n");
	if(args[optind]) {
		do {
			/* permit 0: it reports "BADVFS" */
			if (getargs(nfstype,&arg1,&arg2))
			    for(slot = arg1; slot <= arg2; slot++)
				prvfssw(1,slot,-1,nfstype);
			else
				prvfssw(1,-1,arg1,nfstype);
		}while(args[++optind]);
	}
	else for(slot = 1; slot < nfstype; slot++)
		prvfssw(0,slot,-1,nfstype);
}

/* print vfs switch table */
int
prvfssw(all,slot,addr,max)
int all,slot,max;
long addr;
{
	struct vfssw vfsswbuf;
	char name[FSTYPSZ+1];

	if(addr == -1)
		addr = Vfssw + slot * sizeof(vfsswbuf);
	else
		slot = getslot(addr,Vfssw,sizeof vfsswbuf,0,max);
	readmem(addr,1,-1,
		(char *)&vfsswbuf,sizeof vfsswbuf,"vfssw slot");
	if(!vfsswbuf.vsw_name && !all)
		return; 
	if(slot == -1)
		fprintf(fp,"  - ");
	else
		fprintf(fp, "%4d", slot);
	readmem((long)vfsswbuf.vsw_name,1,-1,name,sizeof name,"vsw_name");
	fprintf(fp,"   %-16s", name);
	fprintf(fp," %x\n",
		vfsswbuf.vsw_flag);
}

char * 
getfsname(slot)
int slot;
{
	struct vfssw vfsswbuf;
	static char name[FSTYPSZ+1];
	int nfstype;

	if (!Nfstype)
		Nfstype = symfindval("nfstype");
	if (!Vfssw)
		Vfssw = symfindval("vfssw");
	readmem(Nfstype,1,-1,&nfstype,sizeof(int),"nfstype");
	if(slot < 0 || slot >= nfstype)
		return(NULL);
	readmem(Vfssw+slot*sizeof vfsswbuf,1,-1,
		&vfsswbuf,sizeof vfsswbuf,"vfssw slot");
	if(!vfsswbuf.vsw_name)
		return(NULL); 
	readmem((vaddr_t)vfsswbuf.vsw_name,1,-1,name,FSTYPSZ,"vsw_name");
	return(name);
}

char *
vnotofsname(vnop)
vnode_t *vnop;
{
	static vaddr_t Spec_vnodeops, Fifo_vnodeops, Proc_vnodeops;
	vfs_t vfsbuf;
	struct syment *sp;

	if(!Spec_vnodeops)
		Spec_vnodeops = symfindval("spec_vnodeops");
	if(!Fifo_vnodeops)
		Fifo_vnodeops = symfindval("fifo_vnodeops");
	if(!Proc_vnodeops) {
		if ((sp = symsrch("prvnodeops")) != NULL)
			Proc_vnodeops = sp->n_value;
		else {
			prerrmes("prvnodeops not found in symbol table\n");
			Proc_vnodeops = -1;
		}
	}

	/* Handle special cases */
	if((vaddr_t)(vnop->v_op) == Spec_vnodeops)
		return("spec");
	if((vaddr_t)(vnop->v_op) == Fifo_vnodeops)
		return("fifo");
	if((vaddr_t)(vnop->v_op) == Proc_vnodeops)
		return("proc");
	if(vnop->v_vfsp == NULL)
		return NULL;

	/* Handle s5,ufs,sfs,vxfs */
	readmem((vaddr_t)vnop->v_vfsp,1,-1,
		&vfsbuf,sizeof(vfsbuf),"vfs structure");
	return(getfsname(vfsbuf.vfs_fstype));
}
