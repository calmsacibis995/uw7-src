#ident	"@(#)mosy-defs.h	1.2"
#ident	"$Header$"
/*      @(#)mosy-defs.h	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      @(#)mosy-defs.h	6.1 Lachman System V STREAMS TCP  source        */
/*      SCCS IDENTIFICATION        */
/* mosy-defs.h - definitions for mosy */

/*
 *      System V STREAMS TCP - Release 5.0
 *
 *  Copyright 1992 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 * 
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <sys/types.h>
#include "snmp.h"


OID	ode2oid (), str2oid ();
int	oid_free ();
char   *oid2ode_aux ();
#define	oid2ode(i)	oid2ode_aux ((i), 1)

typedef	u_char	PElementClass;

#define	PE_CLASS_UNIV	0x0	/*   Universal */
#define	PE_CLASS_APPL	0x1	/*   Application-wide */
#define	PE_CLASS_CONT	0x2	/*   Context-specific */
#define	PE_CLASS_PRIV	0x3	/*   Private-use */

#define	PE_PRIM_ENCR		0x00b	/*   Encrypted */
#define	PE_REAL_INFINITY	99.e99	/*   Largest number */

struct tuple {
    int     t_type;
    char   *t_class;
    char   *t_form;
    char   *t_id;
    PElementClass t_classnum;
    int	    t_idnum;
};

typedef struct ypv {
    int     yv_code;
#define	YV_UNDF		0x00	/* ??? */
#define	YV_NUMBER	0x01	/* LITNUMBER */
#define	YV_BOOL		0x02	/* TRUE | FALSE */
#define	YV_STRING	0x03	/* LITSTRING */
#define	YV_IDEFINED	0x04	/* ID */
#define	YV_IDLIST	0x05	/* IdentifierList */
#define	YV_VALIST	0x06	/* { Values } */
#define	YV_NULL		0x07	/* NULL */
#define YV_ABSENT	0x08	/* WITH COMPONENTS .. ABSENT */
#define YV_PRESENT	0x09	/*  "	"	   .. PRESENT */
#define YV_INCLUDES	0x0a	/* INCLUDES ... */
#define YV_WITHCOMPS	0x0b	/* WITH COMPONENTS */
#define	YV_OIDLIST	0x0c	/* { object identifier } */
#define YV_REAL		0x0d	/* real value */

    union {
	int	    yv_un_number;		/* code = YV_NUMBER
						   code = YV_BOOL */

	double	    yv_un_real;			/* code = YV_REAL */

	char	   *yv_un_string;		/* code = YV_STRING */

	struct {				/* code = YV_IDEFINED */
	    char   *yv_st_module;
	    char   *yv_st_modid;
	    char   *yv_st_identifier;
	}		yv_st;

        struct ypv *yv_un_idlist;		/* code = YV_IDLIST
						   code = YV_VALIST
						   code = YV_OIDLIST */
    }                   yv_un;
#define	yv_number	yv_un.yv_un_number
#define	yv_string	yv_un.yv_un_string
#define	yv_identifier	yv_un.yv_st.yv_st_identifier
#define	yv_module	yv_un.yv_st.yv_st_module
#define yv_modid	yv_un.yv_st.yv_st_modid
#define yv_idlist	yv_un.yv_un_idlist
#define yv_real		yv_un.yv_un_real

    char   *yv_action;
    int	    yv_act_lineno;

    int	    yv_flags;
#define	YV_NOFLAGS	0x00	/* no flags */
#define	YV_ID		0x01	/* ID Value */
#define	YV_NAMED	0x02	/* NamedNumber */
#define	YV_TYPE		0x04	/* TYPE Value */
#define	YV_BOUND	0x08	/* named value */
#define	YVBITS	"\020\01ID\02NAMED\03TYPE\04BOUND"

    char   *yv_id;				/* flags & YV_ID */

    char   *yv_named;				/* flags & YV_NAMED */

    struct ype *yv_type;			/* flags & YV_TYPE */

    struct ypv *yv_next;
}			ypv, *YV;
#define	NULLYV	((YV) 0)

YV	new_value (), add_value (), copy_value ();

/*  */

typedef struct ypt {
    PElementClass   yt_class;

    YV		    yt_value;
}			ypt, *YT;
#define	NULLYT	((YT) 0)

YT new_tag (PElementClass class);

/*  */

