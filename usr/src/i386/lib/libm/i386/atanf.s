	.file	"atanf.s"
	.ident	"@(#)libm:i386/atanf.s	1.6"
/###########################################################
/ 	atanf returns the arctangent of its single-precision argument,
/	in the range [-pi/2, pi/2].
/	There are no error returns.
/	atan2f(y, x) returns the arctangent of y/x,
/	in the range (-pi, pi].
/	atan2f discovers what quadrant the angle is in and calls atanf.
/	If either argument to atan2f is NaN, atan2f returns EDOM error and
/	value NaN.
/
/	In -Xt mode,  
/	atan2f returns EDOM error and value 0 if both arguments are zero.
/
/	In -Xa and -Xc modes, 
/	atan2f returns EDOM error and value +-0 	if y is +-0 and x is +0
/	atan2f returns EDOM error and value +-pi if y is +-0 and x is -0
/
/###########################################################
	.section	.rodata
	.align	4
.one:
	.long	0x3f800000
	.align	4
.zero:
	.long	0
	.align	4
.BIG:
	.long	0x4cbebc20	/ 1.0e8
	.align	4
.PI_2:
	.long	0x3fc90fdb
	.align	4
.DPI_2:		/ double precision for accuracy
	.long	0x54442d18,0x3ff921fb
	.align	4
.NPI_2:
	.long	0xbfc90fdb
/ special value of PI 1 ulp less than M_PI rounded to float
/ to satisfy the range requirements. (float)M_PI is actually
/ greater than true PI
.FM_PI:
	.align	4
	.long	0x40490fda
	.align	8
.N_PI_4:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0xe9,0xbf
	.align	8
.M_PI_4:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0xe9,0x3f
	.align	8
.N_PI_3_4:
	.byte	0xd2,0x21,0x33,0x7f,0x7c,0xd9,0x2,0xc0
	.align	8
.M_PI_3_4:
	.byte	0xd2,0x21,0x33,0x7f,0x7c,0xd9,0x2,0x40
	.text
	.align	4
	.globl	atanf
	.type	atanf,@function
atanf:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	MCOUNT
	pushl	%edi
	movl	8(%ebp),%eax	/ if NaNorINF(x)
	andl	$0x7f800000,%eax
	cmpl	$0x7f800000,%eax
	jne	.L113
	testl	$8388607,8(%ebp)	/ && ! INF(x)
	je	.L113
	pushl	$1		/ x NaN, return atan_err(0,x,1);
	pushl	8(%ebp)
	pushl	$0
	call	atan_err
	addl	$12,%esp
	popl	%edi
	leave
	ret/0
