	.file	"expf.s"
	.ident	"@(#)libm:i386/expf.s	1.4"
//////////////////////////////////////////////////////////////
/
/	float expf(float x)
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
	.long	0x3b885308
	.long	0x3e800000
	.align	4
.q:
	.long	0x3d4cbf5b
	.long	0x3f000000
	.align	4
.xeps:
	.long	0x323504f3
	.align	4
.LNMINFLOAT:
	.long	0xc2ce8ed0
	.align	4
.MINFLOAT:
	.long	0x1
	.align	4
.fzero:
	.long	0x0
	.align	4
.LNMAXFLOAT:
	.long	0x42b17218
	.align	4
.dzero:
	.long	0x0,0x0
.MAXFLOAT:
	.long	0x7f7fffff
	.align	4
.HUGE:
	.long	0xe0000000,0x47efffff
	.align	4
.huge_val:
	.long	0x0,0x7ff00000
.half:
	.long	0x3f000000
	.align	4
.C1:
	.long	0x3f318000
	.align	4
.C2:
	.long	0xb95e8083
	.text
	.align	4
	.globl	expf
expf:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$68,%esp
	MCOUNT
	pushl	%edi
///////
	movl	8(%ebp),%eax
	andl	$0x7f800000,%eax	/ if NaNorINF(x)
	cmpl	$0x7f800000,%eax
	jne	.L96
	pushl	8(%ebp)		/ return exp_err(x)
	call	exp_err
	popl	%ecx
	popl	%edi
	leave
	ret/0
.L96:
	flds	8(%ebp)		/ |x|
	fld	%st(0)
	fabs			
	fcomps	.xeps		/ if (|x| < xeps)
	fstsw	%ax
	sahf	
	jae	.L56
	fld1
	fadd			/ return 1.0 + x
	popl	%edi
	leave	
	ret	
	.align	4
.L56:
	fcoms	.LNMINFLOAT	/ if (x<= LN_MINFLOAT)
	fstsw	%ax
	sahf	
	ja	.L60
	jne	.L62		/ if (x == LN_MINFLOAT)
	flds	.MINFLOAT	/ return MINFLOAT
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.L62:
	ffree	%st(0)
	pushl	8(%ebp)		/ return exp_err(x)
	call	exp_err
	popl	%ecx
	popl	%edi
	leave
	ret/0
	.align	4
.L60:
	fcoms	.LNMAXFLOAT	 / if (x>= LN_MAXFLOAT)
	fstsw	%ax
	sahf	
	jb	.L69
	jne	.L71		/ if (exc.arg1 == LN_MAXFLOAT)
	flds	.MAXFLOAT	/ return MAXFLOAT
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.L71:
	ffree	%st(0)
	pushl	8(%ebp)		/ return exp_err(x)
	call	exp_err
	popl	%ecx
	popl	%edi
	leave
	ret/0
	.align	4
.L69:
	fldl2e			/ log2e
	fmul	%st(1),%st		/ x * log2e
	frndint			/ y = rnd(x * log2e)
	fistl	-48(%ebp)	/ n = (int)y
	fld	%st(0)		/ tmp = y
	fmuls	.C1		/ tmp *= C1
	fsubr	%st,%st(2)	/ x = x - Y * C1
	fxch
	fmuls	.C2		/ y *= C2
	fsubr	%st,%st(2)	/ x = (x - y * C1) - y * C2
	fld	%st(2)
	fmul	%st(0),%st	/ y = x * x
	fst	%st(1)
	fmuls	.p		/ y * p[0]
	fadds	.p+4		/   + p[1]
	fmul	%st,%st(3)	/ x *= ..
	fld	%st(1)
	fmuls	.q		/ y * q[0]
	fadds	.q+4		/   + q[1]
	fsub 	%st(4),%st		/   - x
	fdivr	%st(4),%st	/ x/(_POLY1(y,q) -x)
	fadds	.half		/ + 0.5
	addl	$1,-48(%ebp)	/ n+= 1
	fild	-48(%ebp)	/ return ldexp(x,n+1)
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
	.align	4
	.size	expf,.-expf

///////////////////////////////////////////////

	.section	.rodata1
	.align	4
.X127:
	.byte	0x65,0x78,0x70,0x66,0x00
	.text
	.type	exp_err,@function
	.text
	.align	4

	.align	16
exp_err:			/ static float exp_err(float x)
	pushl	%ebp
	movl	%esp,%ebp
	andl	$-8,%ebp
	subl	$44,%esp
	leal	_lib_version,%edx
	flds	52(%esp)
	fstpl	-24(%ebp)	/ exc.arg1 = (double)x
	movl	$.X127,-28(%ebp) /exc.name = "expf"
	movl	52(%esp),%eax
	movl	$0x7f800000,%ecx / x is NaN or INF
	andl	%ecx,%eax
	cmpl	%ecx,%eax
	jne	.L119
	movl	52(%esp),%ecx	/ if x is INF
	testl	$8388607,%ecx
	je	.L119
	pushl	$4	/ NaN - return _float_domain(x,0,x,"expf", 4)
	pushl	$.X127
	pushl	%ecx
	pushl	$0
	pushl	%ecx
	call	_float_domain
	addl	$20,%esp
	addl	$44,%esp
	popl	%ebp
	ret	
	.align	16,7,4
.L119:
	flds	52(%esp) 	/ if x < 0
	fcomps	.fzero
	fstsw	%ax
	sahf	
	jae	.L120
	movl	$4,-32(%ebp)	/ exc.type = UNDERFLOW
	movl	$0,-36(%ebp)	/ ret = 0.0F
	jmp	.L121
	.align	16,7,4
.L120:
	movl	$3,-32(%ebp)	/ else exc.type = OVERFLOW
	cmpl	$0,(%edx)	/ if (_lib_version == c_issue_4
	jne	.L132
	movl	$0x7f7fffff,-36(%ebp) /ret = MAXFLOAT
	jmp	.L121
	.align	16,7,4
.L132:
	movl	$0x7f800000,-36(%ebp)	/ else ret = inf
.L121:
	cmpl	$0,(%edx)	/ if _lib_version != c_issue_4
	je	..1
	flds	-36(%ebp)	
	addl	$44,%esp
	movl	$34,errno	/ errno = ERANGE, return ret
	popl	%ebp
	ret	
	.align	16,7,4
..1:
	flds	-36(%ebp)	/ exc.retval = (double)ret
	fstpl	-8(%ebp)
	leal	-32(%ebp),%eax
	pushl	%eax
	call	matherr		/if (!matherr(&exc))
	popl	%ecx
	testl	%eax,%eax
	jne	.L123
	movl	$34,errno	/ errno = ERANGE
.L123:
	fldl	-8(%ebp)	/ return (float)exc.retval
	fstps	-40(%ebp)
	flds	-40(%ebp)
.L124:
	addl	$44,%esp
	popl	%ebp
	ret	
	.align	16,7,4
	.size	exp_err,.-exp_err
