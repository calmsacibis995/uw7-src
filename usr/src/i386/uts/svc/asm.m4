.ident	"@(#)kern-i386:svc/asm.m4	1.12"
.ident	"$Header$"

define(`ENTRY',`.text; .type $1,@function; .globl $1; .align 8; $1:')
define(`WEAK_ENTRY',`.text; .type $1,@function; .weak $1; .align 8; $1:')
define(`SIZE', `.size $1,.-$1')
define(`SYM_DEF',`.globl $1; .set $1, $2')
define(`ALIAS',`.globl $1; $1:')

/
/ MACRO
/ LABEL(temp-label [, temp-label ]*)
/
/ Description:
/	Takes one or more arguments.  For each name in the argument
/	list, it defines a macro with that name and a unique value,
/	such that the macro can be used as a unique label within a
/	section of code.  At the end of the section of code using the
/	temporary label(s), popdef should be invoked on each name defined
/	in this manner.
/
/ Remarks:
/	Used chiefly within other macros in order to use labels without
/	worrying about conflicts from multiple invocation of the macro.
/
define(`LABEL',`
	ifdef(`__LABELNDX__',`
		define(`__LABELNDX__',incr(__LABELNDX__))
	',`
		define(`__LABELNDX__',0)
	')
	pushdef(`$1',.`$1'`_'__LABELNDX__)
	ifelse($#,1,`',`LABEL(shift($@))')
')

/
/ MACRO
/ ifndef(symbol,expansion-if-defined,expansion-if-undefined)
/
/ Description:
define(`ifndef',`ifelse($#,0,`',`ifdef(`$1',`$3',`$2')')')

/
/ MACRO
/ if(expr,expansion-if-true,[,expr,expansion-if-true]*[,expansion-if-false])
/
/ Description:
/	Takes two or more arguments.  The first argument is an arithmetic
/	expression.  If this expression evalutes to a non-zero value, then
/	the second argument (expansion-if-true) is substituted.  If the
/	expression is non-zero, and there are more than three argument,
/	then the process is repeated with arguments 3, 4, and 5.  Otherwise,
/	the value is either the third string (expansion-if-false), or null
/	if it is not present.
/
/ Remarks:
/	See the description of `ifelse' on m4(1) man page; `if' is like
/	`ifelse', but it is more readable when testing arithmetic expressions
/	rather than string matching.
/
/	This macro checks explicitly to see if it has no arguments, and
/	does nothing in that case.  This allows the word "if" to be used
/	in a comment without wreaking havoc.
/
define(`if',`ifelse(
	$#,		0,	`',
	eval($1),	1,	`$2',
	eval($# > 3),	1,	`if'(shift(shift($@))),
	`$3'
	)')

/
/ MACRO
/ defined(symbol)
/
/ Description:
/	Takes one argument, substitutes 1 if symbol is defined, 0
/	if symbol is not defined.
/
/ Remarks:
/	This macro checks explicitly to see if it has no arguments, and
/	does nothing in that case.  This allows the word "defined" to be
/	used in a comment without wreaking havoc.
/
define(`defined',`ifelse($#,0,`',`ifdef(`$1',1,0)')')

define(`BPARGOFF',8)
define(`BPARG0',BPARGOFF+0(%ebp))
define(`BPARG1',BPARGOFF+4(%ebp))
define(`BPARG2',BPARGOFF+8(%ebp))
define(`BPARG3',BPARGOFF+12(%ebp))
define(`BPARG4',BPARGOFF+16(%ebp))
define(`BPARG5',BPARGOFF+20(%ebp))
define(`BPARG6',BPARGOFF+24(%ebp))
define(`BPARG7',BPARGOFF+28(%ebp))

define(`SPARGOFF',4)
define(`SPARG0',SPARGOFF+0(%esp))
define(`SPARG1',SPARGOFF+4(%esp))
define(`SPARG2',SPARGOFF+8(%esp))
define(`SPARG3',SPARGOFF+12(%esp))
define(`SPARG4',SPARGOFF+16(%esp))
define(`SPARG5',SPARGOFF+20(%esp))
define(`SPARG6',SPARGOFF+24(%esp))
define(`SPARG7',SPARGOFF+28(%esp))

/ Assym_include contains the name of the correct util/assym*.h file
/ for the base kernel.
/ if these change, change the corresponding defns of ASSYM and ASSYMDBG
/ in depend.rules

define(assym_include, ifdef(`DEBUG', `KBASE/util/assym_dbg.h', `KBASE/util/assym.h'))
