#ident	"@(#)cvtomf:symbol.h	1.1"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/* Enhanced Application Compatibility Support */
/* $$SYMBOLS section record templates -  ISLAND (OMF) symbolic debug data */

#pragma pack(1)
struct psr {	/* Procedure start record */
	long	offset;
	short	type;
	short	length;
	short	d_start;
	short	d_end;
	short	reserved;
	char	near_or_far_p;
	char	name_len;
};

struct bsr {	/* Block Start record */
	long	offset;
	short	length;
	char	name_len;
};

struct bpr {	/* BP-relative Record */
	long	offset;
	short	type;
	char	name_len;
};

struct clr {	/* Code Label Record */
	long	offset;
	char	near_or_far_p;
	char	name_len;
};

struct ldr {	/* Local Data Record */
	long	offset;
	short	seg;
	short	type;
	char	name_len;
};

struct rsr {	/* Register Symbol Record */
	short	type;
	char	reg;
	char	name_len;
};

struct csr {	/* Constant Symbol Record */
	short	type;
	char	val_len;
	/* char	val[val_len];
	   char	name_len;
	   char name[name_len; */
};

struct typerec {	/* Typedef record */
	char		linkage;
	short		length;
	unsigned char	leaf;
};
#pragma pack()


/* OMF/ISLAND i386 Register values */

#define DL_REG_AX	8
#define DL_REG_CX	9
#define DL_REG_DX	10
#define DL_REG_BX	11
#define DL_REG_SI	14
#define DL_REG_DI	15

/* COFF i386 register values used with C_REG and C_REGPARM */
#define C_EAX	0
#define C_ECX	1
#define C_EDX	2
#define C_EBX	3
#define C_ESI	6
#define C_EDI	7

#define C_INVALID	(-1)

/* for line number conversions */
#define LINADJ	1

/* compiler dependant: determines storage space for patching offsets */
#define MAX_BLOCK_NEST	64

/* record for storing $$SYMBOLS fixup records */
struct symfix {
	unsigned long offset;
	int ext;
	int scn;
};
struct symfix *findsfix();
/* End Enhanced Application Compatibility Support */
