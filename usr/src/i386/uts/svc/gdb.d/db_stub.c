#ident	"@(#)kern-i386:svc/gdb.d/db_stub.c	1.2"
#ident	"$Header$"

/****************************************************************************

		THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to db_set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to db_breakpoint().  Db_Breakpoint()
 *  simulates a breakpoint by executing a trap #3.
 *
 *  The external function exceptionHandler(int vecno, void (*handler)()) is
 *  used to attach a specific handler to a specific 386 vector number.
 *  It should use the same privilege level it runs at.  It should
 *  install it as an interrupt gate so that interrupts are masked
 *  while the handler runs.
 *  Also, need to assign exceptionHook and oldExceptionHook.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 * 
 *    Z             do nothing, don't even reply
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 *                  if it is a reply, there is request sequence number
 *                  at the beginning of the packet.
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#ifdef USE_GDB

#include "db_router.h"

#include <io/conssw.h>
#include <util/plocal.h>
#include <util/ipl.h>
#include <util/emask.h>
#include <util/engine.h>
#include <util/debug.h>
#include <proc/cg.h>
#include <mem/vmparam.h>
#include <util/kdb/kdebugger.h>


#define GDB_DRV "iasy"

#define GDBUG(args) if (db_remote_debug) gdb_printf args

/************************************************************************
 *
 * external low-level support routines
 */

#if 0
typedef void (*ExceptionHook)(int);   /* pointer to function with int parm */
#endif
typedef void (*Function)();           /* pointer to a function */

void gdb_printf();
conschan_t gdb_ioc;

boolean_t use_gdb = B_TRUE;

extern void _debugger(int, int *);
extern int kdb_gdb_select_io (char *, int);
extern void t_notpres(), t_pgflt(), t_gpflt();
extern void (* volatile cdebugger)();

extern int db_myProc ();
extern char db_getDebugChar (int);
extern int db_putDebugChar (char);
extern int db_debugCharReady ();
extern void db_stop_all_other_proc();
extern void db_unstop_proc(int);

extern Function exceptionHandler();  /* assign an exception handler */
#if 0
extern ExceptionHook exceptionHook;  /* hook variable for errors/exceptions */
#endif

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 400

static char initialized[GDB_MAXPROC];  /* boolean flag. != 0 means we've 
					  been initialized */

/*
 * debug > 0 prints ill-formed commands in valid packets & checksum errors
 * it is left as shared variable since it is only read and written, and 
 * applies to all processors.
 */
static int db_remote_debug;

static void db_waitabit();

/*
 * this is readonly, thus shared
 */
static const char hexchars[]="0123456789abcdef";

/* Number of bytes of registers.  */
#define NUMREGBYTES 64
enum regnames {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	       PC /* also known as eip */,
	       PS /* also known as eflags */,
	       CS, SS, DS, ES, FS, GS};

/*
 * processor registers at the time of entering debugger.
 */
static int registers[GDB_MAXPROC][NUMREGBYTES/4];

/* we switch to local stack when entering debugger */
#define STACKSIZE 10000
static int remcomStack[GDB_MAXPROC][STACKSIZE/sizeof(int)];
static int * stackPtr[GDB_MAXPROC];

/*
 * In many cases, the system will want to continue exception processing
 * when a continue command is given.
 * oldExceptionHook is a function to invoke in this case.
 */

#if 0
ExceptionHook exceptionHook;	/* unless defined somewhere else */
static ExceptionHook oldExceptionHook;
#endif

/* forward declarations */
static int db_put_proc_char ();
static void ultohex (char *, int, unsigned long);
static void gdb_start_io ();
static void gdb_end_io ();

/***************************  ASSEMBLY CODE MACROS *************************/
/* 									   */

