/*
 *	@(#)pnp.h	7.1	10/22/97	12:29:16
 * File pnp.h for the Plug-and-Play driver
 *
 * @(#) pnp.h 65.4 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */


#ifndef _SYS_PNP_H
#define _SYS_PNP_H

#pragma pack(1)
typedef struct
{
    union
    {
	u_char	byte[4];	/* [00] */
	u_long	dword;
    } sig;
    u_char	rev;		/* [04] */
    u_char	len;		/* [05] */
    u_short	ctrl;		/* [06] */
    u_char	chksum;		/* [08] */
    u_long	event;		/* [09] */
    u_short	rm16offset;	/* [0d] */
    u_short	rm16seg;	/* [0f] */
    u_short	pm16offset;	/* [11] */
    paddr_t	pm16codebase;	/* [13] */
    u_long	oem;		/* [17] */
    u_short	rm16dataseg;	/* [1b] */
    paddr_t	pm16database;	/* [1d] */
} pnp_bios_t;
#pragma pack()

#define PNPSIG		((paddr_t)0x506e5024)	/* "$PnP" */
#define PNPSIGLEN	sizeof(pnp_bios_t)

#ifdef _NO_PROTOTYPE		/* Old crusty kernel linker */
    typedef int PnP_res_type_t;
#   define PNP_NONE	0x00
#   define PNP_DISABLE	0x01
#   define PNP_IRQ_0	0x10
#   define PNP_IRQ_1	0x11
#   define PNP_DMA_0	0x20
#   define PNP_DMA_1	0x21
#   define PNP_IO_0	0x30
#   define PNP_IO_1	0x31
#   define PNP_IO_2	0x32
#   define PNP_IO_3	0x33
#   define PNP_IO_4	0x34
#   define PNP_IO_5	0x35
#   define PNP_IO_6	0x36
#   define PNP_IO_7	0x37
#   define PNP_MEM_0	0x40
#   define PNP_MEM_1	0x41
#   define PNP_MEM_2	0x42
#   define PNP_MEM_3	0x43
#   define PNP_MEM32_0	0x50
#   define PNP_MEM32_1	0x51
#   define PNP_MEM32_2	0x52
#   define PNP_MEM32_3	0x53
#else
    typedef enum
    {
	PNP_NONE = 0x00U,	/* End of list marker, PNP_END_OF_RES_TABLE */
	PNP_DISABLE = 0x01U,	/* Disable this node */

	PNP_IRQ_0 = 0x10U,	/* Up to two seperate interrupt levels */
	PNP_IRQ_1 = 0x11U,

	PNP_DMA_0 = 0x20U,	/* Up to two DMA channels */
	PNP_DMA_1 = 0x21U,

	PNP_IO_0 = 0x30U,	/* Up to eight non-contiguous I/O port ranges */
	PNP_IO_1 = 0x31U,
	PNP_IO_2 = 0x32U,
	PNP_IO_3 = 0x33U,
	PNP_IO_4 = 0x34U,
	PNP_IO_5 = 0x35U,
	PNP_IO_6 = 0x36U,
	PNP_IO_7 = 0x37U,

	PNP_MEM_0 = 0x40U,	/* Up to four non-contiguous memory ranges */
	PNP_MEM_1 = 0x41U,
	PNP_MEM_2 = 0x42U,
	PNP_MEM_3 = 0x43U,

	PNP_MEM32_0 = 0x50U,
	PNP_MEM32_1 = 0x51U,
	PNP_MEM32_2 = 0x52U,
	PNP_MEM32_3 = 0x53U

    } PnP_res_type_t;
#endif

typedef struct
{
    u_long		vendor;
    u_long		serial;
    u_long		devNum;
    PnP_res_type_t	type;
    u_long		value;
    u_long		limit;
} PnP_resSpec_t;

#define PNP_ANY_VENDOR	((u_long)-1)
#define PNP_ANY_SERIAL	((u_long)-1)
#define PNP_ANY_UNIT	(-1)
#define PNP_ANY_NODE	(-1)

#define PNP_END_OF_RES_TABLE { PNP_ANY_VENDOR, PNP_ANY_SERIAL, 0, PNP_NONE }

/* Res-manager resource name for PnP_resSpec_t */
#define CM_PNPVENDOR	"PNPVENDOR"
#define CM_PNPSERNO	"PNPSERNO"
#define CM_PNPDEVNO	"PNPDEVNUM"
#define CM_PNPTYPE	"PNPTYPE"
#define CM_PNPLIMIT	"PNPLIMIT"
#define CM_PNPMODNAME	"PNPMODNAME"

