#ident	"@(#)kern-i386at:svc/corollary.h	1.5.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _SYS_MP_COROLLARY_H
#define _SYS_MP_COROLLARY_H
#ifdef CISCCS
#ident "@(#) corollary.h 1.9 91/10/29    "
#endif
#ifdef __STDC__
#pragma comment(exestr, "@(#) corollary.h 23.2 91/03/03 ")
#else
#ident "@(#) corollary.h 23.2 91/03/03 "
#endif

/*
 *      Copyright (C) Corollary, Inc., 1986, 1987, 1988, 1989, 1990, 1991.
 *      All Rights Reserved.
 *      This Module contains Proprietary Information of
 *      Corollary, Inc., and should be treated as Confidential.
 */

/*
 *	Corollary architecture system defines
 */

/*
 * Processor Types - counting number
 */
#define	PT_NO_PROCESSOR	0x0
#define PT_386		0x1
#define PT_486		0x2
#define PT_586		0x3

/*
 * Processor Attributes - bit field
 */
#define	PA_CACHE_OFF	0x0
#define	PA_CACHE_ON	0x1

/*
 * I/O Function - counting number
 */
#define	IOF_NO_IO		0x0
#define	IOF_SIO			0x1
#define	IOF_SCSI		0x2
#define	IOF_COROLLARY_RESERVED	0x3
#define	IOF_ISA_BRIDGE		0x4
#define	IOF_EISA_BRIDGE		0x5
#define	IOF_HODGE		0x6
#define	IOF_P2			0x7
#define	IOF_INVALID_ENTRY	0x8	/* use to denote whole entry is invalid,
					 note that pm must equal zero as well.*/
#define	IOF_MEMORY		0x9

/*
 * Bit fields of pel_features, independent of whether pm indicates it
 * has an attached processor or not.  new for XM/NT.
 */
#define ELEMENT_SIO		0x00001		/* SIO present */
#define ELEMENT_SCSI		0x00002		/* SCSI present */
#define ELEMENT_IOBUS		0x00004		/* IO bus is accessible */
#define ELEMENT_BRIDGE		0x00008		/* IO bus Bridge */
#define ELEMENT_HAS_8259	0x00010		/* local 8259s present */
#define ELEMENT_HAS_CBC		0x00020		/* local Corollary CBC */
#define ELEMENT_HAS_APIC	0x00040		/* local Intel APIC */
#define ELEMENT_WITH_IO		0x00080		/* some extra I/O device here
						 this could be SCSI, SIO, etc */
#define ELEMENT_RRD_RESERVED	0x20000		/* Old RRDs used this */

/*
 * Due to backwards compatibility, the check for an I/O
 * device is somewhat awkward.
 */

#define ELEMENT_HAS_IO		(ELEMENT_SIO | ELEMENT_SCSI | ELEMENT_WITH_IO)

/*
 * Bit fields of machine types
 */
#define	MACHINE_CBUS1		0x1		/* Original C-bus 1 */
#define	MACHINE_CBUS1_XM	0x2		/* XM C-bus 1 */
#define	MACHINE_CBUS2		0x4		/* C-bus 2 */

/*
 * Bit fields of supported environment types
 */
#define	SCO_UNIX		0x1
#define	USL_UNIX		0x2
#define	WINDOWS_NT		0x4
#define	NOVELL			0x8


#define IOA_BOOT_BRIDGE		0x1

/*
 * cbus OEM number - counting number
 */
#define CBUS_OEM_COROLLARY	1
#define CBUS_OEM_DEC		2
#define CBUS_OEM_ZENITH		3
#define CBUS_OEM_MITAC		4
#define CBUS_OEM_ALR		5
#define CBUS_OEM_SNI		6
#define CBUS_OEM_OLIVETTI_PCI	7
#define CBUS_OEM_IBM_MCA	8
#define CBUS_OEM_CHEN		9

/*
 * OEM sub class - counting number
 */
#define MP_CRLLRYISA		1
#define MP_CRLLRYEISA		2
#define MP_CBUS2EISA		3

#define MB(n)		(1024L * 1024L * (unsigned long)(n))

#define OLDRRD_MAXMB	64		/* max cbus Mb supported by older rrd */
#define OLDRRD_ATMB	16
#define OLDRRD_MAXRAMBOARDS	4
#define OLDRRD_MAXSLOTS	16

struct configuration 
{
	unsigned int	checkword;
	char		mb[OLDRRD_MAXMB];
	char		mbj[OLDRRD_ATMB];
	unsigned char	memcreg[OLDRRD_MAXRAMBOARDS];
	char		slot[OLDRRD_MAXSLOTS];
};

extern struct configuration configuration;


struct ext_cfg_header
{
	unsigned 	ext_cfg_checkword;
	unsigned 	ext_cfg_length;
};


#define	EXT_CHECKWORD	0xfeedbeef
struct processor_configuration
{
	unsigned long	proc_type:4;
	unsigned long	proc_attr:4;
	unsigned long	io_function:8;
	unsigned long	io_attr:8;
	unsigned long	reserved:8;
};


#define EXT_VENDOR_INFO	0xbeeff00d
struct oem_rom_information 
{
	unsigned short	oem_number;
	unsigned short	oem_rom_version;
	unsigned short	oem_rom_release;
	unsigned short	oem_rom_revision;
};

#define	MBT_COROLLARY1	0
#define	MBT_COROLLARY2	1


