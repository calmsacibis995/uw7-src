
/
/ @(#) qvisAssem.s 11.1 97/10/22
/
/ Copyright 1991, COMPAQ COMPUTER CORPORATION.
/
/ Developer   Date    Modification
/ ========  ========  =======================================================
/ mjk       04/07/92  Originated (see RCS log)
/

	.file "qvisAssem.s"
.text
	.align 4
/
/ qvisSquirt - "squirt" out very quickly the right sequence of outb's
/ and outw's for use by the qvisFlushGlyphQueue routine
/
.globl qvisSquirt
qvisSquirt:
	pushl %ebp
	movl %esp,%ebp
/ outw(Y0_BREG, fb_ptr >> 16);
	movl 0x8(%ebp),%eax
	shrl $0x10,%eax
	movw $0x63c2,%dx
        outw (%dx)
/ outw(X0_BREG, fb_ptr & 0xffff);
	movw 0x8(%ebp),%ax
	movw $0x63c0,%dx
	outw (%dx)
/ outw(HEIGHT_REG, h);
	movw 0xc(%ebp),%ax
	movw $0x23c4,%dx
	outw (%dx)
/ outw(WIDTH_REG, w);
	movw 0x10(%ebp),%ax
	movw $0x23c2,%dx
	outw (%dx)
/ outw(X1_BREG, x);
	movw 0x14(%ebp),%ax
	movw $0x63cc,%dx
	outw (%dx)
/ outw(Y1_BREG, y);
	movw 0x18(%ebp),%ax
	movw $0x63ce,%dx
	outw (%dx)
/ outb(BLT_CMD0, 0x1);
	movw $0x1,%ax
	movw $0x33ce,%dx
	outb (%dx)
	leave
	ret

	.align 4
/
/ qvisZip - "zip" out very quickly the right sequence of outb's
/ and outw's for use by the qvisSolidRects routine
/
.globl qvisZip
qvisZip:
	pushl %ebp
	movl %esp,%ebp
/ outw(WIDTH_REG, w);
	movw 0x8(%ebp),%ax
	movw $0x23c2,%dx
	outw (%dx)
/ outw(HEIGHT_REG, h);
	movw 0xc(%ebp),%ax
	movw $0x23c4,%dx
	outw (%dx)
/ outw(X0_BREG, x);
	movw 0x10(%ebp),%ax
	movw $0x63c0,%dx
	outw (%dx)
/ outw(X1_BREG, x);
	movw   $0x63cc,%dx
	outw (%dx)
/ outw(Y0_BREG, y);
	movw 0x14(%ebp),%ax
	movw $0x63c2,%dx
	outw (%dx)
/ outw(Y1_BREG, y);
	movw   $0x63ce,%dx
	outw (%dx)
	leave
	ret

	.align 4
/
/ qvisSpit - "spit" out very quickly the right sequence of outw's
/ for drawing a line
/
.globl qvisSpit
qvisSpit:
        pushl %ebp
        movl %esp,%ebp
/ outw(X0_LREG, x);
        movw 0x8(%ebp),%ax
        movw $0x63c0,%dx
        outw (%dx)
/ outw(Y0_LREG, y);
        movw 0xc(%ebp),%ax
        movw $0x63c2,%dx
        outw (%dx)
/ outw(X1_LREG, x1);
        movw 0x10(%ebp),%ax
        movw $0x83cc,%dx
        outw (%dx)
/ outw(Y1_BREG, y1);
        movw 0x14(%ebp),%ax
        movw   $0x83ce,%dx
        outw (%dx)
        leave
        ret

