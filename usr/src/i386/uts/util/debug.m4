.ident	"@(#)kern-i386:util/debug.m4	1.1"

/
/ MACRO
/ FILE(name)
/
/ Description:
/	Sets the file id for assertions to "name."  See the
/	description of ASSERT.
/
/ MACRO
/ ASSERT(type,dst,relop,src)
/
/ Description:
/	Asserts a simple relational condition within assembly code.
/	Generates code to check that the condition, and to panic
/	the system and print a message if the condition is false.
/
/	The macro takes four arguments:
/
/		type:	A two letter specification describing the
/			comparison.  The first letter may be either
/			s or u to indicate a signed or unsigned comparison
/			respectively.  The second letter is either b,
/			w, or l to indicate a byte, word, or long compare.
/
/		dst,src:The operands of the comparison.  These must
/			be a combination allowed by the assembler.
/			For example, if both are memory operands,
/			this will result in an assembler error, since
/			the i386 does not allow comparison of two
/			memory operands.
/
/		relop:	A relational operand (<,<=,==,!=,>=,>)
/
/	Assertion failure messages generated from such a macro consist
/	of four parts:
/
/		file:	An identification of the file.  This may be
/			set using the `FILE' macro below.  It defaults
/			to "???."
/
/		id:	The ordinal number of the ASSERT within the
/			source, e.g., if the id is 4 then it is the
/			fourth assertion in the file.
/
/		expr:	The actual expression which caused the assertion
/			failure, printed in the form "dst relop src",
/			i.e., without commas or the type specifier.
/
/		regs:	The values of the general registers at the time
/			the assertion failed.
/
/ Remarks:
/	The following assertion checks that %eax is less than 0:
/
/			ASSERT(sl,%eax,<,$0)
/
/	Check that %edx is less than nintr:
/
/			ASSERT(ul,%edx,<,nintr)
/
/	Near the end of the macro definition, the following line appears:
/
/			addl	$ 12, %esp
/
/	The space is needed between the `$' and the `12' because m4 will
/	otherwise interpret `$12', i.e., the 12th argument to the macro,
/	which will generally be nothing, and the assembler will end up
/	seeing
/
/			addl	, %esp
/
/	which is obviously an error.  With the space in there, m4 will
/	simply pass the line through to the assembler.  The assembler does
/	not care about the space, and will simply interpret `$ 12' as
/	an immediate argument of 12, which is what we want.
/
define(`FILE',`
ifdef(`DEBUG',`
	LABEL(`__ASSERTFILE__')
.pushsection	.rodata
__ASSERTFILE__:	.string	"$*"
.popsection
')
')

define(`ASSERT',`
ifdef(`DEBUG',`
/ Set file id to ??? if it is not set already
	ifdef(`__ASSERTFILE__',`',`
		FILE(`???')
	')
	LABEL(`assertion',`pass')

/ Initialize or increment assertion id
	ifdef(`__ASSERTID__',`
		define(`__ASSERTID__',incr(__ASSERTID__))
	',`
		define(`__ASSERTID__',1)
	')

/ define string for assertion
	.pushsection	.rodata
assertion:	.string	`"$2 $3 $4"'
	.popsection

/ generate comparison
	`cmp'substr(`$1',1,1) $4, $2

/ branch on specific condition
/ First, define GT and LT based on whether it is signed or unsigned
	ifelse(substr(`$1',0,1),`u',`
		pushdef(`GT',`a')
		pushdef(`LT',`b')
	',`
		pushdef(`GT',`g')
		pushdef(`LT',`l')
	')
/ Next, generate the branch type based on the relational op
	ifelse(
		`$3',	`==',	`je pass',
		`$3',	`!=',	`jne pass',
		`$3',	`<',	``j'LT pass',
		`$3',	`<=',	``j'LT`e' pass',
		`$3',	`>',	``j'GT pass',
		`$3',	`>=',	``j'GT`e' pass',
		`errprint(`unknown relational op: "$3"')'
	)
/ pop off the definitions of GT and LT
	popdef(`GT',`a')
	popdef(`LT',`b')

/ code for assertion failure
	pushal				/ arg 4 through N: registers
	pushl	$__ASSERTID__		/ arg 3: assertion id
	pushl	$__ASSERTFILE__		/ arg 2: file name
	pushl	$assertion		/ arg 1: assertion text
	call	s_assfail		/ print an assembler assertion failure
/ no return, but leave in for stack traces
	addl	$ 12, %esp		/ See remarks on why the space is there
	popal
/ passes
pass:
	popdef(`assertion',`pass')
')
')
