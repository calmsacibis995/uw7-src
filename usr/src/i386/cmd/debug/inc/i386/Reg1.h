#ifndef REG1_H
#define REG1_H
#ident	"@(#)debugger:inc/i386/Reg1.h	1.3"

//
// NAME
//	Reg1.h
//
// ABSTRACT
//	Register names and indices (machine dependent)
//
// DESCRIPTION
//	Each machine has its own set of register names.  This header file
//	defines the names for the i386, and 80387 co-processor registers.
//  It is included by "Reg.h"
//

//
//	General Purpose registers 
//	(enumerated according to the Intel reference manual pg. 17-5)
//
#define REG_EAX  0
#define REG_ECX  1
#define REG_EDX  2
#define REG_EBX  3
#define REG_ESI  6
#define REG_EDI  7
//
//	Stack register
//
#define REG_ESP  4
#define REG_EBP  5
//
//	Instruction Pointer register
//
#define REG_EIP     8
//
//	Flags Register
//
#define REG_EFLAGS  9
//
//	80387 Floating point registers
//	These registers are 80 bit registers
//
#define FP_STACK	10

// special floating-point control and status registers
#define REG_FPSW	10
#define REG_FPCW	11
#define REG_FPIP	12
#define REG_FPDP	13

// floating-point registers
#define REG_XR0		33
#define REG_XR1		34
#define REG_XR2		35
#define REG_XR3		36
#define REG_XR4		37
#define REG_XR5		38
#define REG_XR6		39
#define REG_XR7		40
#define REG_XWD		43
// 
//  synonyms
//
#define REG_PC REG_EIP
#define REG_FP REG_EBP
#define REG_AP REG_EBP
#define REG_SP REG_ESP

// are stack pointers equivalent before a call and
// after return - check that we are not in a later frame
#define COMPARE_SP(osp, nsp)	((osp) <= (nsp))
//
//
#define FP_INDEX	14

#endif  /*REG1_H */
