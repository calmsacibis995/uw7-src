#ident	"@(#)optim:i386/fp_timings.c	1.14"
#include "defs.h"
#include "fp_timings.h"

static times_t  fld	=		{14	,	4	,	1	};
static times_t  flds	=		{20	,	3	,	1	};
static times_t  fldl	=		{25	,	3	,	1	};
static times_t  fldt	=		{44	,	6	,	3	};
static times_t  fadd	=		{27	,	10	,	1	};
static times_t  fadds	=		{28	,	10	,	1	};
static times_t  faddl	=		{33	,	10	,	1	};
static times_t  faddp	=		{27	,	10	,	1	};
static times_t  fsub	=		{30	,	10	,	1	};
static times_t  fsubs	=		{28	,	10	,	1	};
static times_t  fsubl	=		{32	,	10	,	1	};
static times_t  fsubp	=		{30	,	10	,	1	};
static times_t  fsubr	=		{30	,	10	,	1	};
static times_t  fsubrs	=		{28	,	10	,	1	};
static times_t  fsubrl	=		{32	,	10	,	1	};
static times_t  fsubrp	=		{30	,	10	,	1	};
static times_t  fmul	=		{43	,	16	,	1	};
static times_t  fmuls	=		{31	,	11	,	1	};
static times_t  fmull	=		{45	,	14	,	1	};
static times_t  fmulp	=		{43	,	16	,	1	};
static times_t  fdiv	=		{88	,	73	,	1	};
static times_t  fdivs	=		{89	,	73	,	18	};
static times_t  fdivl	=		{94	,	73	,	32	};
static times_t  fdivp	=		{88	,	73	,	1	};
static times_t  fdivr	=		{88	,	73	,	1	};
static times_t  fdivrs	=		{89	,	73	,	18	};
static times_t  fdivrl	=		{94	,	73	,	32	};
static times_t  fdivrp	=		{88	,	73	,	1	};
static times_t  fst	=		{	11	,	3	,	1	};
static times_t  fsts	=		{44	,	7	,	2	};
static times_t  fstl	=		{45	,	8	,	2	};
static times_t  fstp	=		{12	,	3	,	1	};
static times_t  fstps	=		{44	,	7	,	2	};
static times_t  fstpl	=		{45	,	8	,	2	};
static times_t  fstpt	=		{53	,	6	,	1	};
static times_t  fcom	=		{24	,	4	,	1	};
static times_t  fcoms	=		{26	,	4	,	1	};
static times_t  fcoml	=		{31	,	4	,	1	};
static times_t  fcomp	=		{26	,	4	,	1	};
static times_t  fcomps	=		{26	,	4	,	1	};
static times_t  fcompl	=		{31	,	4	,	1	};
static times_t	fild	=		{63	,	14	,	1	};
static times_t	fildl	=		{48	,	10	,	1	};
static times_t	fildll	=		{61	,	14	,	1	};
static times_t	fiadd	=		{78	,	24	,	4	};
static times_t	fiaddl	=		{64	,	23	,	4	};
static times_t	ficom	=		{73	,	18	,	4	};
static times_t	ficoml	=		{60	,	16	,	4	};
static times_t	ficomp	=		{73	,	18	,	4	};
static times_t	ficompl	=		{60	,	16	,	4	};
static times_t	fidiv	=		{140	,	73	,	42 };
static times_t	fidivl	=		{125	,	73	,	42 };
static times_t	fidivr	=		{140	,	73	,	42 };
static times_t	fidivrl	=		{125	,	73	,	42 };
static times_t	fimul	=		{82	,	25	,	4	};
static times_t	fimull	=		{71	,	23	,	4	};
static times_t	fist	=		{88	,	33	,	6	};
static times_t	fistl	=		{91	,	33	,	6	};
static times_t	fistp	=		{88	,	33	,	6	};
static times_t	fistpl	=		{91	,	33	,	6	};
static times_t	fistpll	=		{88	,	33	,	8	};
static times_t	fisub	=		{77	,	24	,	4	};
static times_t	fisubl	=		{70	,	23	,	4	};
static times_t	fisubr	=		{77	,	24	,	4	};
static times_t	fisubrl	=		{70	,	23	,	4	};

