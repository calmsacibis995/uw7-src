#ident	"@(#)debugger:libexecon/i386/Reg1.C	1.3"

// Reg1.C -- register names and attributes, machine specific data (i386)

#include "Reg.h"
#include "Itype.h"
#include <sys/regset.h>

RegAttrs regs[] = {
//
//	ref		name		size		flags	stype	offset
//
	REG_EAX,	"%eax",		4,		0,	Suint4,	EAX,
	REG_ECX,	"%ecx",		4,		0,	Suint4, ECX,
	REG_EDX,	"%edx",		4,		0,      Suint4, EDX,
	REG_EBX,	"%ebx",		4,		0,	Suint4, EBX,
//
//	Stack registers
//
	REG_ESP,	"%esp",		4,		0,	Suint4, UESP,
	REG_EBP,	"%ebp",		4,		0,	Suint4, EBP,
	REG_ESI,	"%esi",		4,		0,	Suint4, ESI,
	REG_EDI,	"%edi",		4,		0,	Suint4, EDI,
//
//	Instruction Pointer register
//
	REG_EIP,	"%eip",		4,		0,	Suint4, EIP,
//
//	Flags register
//
	REG_EFLAGS,	"%eflags",	4,		0,	Suint4, EFL,
//	Special floating-point registers
	REG_FPSW,	"%fpsw",	4,	FPREG,	Suint4,	0,
	REG_FPCW,	"%fpcw",	4,	FPREG,	Suint4,	0,
	REG_FPIP,	"%fpip",	4,	FPREG,	Suint4,	0,
	REG_FPDP,	"%fpdp",	4,	FPREG,	Suint4,	0,
//
//	floating point stack
//
	REG_XR0,	"%st0",	10,		FPREG,	Sxfloat, 0,
	REG_XR1,	"%st1",	10,		FPREG,	Sxfloat, 0,
	REG_XR2,	"%st2",	10,		FPREG,	Sxfloat, 0,
	REG_XR3,	"%st3",	10,		FPREG,	Sxfloat, 0,
	REG_XR4,	"%st4",	10,		FPREG,	Sxfloat, 0,
	REG_XR5,	"%st5",	10,		FPREG,	Sxfloat, 0,
	REG_XR6,	"%st6",	10,		FPREG,	Sxfloat, 0,
	REG_XR7,	"%st7",	10,		FPREG,	Sxfloat, 0,
//
// end marker
//
	REG_UNK,	0,		0,		0,	SINVALID,	0
};
