#ident	"@(#)debugger:libexecon/i386/RegAccess.C	1.15"

#include "RegAccess.h"
#include "Reg.h"
#include "i_87fp.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Machine.h"
#include "Proctypes.h"
#include "Frame.h"
#include "Fund_type.h"
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include "sys/regset.h"
#include <sys/fp.h>

extern RegAttrs regs[]; // to overcome overloaded name problem
extern void extended2double(void *, double *);

static int emulate_only = -1;
extern int _fp_hw;

RegAccess::RegAccess()
{
	pobj = 0;
	gpreg = 0;
	fpreg = 0;
	core = fpcurrent = gcurrent = 0;
}

int
RegAccess::setup_core( ProcObj *p )
{
	if (emulate_only == -1)
	{
		// Note: we should actually read the value of _fp_hw from
		// the core file, but it might not be available
		emulate_only =  (_fp_hw < FP_HW);
	}
	pobj = p;
	if ((gpreg = pobj->read_greg()) == 0)
		return 0;
	fpreg = pobj->read_fpreg();
	fpcurrent = (fpreg != 0);
	core = gcurrent = 1;
	return 1;
}


int
RegAccess::setup_live( ProcObj *p)
{
	if (emulate_only == -1)
	{
		emulate_only =  (_fp_hw < FP_HW);
	}
	pobj = p;
	return 1;
}

int
RegAccess::update()
{
	if (!core)
	{
		gcurrent = fpcurrent = 0;
		// force float state into u-block
		asm("fnop");
		fpreg = 0;
		gpreg = 0;
	}
	return 1;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	int		i;
	RegAttrs	*rattrs = regattrs(regref);

	if (!pobj || !rattrs)
	{
		return 0;
	}
	if (rattrs->flags != FPREG)
	{
		if (!gcurrent) 
		{
			if ((gpreg = pobj->read_greg()) == 0)
				return 0;
			gcurrent = 1;
		}
		word[0] = gpreg->greg[regs[regref].offset];
	} 
	else if (!fpcurrent && ((fpreg = pobj->read_fpreg()) == 0))
	{
			return 0;
	} 
	else 
	{
		fpcurrent = 1;
		switch(regref)
		{
		case REG_FPSW:
			word[0] = fpreg->fp_state.fp_status;
			break;
		case REG_FPCW:
			word[0] = fpreg->fp_state.fp_control;
			break;
		case REG_FPIP:
			word[0] = fpreg->fp_state.fp_ip;
			break;
		case REG_FPDP:
			word[0] = fpreg->fp_state.fp_data_addr;
			break;
		default:
			i = regref - REG_XR0;
			word[2] = 0;
			memcpy((void *)word, 
				&(fpreg->fp_state.fp_stack[i].ary[0]),
				EXTENDED_SIZE);
		}
	}
	return 1;
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	int		i;
	RegAttrs	*rattrs = regattrs(regref);

	if (!pobj || !rattrs)
	{
		return 0;
	}

	if (rattrs->flags != FPREG)
	{
		if (!gcurrent) 
			gpreg = pobj->read_greg();
		gcurrent = 0;
		if (!gpreg)
			return 0;
		gpreg->greg[regs[regref].offset] = (int)word[0];
		return(pobj->write_greg(gpreg));
	} 
	else 
	{
		if (!fpcurrent) 
			fpreg = pobj->read_fpreg();
		fpcurrent = 0;
		if (!fpreg)
			return 0;
		switch(regref)
		{
		case REG_FPSW:
			fpreg->fp_state.fp_status = (unsigned int)word[0];
			break;
		case REG_FPCW:
			fpreg->fp_state.fp_control = (unsigned int)word[0];
			break;
		case REG_FPIP:
			fpreg->fp_state.fp_ip = (unsigned int)word[0];
			break;
		case REG_FPDP:
			fpreg->fp_state.fp_data_addr = (unsigned int)word[0];
			break;
		default:
			i = regref - REG_XR0;
			memcpy((void *) &(fpreg->fp_state.fp_stack[i].ary[0]),
				(void *)word, EXTENDED_SIZE);
			break;
		}
		return(pobj->write_fpreg(fpreg));
	}
}

