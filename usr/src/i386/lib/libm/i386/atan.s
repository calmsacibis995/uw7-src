	.file	"atan.s"
	.ident	"@(#)libm:i386/atan.s	1.6"
/###########################################################
/ 	atan returns the arctangent of its double-precision argument,
/	in the range [-pi/2, pi/2].
/	There are no error returns.
/	atan2(y, x) returns the arctangent of y/x,
/	in the range (-pi, pi].
/	atan2 discovers what quadrant the angle is in and calls atan.
/
/	If either argument to atan2 is NaN, atan2 returns EDOM error and
/	value NaN.
/
/	In -Xt mode,  
/	atan2 returns EDOM error and value 0 if both arguments are zero.
/
/	In -Xa and -Xc modes, 
/	atan2 returns EDOM error and value +-0 	if y is +-0 and x is +0
/	atan2 returns EDOM error and value +-pi if y is +-0 and x is -0
/	
/
/###########################################################
	.section	.rodata
	.align	8
.zero:
	.long	0x0,0x0
	.align	4
.one:
	.long	0x0,0x3ff00000
	.align	4
.BIG:
	.long	0x78b58c40,0x4415af1d / 1.0e20
	.align	4
.PI_2:
	.long	0x54442d18,0x3ff921fb
	.align	4
.PI:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0x9,0x40
	.align	4
.N_PI:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0x9,0xc0
	.align	4
.XPI_2:		/ extended precision for accuracy
	.long	0x2168c235,0xc90fdaa2,0x3fff
	.align	8
.NPI_2:
	.long	0x54442d18,0xbff921fb
	.align	4
.PI_4:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0xe9,0x3f
	.align	4
.NPI_4:
	.byte	0x18,0x2d,0x44,0x54,0xfb,0x21,0xe9,0xbf
	.align	4
.PI3_4:
	.byte	0xd2,0x21,0x33,0x7f,0x7c,0xd9,0x2,0x40
	.align	4
.NPI3_4:
	.byte	0xd2,0x21,0x33,0x7f,0x7c,0xd9,0x2,0xc0
	.text
	.align	4
	.globl	atan
	.type	atan,@function
atan:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	MCOUNT
	pushl	%edi
	xorl	%edi,%edi
	movl	12(%ebp),%eax	/ if x == NaN or Inf
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.LA90
	movl	12(%ebp),%eax	/ if x != INF 
	andl	$1048575,%eax
	orl	8(%ebp),%eax
	je	.LA90
	pushl	$1	/ call atan_err(0.0, x, 1)
	pushl	12(%ebp)
	pushl	8(%ebp)
	pushl	.zero+4
	pushl	.zero
	call	atan_err
	addl	$20,%esp
	popl	%edi
	leave
	ret/0