/* See if MPGDB is the currently active debugger */
#define CHECK_ACTIVE(pass_to)						\
  asm ("pushl %ds");							\
  asm ("pushl %es");							\
  asm ("pushw %fs");							\
  asm ("pushl %eax");							\
  asm ("pushl %ecx");							\
  asm ("pushl %edx");							\
  asm ("movw  $264, %ax");	/*_A_KDSSEL=264 from util/assym.h */	\
  asm ("movw  %ax, %es");						\
  asm ("movw  %ax, %ds");						\
  asm ("cmpl  $db_breakpoint,cdebugger");				\
  asm ("popl %edx");							\
  asm ("popl %ecx");							\
  asm ("popl %eax");							\
  asm ("je .not_a_" #pass_to);						\
  asm ("popw %fs");							\
  asm ("popl %es");							\
  asm ("popl %ds");							\
  asm ("jmp  " #pass_to);						\
  asm (".not_a_" #pass_to ":");						\
  asm ("popw %fs");							\
  asm ("popl %es");							\
  asm ("popl %ds")

extern void db_return_to_prog ();

/* Restore the program's registers (including the stack pointer, which
   means we get the right stack and don't have to worry about popping our
   return address and any stack frames and so on) and return.  */
asm(".text");
asm(".globl db_return_to_prog");
asm("db_return_to_prog:");
asm("        call myRegoff");
asm("        movw 44(%eax), %ss");
asm("        movl 16(%eax), %esp");
asm("        movl 8(%eax), %edx");
asm("        movl 12(%eax), %ebx");
asm("        movl 20(%eax), %ebp");
asm("        movl 24(%eax), %esi");
asm("        movl 28(%eax), %edi");
asm("        movw 48(%eax), %ds");
asm("        movw 52(%eax), %es");
asm("        movw 56(%eax), %fs");
asm("        movw 60(%eax), %gs");
asm("        movl 36(%eax), %ecx");
asm("        pushl %ecx");  /* saved eflags */
asm("        movl 40(%eax), %ecx");
asm("        pushl %ecx");  /* saved cs */
asm("        movl 32(%eax), %ecx");
asm("        pushl %ecx");  /* saved eip */
asm("        movl 4(%eax), %ecx");
asm("        movl (%eax), %eax");
/* use iret to restore pc and flags together so
   that trace flag works right.  */
asm("        iret");

#define BREAKPOINT() asm("   int $3");

asm (".data");

/* Put the error code here just in case the user cares.  */
static int db_gdb_i386errcode[GDB_MAXPROC];

/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */
static int db_gdb_i386vector[GDB_MAXPROC];

/* Address of a routine to RTE to if we get a memory fault.  */
static void (*volatile db_mem_fault_routine[GDB_MAXPROC])();

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int db_mem_err[GDB_MAXPROC];

/* Processor in debugger flags. */
static volatile int stopped[GDB_MAXPROC];

/*
   return offset in the registers array for db_myProc
*/
static int *
myRegoff()
{
  return &registers[db_myProc()][0];
}

/* GDB stores segment registers in 32-bit words (that's just the way
   m-i386v.h is written).  So zero the appropriate areas in registers.  */
#define SAVE_REGISTERS1()						\
  asm ("pushl %eax");							\
  asm ("pushw %fs");							\
  asm ("pushl %ds");							\
  asm ("pushl %es");							\
  asm ("movw  $264, %ax");	/*_A_KDSSEL=264 from util/assym.h */	\
  asm ("movw  %ax, %es");						\
  asm ("movw  %ax, %ds");						\
  asm ("pushl %edx");							\
  asm ("pushl %ecx");							\
  asm ("call myRegoff");						\
  asm ("popl %ecx");							\
  asm ("movl %ecx, 4(%eax)");						\
  asm ("popl %edx");							\
  asm ("movl %edx, 8(%eax)");						\
  asm ("movl %ebx, 12(%eax)");						\
  asm ("movl %ebp, 20(%eax)");						\
  asm ("movl %esi, 24(%eax)");						\
  asm ("movl %edi, 28(%eax)");						\
  asm ("movw $0, %cx");							\
  asm ("popl %edx");		/* original %es */			\
  asm ("movw %dx, 52(%eax)");						\
  asm ("movw %cx, 54(%eax)");						\
  asm ("popl %edx");		/* original %ds */			\
  asm ("movw %dx, 48(%eax)");						\
  asm ("movw %cx, 50(%eax)");						\
  asm ("popw %fs");							\
  asm ("movw %fs, 56(%eax)");						\
  asm ("movw %cx, 58(%eax)");						\
  asm ("movw %gs, 60(%eax)");						\
  asm ("movw %cx, 62(%eax)");						\
  asm ("popl %ebx");		/* get the %eax into %ebx */		\
  asm ("movl %ebx, (%eax)")	/* save %eax */

static int *
db_myGdb_i386errcode ()
{
  return &db_gdb_i386errcode[db_myProc()];
}

#define SAVE_ERRCODE()				\
  asm ("popl %ebx");				\
  asm ("pushl %eax");				\
  asm ("pushl %ecx");				\
  asm ("call db_myGdb_i386errcode");		\
  asm ("movl %ebx, (%eax)");			\
  asm ("popl %ecx");				\
  asm ("popl %eax")

#define SAVE_REGISTERS2()						   \
  asm ("popl %ebx"); /* old eip */					   \
  asm ("movl %ebx, 32(%eax)");						   \
  asm ("popl %ebx");	 /* old cs */					   \
  asm ("movl %ebx, 40(%eax)");						   \
  asm ("movw %cx, 42(%eax)");						   \
  asm ("popl %ebx");	 /* old eflags */				   \
  asm ("movl %ebx, 36(%eax)");						   \
  /* Now that we've done the pops, we can save the stack pointer.       */ \
  asm ("movw %ss, 44(%eax)");						   \
  asm ("movw %cx, 46(%eax)");						   \
  asm ("movl %esp, 16(%eax)")

static void (* volatile* db_myMem_fault_routine ())()
{
  return &db_mem_fault_routine[db_myProc()];
}

/* See if db_mem_fault_routine is set, if so just IRET to that address.  */
#define CHECK_FAULT()							\
  asm ("pushl %ds");							\
  asm ("pushl %es");							\
  asm ("pushw %fs");							\
  asm ("pushl %eax");							\
  asm ("pushl %ecx");							\
  asm ("pushl %edx");							\
  asm ("movw  $264, %ax");	/*_A_KDSSEL=264 from util/assym.h */	\
  asm ("movw  %ax, %es");						\
  asm ("movw  %ax, %ds");						\
  asm ("call db_myMem_fault_routine");					\
  asm ("cmpl $0, (%eax)");						\
  asm ("popl %edx");							\
  asm ("popl %ecx");							\
  asm ("popl %eax");							\
  asm ("jne db_mem_fault");						\
  asm ("popw %fs");							\
  asm ("popl %es");							\
  asm ("popl %ds")

asm (".text");
asm ("db_mem_fault:");
/* OK to clobber temp registers; we're just going to end up in db_set_mem_err.  */
/* Pop error code from the stack and save it.  */
asm ("     popw %fs");		/* pushed in CHECK_FAULT */
asm ("     popl %es");		/* since we are already in GDB,  */
asm ("     popl %ds");		/* %es and %ds should be OK */
asm ("	   call db_myGdb_i386errcode");
asm ("     popl %ecx");
asm ("     movl %ecx, (%eax)");

asm ("     popl %eax"); /* eip */
/* We don't want to return there, we want to return to the function
   pointed to by db_mem_fault_routine instead.  */
asm ("     call db_myMem_fault_routine");
asm ("     popl %ecx"); /* cs (low 16 bits; junk in hi 16 bits).  */
asm ("     popl %edx"); /* eflags */

/* Remove this stack frame; when we do the iret, we will be going to
   the start of a function, so we want the stack to look just like it
   would after a "call" instruction.  */
asm ("     leave");

/* Push the stuff that iret wants.  */
asm ("     pushl %edx"); /* eflags */
asm ("     pushl %ecx"); /* cs */
asm ("     pushl (%eax)"); /* eip */

/* Zero db_mem_fault_routine.  */
asm ("     movl $0, (%eax)");

asm ("iret");

#define CALL_HOOK() asm("call remcomHandler")

/* This function is called when a i386 exception occurs.  It saves
 * all the cpu regs in the _registers array, munges the stack a bit,
 * and invokes an exception handler (db_remcom_handler).
 *
 * stack on entry:                       stack on exit:
 *   old eflags                          vector number
 *   old cs (zero-filled to 32 bits)
 *   old eip
 *
 */
extern void _catchException3();
asm(".text");
asm(".globl _catchException3");
asm("_catchException3:");
CHECK_ACTIVE(t_int3);
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $3");
CALL_HOOK();

/* Same thing for exception 1.  */
extern void _catchException1();
asm(".text");
asm(".globl _catchException1");
asm("_catchException1:");
CHECK_ACTIVE(t_dbg);
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $1");
CALL_HOOK();

#if 0
/* Same thing for exception 0.  */
extern void _catchException0();
asm(".text");
asm(".globl _catchException0");
asm("_catchException0:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $0");
CALL_HOOK();

/* Same thing for exception 4.  */
extern void _catchException4();
asm(".text");
asm(".globl _catchException4");
asm("_catchException4:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $4");
CALL_HOOK();

/* Same thing for exception 5.  */
extern void _catchException5();
asm(".text");
asm(".globl _catchException5");
asm("_catchException5:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $5");
CALL_HOOK();

/* Same thing for exception 6.  */
extern void _catchException6();
asm(".text");
asm(".globl _catchException6");
asm("_catchException6:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $6");
CALL_HOOK();

/* Same thing for exception 7.  */
extern void _catchException7();
asm(".text");
asm(".globl _catchException7");
asm("_catchException7:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $7");
CALL_HOOK();

/* Same thing for exception 8.  */
extern void _catchException8();
asm(".text");
asm(".globl _catchException8");
asm("_catchException8:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $8");
CALL_HOOK();

/* Same thing for exception 9.  */
extern void _catchException9();
asm(".text");
asm(".globl _catchException9");
asm("_catchException9:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $9");
CALL_HOOK();

/* Same thing for exception 10.  */
extern void _catchException10();
asm(".text");
asm(".globl _catchException10");
asm("_catchException10:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $10");
CALL_HOOK();

/* Same thing for exception 12.  */
extern void _catchException12();
asm(".text");
asm(".globl _catchException12");
asm("_catchException12:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $12");
CALL_HOOK();

/* Same thing for exception 16.  */
extern void _catchException16();
asm(".text");
asm(".globl _catchException16");
asm("_catchException16:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $16");
CALL_HOOK();
#endif

/* For 13, 11, and 14 we have to deal with the CHECK_FAULT stuff.  */

/* Same thing for exception 13.  */
extern void _catchException13 ();
asm (".text");
asm (".globl _catchException13");
asm ("_catchException13:");
CHECK_ACTIVE(t_gpflt);
CHECK_FAULT();
/* if CHECK_FAULT returns OS handles the exception */
asm ("jmp t_gpflt");

/* Same thing for exception 11.  */
extern void _catchException11 ();
asm (".text");
asm (".globl _catchException11");
asm ("_catchException11:");
CHECK_ACTIVE(t_notpres);
CHECK_FAULT();
/* if CHECK_FAULT returns OS handles the exception */
asm ("jmp t_notpres");

/* Same thing for exception 14.  */
extern volatile int prmpt_state;
extern void _catchException14 ();
asm (".text");
asm (".globl _catchException14");
asm ("_catchException14:");
CHECK_ACTIVE(t_pgflt);
CHECK_FAULT();
/* if CHECK_FAULT returns OS handles the exception */
asm ("jmp t_pgflt");


static int *
myStackPtr ()
{
  return stackPtr[db_myProc()];
}

/*
 * remcomHandler is a front end for db_handle_exception.  It moves the
 * stack pointer into an area reserved for debugger use.
 */
asm("remcomHandler:");
asm("           popl %ecx");        /* pop off return address     */
asm("           call myStackPtr"); /* get stackPtr[db_myProc()] in %eax */
asm("           popl %ecx");      /* get the exception number   */
asm("		movl %eax, %esp"); /* move to remcom stack area  */
asm("		pushl %ecx");	/* push exception onto stack  */
asm("		call  db_handle_exception");    /* this never returns */

#if 0
/* this is stolen from util/kdb/kdebugger.h */
asm void db_flushtlb(void)
{
	movl	%cr3, %eax
	movl	%eax, %cr3
}
#endif

static void 
_returnFromException()
{
  db_return_to_prog ();
}


int
isxdigit (char c)
{
  if (('0' <= c && c <= '9') || 
      ('a' <= c && c <= 'f') ||
      ('A' <= c && c <= 'F'))
    return 1;
  return 0;
}

static int
hex(char ch)
{
  if ((ch >= 'a') && (ch <= 'f')) return (ch-'a'+10);
  if ((ch >= '0') && (ch <= '9')) return (ch-'0');
  if ((ch >= 'A') && (ch <= 'F')) return (ch-'A'+10);
  return (-1);
}

long
db_hextol (char *s)
{
  long l;

  for (l = 0; isxdigit(*s); s++)
    l = l*16 + hex(*s);
  return l;
}

void
db_ultohex (char *s, int prec, unsigned long l)
{
  for (prec--; prec>=0; prec--, l /= 16)
    {
      s[prec] = "0123456789abcdef"[l & 0xf];
    }
}

/* sequnce number of the last request, to be used in generating the reply */
static char seq[GDB_MAXPROC][2];

/* scan for the sequence $[<seqnum>:]<data>#<checksum>     */
static void 
getpacket(buffer)
     char * buffer;
{
  unsigned char checksum;
  unsigned char xmitcsum;
  int  i;
  int  count;
  char ch;

    /* wait around for the start character, ignore all other characters */
    while ((ch = (db_get_proc_char() & 0x7f)) != '$');
    checksum = 0;
    xmitcsum = -1;

    count = 0;

    /* now, read until a # or end of buffer is found */
    while (count < BUFMAX-4) {
      ch = db_get_proc_char() & 0x7f;
      if (ch == '#') break;
      checksum = checksum + ch;
      buffer[count] = ch;
      count = count + 1;
      }
    buffer[count] = 0;

    if (ch == '#') {
      xmitcsum = hex(db_get_proc_char() & 0x7f) << 4;
      xmitcsum += hex(db_get_proc_char() & 0x7f);
      if ((db_remote_debug ) && (checksum != xmitcsum)) {
        GDBUG(("bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
		    checksum,xmitcsum,buffer));
      }

      if (checksum != xmitcsum) 
	db_put_proc_char('-');  /* failed checksum */
      else {
	 /* if a sequence char is present, reply the sequence ID */
	if (count > 3 &&
	    isxdigit(buffer[0]) && 
	    isxdigit(buffer[1]) && 
	    buffer[2] == ':') {
	  /* store the sequence number for reply */
	  seq[db_myProc()][0] = buffer[0];
	  seq[db_myProc()][1] = buffer[1];
	  /* remove sequence chars from buffer */
	  GDBUG(("About to bcopy 0x%x bytes\n", count-2));
	  bcopy (&buffer[3], &buffer[0], count-2);
	}
      }
    }
  else
    GDBUG(("Hey, no pound sign?!\n"));
}

/* send the packet in buffer.  */

static void 
putpacket(buffer, reply)
     char * buffer;
     int reply;
{
  unsigned char checksum;
  int  count;
  char ch;

  /* clear the port first */
  db_clear_port (&db_proc_out[db_myProc()]);

  /*  $<packet info>#<checksum>. */
  db_put_proc_char('$');
  
  if (reply)
    {
       /* put the sequence number:
	 $ss.<packet info>#<checksum>.
       */
      db_put_proc_char(seq[db_myProc()][0]);
      db_put_proc_char(seq[db_myProc()][1]);
      db_put_proc_char('.');
      checksum = seq[db_myProc()][0]+seq[db_myProc()][1]+'.';
    }
  else
    {
      checksum = 0;
    }
  count    = 0;

  while (ch=buffer[count]) {
    if (! db_put_proc_char(ch)) return;
    checksum += ch;
    count += 1;
  }

  db_put_proc_char('#');
  db_put_proc_char(hexchars[(checksum >> 4) & 0xf]);
  db_put_proc_char(hexchars[checksum & 0xf]);
}

static char  remcomInBuffer[GDB_MAXPROC][BUFMAX];
static char  remcomOutBuffer[GDB_MAXPROC][BUFMAX];
static short error[GDB_MAXPROC];

static void
db_set_mem_err ()
{
  db_mem_err[db_myProc()] = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to db_mem_fault, they won't get restored, so there better not be any
   saved).  */

static int
db_get_char (addr)
     char *addr;
{
  return *addr;
}

static void
db_set_char (addr, val)
     char *addr;
     int val;
{
  *addr = val;
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set db_mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
static char * 
mem2hex(mem, buf, count, may_fault)
     char* mem;
     char* buf;
     int   count;
     int may_fault;
{
      int i;
      unsigned char ch;

      if (may_fault)
	db_mem_fault_routine[db_myProc()] = db_set_mem_err;
      for (i=0;i<count;i++) {
          ch = db_get_char (mem++);
	  if (may_fault && db_mem_err[db_myProc()])
	    return (buf);
          *buf++ = hexchars[ch >> 4];
          *buf++ = hexchars[ch % 16];
      }
      *buf = 0;
      if (may_fault)
	  db_mem_fault_routine[db_myProc()] = NULL;
      return(buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
static char *
hex2mem(buf, mem, count, may_fault)
     char* buf;
     char* mem;
     int   count;
     int may_fault;
{
      int i;
      unsigned char ch;
      pte_t *ptep;
      unsigned int oldprot;

      if (may_fault)
	  db_mem_fault_routine[db_myProc()] = db_set_mem_err;
      for (i=0;i<count;i++) {
          ch = hex(*buf++) << 4;
          ch = ch + hex(*buf++);

          /* get the right text page, make page writeable, modify page,
             change page back to read only.
           */
          ptep = kvtol2ptep(mem);
          oldprot = ptep->pg_pte & PTE_PROTMASK;
          PG_SETPROT(ptep, PG_RW);
          TLBSflushtlb();

          db_set_char (mem++, ch); /* modify page */

          PG_CLRPROT(ptep);
          PG_SETPROT(ptep, oldprot); /* restore old protection bit */

	  if (may_fault && db_mem_err[db_myProc()])
	    return (mem);
      }
      if (may_fault)
	  db_mem_fault_routine[db_myProc()] = NULL;
      return(mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
static int 
computeSignal( exceptionVector )
     int exceptionVector;
{
  int sigval;
  switch (exceptionVector) {
    case 0 : sigval = 8; break; /* divide by zero */
    case 1 : sigval = 5; break; /* debug exception */
    case 3 : sigval = 5; break; /* breakpoint */
    case 4 : sigval = 16; break; /* into instruction (overflow) */
    case 5 : sigval = 16; break; /* bound instruction */
    case 6 : sigval = 4; break; /* Invalid opcode */
    case 7 : sigval = 8; break; /* coprocessor not available */
    case 8 : sigval = 7; break; /* double fault */
    case 9 : sigval = 11; break; /* coprocessor segment overrun */
    case 10 : sigval = 11; break; /* Invalid TSS */
    case 11 : sigval = 11; break; /* Segment not present */
    case 12 : sigval = 11; break; /* stack exception */
    case 13 : sigval = 11; break; /* general protection */
    case 14 : sigval = 11; break; /* page fault */
    case 16 : sigval = 7; break; /* coprocessor error */
    default:
      sigval = 7;         /* "software generated"*/
  }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
static int
hexToInt(char **ptr, int *intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex(**ptr);
        if (hexValue >=0)
        {
            *intValue = (*intValue <<4) | hexValue;
            numChars ++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}

unsigned long
db_read_debug_reg (int reg)
{
  switch (reg)
    {
    case '0':			/* debug address registers */
      asm("mov	%dr0, %eax");
      break;

    case '1':
      asm("mov	%dr1, %eax");
      break;

    case '2':
      asm("mov	%dr2, %eax");
      break;

    case '3':
      asm("mov	%dr3, %eax");
      break;
		
    case 'S':			/* debug status register */
      asm("mov	%dr6, %eax");
      break;

    case 'C':			/* debug control register */
      asm("mov	%dr7, %eax");
      break;

    default:
      return 0;
    }
}

void
db_write_debug_reg (unsigned long val, int reg)
{
  switch (reg)
    {
    case '0':			/* debug address registers */
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr0");
      break;

    case '1':
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr1");
      break;

    case '2':
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr2");
      break;

    case '3':
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr3");
      break;
		
    case 'S':			/* debug status register */
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr6");
      break;

    case 'C':			/* debug control register */
      asm("mov	8(%ebp), %eax");
      asm("mov	%eax, %dr7");
      break;
    }
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
int reentered[GDB_MAXPROC];	/* just to see if we ever reentered debugger */

static void
db_handle_exception (int exceptionVector)
{
  int    sigval;
  int    addr, length;
  char * ptr;
  int    newPC;

  if (!use_gdb)
    _returnFromException ();

  if (stopped[db_myProc()])
    {
      reentered[db_myProc()] = 1;
      _returnFromException ();
    }

  stopped[db_myProc()] = 1;

  db_gdb_i386vector[db_myProc()] = exceptionVector;

  gdb_start_io ();

  GDBUG(("vector=0x%x, sr=0x%x, pc=0x%x\n",
	 exceptionVector,
	 registers[db_myProc()][ PS ],
	 registers[db_myProc()][ PC ]));

  db_spin_lock_port (&db_proc_in[db_myProc()]);
  db_clear_port (&db_proc_in[db_myProc()]);
  db_unlock_port (&db_proc_in[db_myProc()]);

  db_write_debug_reg (0, 'C');	/* clear debug control register */

  /* reply to host that an exception has occurred */
  sigval = computeSignal( exceptionVector );
  strcpy (remcomOutBuffer[db_myProc()], "T??proc:??;");
  db_ultohex (remcomOutBuffer[db_myProc()]+1, 2, sigval);
  db_ultohex (remcomOutBuffer[db_myProc()]+8, 2, db_myProc());

  putpacket(remcomOutBuffer[db_myProc()], 0);

  while (1==1) {
    error[db_myProc()] = 0;
    remcomOutBuffer[db_myProc()][0] = 0;
    getpacket(remcomInBuffer[db_myProc()]);
    switch (remcomInBuffer[db_myProc()][0]) {
      case '?' :   remcomOutBuffer[db_myProc()][0] = 'S';
                   remcomOutBuffer[db_myProc()][1] =  hexchars[sigval >> 4];
                   remcomOutBuffer[db_myProc()][2] =  hexchars[sigval % 16];
                   remcomOutBuffer[db_myProc()][3] = 0;
                 break;
      case 'd' : db_remote_debug = !(db_remote_debug);  /* toggle debug flag */
                 break;
      case 'g' : /* return the value of the CPU registers */
                mem2hex((char*) &registers[db_myProc()][0], 
			remcomOutBuffer[db_myProc()], NUMREGBYTES, 0);
                break;
      case 'G' : /* set the value of the CPU registers - return OK */
                hex2mem(&remcomInBuffer[db_myProc()][1], 
			(char*) &registers[db_myProc()][0], NUMREGBYTES, 0);
                strcpy(remcomOutBuffer[db_myProc()],"OK");
                break;

      /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      case 'm' :
		    /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[db_myProc()][1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length))
                            {
                                ptr = 0;
				db_mem_err[db_myProc()] = 0;
                                mem2hex((char*) addr, 
					remcomOutBuffer[db_myProc()], length, 1);
				if (db_mem_err[db_myProc()]) {
				    strcpy (remcomOutBuffer[db_myProc()], "E03");
				    GDBUG(("memory fault"));
				}
                            }

                    if (ptr)
                    {
		      strcpy(remcomOutBuffer[db_myProc()],"E01");
		      GDBUG(("malformed read memory command: %s",
			     remcomInBuffer[db_myProc()]));
		    }
	          break;

	/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
	/* B in place of M will do the same thing, for now, later it
	   should be separate command which would allow write access
	   to the page, store the data, and then restore the access
	   rights */
      case 'B':
      case 'M' :
		    /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
                    ptr = &remcomInBuffer[db_myProc()][1];
                    if (hexToInt(&ptr,&addr))
                        if (*(ptr++) == ',')
                            if (hexToInt(&ptr,&length))
                                if (*(ptr++) == ':')
                                {
				    db_mem_err[db_myProc()] = 0;
                                    hex2mem(ptr, (char*) addr, length, 1);

				    if (db_mem_err[db_myProc()]) {
					strcpy (remcomOutBuffer[db_myProc()],
						"E03");
					GDBUG(("memory fault"));
				    } else {
				        strcpy(remcomOutBuffer[db_myProc()],
					       "OK");
				    }

                                    ptr = 0;
                                }
                    if (ptr)
                    {
		      strcpy(remcomOutBuffer[db_myProc()],"E02");
		      GDBUG(("malformed write memory command: %s",
			     remcomInBuffer[db_myProc()]));
		    }
                break;

     /* cAA..AA    Continue at address AA..AA(optional) */
     /* sAA..AA   Step one instruction from AA..AA(optional) */
     case 'c' :
     case 's' :
          /* try to read optional parameter, pc unchanged if no parm */
         ptr = &remcomInBuffer[db_myProc()][1];
         if (hexToInt(&ptr,&addr))
             registers[db_myProc()][ PC ] = addr;

          newPC = registers[db_myProc()][ PC];

          /* clear the trace bit */
          registers[db_myProc()][ PS ] &= 0xfffffeff;

          /* set the trace bit if we're stepping */
          if (remcomInBuffer[db_myProc()][0] == 's') 
	    registers[db_myProc()][ PS ] |= 0x100;

          /*
           * If we found a match for the PC AND we are not returning
           * as a result of a breakpoint (33),
           * trace exception (9), nmi (31), jmp to
           * the old exception handler as if this code never ran.
           */
#if 0
	  /* Don't really think we need this, except maybe for protection
	     exceptions.  */
                  /*
                   * invoke the previous handler.
                   */
                  if (oldExceptionHook)
                      (*oldExceptionHook) (frame->exceptionVector);
                  newPC = registers[db_myProc()][ PC ]; /* pc may have changed  */
#endif /* 0 */

	  stopped[db_myProc()] = 0;
	  gdb_end_io ();
	  db_unstop_proc(db_myProc());
	  db_write_debug_reg (0, 'S'); /* clear debug status register */
	  _returnFromException(); /* this is a jump */

          break;

      /* kill the program */
      case 'k' :  /* do nothing */
#if 0
	/* Huh? This doesn't look like "nothing".
	   m68k-stub.c and sparc-stub.c don't have it.  */
		BREAKPOINT();
#endif
                break;

	/* this is special command used for stopping processor. no
	   reply is expected.
	 */
      case 'Z':
	continue;		/* no reply! */

      case 'Q':
	if (strcmp(&remcomInBuffer[db_myProc()][1], "newdebug") == 0)
	  {
	    /* switch to the next debugger */
	    kdb_next_debugger ();
	    strcpy (remcomOutBuffer[db_myProc()], "OK");
	  }
	else if (strncmp(&remcomInBuffer[db_myProc()][1], "DR", 2) == 0)
	  {
	    /* debug register commands */

	    char *p = &remcomInBuffer[db_myProc()][1];
	    unsigned long val;

	    if (p[3] == '=')
	      {
		/* write */
		val = db_hextol (p+4);
		db_write_debug_reg (val, p[2]);
		continue;	/* no reply! */
	      }
	    else
	      {
		/* read */
		val = db_read_debug_reg (p[2]);
		db_ultohex(remcomOutBuffer[db_myProc()], 8, val);
		remcomOutBuffer[db_myProc()][8] = 0;
	      }
	  }
	break;

      } /* switch */

    /* reply to the request */
    putpacket(remcomOutBuffer[db_myProc()], 1);
  }
}

/*
   Output to the GDB window. It should be used cautiously since it can 
   interfere with the GDB communication protocol. In particular, it 
   cannot be called from itself or any other function it calls directly or
   indirectly (e.g. putpacket, db_proc_putpkt, etc.)
*/

static db_lock_t print_lck;

void
gdb_printf(fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)
     char *fmt;
{
  int i;
  static char buf[BUFMAX];
  
  gdb_start_io ();

  db_spin_lock (&print_lck);

  sprintf (buf, "O%x ", db_myProc());
  sprintf (buf+3, 
	   fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  for (i = 0; i < strlen(buf); i++)
    {
      if (buf[i] == '#' ||
	  buf[i] == '$')
	buf[i] = '@';
    }
  putpacket(buf, 0);

  sprintf (buf, "gdb_printf before unlock\n");
  putpacket(buf, 0);

  db_unlock (&print_lck);

  sprintf (buf, "gdb_printf before gdb_end_io\n");
  putpacket(buf, 0);
  
  gdb_end_io ();
}

/* 
   this function is used to set up exception handlers for tracing and
   breakpoints.
   Called from gdb_init ().
*/

static void
db_set_debug_traps()
{
  extern void remcomHandler();
  int exception;
  int i;

  DB_FOR_EACH_PROC(i)
    {
      stackPtr[i] = &remcomStack[i][STACKSIZE/sizeof(int) - 1];
      db_gdb_i386vector[i] = -1;
      stopped[i] = 0;
      initialized[i] = 1;
    }

#if 0
  /* these are not used since the handlers are hardcoded in sysinit.c */

  exceptionHandler (0, _catchException0);
  exceptionHandler (1, _catchException1);
  exceptionHandler (3, _catchException3);
  exceptionHandler (4, _catchException4);
  exceptionHandler (5, _catchException5);
  exceptionHandler (6, _catchException6);
  exceptionHandler (7, _catchException7);
  exceptionHandler (8, _catchException8);
  exceptionHandler (9, _catchException9);
  exceptionHandler (10, _catchException10);
  exceptionHandler (11, _catchException11);
  exceptionHandler (12, _catchException12);
  exceptionHandler (13, _catchException13);
  exceptionHandler (14, _catchException14);
  exceptionHandler (16, _catchException16);
#endif

}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger. */

void
db_breakpoint()
{
  /* if stopped already, just return */
  if (initialized[db_myProc()] && !stopped[db_myProc()])
    {
      BREAKPOINT();
    }
}

/* debugger ports */
db_port_t gdb_in, gdb_out;

/* processor ports */
db_port_t db_proc_in[GDB_MAXPROC], db_proc_out[GDB_MAXPROC];

/*
   Put char CH to processor _out port. If port gets filled up
   poll the router until it is emptied.
*/

static int 
db_put_proc_char (ch)
     char ch;
{
  db_port_t *p = &db_proc_out[db_myProc()];

  db_spin_lock_port(p);

  if (p->p_last >= PBUFSIZ - 2)
    {
      db_unlock_port (p);
      db_router_poll ();
      return 0;
    }

  db_port_append_char (p, ch);
  
  if (db_port_full(p))
    {
      while (!db_port_empty(p))
	{
	  db_unlock_port(p);
	  db_router_poll ();
	  db_spin_lock_port(p);
	}
    }

  db_unlock_port (p);

  return 1;
}

/*
   If there is no more characters, poll the router.
   If it's the last character, clear the buffer and return the character.
   If there is a character, return it.
*/

static int
db_get_proc_char ()
{
  db_port_t *p = &db_proc_in[db_myProc()];
  char c;
  
  db_spin_lock_port (p);

  c = db_port_next_char (p);
  if (c == -1)
    {
      db_clear_port (p);
      db_unlock_port (p);
      db_router_poll();
      return -1;
    }

  if (p->p_last <= p->p_first)
    {
      db_clear_port (p);
    }

  db_unlock_port (p);

  return c;
}

/*
   Port "device" is ready is the port is full.
*/

static int 
db_proc_read_ready (p)
     db_port_t *p;
{
  return db_port_full(p);
}

/*
   Copy data from BUF to port P. If the port owner processor is other 
   than me, and the processor is not stopped and is active, interrupt
   him.
*/

static int
db_proc_putpkt (p, buf)
     db_port_t *p;
     char *buf;
{
#ifndef UNIPROC
  int e = (int)p->p_info;
#endif

  strcpy (p->p_buf, buf);
  p->p_last = strlen (buf)-1;

#ifndef UNIPROC
  if (db_port_full (p) &&
      e != db_myProc() &&
      !stopped[e] &&
      !(engine[e].e_flags & E_NOWAY))
    {
      emask_t targets, responders;
      EMASK_CLRALL(&targets);
      EMASK_SET1(&targets, e);
      xcall (&targets, &responders, db_breakpoint, 0);
    }
#endif
}

/*
   Return 1 if port is full, 0 otherwise.
*/

static int
db_proc_getpkt (p)
     db_port_t *p;
{
  if (db_port_full(p))
    {
      int procid = (int)p->p_info;

      p->p_buf[p->p_last+1] = 0;

      return 1;		/* got a full message! */
    }

  return 0;			/* not full message yet */
}

/*
   GDB device is always ready. gdb_getpkt will use nonblocking
   db_getDebugChar to read the device and avoid blocking if it
   is actually not ready for reading.
*/

static int 
db_gdb_read_ready (p)
     db_port_t *p;
{
  return 1;
}

/*
   Send LEN bytes from BUF to GDB device.
*/

static int
db_writen (buf, len)
     char *buf;
     int len;
{
  int n;
  int all = len;

  while (len)
    {
      n = db_putDebugChar(*buf);
      if (n<0)
	{
	  return n;
	}
      len -= n;
      buf += n;
    }
  return all;
}

/*
   Send a character down the GDB device. Block until the char is 
   accepted by the driver.
*/

static int 
db_putDebugChar (char c)
{
  while (console_putc(&gdb_ioc, c) == 0)
    ;
  return 1;
}

/*
   Read a character from GDB device. Block indicates if we should 
   block until a character arrives.
*/

static char
db_getDebugChar (int block)
{
  char c;

  if (block)
    while ((c = console_getc(&gdb_ioc)) == -1)
      ;
  else
    c = console_getc(&gdb_ioc);

  return c;
}

/*
   Read characters from GDB through the end of current packet.
*/

static void
db_seek_eom ()
{
  char c, c2[2];
  
  while (1)
    {
      c = db_getDebugChar (1);
      if (c == '#')
	{
	  c2[0] = db_getDebugChar (1);
	  c2[1] = db_getDebugChar (1);
	  if (isxdigit(c2[0]) && isxdigit(c2[1]))
	    return;
	}
    }
}

/*
   Send packet down the GDB device. Insert sequnce number at the beginning,
   after '$' sign. Wait for an ACK.
*/

static int
db_gdb_putpkt (p, buf)
     db_port_t *p;
     char *buf;
{
  char seqstr[4];
  char ackreply[4];
  unsigned char csum;
  char c;
  int missed_some = 0;
  int toprocid = (int)p->p_info;
  
  p->p_last = strlen (buf)-1;
  /* insert the seqnum */
  bcopy (buf, &p->p_buf[3], p->p_last+1);
  p->p_buf[0] = '$';
  p->p_last += 3;
  strcpy (seqstr, "??:");
  db_ultohex (seqstr, 2, db_get_sendseqnum());
  strncpy (&p->p_buf[1], seqstr, 3);
  
  /* recompute checksum now */
  db_ultohex (&p->p_buf[p->p_last-1], 2, db_checksum(p->p_buf));
  
  do {
    db_writen (p->p_buf, p->p_last+1);
    c = db_getDebugChar (1);
    if (c != '+')
      {
	db_seek_eom ();
	missed_some = 1;
	continue;
      }
    ackreply[0] = db_getDebugChar (1);
    ackreply[1] = db_getDebugChar (1);
    ackreply[2] = 0;
  } while (strncmp (ackreply, seqstr, 2));
  db_inc_sendseqnum ();
  if (missed_some)
    {
      db_writen ("-", 1);
    }

  db_clear_port (p);
  return 0;
}

/*
   Read a character from GDB device and send an ACK if packet is complete.
   Drop out of sequence packets and bad checksum packets.
   Flush buffer on arrival of a '$'.
   Return 1, if a complete packet is received.
   Return 0, if not complete yet.
*/

static int
db_gdb_getpkt (p)
     db_port_t *p;
{
  int c;
  int procid = (int)p->p_info;

  c = db_getDebugChar (0);

  if (c == -1)
    return 0;

  if (c == '$')
    {
      db_clear_port(p);
    }
  else
    {
      /* ignore the character other than '$' at the beginning */
      if (db_port_empty(p))
	return 0;
    }
  db_port_append_char (p, c);

  if (db_port_full(p))
    {
      int i, pktcsum=0;

      p->p_buf[p->p_last+1] = 0;

      pktcsum = hex(p->p_buf[p->p_last-1])*16 + hex(p->p_buf[p->p_last]);

      if (pktcsum != db_checksum(p->p_buf))
	{
	  /* drop an invalid message from a processor */
	  db_writen ("-", 1);
	}
      else
	{
	  /* OK checksum */
	  char ackstr[4];
	  int seq;
	  
	  ackstr[0] = '+';
	  ackstr[1] = p->p_buf[1];
	  ackstr[2] = p->p_buf[2];
	  ackstr[3] = 0;
	  db_writen (ackstr, 3);
	  seq = hex(ackstr[1])*16 + hex(ackstr[2]);
	  if (db_old_recvseqnum (seq) && 
	      strncmp(&(p->p_buf[4]),"Qproc=", 6) == 0)
	    {
	      db_clear_port (p);
	      return 0;		/* drop an out of sequence packet */
	    }
	  else
	    {
	      db_set_recvseqnum (seq);
	      return 1;		/* full message! */
	    }
	}
    }
  return 0;			/* not full message yet */
}

static db_port_ops_t db_gdb_ops = {
  db_gdb_read_ready,
  db_gdb_getpkt,
  db_gdb_putpkt,
};

static db_port_ops_t db_proc_ops = {
  db_proc_read_ready,
  db_proc_getpkt,
  db_proc_putpkt,
};

/*
   Initialize processor and debugger ports.
*/

static void
db_init_ports ()
{
  int i;

  db_init_port (&gdb_in, &db_gdb_ops, -1);
  db_init_port (&gdb_out, &db_gdb_ops, -1);

  DB_FOR_EACH_PROC(i)
    {
      db_init_port (&db_proc_in[i], &db_proc_ops, i);
      db_init_port (&db_proc_out[i], &db_proc_ops, i);
    }
}

/*
   Current processor number.
*/

static int
db_myProc ()
{
#ifdef UNIPROC
  return 0;
#else
  return myengnum;
#endif
}

/*
   Interrupt all processors.
*/

void
db_stop_all_other_proc ()
{
#ifndef UNIPROC
  emask_t responders;
  xcall_all (&responders, B_TRUE, db_breakpoint, 0);
#endif
}

/*
   called before processor continues.
*/

void
db_unstop_proc (int i)
{
  /* nothing ?! */
}

static db_lock_t db_io_lock;
static int in_debugger;

/*
   Suspend console unless already suspended. Do it serially using 
   db_io_lock.
*/

static void
gdb_start_io ()
{
  db_spin_lock (&db_io_lock);

  in_debugger++;

  if (!(gdb_ioc.cnc_flags & CNF_SUSPENDED))
    console_suspend (&gdb_ioc);

  db_unlock (&db_io_lock);
}

/*
   Resume console if suspended. Do it serially using 
   db_io_lock.
*/

static void
gdb_end_io ()
{
  db_spin_lock (&db_io_lock);

  in_debugger--;

  if (gdb_ioc.cnc_flags & CNF_SUSPENDED && in_debugger == 0)
    console_resume (&gdb_ioc);

  db_unlock (&db_io_lock);
}

/*
   This function should be called whenever a character is available at
   the debugging IO device. The main job of this function is to do router
   polling.

   If we get a packet, we just call debugger.
*/

void
db_dev_ready ()
{
  db_port_t *p = &db_proc_in[db_myProc()];

  gdb_start_io ();

  db_router_poll();

  gdb_end_io ();

  db_spin_lock_port (p);

  if (db_port_full (p))
    {
      db_unlock_port (p);
      (*cdebugger)();
    }
  else
    {
      db_unlock_port (p);
#ifndef UNIPROC
      xcall_intr ();
#endif
    }
}

/*
   Initialize GDB.
*/

void
gdb_init ()
{
  printf ("Initializing GDB...");
  in_debugger = 0;
  db_init_lock (&db_io_lock);
  db_init_lock (&print_lck);
  db_init_ports ();
  db_set_debug_traps ();
  {
    static struct kdebugger gdb;
    if (!kdb_gdb_select_io (GDB_DRV, GDB_DEV))
      {
	printf ("kdb_gdb_select_io failed, cannot open GDB device\n");
	return;
      }
    gdb.kdb_entry = (void (*)())db_breakpoint;
    gdb.kdb_slave_cmd = (void (*)())0;
    gdb.kdb_printf = (void (*)())gdb_printf;
    kdb_register (&gdb);
    if (use_gdb)
      /* make GDB current debugger */
      while (cdebugger != db_breakpoint) 
	kdb_next_debugger ();
    else
      /* make sure GDB is not current debugger */
      while (cdebugger == db_breakpoint) 
	kdb_next_debugger ();
  }
  if (!use_gdb)
    {
      printf ("disabled because USE_GDB boot parameter was set to NO.\n");
      return;
    }
  printf ("done.\n");
  db_breakpoint ();
}

#endif /* USE_GDB */