/*
 * PnP user interface
 */

typedef struct
{
    u_char	Rev;
    u_char	NumNodes;
    u_short	ReadDataPort;
    u_short	Reserved;
} PnP_ISA_Config_t;

typedef enum
{
    pnp_bus_none,
    pnp_bus_bios,
    pnp_bus_os
} PnP_bus_t;

typedef struct
{
    PnP_bus_t	BusType;
    u_long	NodeSize;
    u_long	NumNodes;
} PnP_busdata_t;

typedef struct
{
    u_long		vendor;
    u_long		serial;
    int			unit;
    int			node;
} PnP_unitID_t;

typedef struct
{
    PnP_unitID_t	unit;
    u_long		NodeSize;
    u_long		ResCnt;
    u_char		devCnt;
} PnP_findUnit_t;

typedef struct
{
    PnP_unitID_t	unit;
    u_long		resNum;
    u_long		tagLen;
    u_char		*tagPtr;
} PnP_TagReq_t;

typedef struct
{
    PnP_unitID_t	unit;
    u_char		devNum;
    u_char		regNum;
    u_char		regCnt;
    void		*regBuf;
} PnP_RegReq_t;

typedef struct
{
    PnP_unitID_t	unit;
    u_char		device;
    u_char		active;
} PnP_Active_t;

#define PNPIOC			(('P' << 24) | ('n' << 16) | ('P' << 8))

#define PNP_BUS_PRESENT		(PNPIOC | 0x81)	/* PnP_busdata_t *arg */
#define PNP_FIND_UNIT		(PNPIOC | 0x82)	/* PnP_findUnit_t *arg */
#define PNP_READ_TAG		(PNPIOC | 0x83)	/* PnP_TagReq_t *arg */
#define PNP_READ_REG		(PNPIOC | 0x85)	/* PnP_RegReq_t *arg */
#define PNP_WRITE_REG		(PNPIOC | 0x86)	/* PnP_RegReq_t *arg */
#define PNP_READ_ACTIVE		(PNPIOC | 0x87)	/* PnP_Active_t *arg */
#define PNP_WRITE_ACTIVE	(PNPIOC | 0x88)	/* PnP_Active_t *arg */

/*
 * PnP services to card drivers.
 */

#if( _INKERNEL || (UNIXWARE && _KERNEL) )

