#ifndef _PSM_CBUS_CBUS_TABLES_H
#define _PSM_CBUS_CBUS_TABLES_H

#ident	"@(#)kern-i386at:psm/cbus/cbus_tables.h	1.1"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus_tables.h

Abstract:

    this module provides the C-bus II configuration table definitions.

--*/

#if defined(__cplusplus)
extern "C" {
#endif

//
// Corollary RRD configuration table defines.
//
#define CTAB_OLDRRD_MAXMB		64		// max C-bus Mb supported by RRD
#define CTAB_OLDRRD_ATMB		16
#define CTAB_OLDRRD_MAXRAMBRDS	4
#define CTAB_OLDRRD_MAXSLOTS	16

//
// processor types - counting number.
//
#define	CTAB_PT_NO_PROCESSOR	0x0
#define	CTAB_PT_386				0x1
#define	CTAB_PT_486				0x2
#define	CTAB_PT_586				0x3

//
// processor attributes - bit field.
//
#define	CTAB_PA_CACHE_OFF		0x0
#define	CTAB_PA_CACHE_ON		0x1

//
// I/O function - counting number.
//
#define	CTAB_IOF_NO_IO			0x0
#define	CTAB_IOF_SIO			0x1
#define	CTAB_IOF_SCSI			0x2
#define	CTAB_IOF_COROLLARY_RSVD	0x3
#define	CTAB_IOF_ISA_BRIDGE		0x4
#define	CTAB_IOF_EISA_BRIDGE	0x5
#define	CTAB_IOF_HODGE			0x6
#define	CTAB_IOF_P2				0x7
#define	CTAB_IOF_INVALID_ENTRY	0x8		// use to denote whole entry
									    // is invalid, note that pm must
									    // equal zero as well.
#define	CTAB_IOF_MEMORY			0x9

#define CTAB_ATSIO386			1		// 386 w/ serial I/O
#define CTAB_ATSCSI386			2		// 386 w/ SCSI
#define CTAB_ATSIO486			3		// 486 w/ serial I/O
#define CTAB_ATSIO486C			4		// 486 w/ serial I/O & int cache enabled
#define CTAB_ATBASE386			5		// 386 base
#define CTAB_ATBASE486			6		// 486 base
#define CTAB_ATBASE486C			7		// 486 base w/ internal cache enabled
#define CTAB_ATP2486C			8		// 486-P2   w/ internal cache enabled

//
// bit fields of pel_features, independent of whether pm indicates it
// has an attached processor or not.
//
#define CTAB_ELEMENT_SIO		0x00001	// SIO present
#define CTAB_ELEMENT_SCSI		0x00002	// SCSI present
#define CTAB_ELEMENT_IOBUS		0x00004	// IO bus is accessible
#define CTAB_ELEMENT_BRIDGE		0x00008	// IO bus Bridge
#define CTAB_ELEMENT_HAS_8259	0x00010	// local 8259s present
#define CTAB_ELEMENT_HAS_CBC	0x00020	// local Corollary CBC
#define CTAB_ELEMENT_HAS_APIC	0x00040	// local Intel APIC
#define CTAB_ELEMENT_WITH_IO	0x00080	// some extra I/O device here
						                // this could be SCSI, SIO, etc
#define CTAB_ELEMENT_RRD_RSVD	0x20000	// Old RRDs used this

//
// due to backwards compatibility, the check for an I/O
// device is somewhat awkward.
//
#define CTAB_ELEMENT_HAS_IO		(CTAB_ELEMENT_SIO | CTAB_ELEMENT_SCSI | \
								 CTAB_ELEMENT_WITH_IO)

//
// bit fields of machine types.
//
#define	CTAB_MACHINE_CBUS1		0x1		// C-bus
#define	CTAB_MACHINE_CBUS1_XM	0x2		// C-bus XM
#define	CTAB_MACHINE_CBUS2		0x4		// C-bus II

//
// bit fields of supported environment types.
//
#define	CTAB_SCO_UNIX			0x1
#define	CTAB_UNIXWARE			0x2
#define	CTAB_WINDOWS_NT			0x4
#define	CTAB_NOVELL				0x8

//
// C-bus OEM number - counting number.
//
#define CTAB_OEM_COROLLARY		1
#define CTAB_OEM_DEC			2
#define CTAB_OEM_ZENITH			3
#define CTAB_OEM_MITAC			4
#define CTAB_OEM_ALR			5
#define CTAB_OEM_SNI			6
#define CTAB_OEM_OLIVETTI_PCI	7
#define CTAB_OEM_IBM_MCA		8
#define CTAB_OEM_CHEN			9

//
// OEM sub class - counting number.
//
#define CTAB_CRLLRYISA			1
#define CTAB_CRLLRYEISA			2
#define CTAB_CBUS2EISA			3

typedef struct _cbus_configuration_t {
	ULONG		Checkword;
	UCHAR		Mb[CTAB_OLDRRD_MAXMB];
	UCHAR		MbJumper[CTAB_OLDRRD_ATMB];
	UCHAR		MemCreg[CTAB_OLDRRD_MAXRAMBRDS];
	UCHAR		Slot[CTAB_OLDRRD_MAXSLOTS];
} CBUS_CONFIGURATION_T, *PCBUS_CONFIGURATION;

