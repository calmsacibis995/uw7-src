#ifndef PMREGSYS_H
#define PMREGSYS_H
#ident  "@(#)kern-i386:util/pmregsys.h	1.2.1.2"

struct component_license
{
  int   product_id;
  short licensed;
  int   activation_state;
  int   policy;
};

typedef struct callback
{
  int   product_id;
  int   flags;
  void  (*func)(int,int,int);
  struct callback *next;
} callback;

/* Commands for pmregsys */
#define PMD_INITIALIZE  	1
#define PMD_ADD_LICENSE 	2
#define PMD_GET_COUNT   	3
#define PMD_INC_COUNT   	4
#define PMD_DEC_COUNT   	5
#define PMD_KERN_INFO   	6
#define PMD_KERN_INIT  		7
#define PMD_SET_OS_POLICY 	8
#define PMD_GET_OS_POLICY 	9
#define PMD_SET_OS_REGSTATE 	10
#define PMD_GET_OS_REGSTATE 	11

/*
 * These data types are explicitly not included in a header file, and
 * this module should not be distributed as part of the source code
 * product, except in binary form.  The caller of this module knows
 * of these structures. If these ever change, be sure to make sure
 * that usr/src/common/subsys/license/i4/iforpm/daemons/pmd/register.C
 * is modified accordingly.
 */

#define	PMD_SERIAL_MAX	25

struct prodreg
{
	struct component_license *p;
	int nelem;
	int counter;
};

struct sysreg
{
	int users;
	int cpus;
	int cgs;
	char serial[PMD_SERIAL_MAX];
};

struct prodchk
{
	int product_id;
	int *counter;
};

struct prodmod
{
	int product_id;
	int modval;
};

#define PMD_SET_IVAL 0
#define PMD_GET_IVAL 1

struct pmd_ival
{
	int *ival;
};

struct pmreg
{
	int cmd;
	int version;
	unsigned long s;
	union 
	{
		struct prodreg pr;
		struct sysreg sr;
		struct prodchk pc;
		struct prodmod pm;
		struct pmd_ival iv;
	} pcmd;
};

struct pmregargs {
	struct pmreg *regs;
	int ret;
};

/* Flags for pmd_driver_licensed call. */

#define 	MP_SAFE			1 /* compat. register */
#define 	PMD_DRV_REGISTER	2 /* register the handler */
#define 	PMD_DRV_DEREGISTER	4 /* deregister the handler */
#define 	PMD_DRV_QUERY		6 /* query only - no registration required */

#endif  /* PMREGSYS_H */
