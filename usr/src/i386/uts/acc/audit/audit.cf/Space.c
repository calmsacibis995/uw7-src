#ident	"@(#)Space.c	1.2"

#include <config.h>
#include <sys/types.h>
#include <sys/audit.h>

#define ADT_VER		"4.0"
#define ADT_BSIZE	20480
#define ADT_NLVLS	4
#define ADT_NBUF	2

char adt_ver[8] = ADT_VER;
int adt_bsize = ADT_BSIZE;
int adt_lwp_bsize = ADT_LWP_BSIZE;
uint adt_nbuf = ADT_NBUF;

int adt_nlvls = ADT_NLVLS;