#define EXT_MEM_BOARD	0xdeadface
struct ext_memory_board
{
	unsigned long	mem_start;
	unsigned long	mem_size;
	unsigned short	mem_attr;
	unsigned char	mem_board_type;
	unsigned char	reserved;
};

#define EXT_CFG_OVERRIDE	0xdeedcafe
struct ext_cfg_override
{
	unsigned int	baseram;
	unsigned int	memory_ceiling;
	unsigned int	resetvec;
	unsigned int	cbusio;

	unsigned char	bootid;
	unsigned char	useholes;
	unsigned char	rrdarb;
	unsigned char	nonstdecc;
	unsigned int	ci_creset;
	unsigned int	ci_creset_val;
	unsigned int	ci_sreset;

	unsigned int	ci_sreset_val;
	unsigned int	ci_contend;
	unsigned int	ci_contend_val;
	unsigned int	ci_setida;

	unsigned int	ci_setida_val;
	unsigned int	ci_cswi;
	unsigned int	ci_cswi_val;
	unsigned int	ci_sswi;

	unsigned int	ci_sswi_val;
	unsigned int	ci_cnmi;
	unsigned int	ci_cnmi_val;
	unsigned int	ci_snmi;

	unsigned int	ci_snmi_val;
	unsigned int	ci_sled;
	unsigned int	ci_sled_val;
	unsigned int	ci_cled;

	unsigned int	ci_cled_val;
	unsigned int	ci_machine_type;		/* new for XM/NT */
	unsigned int	ci_supported_environments;	/* new for XM/NT */
	unsigned int	ci_broadcast_id;		/* new for XM/NT */

};
extern struct ext_cfg_override	corollary_global;

#define EXT_ID_INFO	0x01badcab
struct ext_id_info
{
	unsigned long		id:7;

	/*
	 * pm == 1 indicates CPU, pm == 0 indicates non-CPU (ie: memory or I/O)
	 */
	unsigned long		pm:1;

	unsigned long		proc_type:4;
	unsigned long		proc_attr:4;

	/*
	 * io_function != 0 indicates I/O,
	 * io_function == 0 or 9 indicates memory
	 */
	unsigned long		io_function:8;

	/*
	 * io_attr can pertain to an I/O card or memory card
	 */
	unsigned long		io_attr:8;

	/*
	 * pel_start & pel_size can pertain to a CPU card,
	 * I/O card or memory card
	 */
	unsigned long		pel_start;
	unsigned long		pel_size;

	unsigned long		pel_features;

	/*
	 * below two fields can pertain to an I/O card or memory card
	 */
	unsigned long		io_start;
	unsigned long		io_size;

};

extern int			corollary_valid_ids;
extern struct ext_id_info	corollary_ext_id_info[];

#define EXT_CFG_END	0


struct corollary_hardware_info {
	struct processor_configuration	proc_config[OLDRRD_MAXSLOTS];
	struct oem_rom_information	oem_rom_info;
	struct ext_memory_board		ext_mem_board[OLDRRD_MAXSLOTS];
};

extern struct corollary_hardware_info corollary_hw_info;

#define	SMP_IS_ROM(number,version,release)				\
	((corollary_hw_info.oem_rom_info.oem_number == number) &&	\
	(corollary_hw_info.oem_rom_info.oem_rom_version == version) &&	\
	(corollary_hw_info.oem_rom_info.oem_rom_release == release))

#define ATSIO386	1	/* 386 w/ serial I/O			      */
#define ATSCSI386	2	/* 386 w/ SCSI				      */
#define ATSIO486	3	/* 486 w/ serial I/O			      */
#define ATSIO486C	4	/* 486 w/ serial I/O & internal cache enabled */
#define ATBASE386	5	/* 386 base				      */
#define ATBASE486	6	/* 486 base				      */
#define ATBASE486C	7	/* 486 base w/ internal cache enabled	      */
#define ATP2486C	8	/* 486-P2   w/ internal cache enabled	      */

#define CRLLRY_THREEBOARD	1
#define CRLLRY_TWOBOARD		2

struct memory_pair {
	int	lo;
	int	hi;
};

extern unsigned int	*crllry_memory[];

#define MAXSPANS	25

#define MAXHOLES	10

#define MAXACPUS	32

#define COUTB(addr, reg, value)	(((char *)(addr))[reg] = (char)value)
#define READB(addr)	(*((char *)phystokv(addr)))

#define FLUSH_ALL	0xFFFFF		/* 1MB works for all cbus */

extern int corollary_vendortype;
extern int mpvendor;
extern int mpvendorclass;

extern int corollary_num_cpus;

#define SMP_MAX_IDS		64

#define CI_START		0
#define CI_START_ACK		1
#define CI_ACTIVATE		2

#define	IS_BOOT_ENG(engnum)	((engnum) == BOOTENG)

struct corollary_interrupt {
	int                     irq_line;
	pl_t                    spl_level;
	int                     engnum;
	volatile unsigned       *cookie_addr;
};

int corollary_pres();
void corollary_startcpu();
void corollary_intr_init();
void corollary_set_intr();
void corollary_clr_intr();
int corollary_main();
void corollary_intr();
void corollary_init();
void corollary_ledon();
void corollary_ledoff();

typedef int	(*int_routine_ptr)();

extern int_routine_ptr		corollary_add_intr();

struct crllry_intbuf {
	char fill[9];
};

extern int corollary_proceed[];

#define IN
#define OUT

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_MP_COROLLARY_H */