Iaddr
RegAccess::getreg( RegRef regref )
{
	long	word[3];

	if (core) 
	{
		if (!readcore(regref, word))
		{
			return 0;
		}
	}
	else 
	{
		if (!readlive(regref, word))
		{
			return 0;
		}
	}
	return word[0];
}

int
RegAccess::readreg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	if (core) 
	{
		if (!readcore(regref, word))
		{
			return 0;
		}
	}
	else 
	{
		if (!readlive(regref, word))
		{
			return 0;
		}
	}
	switch (stype)
	{
	case SINVALID:	return 0;
	case Schar:	itype.ichar = (char)word[0];		break;
	case Suchar:	itype.iuchar = (unsigned char)word[0];	break;
	case Sint1:	itype.iint1 = (char)word[0];		break;
	case Suint1:	itype.iuint1 = (unsigned char)word[0];	break;
	case Sint2:	itype.iint2 = (short)word[0];		break;
	case Suint2:	itype.iuint2 = (unsigned short)word[0];	break;
	case Sint4:	itype.iint4 = word[0];		break;
	case Suint4:	itype.iuint4 = word[0];		break;
	case Saddr:	itype.iaddr = word[0];		break;
	case Sbase:	itype.ibase = word[0];		break;
	case Soffset:	itype.ioffset = word[0];	break;
	case Sxfloat:	itype.rawwords[2] = (int)word[2];
	case Sdfloat:	itype.rawwords[1] = (int)word[1];
	case Ssfloat:	itype.rawwords[0] = (int)word[0];	break;
#if LONG_LONG
	case Sint8:	itype.iint8 = (long long)word[0]; break;
	case Suint8:	itype.iuint8 = (unsigned long long)word[0]; break;
#endif
	default:	return 0;
	}
	return 1;
}

#if LONG_LONG
// read a pair of registers - must be integer regs
int
RegAccess::readreg( RegRef ref1, RegRef ref2, Stype stype, 
	Itype & itype )
{
	long	word1[3];
	long	word2[3];
	RegAttrs	*rattrs;
	union {
		unsigned long long ull;
		unsigned long ul[2];
	} lul;

	rattrs = regattrs(ref1);
	if (!rattrs || rattrs->flags == FPREG)
		return 0;
	rattrs = regattrs(ref2);
	if (!rattrs || rattrs->flags == FPREG)
		return 0;

	if (core) 
	{
		if (!readcore(ref1, word1) || !readcore(ref2, word2))
		{
			return 0;
		}
	}
	else 
	{
		if (!readlive(ref1, word1) || !readlive(ref2, word2))
		{
			return 0;
		}
	}

	lul.ul[0] = word1[0];
	lul.ul[1] = word2[0];

	switch (stype)
	{
	case Schar:	itype.ichar = (char)lul.ull;		break;
	case Suchar:	itype.iuchar = (unsigned char)lul.ull;	break;
	case Sint1:	itype.iint1 = (char)lul.ull;		break;
	case Suint1:	itype.iuint1 = (unsigned char)lul.ull;	break;
	case Sint2:	itype.iint2 = (short)lul.ull;		break;
	case Suint2:	itype.iuint2 = (unsigned short)lul.ull;	break;
	case Sint4:	itype.iint4 = (int)lul.ull;		break;
	case Suint4:	itype.iuint4 = (unsigned int)lul.ull;	break;
	case Saddr:	itype.iaddr = (Iaddr)lul.ull;	break;
	case Sbase:	itype.ibase = (Ibase)lul.ull;		break;
	case Soffset:	itype.ioffset = (Ioffset)lul.ull;	break;
	case Sint8:	itype.iint8 = lul.ull; break;
	case Suint8:	itype.iuint8 = lul.ull; break;
	default:	return 0;
	}
	return 1;
}

