/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/io_intr.s	1.1.1.2"
	.ident	"$Header$"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/
include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/svc/intr.m4)
include(KBASE/util/debug.m4)

FILE(`io_intr.s')

/
/      Null interrupt routine.  Used to fill in empty slots in the
/      interrupt handler table.
/	Should NEVER be called.
/
ENTRY(intnull)
	ret



ENTRY(devint20)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x20
	jmp	.vectint

ENTRY(devint21)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x21
	jmp	.vectint

ENTRY(devint22)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x22
	jmp	.vectint

ENTRY(devint23)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x23
	jmp	.vectint

ENTRY(devint24)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x24
	jmp	.vectint

ENTRY(devint25)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x25
	jmp	.vectint

ENTRY(devint26)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x26
	jmp	.vectint

ENTRY(devint27)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x27
	jmp	.vectint

ENTRY(devint28)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x28
	jmp	.vectint

ENTRY(devint29)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x29
	jmp	.vectint

ENTRY(devint2a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2a
	jmp	.vectint

ENTRY(devint2b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2b
	jmp	.vectint

ENTRY(devint2c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2c
	jmp	.vectint

ENTRY(devint2d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2d
	jmp	.vectint

ENTRY(devint2e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2e
	jmp	.vectint

ENTRY(devint2f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x2f
	jmp	.vectint

ENTRY(devint30)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x30
	jmp	.vectint

ENTRY(devint31)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x31
	jmp	.vectint

ENTRY(devint32)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x32
	jmp	.vectint

ENTRY(devint33)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x33
	jmp	.vectint

ENTRY(devint34)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x34
	jmp	.vectint

ENTRY(devint35)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x35
	jmp	.vectint

ENTRY(devint36)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x36
	jmp	.vectint

ENTRY(devint37)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x37
	jmp	.vectint

ENTRY(devint38)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x38
	jmp	.vectint

ENTRY(devint39)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x39
	jmp	.vectint

ENTRY(devint3a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3a
	jmp	.vectint

ENTRY(devint3b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3b
	jmp	.vectint

ENTRY(devint3c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3c
	jmp	.vectint

ENTRY(devint3d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3d
	jmp	.vectint

ENTRY(devint3e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3e
	jmp	.vectint

ENTRY(devint3f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x3f
	jmp	.vectint

ENTRY(devint40)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x40
	jmp	.vectint

ENTRY(devint41)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x41
	jmp	.vectint

ENTRY(devint42)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x42
	jmp	.vectint

ENTRY(devint43)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x43
	jmp	.vectint

ENTRY(devint44)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x44
	jmp	.vectint

ENTRY(devint45)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x45
	jmp	.vectint

ENTRY(devint46)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x46
	jmp	.vectint

ENTRY(devint47)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x47
	jmp	.vectint

ENTRY(devint48)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x48
	jmp	.vectint

ENTRY(devint49)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x49
	jmp	.vectint

ENTRY(devint4a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4a
	jmp	.vectint

ENTRY(devint4b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4b
	jmp	.vectint

ENTRY(devint4c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4c
	jmp	.vectint

ENTRY(devint4d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4d
	jmp	.vectint

ENTRY(devint4e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4e
	jmp	.vectint

ENTRY(devint4f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x4f
	jmp	.vectint

ENTRY(devint50)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x50
	jmp	.vectint

ENTRY(devint51)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x51
	jmp	.vectint

ENTRY(devint52)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x52
	jmp	.vectint

ENTRY(devint53)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x53
	jmp	.vectint

ENTRY(devint54)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x54
	jmp	.vectint

ENTRY(devint55)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x55
	jmp	.vectint

ENTRY(devint56)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x56
	jmp	.vectint

ENTRY(devint57)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x57
	jmp	.vectint

ENTRY(devint58)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x58
	jmp	.vectint

ENTRY(devint59)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x59
	jmp	.vectint

ENTRY(devint5a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5a
	jmp	.vectint

ENTRY(devint5b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5b
	jmp	.vectint

ENTRY(devint5c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5c
	jmp	.vectint

ENTRY(devint5d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5d
	jmp	.vectint

ENTRY(devint5e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5e
	jmp	.vectint

ENTRY(devint5f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x5f
	jmp	.vectint

ENTRY(devint60)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x60
	jmp	.vectint

ENTRY(devint61)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x61
	jmp	.vectint

ENTRY(devint62)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x62
	jmp	.vectint

ENTRY(devint63)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x63
	jmp	.vectint

ENTRY(devint64)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x64
	jmp	.vectint

ENTRY(devint65)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x65
	jmp	.vectint

ENTRY(devint66)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x66
	jmp	.vectint

ENTRY(devint67)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x67
	jmp	.vectint

ENTRY(devint68)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x68
	jmp	.vectint

ENTRY(devint69)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x69
	jmp	.vectint

ENTRY(devint6a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6a
	jmp	.vectint

ENTRY(devint6b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6b
	jmp	.vectint

ENTRY(devint6c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6c
	jmp	.vectint

ENTRY(devint6d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6d
	jmp	.vectint

ENTRY(devint6e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6e
	jmp	.vectint

ENTRY(devint6f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x6f
	jmp	.vectint

ENTRY(devint70)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x70
	jmp	.vectint

ENTRY(devint71)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x71
	jmp	.vectint

ENTRY(devint72)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x72
	jmp	.vectint

ENTRY(devint73)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x73
	jmp	.vectint

ENTRY(devint74)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x74
	jmp	.vectint

ENTRY(devint75)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x75
	jmp	.vectint

ENTRY(devint76)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x76
	jmp	.vectint

ENTRY(devint77)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x77
	jmp	.vectint

ENTRY(devint78)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x78
	jmp	.vectint

ENTRY(devint79)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x79
	jmp	.vectint

ENTRY(devint7a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7a
	jmp	.vectint

ENTRY(devint7b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7b
	jmp	.vectint

ENTRY(devint7c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7c
	jmp	.vectint

ENTRY(devint7d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7d
	jmp	.vectint

ENTRY(devint7e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7e
	jmp	.vectint

ENTRY(devint7f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x7f
	jmp	.vectint

ENTRY(devint80)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x80
	jmp	.vectint

ENTRY(devint81)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x81
	jmp	.vectint

ENTRY(devint82)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x82
	jmp	.vectint

ENTRY(devint83)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x83
	jmp	.vectint

ENTRY(devint84)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x84
	jmp	.vectint

ENTRY(devint85)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x85
	jmp	.vectint

ENTRY(devint86)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x86
	jmp	.vectint

ENTRY(devint87)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x87
	jmp	.vectint

ENTRY(devint88)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x88
	jmp	.vectint

ENTRY(devint89)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x89
	jmp	.vectint

ENTRY(devint8a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8a
	jmp	.vectint

ENTRY(devint8b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8b
	jmp	.vectint

ENTRY(devint8c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8c
	jmp	.vectint

ENTRY(devint8d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8d
	jmp	.vectint

ENTRY(devint8e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8e
	jmp	.vectint

ENTRY(devint8f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x8f
	jmp	.vectint

ENTRY(devint90)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x90
	jmp	.vectint

ENTRY(devint91)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x91
	jmp	.vectint

ENTRY(devint92)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x92
	jmp	.vectint

ENTRY(devint93)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x93
	jmp	.vectint

ENTRY(devint94)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x94
	jmp	.vectint

ENTRY(devint95)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x95
	jmp	.vectint

ENTRY(devint96)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x96
	jmp	.vectint

ENTRY(devint97)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x97
	jmp	.vectint

ENTRY(devint98)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x98
	jmp	.vectint

ENTRY(devint99)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x99
	jmp	.vectint

ENTRY(devint9a)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9a
	jmp	.vectint

ENTRY(devint9b)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9b
	jmp	.vectint

ENTRY(devint9c)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9c
	jmp	.vectint

ENTRY(devint9d)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9d
	jmp	.vectint

ENTRY(devint9e)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9e
	jmp	.vectint

ENTRY(devint9f)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0x9f
	jmp	.vectint

ENTRY(devinta0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa0
	jmp	.vectint

ENTRY(devinta1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa1
	jmp	.vectint

ENTRY(devinta2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa2
	jmp	.vectint

ENTRY(devinta3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa3
	jmp	.vectint

ENTRY(devinta4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa4
	jmp	.vectint

ENTRY(devinta5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa5
	jmp	.vectint

ENTRY(devinta6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa6
	jmp	.vectint

ENTRY(devinta7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa7
	jmp	.vectint

ENTRY(devinta8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa8
	jmp	.vectint

ENTRY(devinta9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xa9
	jmp	.vectint

ENTRY(devintaa)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xaa
	jmp	.vectint

ENTRY(devintab)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xab
	jmp	.vectint

ENTRY(devintac)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xac
	jmp	.vectint

ENTRY(devintad)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xad
	jmp	.vectint

ENTRY(devintae)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xae
	jmp	.vectint

ENTRY(devintaf)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xaf
	jmp	.vectint

ENTRY(devintb0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb0
	jmp	.vectint

ENTRY(devintb1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb1
	jmp	.vectint

ENTRY(devintb2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb2
	jmp	.vectint

ENTRY(devintb3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb3
	jmp	.vectint

ENTRY(devintb4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb4
	jmp	.vectint

ENTRY(devintb5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb5
	jmp	.vectint

ENTRY(devintb6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb6
	jmp	.vectint

ENTRY(devintb7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb7
	jmp	.vectint

ENTRY(devintb8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb8
	jmp	.vectint

ENTRY(devintb9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xb9
	jmp	.vectint

ENTRY(devintba)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xba
	jmp	.vectint

ENTRY(devintbb)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xbb
	jmp	.vectint

ENTRY(devintbc)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xbc
	jmp	.vectint

ENTRY(devintbd)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xbd
	jmp	.vectint

ENTRY(devintbe)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xbe
	jmp	.vectint

ENTRY(devintbf)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xbf
	jmp	.vectint

ENTRY(devintc0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc0
	jmp	.vectint

ENTRY(devintc1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc1
	jmp	.vectint

ENTRY(devintc2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc2
	jmp	.vectint

ENTRY(devintc3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc3
	jmp	.vectint

ENTRY(devintc4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc4
	jmp	.vectint

ENTRY(devintc5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc5
	jmp	.vectint

ENTRY(devintc6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc6
	jmp	.vectint

ENTRY(devintc7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc7
	jmp	.vectint

ENTRY(devintc8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc8
	jmp	.vectint

ENTRY(devintc9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xc9
	jmp	.vectint

ENTRY(devintca)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xca
	jmp	.vectint

ENTRY(devintcb)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xcb
	jmp	.vectint

ENTRY(devintcc)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xcc
	jmp	.vectint

ENTRY(devintcd)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xcd
	jmp	.vectint

ENTRY(devintce)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xce
	jmp	.vectint

ENTRY(devintcf)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xcf
	jmp	.vectint

ENTRY(devintd0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd0
	jmp	.vectint

ENTRY(devintd1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd1
	jmp	.vectint

ENTRY(devintd2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd2
	jmp	.vectint

ENTRY(devintd3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd3
	jmp	.vectint

ENTRY(devintd4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd4
	jmp	.vectint

ENTRY(devintd5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd5
	jmp	.vectint

ENTRY(devintd6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd6
	jmp	.vectint

ENTRY(devintd7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd7
	jmp	.vectint

ENTRY(devintd8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd8
	jmp	.vectint

ENTRY(devintd9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xd9
	jmp	.vectint

ENTRY(devintda)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xda
	jmp	.vectint

ENTRY(devintdb)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xdb
	jmp	.vectint

ENTRY(devintdc)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xdc
	jmp	.vectint

ENTRY(devintdd)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xdd
	jmp	.vectint

ENTRY(devintde)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xde
	jmp	.vectint

ENTRY(devintdf)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xdf
	jmp	.vectint

ENTRY(devinte0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe0
	jmp	.vectint

ENTRY(devinte1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe1
	jmp	.vectint

ENTRY(devinte2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe2
	jmp	.vectint

ENTRY(devinte3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe3
	jmp	.vectint

ENTRY(devinte4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe4
	jmp	.vectint

ENTRY(devinte5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe5
	jmp	.vectint

ENTRY(devinte6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe6
	jmp	.vectint

ENTRY(devinte7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe7
	jmp	.vectint

ENTRY(devinte8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe8
	jmp	.vectint

ENTRY(devinte9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xe9
	jmp	.vectint

ENTRY(devintea)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xea
	jmp	.vectint

ENTRY(devinteb)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xeb
	jmp	.vectint

ENTRY(devintec)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xec
	jmp	.vectint

ENTRY(devinted)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xed
	jmp	.vectint

ENTRY(devintee)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xee
	jmp	.vectint

ENTRY(devintef)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xef
	jmp	.vectint

ENTRY(devintf0)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf0
	jmp	.vectint

ENTRY(devintf1)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf1
	jmp	.vectint

ENTRY(devintf2)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf2
	jmp	.vectint

ENTRY(devintf3)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf3
	jmp	.vectint

ENTRY(devintf4)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf4
	jmp	.vectint

ENTRY(devintf5)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf5
	jmp	.vectint

ENTRY(devintf6)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf6
	jmp	.vectint

ENTRY(devintf7)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf7
	jmp	.vectint

ENTRY(devintf8)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf8
	jmp	.vectint

ENTRY(devintf9)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xf9
	jmp	.vectint

ENTRY(devintfa)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xfa
	jmp	.vectint

ENTRY(devintfb)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xfb
	jmp	.vectint

ENTRY(devintfc)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xfc
	jmp	.vectint

ENTRY(devintfd)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xfd
	jmp	.vectint

ENTRY(devintfe)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xfe
	jmp	.vectint

ENTRY(devintff)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	pushl	$0xff

.vectint:
	cld			/ Don't want interrupted context's DF
	call	intr_handle
	addl	$4,%esp		/ Pop off vector number.

	jmp	intr_return



/
/ These entry points are reached from theoretically impossible interrupts,
/ usually because the main initialization path is turning on interrupts
/ when releasing an FSPIN before initpsm has been called.  The debugger
/ is the prime offender here.
/ These interrupts are usually quietly ignored, but will generate a message
/ in a  debug kernel.
/
ENTRY(spurious15)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$15		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious19)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$19		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious20)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$20		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious21)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$21		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious22)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$22		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious23)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$23		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious24)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$24		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious25)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$25		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious26)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$26		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious27)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$27		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious28)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$28		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious29)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$29		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious30)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$30		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return

ENTRY(spurious31)
	SAVE_DSEGREGS
	INTR_SAVE_REGS
	SETUP_KDSEGREGS
	cld			/ Don't want interrupted context's DF
	pushl	$0		/ dummy parameter for intr_stray
	pushl	$31		/ Pass vector number.
	call	intr_stray
	addl	$8,%esp		/ Pop off vector number.
	jmp	intr_return



