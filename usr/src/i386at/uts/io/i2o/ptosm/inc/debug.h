#ident  "@(#)debug.h	1.2"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ifndef I2O_DEBUG_H
#define	I2O_DEBUG_H

#ifdef _KERNEL_HEADERS

#include <util/cmn_err.h>

#elif defined(_KERNEL)

#include <sys/cmn_err.h>

#endif  /* _KERNEL_HEADERS */

extern U32 i2opt_debug_flags;

#ifdef I2O_DEBUG
#include <sys/xdebug.h>
#define ENTER_DEBUGGER()    (*cdebugger)(DR_OTHER, NO_FRAME)
#else
#define ENTER_DEBUGGER()
#endif

#ifdef I2O_DEBUG
#define I2O_DBGPR(m, args) (i2opt_debug_flags & (m) ? \
                           cmn_err(CE_CONT, "level%x, %s\n", m, args):(void) 0)
#else	/* ! defined(I2O_DEBUG) */
#define	I2O_DBGPR(m, args)
#endif	/* defined(I2O_DEBUG) */

#define	I2ODBG0(args)	I2O_DBGPR(0x01, args)
#define	I2ODBG1(args)	I2O_DBGPR(0x02, args)
#define	I2ODBG2(args)	I2O_DBGPR(0x04, args)
#define	I2ODBG3(args)	I2O_DBGPR(0x08, args)

#define	I2ODBG4(args)	I2O_DBGPR(0x10, args)
#define	I2ODBG5(args)	I2O_DBGPR(0x20, args)
#define	I2ODBG6(args)	I2O_DBGPR(0x40, args)
#define	I2ODBG7(args)	I2O_DBGPR(0x80, args)

#define	I2ODBG8(args)	I2O_DBGPR(0x0100, args)
#define	I2ODBG9(args)	I2O_DBGPR(0x0200, args)
#define	I2ODBG10(args)	I2O_DBGPR(0x0400, args)
#define	I2ODBG11(args)	I2O_DBGPR(0x0800, args)

#define	I2ODBG12(args)	I2O_DBGPR(0x1000, args)
#define	I2ODBG13(args)	I2O_DBGPR(0x2000, args)
#define	I2ODBG14(args)	I2O_DBGPR(0x4000, args)
#define	I2ODBG15(args)	I2O_DBGPR(0x8000, args)

#define	I2ODBG16(args)	I2O_DBGPR(0x010000, args)
#define	I2ODBG17(args)	I2O_DBGPR(0x020000, args)
#define	I2ODBG18(args)	I2O_DBGPR(0x040000, args)
#define	I2ODBG19(args)	I2O_DBGPR(0x080000, args)

#define	I2ODBG20(args)	I2O_DBGPR(0x100000, args)
#define	I2ODBG21(args)	I2O_DBGPR(0x200000, args)
#define	I2ODBG22(args)	I2O_DBGPR(0x400000, args)
#define	I2ODBG23(args)	I2O_DBGPR(0x800000, args)

#define	I2ODBG24(args)	I2O_DBGPR(0x1000000, args)
#define	I2ODBG25(args)	I2O_DBGPR(0x2000000, args)
#define	I2ODBG26(args)	I2O_DBGPR(0x4000000, args)
#define	I2ODBG27(args)	I2O_DBGPR(0x8000000, args)

#define	I2ODBG28(args)	I2O_DBGPR(0x10000000, args)
#define	I2ODBG29(args)	I2O_DBGPR(0x20000000, args)
#define	I2ODBG30(args)	I2O_DBGPR(0x40000000, args)
#define	I2ODBG31(args)	I2O_DBGPR(0x80000000, args)

#define DEBUG_0         0x00000001
#define DEBUG_1         0x00000002
#define DEBUG_2         0x00000004
#define DEBUG_3         0x00000008
#define DEBUG_4         0x00000010
#define DEBUG_5         0x00000020
#define DEBUG_6         0x00000040
#define DEBUG_7         0x00000080
#define DEBUG_8         0x00000100
#define DEBUG_9         0x00000200
#define DEBUG_10        0x00000400
#define DEBUG_11        0x00000800
#define DEBUG_12        0x00001000
#define DEBUG_13        0x00002000
#define DEBUG_14        0x00004000
#define DEBUG_15        0x00008000
#define DEBUG_16        0x00010000
#define DEBUG_17        0x00020000
#define DEBUG_18        0x00040000
#define DEBUG_19        0x00080000
#define DEBUG_20        0x00100000
#define DEBUG_21        0x00200000
#define DEBUG_22        0x00400000
#define DEBUG_23        0x00800000
#define DEBUG_24        0x01000000
#define DEBUG_25        0x02000000
#define DEBUG_26        0x04000000
#define DEBUG_27        0x08000000
#define DEBUG_28        0x10000000
#define DEBUG_29        0x20000000
#define DEBUG_30        0x40000000
#define DEBUG_31        0x80000000

#endif  /* SYBIOS_DEBUG_H */