typedef struct ype {
    int     yp_code;
#define	YP_UNDF		0x00	/* type not yet known */
#define	YP_BOOL		0x01	/* BOOLEAN */
#define	YP_INT		0x02	/* INTEGER */
#define	YP_INTLIST	0x03	/* INTEGER [ NamedNumberList ] */
#define	YP_BIT		0x04	/* BITSTRING */
#define	YP_BITLIST	0x05	/* BITSTRING [ NamedNumberList ] */
#define	YP_OCT		0x06	/* OCTETSTRING */
#define	YP_NULL		0x07	/* NULL */
#define	YP_SEQ		0x08	/* SEQUENCE */
#define	YP_SEQTYPE	0x09	/* SEQUENCE OF Type */
#define	YP_SEQLIST	0x0a	/* SEQUENCE [ ElementTypes ] */
#define	YP_SET		0x0b	/* SET */
#define	YP_SETTYPE	0x0c	/* SET OF Type */
#define	YP_SETLIST	0x0d	/* SET [ MemberTypes ] */
#define	YP_CHOICE	0x0e	/* CHOICE [ AlternativeTypeList ] */
#define	YP_ANY		0x0f	/* ANY */
#define	YP_OID		0x10	/* OBJECT IDENTIFIER */
#define	YP_IDEFINED	0x11	/* identifier */
#define YP_ENUMLIST	0x12	/* ENUMERATED */
#define YP_REAL		0x13	/* Real (floating-point) */

    int     yp_direction;
#define YP_DECODER	0x01
#define YP_ENCODER	0x02
#define	YP_PRINTER	0x04

    union {
	struct {				/* code = YP_IDEFINED */
	    char   *yp_st_module;		    /* module name */
	    OID	    yp_st_modid;		    /* module id */
	    char   *yp_st_identifier;		    /* definition name */
	}		yp_st;

	struct ype *yp_un_type;			/* code = YP_SEQTYPE
						   code = YP_SEQLIST
						   code = YP_SETTYPE
						   code = YP_SETLIST
						   code = YP_CHOICE */

	YV	    yp_un_value;		/* code = YP_INTLIST
						   code = YP_BITLIST */
    }                   yp_un;
#define	yp_identifier	yp_un.yp_st.yp_st_identifier
#define	yp_module	yp_un.yp_st.yp_st_module
#define yp_modid	yp_un.yp_st.yp_st_modid
#define	yp_type		yp_un.yp_un_type
#define	yp_value	yp_un.yp_un_value

    char   *yp_intexp;		/* expressions to pass (use) as extra */
    char   *yp_strexp;		/* parameters (primitive values) */
    char    yp_prfexp;

    char   *yp_declexp;
    char   *yp_varexp;

    char   *yp_structname;
    char   *yp_ptrname;

    char   *yp_param_type;

    char   *yp_action0;
    int     yp_act0_lineno;

    char   *yp_action05;
    int	    yp_act05_lineno;

    char   *yp_action1;
    int	    yp_act1_lineno;

    char   *yp_action2;
    int	    yp_act2_lineno;

    char   *yp_action3;
    int	    yp_act3_lineno;

    int     yp_flags;
#define	YP_NOFLAGS	0x0000	/* no flags */
#define	YP_OPTIONAL	0x0001	/* OPTIONAL */
#define	YP_COMPONENTS	0x0002	/* COMPONENTS OF */
#define	YP_IMPLICIT	0x0004	/* IMPLICIT */
#define	YP_DEFAULT	0x0008	/* DEFAULT */
#define	YP_ID		0x0010	/* ID */
#define	YP_TAG		0x0020	/* Tag */
#define	YP_BOUND	0x0040	/* ID LANGLE */
#define	YP_PULLEDUP	0x0080	/* member is a choice */
#define YP_PARMVAL	0x0100	/* value to be passed to parm is present */
#define YP_CONTROLLED	0x0200	/* encoding item has a controller */
#define	YP_OPTCONTROL	0x0400	/*   .. */
#define	YP_ACTION1	0x0800	/* action1 acted upon */
#define	YP_PARMISOID	0x1000	/* value to be passed to parm is OID */
#define YP_ENCRYPTED	0x2000	/* encypted - which is a bit hazy */
#define YP_IMPORTED	0x4000  /* value imported from another module */
#define YP_EXPORTED	0x8000  /* value exported to another module */
#define	YPBITS	"\020\01OPTIONAL\02COMPONENTS\03IMPLICIT\04DEFAULT\05ID\06TAG\
\07BOUND\010PULLEDUP\011PARMVAL\012CONTROLLED\013OPTCONTROL\
\014ACTION1\015PARMISOID\016ENCRYPTED\017IMPORTED\020EXPORTED"

    YV	    yp_default;				/* flags & YP_DEFAULT */

    char   *yp_id;				/* flags & YP_ID */

    YT	    yp_tag;				/* flags & YP_TAG */

    char   *yp_bound;				/* flags & YP_BOUND */

    char   *yp_parm;				/* flags & YP_PARMVAL */

    char   *yp_control;				/* flags & YP_CONTROLLED */

    char   *yp_optcontrol;			/* flags & YP_OPTCONTROL */

    char   *yp_offset;

    struct ype *yp_next;
} 			ype, *YP;
#define	NULLYP	((YP) 0)

YP	new_type (), add_type (), copy_type ();

char   *new_string ();

#define	TBL_EXPORT	0
#define TBL_IMPORT	1
#define MAX_TBLS	2

extern int tagcontrol;
#define TAG_UNKNOWN 	0
#define TAG_IMPLICIT	1
#define TAG_EXPLICIT	2

#define CH_FULLY	0
#define CH_PARTIAL	1

typedef struct yop {
    char   *yo_name;

    YP	    yo_arg;
    YP	    yo_result;
    YV	    yo_errors;
    YV	    yo_linked;

    int	    yo_opcode;
}		yop, *YO;
#define	NULLYO	((YO) 0)


typedef struct yerr {
    char   *ye_name;

    YP	    ye_param;

    int	    ye_errcode;

    int	    ye_offset;
}	    yerr, *YE;
#define	NULLYE	((YE) 0)

/*  */

extern int yysection;
extern char *yyencpref;
extern char *yydecpref;
extern char *yyprfpref;
extern char *yyencdflt;
extern char *yydecdflt;
extern char *yyprfdflt;

extern int yydebug;
extern int yylineno;

#ifndef	HPUX
extern char yytext[];
#else
extern unsigned char yytext[];
#endif

extern char *mymodule;

extern OID   mymoduleid;

extern char *bflag;
extern int   Cflag;
extern int   dflag;
extern int   Pflag;
extern char *sysin;

extern char *module_actions;

OID	addoid ();
OID	int2oid ();
OID	oidlookup ();
char	*oidname ();
char	*oidprint ();

extern int errno;