static times_t
timesof(op) unsigned int op;
{
	switch(op) {
		case  FLD:	return fld;
		case  FLDS:	return flds;
		case  FLDL:	return fldl;
		case  FLDT:	return fldt;
		case  FADD:	return	fadd;
		case  FADDS:	return	fadds;
		case  FADDL:	return	faddl;
		case  FADDP:	return	faddp;
		case  FSUB:	return	fsub;
		case  FSUBS:	return	fsubs;
		case  FSUBL:	return	fsubl;
		case  FSUBP:	return	fsubp;
		case  FSUBR:	return	fsubr;
		case  FSUBRS:	return	fsubrs;
		case  FSUBRL:	return	fsubrl;
		case  FSUBRP:	return	fsubrp;
		case  FMUL:	return	fmul;
		case  FMULS:	return	fmuls;
		case  FMULL:	return	fmull;
		case  FMULP:	return	fmulp;
		case  FDIV:	return	fdiv;
		case  FDIVS:	return	fdivs;
		case  FDIVL:	return	fdivl;
		case  FDIVP:	return	fdivp;
		case  FDIVR:	return	fdivr;
		case  FDIVRS:	return	fdivrs;
		case  FDIVRL:	return	fdivrl;
		case  FDIVRP:	return	fdivrp;
		case  FST:	return	fst;
		case  FSTS:	return	fsts;
		case  FSTL:	return	fstl;
		case  FSTP:	return	fstp;
		case  FSTPS:	return	fstps;
		case  FSTPL:	return	fstpl;
		case  FSTPT:	return	fstpt;
		case  FCOM:	return	fcom;
		case  FCOMS:	return	fcoms;
		case  FCOML:	return	fcoml;
		case  FCOMP:	return	fcomp;
		case  FCOMPS:	return	fcomps;
		case  FCOMPL:	return	fcompl;
		case  FIADD:	return	fiadd;
		case  FIADDL:	return	fiaddl;
		case  FICOM:	return	ficom;
		case  FICOML:	return	ficoml;
		case  FICOMP:	return	ficomp;
		case  FICOMPL:	return	ficompl;
		case  FIDIV:	return	fidiv;
		case  FIDIVL:	return	fidivl;
		case  FIDIVR:	return	fidivr;
		case  FIDIVRL:	return	fidivrl;
		case  FILD:	return	fild;
		case  FILDL:	return	fildl;
		case  FILDLL:	return	fildll;
		case  FIMUL:	return	fimul;
		case  FIMULL:	return	fimull;
		case  FIST:	return	fist;
		case  FISTL:	return	fistl;
		case  FISTP:	return	fistp;
		case  FISTPL:	return	fistpl;
		case  FISTPLL:	return	fistpll;
		case  FISUB:	return	fisub;
		case  FISUBL:	return	fisubl;
		case  FISUBR:	return	fisubr;
		case  FISUBRL:	return	fisubrl;
		default: fatal(__FILE__,__LINE__,"timesof: dont know this op %d\n",op);
	}
	/* NOTREACHED */
}/*end timesof*/

static int
timeof(op) unsigned int op;
{
times_t x;
	x = timesof(op);
	if (target_cpu & (blend|P5))
		return x.t_ptm;
	else if (target_cpu == P4)
		return x.t_486;
	else
		return x.t_387;
}/*end timeof*/

static opopcode	fld_c	= { FLD	,	"fld" };
static opopcode	fadd_c	= { FADD , "fadd" };
static opopcode	fsub_c	= { FSUB , "fsub" };
static opopcode	fsubr_c	= { FSUBR , "fsubr" };
static opopcode	fmul_c	= { FMUL , "fmul" };
static opopcode	fdiv_c	= { FDIV , "fdiv" };
static opopcode	fdivr_c	= { FDIVR , "fdivr" };
static opopcode	fst_c	= { FST , "fst" };
static opopcode	fstp_c	= { FSTP , "fstp" };
static opopcode	fcom_c	= { FCOM , "fcom" };
static opopcode	fcomp_c	= { FCOMP , "fcomp" };

opopcode
mem2st(op) unsigned int op;
{
		switch (op) {
			case  FLDS:  case  FLDL: case FLDT:
			case FILD: case FILDL: case FILDLL: return fld_c;
			case FADDS: case FADDL: case FIADD: case FIADDL: return fadd_c;
			case FSUBS: case FSUBL: case FISUB: case FISUBL: return fsub_c;
			case FSUBRS: case FSUBRL: case FISUBR: case FISUBRL: return fsubr_c;
			case FMULS: case FMULL: case FIMUL: case FIMULL: return fmul_c;
			case FDIVS: case FDIVL: case FIDIV: case FIDIVL: return fdiv_c;
			case FDIVRS: case FDIVRL: case FIDIVR: case FIDIVRL: return fdivr_c;
			case  FSTS: case  FSTL: case FIST: case FISTL: return fst_c;
			case FISTP: case FISTPL: case FISTPLL:
			case  FSTPS: case  FSTPL: case  FSTPT: return fstp_c;
			case FCOMS: case FCOML: case FICOM: case FICOML: return fcom_c;
			case FCOMPS: case FCOMPL: case FICOMP: case FICOMPL: return fcomp_c;
			default:
				fatal(__FILE__,__LINE__,"mem2st: dont know this op %d\n",op);
		}
	/* NOTREACHED */
}/*end mem2st*/

int
gain_by_mem2st(op) unsigned int op;
{
int time_in_mem;
opopcode opop;
unsigned int st_op ;
int time_in_st;

	time_in_mem = timeof(op);
	opop = mem2st(op);
	st_op = opop.op;
	time_in_st = timeof(st_op);
	return time_in_mem - time_in_st;
}/*end gain_by_mem2st*/
