	.file	"exp.s"
	.ident	"@(#)libm:i386/exp.s	1.6"
//////////////////////////////////////////////////////////////
/
/	double exp(x)
/	double x;
/	
/	algorithm and coefficients from Cody and Waite
/	If x is a NaN, return error EDOM and value NaN.  If x is also a
/	signalling NaN, raise an invalid op exception.
/
/	On underflow, return error ERANGE and value 0.
/
/	In -Xt mode,
/	On overflow, return error ERANGE and value +HUGE.
/
/	In -Xa and -Xc modes,
/	On overflow, return error ERANGE and value +HUGE_VAL.
/
//////////////////////////////////////////////////////////////
	.section	.rodata
	.align	4
.p:
	.long	0x2ae6921e,0x3f008b44	/ 3.1555192765684646400000e-05
	.long	0xf22a12a6,0x3f7f074b   / 7.5753180159422777100000e-03
	.long	0x0,0x3fd00000	        / 2.5000000000000000000000e-01
	.align	4
.q:
	.long	0xce50455,0x3ea93363   / 7.5104028399870046400000e-07
	.long	0x5c28d4df,0x3f44af0c  / 6.3121894374398502800000e-04
	.long	0x51dfd9ff,0x3fad1728  / 5.6817302698551221900000e-02
	.long	0x0,0x3fe00000	       / 5.0000000000000000000000e-01
	.align	4
.xeps:
	.long	0x667f3bcd,0x3e46a09e  / 1.0536712127723507900000e-08
	.align	4
.LNMINDOUBLE:
	.long	0x446d71c3,0xc0874385  / -7.4444007192138126000000e+02
	.align	4
.MINDOUBLE:
	.long	0x1,0x0		       / 4.9406564584124654e-324
	.align	4
.dzero:
	.long	0x0,0x0
	.align	4
.LNMAXDOUBLE:
	.long	0xfefa39ef,0x40862e42  / 7.0978271289338400100000e+02
	.align	4
.MAXDOUBLE:
	.long	0xffffffff,0x7fefffff  / 1.79769313486231470e308
	.align	4
.HUGE:
	.long	0xe0000000,0x47efffff  / 3.4028234663852886e38
	.align	4
.half:
	.long	0x0,0x3fe00000	       / 1/2
	.align	4
.L81:
	.long	0x652b82fe,0x3ff71547  / 1.4426950408889634200000e+00
	.align	4
.C1:
	.long	0x0,0x3fe63000	       / 6.9335937500000000000000e-01
	.align	4
.C2:
	.long	0x5c610ca8,0xbf2bd010  / -2.1219444005469058000000e-04
	.text
	.align	4
	.globl	exp
	.type	exp,@function
exp:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$68,%esp
	MCOUNT
	pushl	%edi
	fldl	8(%ebp)		/ x
	movl	12(%ebp),%eax	/ is x NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	je	.err
	fld	%st(0)
	fabs			/ |x|
	fcompl	.xeps		/ if (|x| < xeps)
	fstsw	%ax
	sahf	
	jae	.L56
	fld1
	fadd			/ return 1.0 + x
.L49:
	popl	%edi
	leave	
	ret	
	.align	4
.L56:
	fcoml	.LNMINDOUBLE	/ if (x<= LN_MINDOUBLE)
	fstsw	%ax
	sahf	
	ja	.L60
	jne	.err		/ if (exc.arg1 == LN_MINDOUBLE)
	fldl	.MINDOUBLE	/ return MINDOUBLE
	ffree	%st(1)
	popl	%edi
	leave	
	ret	

	.align	4
.L60:
	fcoml	.LNMAXDOUBLE	 / if (x>= LN_MAXDOUBLE)
	fstsw	%ax
	sahf	
	jb	.good
	jne	.err		/ if (exc.arg1 == LN_MAXDOUBLE)
	fldl	.MAXDOUBLE	/ return MAXDOUBLE
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.err:
	ffree	%st(0)
	pushl	12(%ebp)
	pushl	8(%ebp)
	call	exp_err
	addl	$8,%esp
	popl	%edi
	leave	
	ret
	.align	4