typedef enum
{
	/* Card control				0x00-0x07 */
    PNP_REG_SetReadDataPort = 0x00,
    PNP_REG_ConfigIsolate = 0x01,
    PNP_REG_ConfigControl = 0x02,
    PNP_REG_WakeCommand = 0x03,
    PNP_REG_ResourceData = 0x04,
    PNP_REG_Status = 0x05,
    PNP_REG_CSN = 0x06,
    PNP_REG_LogicalDevNum = 0x07,

	/* Card-level, reserved			0x08-0x1f */

	/* Card-level, vendor defined		0x20-0x2f */

	/* Logical device control		0x30-0x31 */
    PNP_REG_Activate = 0x30,
    PNP_REG_IOrangeCheck = 0x31,

	/* Logical device control, reserved	0x32-0x37 */

	/* Logical device control, vendor def	0x38-0x3f */

	/* Logical device configuration		0x40-0x75 */
    PNP_REG_Memory_0_BaseHi = 0x40,
    PNP_REG_Memory_0_BaseLo = 0x41,
    PNP_REG_Memory_0_Control = 0x42,
    PNP_REG_Memory_0_LimitHi = 0x43,
    PNP_REG_Memory_0_LimitLo = 0x44,

    PNP_REG_Memory_1_BaseHi = 0x48,
    PNP_REG_Memory_1_BaseLo = 0x49,
    PNP_REG_Memory_1_Control = 0x4a,
    PNP_REG_Memory_1_LimitHi = 0x4b,
    PNP_REG_Memory_1_LimitLo = 0x4c,

    PNP_REG_Memory_2_BaseHi = 0x50,
    PNP_REG_Memory_2_BaseLo = 0x51,
    PNP_REG_Memory_2_Control = 0x52,
    PNP_REG_Memory_2_LimitHi = 0x53,
    PNP_REG_Memory_2_LimitLo = 0x54,

    PNP_REG_Memory_3_BaseHi = 0x58,
    PNP_REG_Memory_3_BaseLo = 0x59,
    PNP_REG_Memory_3_Control = 0x5a,
    PNP_REG_Memory_3_LimitHi = 0x5b,
    PNP_REG_Memory_3_LimitLo = 0x5c,

    PNP_REG_IO_0_Hi = 0x60,
    PNP_REG_IO_0_Lo = 0x61,
    PNP_REG_IO_1_Hi = 0x62,
    PNP_REG_IO_1_Lo = 0x63,
    PNP_REG_IO_2_Hi = 0x64,
    PNP_REG_IO_2_Lo = 0x65,
    PNP_REG_IO_3_Hi = 0x66,
    PNP_REG_IO_3_Lo = 0x67,
    PNP_REG_IO_4_Hi = 0x68,
    PNP_REG_IO_4_Lo = 0x69,
    PNP_REG_IO_5_Hi = 0x6a,
    PNP_REG_IO_5_Lo = 0x6b,
    PNP_REG_IO_6_Hi = 0x6c,
    PNP_REG_IO_6_Lo = 0x6d,
    PNP_REG_IO_7_Hi = 0x6e,
    PNP_REG_IO_7_Lo = 0x6f,

    PNP_REG_IRQ_0_lvl = 0x70,
    PNP_REG_IRQ_0_type = 0x71,
    PNP_REG_IRQ_1_lvl = 0x72,
    PNP_REG_IRQ_1_type = 0x72,

    PNP_REG_DMA_0 = 0x74,
    PNP_REG_DMA_1 = 0x75,

	/* Logical device config, reserved	0x76-0xef */

    PNP_REG_Memory32_0_Base3 = 0x76,
    PNP_REG_Memory32_0_Base2 = 0x77,
    PNP_REG_Memory32_0_Base1 = 0x78,
    PNP_REG_Memory32_0_Base0 = 0x79,
    PNP_REG_Memory32_0_Control = 0x7a,
    PNP_REG_Memory32_0_Limit3 = 0x7b,
    PNP_REG_Memory32_0_Limit2 = 0x7c,
    PNP_REG_Memory32_0_Limit1 = 0x7d,
    PNP_REG_Memory32_0_Limit0 = 0x7e,

    PNP_REG_Memory32_1_Base3 = 0x80,
    PNP_REG_Memory32_1_Base2 = 0x81,
    PNP_REG_Memory32_1_Base1 = 0x82,
    PNP_REG_Memory32_1_Base0 = 0x83,
    PNP_REG_Memory32_1_Control = 0x84,
    PNP_REG_Memory32_1_Limit3 = 0x85,
    PNP_REG_Memory32_1_Limit2 = 0x86,
    PNP_REG_Memory32_1_Limit1 = 0x87,
    PNP_REG_Memory32_1_Limit0 = 0x88,

    PNP_REG_Memory32_2_Base3 = 0x90,
    PNP_REG_Memory32_2_Base2 = 0x91,
    PNP_REG_Memory32_2_Base1 = 0x92,
    PNP_REG_Memory32_2_Base0 = 0x93,
    PNP_REG_Memory32_2_Control = 0x94,
    PNP_REG_Memory32_2_Limit3 = 0x95,
    PNP_REG_Memory32_2_Limit2 = 0x96,
    PNP_REG_Memory32_2_Limit1 = 0x97,
    PNP_REG_Memory32_2_Limit0 = 0x98,

    PNP_REG_Memory32_3_Base3 = 0xa0,
    PNP_REG_Memory32_3_Base2 = 0xa1,
    PNP_REG_Memory32_3_Base1 = 0xa2,
    PNP_REG_Memory32_3_Base0 = 0xa3,
    PNP_REG_Memory32_3_Control = 0xa4,
    PNP_REG_Memory32_3_Limit3 = 0xa5,
    PNP_REG_Memory32_3_Limit2 = 0xa6,
    PNP_REG_Memory32_3_Limit1 = 0xa7,
    PNP_REG_Memory32_3_Limit0 = 0xa8,

	/* Logical device config, vendor def	0xf0-0xfe */

	/* Reserved				0xff */
    PNP_REG_Reserved = 0xff

} PnP_Register_t;

#ifdef _NO_PROTOTYPE
    int PnP_Bios();
    int PnP_bus_data();
    int PnP_ReadTag();
    char *PnP_idStr();
    u_long PnP_idVal();
#else
    int PnP_Bios(int Func, ...);
    int PnP_bus_data(PnP_busdata_t *bus);
    int PnP_ReadTag(PnP_TagReq_t *tagReq);
    const char *PnP_idStr(u_long vendor);
    u_long PnP_idVal(const char *idStr);
#endif

#endif

#endif	/* inclusion protection: _SYS_PNP_H */
