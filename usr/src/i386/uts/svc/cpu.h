#ifndef _SVC_CPU_H	/* wrapper symbol for kernel use */
#define _SVC_CPU_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/cpu.h	1.8.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Legal values for cpu_id */

#define CPU_UNK	0
#define CPU_386	3
#define CPU_486	4
#define CPU_P5	5
#define CPU_P6	6

/* Legal values for cpu_model */

#define P6_MODEL	1

/* Legal values for cpu_stepping */

#define STEP_UNK	0

/* Feature bits */

#ifndef NCPUFEATWORD
#define NCPUFEATWORD	27	/* # (32-bit) words in feature bit array */
#endif

/*   Feature bits in word 0: */
/*   (Details of features are confidential.) */
#define CPUFEAT_FPU	0x00000001
#define CPUFEAT_VME	0x00000002
#define CPUFEAT_DE	0x00000004
#define CPUFEAT_PSE	0x00000008
#define CPUFEAT_TSC	0x00000010
#define CPUFEAT_MSR	0x00000020
#define CPUFEAT_PAE	0x00000040
#define CPUFEAT_MCE	0x00000080
#define CPUFEAT_CX8	0x00000100
#define CPUFEAT_APIC	0x00000200
#define CPUFEAT_MTRR	0x00001000
#define CPUFEAT_PGE	0x00002000
#define CPUFEAT_MCA	0x00004000
#define CPUFEAT_CMOV	0x00008000

/*   Memory types: */
#define MT_UC		0
#define MT_USWC		1
#define MT_WT		4
#define MT_WP		5
#define MT_WB		6

#ifdef _KERNEL

extern boolean_t ignore_machine_check;
extern boolean_t disable_cache;

/* true if PGE should be disabled */
extern boolean_t disable_pge;

/* true if we are using pae mode */
extern boolean_t using_pae;

#endif /* _KERNEL */

#define PGE_ENABLED()	((l.cpu_features[0] & CPUFEAT_PGE) && !disable_pge)
#define PAE_ENABLED()	using_pae

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CPU_H */