// write a pair of registers - must be integer regs
int
RegAccess::writereg( RegRef ref1, RegRef ref2, Stype stype, 
	Itype & itype )
{
	RegAttrs	*rattrs;
	union {
		unsigned long long ull;
		long ul[2];
	} lul;

	if (core) 
	{
		printe(ERR_core_write_regs, E_ERROR);
		return 0;
	}
	rattrs = regattrs(ref1);
	if (!rattrs || rattrs->flags == FPREG)
		return 0;
	rattrs = regattrs(ref2);
	if (!rattrs || rattrs->flags == FPREG)
		return 0;

	switch (stype)
	{
	case Schar:	lul.ull = itype.ichar;		break;
	case Suchar:	lul.ull = itype.iuchar;		break;
	case Sint1:	lul.ull = itype.iint1;		break;
	case Suint1:	lul.ull = itype.iuint1;		break;
	case Sint2:	lul.ull = itype.iint2;		break;
	case Suint2:	lul.ull = itype.iuint2;		break;
	case Sint4:	lul.ull = itype.iint4;		break;
	case Suint4:	lul.ull = itype.iuint4;		break;
	case Suint8:	lul.ull = itype.iuint8;	break;
	case Sint8:	lul.ull = itype.iint8;	break;
	case Saddr:	lul.ull = itype.iaddr;		break;
	case Sbase:	lul.ull = itype.ibase;		break;
	default:	return 0;
	}
	
	return(writelive(ref1, &lul.ul[0]) && 
		writelive(ref2, &lul.ul[1]));
}

#else
int
RegAccess::readreg( RegRef, RegRef, Stype, Itype &)
{
	return 0;
}
int
RegAccess::writereg( RegRef, RegRef, Stype, Itype &)
{
	return 0;
}
#endif

int
RegAccess::readcore( RegRef regref, long * word )
{
	
	RegAttrs		*rattrs = regattrs(regref);
	int i;

	if ( core == 0 || !rattrs)
		return 0;

	if (rattrs->flags != FPREG)
	{
		*word   = gpreg->greg[regs[regref].offset];
	}
	else if (fpcurrent == 0)
	{
		return 0;
	}
	else
	{
		switch(regref)
		{
		case REG_FPSW:
			word[0] = fpreg->fp_state.fp_status;
			break;
		case REG_FPCW:
			word[0] = fpreg->fp_state.fp_control;
			break;
		case REG_FPIP:
			word[0] = fpreg->fp_state.fp_ip;
			break;
		case REG_FPDP:
			word[0] = fpreg->fp_state.fp_data_addr;
			break;
		default:
			i = regref - REG_XR0;
			word[2] = 0;
			memcpy((void *)word, &(fpreg->fp_state.fp_stack[i].ary[0]),
				EXTENDED_SIZE);
		}
	}
	return 1;
}

int
RegAccess::writereg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	switch (stype)
	{
		case SINVALID:	return 0;
		case Schar:	word[0] = itype.ichar;		break;
		case Suchar:	word[0] = itype.iuchar;		break;
		case Sint1:	word[0] = itype.iint1;		break;
		case Suint1:	word[0] = itype.iuint1;		break;
		case Sint2:	word[0] = itype.iint2;		break;
		case Suint2:	word[0] = itype.iuint2;		break;
		case Sint4:	word[0] = itype.iint4;		break;
		case Suint4:	word[0] = itype.iuint4;		break;
#if LONG_LONG
		case Suint8:	word[0] = (long)itype.iuint8;	break;
		case Sint8:	word[0] = (long)itype.iint8;	break;
#endif
		case Saddr:	word[0] = itype.iaddr;		break;
		case Sbase:	word[0] = itype.ibase;		break;
		case Soffset:	word[0] = itype.ioffset;	break;
		case Sxfloat:	word[2] = itype.rawwords[2];
		case Sdfloat:	word[1] = itype.rawwords[1];
		case Ssfloat:	word[0] = itype.rawwords[0];	break;
		default:	return 0;
	}
	if (core) 
	{
		printe(ERR_core_write_regs, E_ERROR);
		return 0;
	}
	else 
	{
		return writelive(regref, word);
	}
}


