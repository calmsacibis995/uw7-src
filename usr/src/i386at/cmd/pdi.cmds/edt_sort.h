#ident	"@(#)pdi.cmds:edt_sort.h	1.4.1.1"

#include	<sys/sdi_edt.h>

#ifdef TRACE
#include	<stdio.h>
#define DTRACE fprintf(stderr,"%d-%s\n",__LINE__,__FILE__)
#else
#define DTRACE
#endif

#define ORDINAL_A(a,b,c)	(((((a*MAX_BUS)+b)*MAX_EXTCS)+c)*MAX_EXLUS)
#define ORDINAL(p)	ORDINAL_A(p->xedt_ctl,p->xedt_bus,p->xedt_target)
#define BOOTABLE(p)	(!strcmp(p->xedt_drvname,"vtoc"))

/* Type Definitions */
typedef	struct	scsi_xedt	EDT;

struct HBA {
	int	index;		/* controller number of the c'th HBA */
	int	order;		/* translation from HBA to device order */
	int	ntargets;	/* number of targets configured on this HBA */
	EDT	*edtptr;	/* ptr to HBA[c]'s EDT starting point */
	EDT	*cntl;	    /* ptr to HBA[c]'s controller entry */
};

extern int edt_sort(EDT *, int, struct HBA *, int, char);
extern int edt_fix(EDT *, int);
extern int *readhbamap();
