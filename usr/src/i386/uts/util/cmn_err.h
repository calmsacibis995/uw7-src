#ifndef _UTIL_CMN_ERR_H	/* wrapper symbol for kernel use */
#define _UTIL_CMN_ERR_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/cmn_err.h	1.12"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * Common error handling severity levels
 */

#define CE_CONT  0	/* continuation	*/
#define CE_NOTE  1	/* notice	*/
#define CE_WARN  2	/* warning	*/
#define CE_PANIC 3	/* panic	*/

#define VA_LIST void *
#define VA_START(list, name) list = \
  (void*)((char*)&name+((sizeof(name)+(sizeof(int)-1))&~(sizeof(int)-1)))
#define VA_ARG(list, mode) ((mode *) \
  (list=(void*)((char*)list+sizeof(mode))))[-1]

/*
 * Machine-dependent macros for s_assfail.
 *  (1)	S_ASSFAIL_ARGDECL declares the general registers in the
 *	order in which they are pushed on the stack
 *
 *  (2)	S_ASSFAIL_FMT is the format used by the cmn_err call which
 *	actually prints out the general registers for the assertion failure
 *
 *  (3)	S_ASSFAIL_ARGVAL list the general registers as they are passed to
 *	cmn_err to print the assertion failure.
 */
#define	S_ASSFAIL_ARGDECL	uint_t edi, uint_t esi, uint_t ebp, \
				uint_t esp, uint_t ebx, uint_t edx, \
				uint_t ecx, uint_t eax

#define	S_ASSFAIL_FMT		"\teax:%8X ebx:%8X ecx:%8X edx:%8X\n\tesi:%8X edi:%8X ebp:%8X esp:%8X"

#define	S_ASSFAIL_ARGVAL	eax, ebx, ecx, edx, esi, edi, ebp, esp

#ifdef __STDC__
extern void cmn_err_init(void);
/*PRINTFLIKE2*/
extern void cmn_err(int, const char *, ...);
/*PRINTFLIKE1*/
extern void printf(const char *, ...);
/*PRINTFLIKE1*/
extern void debug_printf(const char *, ...);
extern boolean_t debug_output_aborted(void);
extern int conslog_set(int);
#else /* !__STDC__ */
extern void cmn_err();
#endif

extern int putbufrpos;
extern int putbufwpos;
extern lock_t putbuf_lock;

/*
 * console logging attribute (see conslog_set())
 */
#define	CONSLOG_DIS	0
#define	CONSLOG_ENA	1
#define	CONSLOG_STAT	2

/*
 * _DEBUG_PRINTF hook, for calling a kernel debugger printing function.
 * See debug_printf() for details.  Should only be invoked from debug_printf().
 *
 * _DEBUG_OUTPUT_ABORTED hook to check if the debugger has aborted the
 * output for the current command.
 */
#ifndef NODEBUGGER
#ifdef _KERNEL_HEADERS
#include <util/kdb/kdebugger.h>	/* PORTABILITY */
#endif
#define _DEBUG_PRINTF(fmt, va)	kdb_printf(fmt, va)
#define _DEBUG_OUTPUT_ABORTED()	kdb_check_aborted()
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_CMN_ERR_H */