#define NUM_PER_LINE	3

int
RegAccess::display_regs(Frame *frame)
{
	RegAttrs *p;
	Itype	  x;
	int	  i, k, tag;
#ifdef NO_LONG_DOUBLE
	int	fpregvals[16];
#endif
	static char *tagname[] = {"VALID","ZERO ","INVAL","EMPTY" };

	if (!frame && !gcurrent && !core)
	{
		if ((gpreg = pobj->read_greg()) == 0) 
			return 0;
		gcurrent = 1;
	}
	i = 1;
	for( p = regs;  !(p->flags & FPREG);  p++ ) 
	{
		if (frame)
			frame->readreg( p->ref, Suint4, x );
		else
			readreg( p->ref, Suint4, x );
		if ( i >= NUM_PER_LINE )
		{
			printm(MSG_int_reg_newline, p->name, x.iuint4);
			i = 1;
		}
		else
		{
			i++;
			printm(MSG_int_reg, p->name, x.iuint4);
		}
	}
	if (core != 0) 
	{
		if (fpcurrent == 0)
		{
			printm(MSG_newline);
			return 0;
		}
	} 
	else if (!fpcurrent && ((fpreg = pobj->read_fpreg()) == 0)) 
	{
		printm(MSG_newline);
		return 0;
	} 
	else 
	{
		fpcurrent = 1;
	}
#ifdef NO_LONG_DOUBLE
// target system printf doesn't support long double
// convert to double before printing
	for (i = 0; i < 8; i++ )
		extended2double((void *)&fpreg->fp_state.fp_stack[i],
			(double *)&fpregvals[i*2]);
#endif

	unsigned int fpsp = fpreg->fp_state.fp_status >> 11 & 0x7;
	printm(MSG_int_reg, "%fpsw", fpreg->fp_state.fp_status);
	printm(MSG_int_reg_newline, "%fpcw", fpreg->fp_state.fp_control);
	printm(MSG_int_reg, "%fpip", fpreg->fp_state.fp_ip);
	printm(MSG_int_reg_newline, "%fpdp", fpreg->fp_state.fp_data_addr);
	for (i = 0; i < 8 ; i++ )
	{
		char	fpname[15];
		char	fpval[64];
		// registers are ordered differently in emulator from
		// the way the 387 orders them
		k = emulate_only ? i : (fpsp + i) % 8;
		tag = fpreg->fp_state.fp_tag >> (k * 2) & 0x3 ;
		sprintf(fpname, "%.5s [ %s ]",regs[FP_INDEX+i].name,
					  tagname[tag]);
#ifdef NO_LONG_DOUBLE
		sprintf(fpval, "0x%.4x %.4x %.4x %.4x %.4x ==\t%.14g",
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[8]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[6]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[4]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[2]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[0]),
			*(double *)&fpregvals[i*2]);
#else
// Cfront 1.2 doesn't support long double
// pass the long double as 3 longs
		sprintf(fpval, "0x%.4x %.4x %.4x %.4x %.4x ==\t%.18Lg",
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[8]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[6]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[4]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[2]),
			*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[0]),
			*(unsigned long *)&(fpreg->fp_state.fp_stack[i].ary[0]),
			*(unsigned long *)&(fpreg->fp_state.fp_stack[i].ary[4]),
			(unsigned long)*(unsigned short *)&(fpreg->fp_state.fp_stack[i].ary[8]));
#endif
		printm(MSG_flt_reg, fpname, fpval);
	}
	return 1;
}

Fund_type
regtype(RegRef ref)
{
    RegAttrs *regattr = regattrs(ref);

    switch(regattr->stype)
    {
	case Suint4:	return ft_pointer;
	case Sxfloat:	return ft_xfloat;
	default:
			return ft_none;
    }
}

int
RegAccess::set_pc(Iaddr addr)
{
	Itype	itype;

	itype.iaddr = addr;
	return(writereg(REG_EIP, Saddr, itype));
}