.LA90:
	fldl	8(%ebp)		/ if (x < 0.0) {
	ftst
	fstsw	%ax
	sahf	
	jae	.L51
	fchs			/ x = -x;
	movl	$1,%edi		/ neg_x = 1;
.L51:
	fcoml	.BIG		/ if (x >= 1e20)
	fstsw	%ax
	sahf	
	jb	.L53
	testl	%edi,%edi
	je	.L57		/ return(neg_x ? -M_PI_2 : M_PI_2)
	fldl	.NPI_2
	ffree	%st(1)
.L47:
	popl	%edi
	leave	
	ret	
	.align	4
.L57:
	fldl	.PI_2
	ffree	%st(1)
	popl	%edi
	leave	
	ret	
	.align	4
.L53:
	fcoml	.one		/ if (x < 1.0)
	fstsw	%ax
	sahf	
	jae	.L59
	ftst			/ if (!x)
	fstsw	%ax
	sahf	
	jne	.L60
	fldl	8(%ebp)		/ return +-0
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
	fldt	.XPI_2
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
	.size	atan,.-atan
//////////////////////////////////////////////////////////////////
	.text
	.align	4
	.globl	atan2
	.type	atan2,@function
atan2:				/ atan2(y, x)
	pushl	%ebp
	movl	%esp,%ebp
	subl	$40,%esp
	MCOUNT
	pushl	%edi
	pushl	%esi
	movl	12(%ebp),%eax	/ is y NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.L134
	movl	12(%ebp),%eax		/ if !INF(y)
	andl	$1048575,%eax
	orl	8(%ebp),%eax
	je	.L135
	pushl	$2			/ NaN
	pushl	12(%ebp)		/ return _atan_err(x, y, 2)
	pushl	8(%ebp)
	pushl	20(%ebp)
	pushl	16(%ebp)
	call	atan_err
	addl	$20,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L135:					/ y == INF
	movl	20(%ebp),%eax		/ is x NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.L136
	movl	20(%ebp),%eax		/ if !INF(x)
	andl	$1048575,%eax
	orl	16(%ebp),%eax
	je	.L137	
	pushl	$2			/ y == INF, x == NaN
	pushl	12(%ebp)		/ return _atan_err
	pushl	8(%ebp)
	pushl	20(%ebp)
	pushl	16(%ebp)
	call	atan_err
	addl	$20,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L137:					/ y == inf, x == inf
	fldl	16(%ebp)		/ if (x>0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L138
	fldl	8(%ebp)		/ if (y > 0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L154
	fldl	.PI_4		/ x and y > 0 - return pi/4
	popl	%esi
	popl	%edi
	leave
	ret/0
.L154:
	fldl	.NPI_4		/ x > 0, y < 0, return -pi/4
	popl	%esi
	popl	%edi
	leave
	ret/0
.L138:
	fldl	8(%ebp)		/ x < 0
	fcompl	.zero		/ if (y > 0)
	fstsw	%ax
	sahf
	jbe	.L159		
	fldl	.PI3_4		/ return 3pi/4
	popl	%esi
	popl	%edi
	leave
	ret/0
.L159:
	fldl	.NPI3_4		/ x < 0, y<0, return -3pi/4
	popl	%esi
	popl	%edi
	leave
	ret/0
.L136:				/ y inf, x finite
	fldl	8(%ebp)		/ if (y>0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L164
	fldl	.PI_2		/ return pi/2
	popl	%esi
	popl	%edi
	leave
	ret/0
.L164:
	fldl	.NPI_2		/ y < 0, return -pi/2
	popl	%esi
	popl	%edi
	leave
	ret/0
.L134:				/ y not NaN or INF
	movl	20(%ebp),%eax	/ is x NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.notnan
	movl	20(%ebp),%eax	/ if !INF(x)
	andl	$1048575,%eax
	orl	16(%ebp),%eax
	je	.L140
	pushl	$2		/  x == NaN - return _atan_err
	pushl	12(%ebp)
	pushl	8(%ebp)
	pushl	20(%ebp)
	pushl	16(%ebp)
	call	atan_err
	addl	$20,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L140:
	fldl	8(%ebp)		/ x == inf; if (y!= 0)
	ftst
	fstsw	%ax
	sahf
	ffree	%st(0)
	je	.notnan
	fldl	16(%ebp)	/ if (x > 0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L142
	fldl	8(%ebp)		/ if (y > 0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L170		/ x == +inf, y > 0
	fldz			/ return 0.0
	popl	%esi
	popl	%edi
	leave
	ret/0
.L170:
	fldz			/ x == +inf, y < 0
	fchs			/ return -0.0
	popl	%esi
	popl	%edi
	leave
	ret/0
.L142:
	fldl	8(%ebp)		/ x == -inf; if (y > 0)
	fcompl	.zero
	fstsw	%ax
	sahf
	jbe	.L175
	fldl	.PI		/ x == -inf, y > 0
	popl	%esi
	popl	%edi
	leave			/ return pi
	ret/0
.L175:
	fldl	.N_PI		/ x == -inf, y < 0
	popl	%esi
	popl	%edi
	leave			/ return -pi
	ret/0
.notnan:
	/ neither y nor x are NaN - check for both 0
	xorl	%edi,%edi	/ neg_x = 0
	xorl	%esi,%esi	/ neg_y = 0
	fldl	16(%ebp)
	ftst	
	fstsw	%ax
	sahf	
	jne	.L71		/ if !x
	fldl	8(%ebp)
	ftst	
	fstsw	%ax
	sahf	
	jne	.L73		/ && !y
	ffree	%st(0)
	ffree	%st(1)
	pushl	$2		/  !x && !y, return atan_err
	pushl	12(%ebp)
	pushl	8(%ebp)
	pushl	20(%ebp)
	pushl	16(%ebp)
	call	atan_err
	addl	$20,%esp
	popl	%esi
	popl	%edi
	leave
	ret/0
.L71:
	fldl	8(%ebp)
.L73:
	fld	%st(0)
	fadd	%st(2),%st	/ if (y+x==y)
	fcomp
	fstsw	%ax
	sahf	
	jne	.L80
				/ |x| negligible compared to |y|
	ftst			/ 	if (y > 0.0)
	fstsw	%ax
	sahf	
	jbe	.L81
	fldl	.PI_2		/		return pi/2
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L81:
	fldl	.NPI_2		/		else return -pi/2
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
	testl	%edi,%edi	/			if(!neg_x)
	jne	.L91
	fldl	8(%ebp)		/				return y
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L91:
	fldpi			/			else return +-pi
	movl	12(%ebp),%eax
	testl	$0x80000000,%eax 	/ (y -0.0, return -pi
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
	fldl	.PI_2		/		return pi/2
	popl	%esi
	popl	%edi
	ffree	%st(1)
	ffree	%st(2)
	leave	
	ret	
	.align	4
.L96:
	fldl	.NPI_2		/		else return -pi/
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
	fldt	.XPI_2
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
	.size	atan2,.-atan2

	.section	.rodata1
	.align	4
.X218:
	.byte	0x61,0x74,0x61,0x6e,0x32,0x00
	.text
	.section	.rodata1
	.align	4
.X219:
	.byte	0x61,0x74,0x61,0x6e,0x00
	.text
	.section	.rodata1
	.align	4
.X222:
	.byte	0x3a,0x20,0x44,0x4f,0x4d,0x41,0x49,0x4e,0x20,0x65
	.byte	0x72,0x72,0x6f,0x72,0x0a,0x00
	.text
	.type	atan_err,@function
	.text
	.align	4

	.align	16
atan_err:
	pushl	%ebp
	movl	%esp,%ebp
	andl	$-8,%ebp
	subl	$52,%esp
	leal	_lib_version,%edx
	movl	64(%esp),%eax	/ is x NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.xokay
	movl	64(%esp),%eax		/ && !INF(x)
	andl	$1048575,%eax
	orl	60(%esp),%eax
	je	.xokay
	movl	64(%esp),%eax		/ NaN - exc.retval = x
	movl	%eax,-4(%ebp)
	movl	60(%esp),%eax
	movl	%eax,-8(%ebp)
	movl	-4(%ebp),%eax
	testl	$0x80000,%eax		/ test quiet NaN bit
	jne	.L199
	fldz				/ signalling NaN - raise exc.
	fldz
	fdivp				/ 0.0 / 0.0
	ffree	%st(0)
	orl	$0x80000,-4(%ebp)	/ Set qnan bit
	jmp	.L199
	.align	16,7,4
.xokay:
	movl	72(%esp),%eax	/ is y NaN or INF ?
	andl	$0x7ff00000,%eax
	cmpl	$0x7ff00000,%eax
	jne	.L193
	movl	72(%esp),%eax		/ exc.retval = y
	movl	%eax,-4(%ebp)
	movl	68(%esp),%eax
	movl	%eax,-8(%ebp)		
	movl	-4(%ebp),%eax	
	testl	$0x80000,%eax		/ test quiet NaN bit
	jne	.L199
	fldz				/ signalling NaN - raise exc.
	fldz
	fdivp				/ 0.0 / 0.0
	ffree	%st(0)
	orl	$0x80000,-4(%ebp)	/ set qnan bit
	jmp	.L199
	.align	16,7,4
.L193:
	cmpl	$0,(%edx)		/ both args 0.0
	jne	.L196			/ if _lib_version == c_issue_4
	movl	.zero+4,%eax		/ exc.retval = 0.0
	movl	%eax,-4(%ebp)
	movl	.zero,%eax
	movl	%eax,-8(%ebp)
	jmp	.L199
	.align	16,7,4
.L196:
	movl	64(%esp),%eax		/ else
	movl	$0x80000000,%ecx	/ test sign bit of x
	testl	%ecx,%eax
	je	.L198
	movl	72(%esp),%eax		/ x == -0.0 ; test sign of y
	testl	%ecx,%eax
	je	.L214
	movl	.N_PI+4,%eax		/ x and y == -0.0 
	movl	%eax,-4(%ebp)		/ return -pi
	movl	.N_PI,%eax
	movl	%eax,-8(%ebp)
	jmp	.L199
	.align	16,7,4
.L214:
	movl	.PI+4,%eax		/ y == 0.0, x == -0.0
	movl	%eax,-4(%ebp)		/ return pi
	movl	.PI,%eax
	movl	%eax,-8(%ebp)
	jmp	.L199
	.align	16,7,4
.L198:
	movl	72(%esp),%eax		/ x == 0.0, return y
	movl	%eax,-4(%ebp)
	movl	68(%esp),%eax
	movl	%eax,-8(%ebp)
.L199:
	cmpl	$0,(%edx)		/ if _lib_version == c_issue_4
	jne	.L203
	movl	72(%esp),%eax		/ exc.arg1 = y
	movl	%eax,-20(%ebp)
	movl	68(%esp),%eax
	movl	%eax,-24(%ebp)
	movl	64(%esp),%eax		/ exc.arg2 = x
	movl	%eax,-12(%ebp)	
	movl	60(%esp),%eax
	movl	%eax,-16(%ebp)
	movl	$1,-32(%ebp)		/ exc.type = DOMAIN
	movl	76(%esp),%edx
	cmpl	$2,%edx			/ if num == 2, then
	jne	.L220
	movl	$.X218,-28(%ebp)	/ exc.name = "atan2"
	jmp	.L221
	.align	16,7,4
.L220:
	movl	$.X219,-28(%ebp)	/ else exc.name = "atan"
.L221:
	leal	-32(%ebp),%eax
	pushl	%eax
	call	matherr			/ if !matherr(&exc)
	popl	%ecx
	testl	%eax,%eax
	jne	.L205
	movl	76(%esp),%edx
	cmpl	$2,%edx			/ _write(2, exc.name, num == 2?
					/ 	5 : 4);
	jne	.L222
	movl	$5,%eax
	jmp	.L223
	.align	16,7,4
.L222:
	movl	$4,%eax
.L223:
	pushl	%eax
	movl	-28(%ebp),%edx
	pushl	%edx
	pushl	$2
	call	_write
	pushl	$15
	pushl	$.X222
	pushl	$2
	call	_write
	addl	$24,%esp
.L203:
	movl	$33,errno		/ errno = EDOM
.L205:
	fldl	-8(%ebp)		/ return exc.retval
	addl	$52,%esp
	popl	%ebp
	ret	
	.align	16,7,4
	.size	atan_err,.-atan_err
