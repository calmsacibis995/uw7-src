#ident	"@(#)kern-i386:util/kdb/scodb/stunv.h	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) 1989-1992 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */

/*
*	structure/union/variables definitions
*______________________________________________________________________
*/




/*
*	files of kernel-loadable stuff
*/
#define		DF_VARIDEF	"vari.def"
#define		DF_STUNDEF	"stun.def"

/*
*	intermediate defs files
*/
#define		IDF_DEF		"stv.idef"
#define		IDF_MAGIC	"idef"
#define		IDF_MAGICL	(sizeof(IDF_MAGIC) - 1)
#define		IDF_VERSION	2

#define		NAMEL		32
#define		VNAMEL		32
#define		NDIM		4

#define		MD_STRUCTS	0x01
#define		MD_VARIABLES	0x02
#define			MD_ALL	(MD_STRUCTS|MD_VARIABLES)

#pragma pack(1)
struct cvari {
	union {
		long	 cvu_tbloffset;
		char	*cvu_memptr;
	}		cv_nameval;
#define			cv_names	cv_nameval.cvu_memptr
#define			cv_nameo	cv_nameval.cvu_tbloffset
	unsigned short	cv_dim[NDIM];
	unsigned short	cv_type;
	unsigned short	cv_size;
	int		cv_index;
};
#define		CVARI_SZ	(sizeof(struct cvari))

struct vari {
	struct cvari	 va_cvari;
#define			 va_names	va_cvari.cv_names
#define			 va_nameo	va_cvari.cv_nameo
#define			 va_dim		va_cvari.cv_dim
#define			 va_type	va_cvari.cv_type
#define			 va_size	va_cvari.cv_size
#define			 va_index	va_cvari.cv_index
	char		 va_name[NAMEL];
	char		 va_tag[NAMEL];
	struct vari	*va_next;
};
#define		VARI_SZ	(sizeof(struct vari) - 4)

#define		SNF_FIELD	0x01
struct cstel {
	struct cvari	ce_cvari;
#define			ce_nameo	ce_cvari.cv_nameo
#define			ce_names	ce_cvari.cv_names
#define			ce_dim		ce_cvari.cv_dim
#define			ce_type		ce_cvari.cv_type
#define			ce_size		ce_cvari.cv_size
#define			ce_index	ce_cvari.cv_index
	short		ce_flags;
	unsigned short	ce_offset;
};
#define		CSTEL_SZ	(sizeof(struct cstel))
struct stel {
	struct cstel	 sl_cstel;
#define			 sl_nameo	sl_cstel.ce_nameo
#define			 sl_names	sl_cstel.ce_names
#define			 sl_dim		sl_cstel.ce_dim
#define			 sl_type	sl_cstel.ce_type
#define			 sl_size	sl_cstel.ce_size
#define			 sl_index	sl_cstel.ce_index
#define			 sl_flags	sl_cstel.ce_flags
#define			 sl_offset	sl_cstel.ce_offset
	char		 sl_name[NAMEL];
	char		 sl_tag[NAMEL];
	struct stel	*sl_next;
};
#define		STEL_SZ	(sizeof(struct stel) - 4)

#define		SUF_STRUCT	001
#define		SUF_UNION	002

/*
*	structure pointer
*/
struct cstun {
	union {
		long	 spu_tbloffset;
		char	*spu_memptr;
	}	cs_nameval;		/*  5	4 */
#define		cs_names	cs_nameval.spu_memptr
#define		cs_nameo	cs_nameval.spu_tbloffset
	union {
		long		 spu_offset;
		struct cstel	*spu_cstel;
	}	cs_stel;
#define		cs_offset	cs_stel.spu_offset
#define		cs_cstel	cs_stel.spu_cstel
	short	cs_flags;
	short	cs_size;
	short	cs_nmel;
};
#define		CSTUN_SZ	(sizeof(struct cstun))

struct stun {
	short		 st_flags;
	short		 st_size;
	short		 st_nmel;
	char		 st_name[NAMEL];
	struct stel	*st_stels;
	struct stun	*st_next;
};
#define		STUN_SZ	(sizeof(struct stun) - 8)

#pragma pack()

struct btype {
	char	*bt_nm;
	int	 bt_sz;
};

/*
*	input line from /lib/comp
*/
struct line {
	char		l_name[NAMEL];
	char		l_sclass;
	int		l_size;
	int		l_offset;
	unsigned short	l_type;
	unsigned short	l_dim[NDIM];
	char		l_tag[NAMEL];
};
