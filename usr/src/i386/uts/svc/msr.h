#ifndef _SVC_MSR_H	/* wrapper symbol for kernel use */
#define _SVC_MSR_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/msr.h	1.1.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/* Model-specific registers for Pentium: */

#define P5_MC_ADDR	0
#define P5_MC_TYPE	1

/* Model-specific registers for P6: */

#define TSC			0x010
#define APIC_BASE_MSR		0x01B
#define PerfCtr0		0x0C1
#define PerfCtr1		0x0C2
#define MTRRcap			0x0FE
#define MCG_CAP			0x179
#define MCG_STATUS		0x17A
#define PerfEvtSel0		0x186
#define PerfEvtSel1		0x187
#define DebugCtlMSR		0x1D9
#define LastBranchFromIP	0x1DB
#define LastBranchToIP		0x1DC
#define LastExceptionFromIP	0x1DD
#define LastExceptionToIP	0x1DE
#define MTRRphysBase0		0x200
#define MTRRphysMask0		0x201
#define MTRRphysBase1		0x202
#define MTRRphysMask1		0x203
#define MTRRphysBase2		0x204
#define MTRRphysMask2		0x205
#define MTRRphysBase3		0x206
#define MTRRphysMask3		0x207
#define MTRRphysBase4		0x208
#define MTRRphysMask4		0x209
#define MTRRphysBase5		0x20A
#define MTRRphysMask5		0x20B
#define MTRRphysBase6		0x20C
#define MTRRphysMask6		0x20D
#define MTRRphysBase7		0x20E
#define MTRRphysMask7		0x20F
#define MTRRfix64K_00000	0x250
#define MTRRfix16K_80000	0x258
#define MTRRfix16K_A0000	0x259
#define MTRRfix4K_C0000		0x268
#define MTRRfix4K_C8000		0x269
#define MTRRfix4K_D0000		0x26A
#define MTRRfix4K_D8000		0x26B
#define MTRRfix4K_E0000		0x26C
#define MTRRfix4K_E8000		0x26D
#define MTRRfix4K_F0000		0x26E
#define MTRRfix4K_F8000		0x26F
#define MTRRdefType		0x2FF
#define MC0_CTL			0x400
#define MC0_STATUS		0x401
#define MC0_ADDR		0x402
#define MC0_MISC		0x403
#define MC1_CTL			0x404
#define MC1_STATUS		0x405
#define MC1_ADDR		0x406
#define MC1_MISC		0x407
#define MC2_CTL			0x408
#define MC2_STATUS		0x409
#define MC2_ADDR		0x40A
#define MC2_MISC		0x40B
#define MC3_CTL			0x40C
#define MC3_STATUS		0x40D
#define MC3_ADDR		0x40E
#define MC3_MISC		0x40F
#define MC4_CTL			0x410
#define MC4_STATUS		0x411
#define MC4_ADDR		0x412
#define MC4_MISC		0x413
#define MC5_CTL			0x414
#define MC5_STATUS		0x415
#define MC5_ADDR		0x416
#define MC5_MISC		0x417

/* P6 Debug Extensions (DebugCtlMSR bits) */

#define LBR		(1<<0)	/* Record last branch and last exception */
#define BTF		(1<<1)	/* Enable branch step when TF set */

/*
 *  Machine Check Architecture defines
 */
 
#define MCA_REGS		4	/* number of MSRs per bank */

/*
 *  Masks for the MCG_CAP register:
 */

#define MCG_CAP_CNT		0xff	/* # of hardware reporting banks */
#define MCG_CTL_PRESENT		0x100	/* MCG_CTL register present */

/*
 *  Masks for the MCG_STATUS register:
 */

#define MCG_STATUS_MCIP		0x4	/* Machine check in progress */
#define	MCG_STATUS_EIPV		0x2	/* Error IP Valid */
#define	MCG_STATUS_RIPV		0x1	/* Restart IP Valid */

/*
 *  Masks for the hardware reporting banks
 */

#define	MCi_STATUS_DAM		0x02000000	/* Processor state damage */
#define	MCi_STATUS_ADDRV	0x04000000	/* MCi_ADDR is valid */
#define	MCi_STATUS_MISCV	0x08000000	/* MCi_MISC is valid */
#define	MCi_STATUS_EN		0x10000000	/* Error enabled */
#define	MCi_STATUS_UC		0x20000000	/* Error uncorrected */
#define	MCi_STATUS_O		0x40000000	/* Overflow */
#define	MCi_STATUS_V		0x80000000	/* Bank valid */


#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_MSR_H */