.L113:
	xorl	%edi,%edi	/ neg_x = 0
	flds	8(%ebp)		/ if (x < 0.0) {
	ftst
	fstsw	%ax
	sahf	
	jae	.L51
	fchs			/ x = -x;
	movl	$1,%edi		/ neg_x = 1;
.L51:
	fcoms	.BIG		/ if (x >= 1e20)
	fstsw	%ax
	sahf	
	jb	.L53
	testl	%edi,%edi
	je	.L57		/ return(neg_x ? -M_PI_2 : M_PI_2)
	flds	.NPI_2
	ffree	%st(1)
.L47:
	popl	%edi
	leave	
	ret	
	.align	4
.L57:
	flds	.PI_2
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.L53:
	fcoms	.one		/ if (x < 1.0)
	fstsw	%ax
	sahf	
	jae	.L59
	ftst			/ if (!x)
	fstsw	%ax
	sahf	
	jne	.L60
	flds	8(%ebp)		/ return(+-0.0)
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.L60:
/ASM
	fld1			/ else z = xatan(x,1.0)
	fpatan
/ASMEND0
	jmp	..0
	.align	4
.L59:
/ASM
	fld1			/ else z = xatan(1.0,x)
	fxch
	fpatan
/ASMEND1
	fldl	.DPI_2
	fsub	%st(1),%st		/ z = PI_2 -z
	ffree	%st(1)
..0:
	testl	%edi,%edi	/ return(neg_x ? -z : z);
	je	.L64
	fchs	
.L64:
	popl	%edi
	leave	
	ret	
	.size	atanf,.-atanf
//////////////////////////////////////////////////////////////////
	.text
	.align	4
	.globl	atan2f
	.type	atan2f,@function
atan2f:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$52,%esp
	MCOUNT
	pushl	%edi
	pushl	%esi
	xorl	%edi,%edi	/ neg_x = 0
	xorl	%esi,%esi	/ neg_y = 0
	movl	8(%ebp),%eax	/ if NaNorINF(y)
	andl	$0x7f800000,%eax
	cmpl	$0x7f800000,%eax
	jne	.L134
	testl	$8388607,8(%ebp) / if !INF(y)
	je	.L135
	pushl	$2		/ NaN - return atan_err(x,y,2);
	pushl	8(%ebp)
	pushl	12(%ebp)
	call	atan_err
	addl	$12,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L135:
	movl	12(%ebp),%eax	/ y inf; if (NANorINF(x)
	andl	$0x7f800000,%eax
	cmpl	$0x7f800000,%eax
	jne	.L136
	testl	$8388607,12(%ebp) / if !INF(x)
	je	.L137
	pushl	$2		/ x NaN - return atan_err(x,y,2)
	pushl	8(%ebp)
	pushl	12(%ebp)
	call	atan_err
	addl	$12,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L137:
	flds	12(%ebp)	/ x and y inf
	fcomps	.zero		/ x > 0?
	fstsw	%ax
	sahf
	jbe	.L138
	flds	8(%ebp)		/ y > 0?
	fcomps	.zero
	fstsw	%ax
	sahf
	jbe	.L155
	fldl	.M_PI_4		/ return (float)pi/4
	fstps	-44(%ebp)
	flds	-44(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L155:
	fldl	.N_PI_4		/ y < 0, return (float)-pi/4
	fstps	-48(%ebp)
	flds	-48(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L138:
	flds	8(%ebp)		/ x < 0, y > 0?
	fcomps	.zero
	fstsw	%ax
	sahf
	jbe	.L160
	fldl	.M_PI_3_4	/ y > 0, return (float)3pi/4
	fstps	-44(%ebp)
	flds	-44(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L160:
	fldl	.N_PI_3_4	/ y < 0, return (float)-3pi/4
	fstps	-48(%ebp)
	flds	-48(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L136:
	flds	8(%ebp)		/ y inf, x finite
	fcomps	.zero		/ if (y > 0)
	fstsw	%ax
	sahf
	jbe	.L165
	flds	.PI_2		/ return pi/2
	popl	%esi
	popl	%edi
	leave
	ret/0
.L165:
	flds	.NPI_2		/ return -pi/2
	popl	%esi
	popl	%edi
	leave
	ret/0
.L134:				/ y finite
	movl	12(%ebp),%eax	/ if (NaNorINF(x)
	andl	$0x7f800000,%eax
	cmpl	$0x7f800000,%eax
	jne	.L139
	testl	$8388607,12(%ebp)	/ if !INF(x)
	je	.L140
	pushl	$2		/ x NaN, return atan_err(x,y,2)
	pushl	8(%ebp)
	pushl	12(%ebp)
	call	atan_err
	addl	$12,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L140:
	flds	8(%ebp)		/ x inf, if y!=0
	ftst
	fstsw	%ax
	sahf
	fstp	%st(0)
	je	.L139
	flds	12(%ebp)	/ if (x > 0)
	fcomps	.zero
	fstsw	%ax
	sahf
	jbe	.L142
	flds	8(%ebp)		/ if (y > 0)
	fcomps	.zero
	fstsw	%ax
	sahf
	jbe	.L171
	movl	$0, -44(%ebp)	/ return +0.0
	flds	-44(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L171:
	movl	$0x80000000,-48(%ebp)	 / y < 0, return -0.0
	flds	-48(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L142:
	flds	8(%ebp)		/ x < 0, y!= 0
	fcomps	.zero		/ if (y > 0)
	fstsw	%ax
	sahf
	jbe	.L176
	fldl	.M_PI		/ return (float)pi
	fstps	-44(%ebp)
	flds	-44(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L176:
	fldl	.N_PI		/return (float)-pi
	fstps	-48(%ebp)
	flds	-48(%ebp)
	popl	%esi
	popl	%edi
	leave
	ret/0
.L139:
	testl	$0x7fffffff,12(%ebp)	/ x and y finite
	jne	.L71		/ if ! x
	testl	$0x7fffffff,8(%ebp)	/ && !y
	jne	.L71
	pushl	$2		/ return atan_err(x,y,2)
	pushl	8(%ebp)
	pushl	12(%ebp)
	call	atan_err
	addl	$12,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L71:
	flds	12(%ebp)
	flds	8(%ebp)
	fld	%st(0)
	fadd	%st(2),%st	/ if (y+x==y)
	fcomp
	fstsw	%ax
	sahf	
	jne	.L80
	ftst			/ 	if (y > 0.0)
	fstsw	%ax
	sahf	
	jbe	.L81
	flds	.PI_2		/		return pi/2
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L81:
	flds	.NPI_2		/		else return -pi/2
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L80:
	ftst			/ if (y < 0.0)
	fstsw	%ax
	sahf	
	jae	.L85
	movl	$1,%esi		/ neg_y = 1
	fchs			/ y = -y
.L85:
	fxch
	ftst			/ if (x < 0.0)
	fstsw	%ax
	sahf	
	jae	.L87
	movl	$1,%edi		/ neg_x = 1
	fchs			/ x = -x
.L87:
	fcom			/ if( ay < ax)
	fstsw	%ax
	sahf	
	jb	.L89
	fxch
	ftst			/		if (!y)
	fstsw	%ax
	sahf	
	jne	.L90
	testl	%edi,%edi	/		if(!neg_x)
	jne	.L91
	flds	8(%ebp)		/			return y
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L91:
	flds	.FM_PI			/	else return +-pi
	movl	8(%ebp),%eax
	testl	$0x80000000,%eax	/ y == -0.0, return -pi
	je	.L911
	fchs
.L911:
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L90:
/ASM
	fxch
	fpatan			/		else z = xatan(ay,ax)
/ASMEND2
	jmp	..2
	.align	4
.L89:
	ftst			/ else if (!x)
	fstsw	%ax
	sahf	
	jne	.L95
	testl	%esi,%esi	/ 	if (!neg_y)
	jne	.L96
	flds	.PI_2		/		return pi/2
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L96:
	flds	.NPI_2		/		else return -pi/
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L95:
/ASM
	fxch
	fpatan			/ else z = xatan(ax,ay)
/ASMEND3
	fldl	.DPI_2
	fsub	%st(1),%st		/ z = PI_2 -z
	ffree	%st(2)
..2:
	testl	%edi,%edi	/ if (neg_x)
	je	.L100
	fldpi
	fsub	%st(1),%st	/ z = pi-z
	ffree	%st(2)
.L100:
	testl	%esi,%esi	/ if (neg_y)
	je	.L102
	fchs			/ z = -z
.L102:
	popl	%esi
	popl	%edi
	ffree	%st(1)
	leave			/ return z
	ret	
	.align	4
	.size	atan2f,.-atan2f
	.section	.rodata
	.align	8
.M_PI:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0x9,0x40
	.align	8
.N_PI:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0x9,0xc0
	.section	.rodata1
	.align	4
.X217:
	.byte	0x61,0x74,0x61,0x6e,0x32,0x66,0x00
	.align	4
.X218:
	.byte	0x61,0x74,0x61,0x6e,0x66,0x00
	.align	4
.X223:
	.byte	0x3a,0x20,0x44,0x4f,0x4d,0x41,0x49,0x4e,0x20,0x65
	.byte	0x72,0x72,0x6f,0x72,0x0a,0x00
	.text
////////////////////////////////////////////////////////
//       float(atan_err(float x, float y, int num //////
//	 either both x and y are 0, or 1 is a NaN //////
////////////////////////////////////////////////////////
	.type	atan_err,@function
	.text
	.align	16
atan_err:
	subl	$40,%esp
	leal	_lib_version,%edx
	movl	44(%esp),%eax
	movl	$0x7f800000,%ecx	/ if NaNorINF(x)
	andl	%ecx,%eax
	cmpl	%ecx,%eax
	jne	.L192
	movl	44(%esp),%ecx		/ if !FINF(x)
	testl	$8388607,%ecx
	je	.L192
	movl	%ecx,36(%esp)		/ x NaN, ret = x
	testl	$0x400000,%ecx		/ if qnanbit(x)
	jne	.L194
	fldz
	fldz
	fdivp
	ffree	%st(0)			/ raise invalid op 0/0
	orl	$0x400000,36(%esp)	/ set qnanbit(ret)
	jmp	.L194
	.align	16,7,4
.L192:
	movl	48(%esp),%eax		/ if NaNorINF(y)
	movl	$0x7f800000,%ecx
	andl	%ecx,%eax
	cmpl	%ecx,%eax
	jne	.L195
	movl	48(%esp),%eax		/ ret = x
	movl	%eax,36(%esp)
	testl	$0x400000,%eax		/ if qnanbit(x)
	jne	.L194
	fldz
	fldz
	fdivp
	ffree	%st(0)			/ raise invalid op 0/0
	orl	$0x400000,36(%esp)	/ set qnanbit(ret)
	fstps	32(%esp)
	jmp	.L194
	.align	16,7,4
.L195:					/ x and y == 0
	cmpl	$0,(%edx)		/ if _lib_version == c_issue_4
	jne	.L198
	movl	$0,36(%esp)		/ ret = 0
	jmp	.L194
	.align	16,7,4
.L198:
	movl	44(%esp),%eax		
	cmpl	$0x80000000,%eax	/ x == -0.0
	jne	.L200
	movl	48(%esp),%eax
	cmpl	$0x80000000,%eax	/ y == -0.0?
	jne	.L213
	fldl	.N_PI			/ ret = (float)-pi
	fstps	36(%esp)
	jmp	.L194
	.align	16,7,4
.L213:
	fldl	.M_PI			/ y == +0.0
	fstps	36(%esp)		/ ret = (float)pi
	jmp	.L194
	.align	16,7,4
.L200:
	movl	48(%esp),%eax		/ x == +0.0, ret = y
	movl	%eax,36(%esp)
.L194:
	cmpl	$0,(%edx)		/ if _lib_version == c_issue_4
	jne	.L202
	flds	48(%esp)		/ exc.arg1 = (double)y
	fstpl	12(%esp)
	flds	44(%esp)		/ exc.arg2 = (double)x
	fstpl	20(%esp)
	flds	36(%esp)		/ exc.retval = (double)ret
	fstpl	28(%esp)
	movl	$1,4(%esp)		/ exc.type = DOMAIN
	movl	52(%esp),%edx		/ if (num == 2)
	cmpl	$2,%edx
	jne	.L219
	movl	$.X217,8(%esp)		/ exc.name = "atan2f"
	jmp	.L220
	.align	16,7,4
.L219:
	movl	$.X218,8(%esp)		/ else exc.name = "atanf"
.L220:
	leal	4(%esp),%eax
	pushl	%eax
	call	matherr			/ if (!matherr(&exc))
	popl	%ecx
	testl	%eax,%eax
	jne	.L203
	movl	52(%esp),%edx
	cmpl	$2,%edx
	jne	.L221
	movl	$6,%eax
	jmp	.L222
	.align	16,7,4
.L221:
	movl	$5,%eax
.L222:
	pushl	%eax		/ _write(2, exc.name, num == 2 ?6 : 5);
	movl	12(%esp),%edx
	pushl	%edx
	pushl	$2
	call	_write
	pushl	$15		/ _write(2, ": DOMAIN error\n", 15);
	pushl	$.X223
	pushl	$2
	call	_write
	movl	$33,errno	/ errno = EDOM
	addl	$24,%esp
.L203:
	fldl	28(%esp)	/ return (float)exc.retval
	fstps	0(%esp)
	flds	0(%esp)
.L205:
	addl	$40,%esp
	ret	
	.align	16,7,4
.L202:
	movl	$33,errno	/ -Xa or -Xc , errno = EDOM
	flds	36(%esp)	/ return ret
	addl	$40,%esp
	ret	
	.align	16,7,4
	.size	atan_err,.-atan_err
	.text