.good:
	fldl2e			/ log2e
	fmul	%st(1),%st		/ x * log2e
	frndint			/ y = rnd(x * log2e)
	fistl	-48(%ebp)	/ n = (int)y
	fld	%st(0)		/ tmp = y
	fmull	.C1		/ tmp *= C1
	fsubr	%st,%st(2)	/ x = x - Y * C1
	fxch
	fmull	.C2		/ y *= C2
	fsubr	%st,%st(2)	/ x = (x - y * C1) - y * C2
	fld	%st(2)
	fmul	%st(0),%st	/ y = x * x
	fst	%st(1)
	fmull	.p		/ y * p[0]
	faddl	.p+8		/   + p[1]
	fmul	%st(1),%st		/   * y
	faddl	.p+16		/   + p[2]
	fmul	%st,%st(3)	/ x *= ..
	fld	%st(1)
	fmull	.q		/ y * q[0]
	faddl	.q+8		/   + q[1]
	fmul	%st(2),%st		/   * y
	faddl	.q+16		/   + q[2]
	fmul	%st(2),%st	/   * y
	faddl	.q+24		/   + q[3]
	fsub 	%st(4),%st		/   - x
	fdivr	%st(4),%st	/ x/(_POLY3(y,q) -x)
	faddl	.half		/ + 0.5
	addl	$1,-48(%ebp)	/ n+= 1
	fildl	-48(%ebp)	/ return ldexp(x,n+1)
	fxch
	fscale
	ffree	%st(1)
	ffree	%st(2)
	ffree	%st(3)
	ffree	%st(4)
	ffree	%st(5)
	popl	%edi
	leave	
	ret	
	.size	exp,.-exp

	.align	4
	.section	.rodata1
.X133:
	.byte	0x65,0x78,0x70,0x00
	.text
	.type	exp_err,@function
	.align	4

	.align	16
exp_err:
	pushl	%ebp
	movl	%esp,%ebp
	andl	$-8,%ebp
	subl	$36,%esp
	leal	_lib_version,%edx
	movl	48(%esp),%eax		/ exc.retval = x
	movl	%eax,-20(%ebp)
	movl	44(%esp),%eax
	movl	%eax,-24(%ebp)
	movl	$.X133,-28(%ebp)	/ exc.name = "exp"
	movl	48(%esp),%eax
	movl	$0x7ff00000,%ecx
	andl	%ecx,%eax
	cmpl	%ecx,%eax		/ if IsNaNorInf
	jne	.L126
	movl	48(%esp),%eax
	andl	$1048575,%eax
	orl	44(%esp),%eax
	je	.L126			/ if !INF
	pushl	$3			/ (NaN)
	pushl	$.X133
	movl	56(%esp),%edx
	pushl	%edx
	movl	56(%esp),%edx
	pushl	%edx
	movl	.dzero+4,%edx
	pushl	%edx
	movl	.dzero,%edx
	pushl	%edx
	movl	72(%esp),%edx
	pushl	%edx
	movl	72(%esp),%edx
	pushl	%edx
	call	_domain_err		/ return _domain_err(x,0.0,x,"exp",3)
	addl	$32,%esp
.L130:
	addl	$36,%esp
	popl	%ebp
	ret	
	.align	16,7,4
.L126:
	fldl	44(%esp)	/ if (x < 0)
	fcompl	.dzero
	fstsw	%ax
	sahf	
	jae	.L127
	movl	$4,-32(%ebp)	/ exc.type = underflow
	movl	.dzero+4,%eax	/ exc.retval = 0.0
	movl	%eax,-4(%ebp)
	movl	.dzero,%eax
	movl	%eax,-8(%ebp)
	jmp	.L128
	.align	16,7,4
.L127:
	movl	$3,-32(%ebp)	/ exc.retval = overflow
	cmpl	$0,(%edx)	/ if _lib_version = c_issue_4
	jne	.L138
	movl	.HUGE+4,%eax	/ exc.retval = HUGE
	movl	%eax,-4(%ebp)
	movl	.HUGE,%eax
	movl	%eax,-8(%ebp)
	jmp	.L139
	.align	16,7,4
.L138:
	movl	__huge_val+4,%eax / exc.retval = HUGE_VAL
	movl	%eax,-4(%ebp)
	movl	__huge_val,%eax
	movl	%eax,-8(%ebp)
.L139:
.L128:
	cmpl	$0,(%edx)	/ if _lib_version = c_issue_4 && !matherr(exc)
	jne	.L141
	leal	-32(%ebp),%eax
	pushl	%eax
	call	matherr
	popl	%ecx
	testl	%eax,%eax
	jne	.L129
.L141:
	movl	$34,errno	/ errno = ERANGE
.L129:
	fldl	-8(%ebp)	/ return exc.retval
	addl	$36,%esp
	popl	%ebp
	ret	
	.align	16,7,4
	.size	exp_err,.-exp_err
	.text
