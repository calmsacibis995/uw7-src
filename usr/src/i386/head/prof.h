#ifndef _PROF_H
#define _PROF_H
#ident	"@(#)sgs-head:i386/head/prof.h	1.10.4.3"

#ifndef MARK
#define MARK(K)	{}
#else
#undef MARK

#if #machine(i386)
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align 4");\
		asm("."#K".:");\
		asm("	.long 0");\
		asm("	.text");\
		asm("M."#K":");\
		asm("	movl	$."#K".,%edx");\
		asm("	call _mcount");\
		}
#elif #machine(m68k)
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   long    0");\
                asm("   text");\
                asm("   pea	K");\
                asm("   jsr     mcount");\
                asm("   addq.l	4,%sp");\
                }
#elif #machine(m88k)
#define MARK(K)	{\
                asm("   data");\
                asm("   align 4");\
                asm("K:");\
                asm("   byte    0,0,0,0");\
                asm("   text");\
                asm("   subu    r31,r31,4");\
                asm("   st      r2,r31,0");\
                asm("   or.u    r2,r0,hi16(L)");\
                asm("   bsr.n   mcount");\
                asm("   or      r2,r2,lo16(L)");\
                asm("   ld	r2,r31,0");\
                asm("   addu    r31,r31,4");\
                }
#elif #machine(sparc)			
#define MARK(K) {\
		asm("	.reserve	."#K"., 4, \"data\", 4");\
		asm("M."#K":");\
		asm("	sethi	%hi(."#K".), %o0");\
		asm("	call	.mcount");\
		asm("	or	%o0, %lo(."#K".), %o0");\
		}				
#else /*Digital(?)*/
#define MARK(K)	{\
		asm("	.data");\
		asm("	.align	4");\
		asm("."#K".:");\
		asm("	.word	0");\
		asm("	.text");\
		asm("M."#K":");\
		asm("	movw	&."#K".,%r0");\
		asm("	jsb	_mcount");\
		}
#endif /*#machine(...)*/

#endif /*MARK*/

#endif /*_PROF_H*/
