	.ident	"@(#)kern-i386:util/mod/stub.m4	1.9"
	.ident	"$Header$"
/
/ This file contains macros for the stubs mechanism of
/ the Dynamic Loadable Modules.
/

define(`STUB_UNLOADABLE', `1')
define(`STUB_LOADONLY', `0')

/
/ MODULE(module_name, type)
/
define(`MODULE',`
	.data
.$1_modinfo_name:
	.string "$1"
	.align	4
	.globl	$1_modinfo
$1_modinfo:
	.long	$2
	.long	.$1_modinfo_name
	.long	0
	.long	0
')
	
/
/ END_MODULE(module_name)
/
define(`END_MODULE',`
	.long	0
')

/
/ STUB_COMMON(module_name, fcnname, install_fcn, retfcn)
/
define(`STUB_COMMON',`
	.text
	.type	$2,@function
	.globl	$2
	.align	8
.$2_install:
	pushl	$$1_modinfo
	call	mod_stub_load
	addl	$ 4, %esp
	orl	%eax, %eax
	jne	.$2_fail
$2:
	jmp	*.$2_info
	.align 8
.$2_fail:
	jmp	*.$2_errfcn

	.data
.$2_info:
	.long	$3
	.long	$1_modinfo
	.long	$2
	.long	$3
.$2_errfcn:
	.long	$4
')

/
/ STUB(module_name, fcnname, retfcn)
/
define(`STUB', `
	STUB_COMMON($1, $2, .$2_install, $3)')

/
/ Weak stub; will not cause autoloading of the module.
/
/ NOTE: When weak stubs are used with unloadable modules, the
/ caller must ensure that the module is loaded when the stub
/ is called and remains loaded for the duration of the function
/ call.
/
/ WSTUB(module_name, fcnname, retfcn)
/
define(`WSTUB', `
	STUB_COMMON($1, $2, $3, $3)')

/
/ USTUB(module_name, fcnname, retfcn, arg_nword)
/
define(`USTUB',`
	.text
	.type	$2,@function
	.globl	$2
	.align	8
$2:
	pushl	$.$2_info	/ push mod_stub_info arg
	call	mod_ustub_hold	/ load/hold module
	addl	$ 4, %esp
	orl	%eax, %eax
	jne	.$2_fail	/ mod_ustub_hold failure
	/
	/ Prepare to call the target routine.
	/
	movl	$$4, %ecx
	cmpl	$ 0, %ecx
	jz	.$2_argsdone
.$2_argloop:
	pushl	[4 \* $4](%esp)
	loop	.$2_argloop
.$2_argsdone:
	call	*.$2_info	/ call function from module
	addl	$[4 \* $4], %esp
	pushl	%eax		/ save return code
	pushl	$$1_modinfo	/ the mod_stub_modinfo arg
	call	mod_ustub_rele	/ release module
	addl	$ 4, %esp
	popl	%eax		/ restore return code
	ret
	.align 8
.$2_fail:
	jmp	*.$2_errfcn	/ call error func on failure
	/
	/ struct mod_stub_info
	/
	.data
.$2_info:
	.long	-1		/ mods_func_adr
	.long	$1_modinfo	/ mods_modinfo
	.long	$2		/ mods_stub_adr
	.long	-1		/ mods_func_save
.$2_errfcn:
	.long	$3		/ mods_errfcn
')