typedef struct _cbus_ext_cfg_header_t {
	ULONG		ExtCfgCheckword;
	ULONG		ExtCfgLength;
} CBUS_EXT_CFG_HEADER_T, *PCBUS_EXT_CFG_HEADER;

#define	CTAB_EXT_CHECKWORD		0xfeedbeef
typedef struct _cbus_processor_configuration_t {
	ULONG		ProcType:4;
	ULONG		ProcAttr:4;
	ULONG		IoFunction:8;
	ULONG		IoAttr:8;
	ULONG		Reserved:8;
} CBUS_PROCESSOR_CONFIGURATION_T, *PCBUS_PROCESSOR_CONFIGURATION;

#define CTAB_EXT_VENDOR_INFO	0xbeeff00d
typedef struct _cbus_oem_rom_information_t {
	USHORT		OemNumber;
	USHORT		OemRomVersion;
	USHORT		OemRomRelease;
	USHORT		OemRomRevision;
} CBUS_OEM_ROM_INFORMATION_T, *PCBUS_OEM_ROM_INFORMATION;


#define CTAB_EXT_MEM_BOARD		0xdeadface
typedef struct _cbus_ext_memory_board_t {
	ULONG		MemStart;
	ULONG		MemSize;
	USHORT		MemAttr;
	UCHAR		MemBoardType;
	UCHAR		Reserved;
} CBUS_EXT_MEMORY_BOARD_T, *PCBUS_EXT_MEMORY_BOARD;

#define CTAB_EXT_CFG_OVERRIDE	0xdeedcafe
typedef struct _cbus_ext_cfg_override_t {
	ULONG		BaseRam;
	ULONG		MemoryCeiling;
	ULONG		ResetVec;
	ULONG		CbusIo;

	UCHAR		BootId;
	UCHAR		UseHoles;
	UCHAR		RrdArb;
	UCHAR		NonStdEcc;
	ULONG		CReset;
	ULONG		CResetVal;
	ULONG		SReset;

	ULONG		SResetVal;
	ULONG		Contend;
#define IntrControlMask		Contend
	ULONG		ContendVal;
#define FaultControlMask	ContendVal
	ULONG		SetIda;
#define Cbus2Features		SetIda

	ULONG		SetIdaVal;
#define Control8259Mode		SetIdaVal
	ULONG		CSwi;
#define Control8259ModeVal	CSwi
	ULONG		CSwiVal;
	ULONG		SSwi;

	ULONG		SSwiVal;
	ULONG		CNmi;
	ULONG		CNmiVal;
	ULONG		SNmi;

	ULONG		SNmiVal;
	ULONG		SLed;
	ULONG		SLedVal;
	ULONG		CLed;

	ULONG		CLedVal;
	ULONG		MachineType;
	ULONG		SupportedEnvironments;
	ULONG		BroadcastId;

} CBUS_EXT_CFG_OVERRIDE_T, *PCBUS_EXT_CFG_OVERRIDE;

#define CTAB_EXT_ID_INFO		0x01badcab
typedef struct _cbus_ext_id_info_t {
	ULONG		Id:7;
	//
	// pm == 1 indicates CPU, pm == 0 indicates non-CPU (ie: memory or I/O).
	//
	ULONG		Pm:1;
	ULONG		ProcType:4;
	ULONG		ProcAttr:4;
	//
	// IoFunction != 0 indicates I/O,
	// IoFunction == 0 or 9 indicates memory.
	//
	ULONG		IoFunction:8;
	//
	// IoAttr can pertain to an I/O card or memory card.
	//
	ULONG		IoAttr:8;

	//
	// PelStart & PelSize can pertain to a CPU card,
	// I/O card or memory card.
	//
	ULONG		PelStart;
	ULONG		PelSize;
	ULONG		PelFeatures;
	//
	// below two fields can pertain to an I/O card or memory card.
	//
	ULONG		IoStart;

	ULONG		IoSize;

} CBUS_EXT_ID_INFO_T, *PCBUS_EXT_ID_INFO;

#define CTAB_EXT_CFG_END		0

typedef struct _cbus_hardware_info_t {
	CBUS_PROCESSOR_CONFIGURATION_T ProcessorConfiguration[CTAB_OLDRRD_MAXSLOTS];
	CBUS_OEM_ROM_INFORMATION_T		OemRomInfo;
	CBUS_EXT_MEMORY_BOARD_T			ExtMemBoard[CTAB_OLDRRD_MAXSLOTS];
} CBUS_HARDWARE_INFO_T, *PCBUS_HARDWARE_INFO;

#define	CTAB_ROM(number, version, release)			\
	((CbusHwInfo.OemRomInfo.OemNumber == number) &&		\
	(CbusHwInfo.OemRomInfo.OemRomVersion == version) &&	\
	(CbusHwInfo.OemRomInfo.OemRomRelease == release))

#if defined(__cplusplus)
	}
#endif

#endif // _PSM_CBUS_CBUS_TABLES_H
