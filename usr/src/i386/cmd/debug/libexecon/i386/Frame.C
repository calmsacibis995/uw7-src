#ident	"@(#)debugger:libexecon/i386/Frame.C	1.27"

// Frame.C -- stack frames and register access, i386 version

#include "Reg.h"
#include "Frame.h"
#include "RegAccess.h"
#include "ProcObj.h"
#include "Interface.h"
#include "Symtab.h"
#include "Attribute.h"
#include "Procctl.h"
#include "Instr.h"
#include "global.h"
#include <string.h>
#include <ucontext.h>

/*
 *  This code makes some assumptions about what a "standard"
 *  stack frame looks like.  This is compiler-dependent,
 *  but follows the conventions set down in the 386 ABI.
 *  Registers %esi, %edi and %ebx are callee saved,
 *  and are preserved in the stack frame.
 *  The standard stack looks like this:
 *-----------------------------------------------------*
 *  4n+8(%ebp)	arg word n	|	High Addresses *
 *  		...		|		       *
 *     8(%ebp)	arg word 0	|	Previous frame *
 *-----------------------------------------------------*
 *     4(%ebp)	return addr	|		       *
 *     0(%ebp)	previous %ebp	|	Current frame  *
 *    -4(%ebp)	x words local	|		       *
 *   		...		|		       *
 *   -4x(%ebp)	0th local	|		       *
 *     8(%esp)  caller's %edi	|	(if necessary) *
 *     4(%esp)  caller's %esi	|	  "  "         *
 *     0(%esp)  caller's %ebx	|	Low Addresses  *
 *-----------------------------------------------------*
 *
 * While there is no "standard" function prolog, for heuristic
 * purposes we assume a prolog has the following sequence of
 * instructions:
 * 1)
 *   pushl	%ebp		/ save old frame pointer
 * 2)
 *   movl	%esp,%ebp	/ set new frame pointer
 * 3)
 *   
 * At position 1), %esp points to the return address
 * At position 2), 4(%esp) points to the return address
 * At position 3), 4(%esp) or 4(%ebp) point to the return address
 *
 * For functions returning structures, an extra "argument"
 * is pushed on the stack, the address where the callee should
 * put the structure being returned.
 * In this case there are 2 extra instructions before the standard
 * prolog:
 * 1)
 *  popl	%eax		/ save the return address in eax
 * 2)
 *  xchgl	0(%esp), %eax   / put return address at stack top
 *				/ and save structure address
 *
 * In either case, the prolog may come at the first instruction
 * of the function, or may be the target of a jump from the
 * first instruction
 *
 * If no prolog is present we use disassembly and heuristics
 * to find previous stack frames and arguments.
 */

// Registers for a frame are recorded using the framedata's
// accv (access vector) and saved_regs members.
// The access vector describes how to intepret the values
// contained in saved regs.  These may be an offset off of
// %ebp or %esp, in which case the values are obtained
// by reading the process memory; the values may also be the
// actual register value; actual values are cached once
// the register is read once.  The access vector may also indicate
// that no special information has been saved for a register.
// In this case, for the top level frame or for scratch registers,
// the current register value is read (and cached).  For callee-saved
// registers in earlier frames, we look ahead in the stack to see
// if we can find a saved value.  If not, we read and cache the
// current value.

// callee saved registers have value 0
static unsigned char	scratch_regs[] = {
	1,	// EAX
	1,	// ECX
	1,	// EDX
	0,	// EBX
	0,	// ESI
	0,	// EDI
	0,	// ESP
	0,	// EBP
	1,	// EIP
};

// values for access vector
#define REG_NO_INFO	0
#define REG_ACTUAL	1
#define REG_ADDR	2

struct framedata {
	unsigned char	accv[REG_EIP+1]; // "access vector"
	Iaddr		saved_regs[REG_EIP+1]; // saved registers
	Iaddr		cur_saved[REG_EIP+1]; // setup by fcn_prolog
	int		nargwds;	// no. of words of args
	Iaddr		prevpc;		// return address
	Iaddr		argaddr;	// address of args on stack
	Iaddr		prevsp;		// previous stack pointer
	Iaddr		prevframe;	// previous ebp - special
	short		noprolog;	// does fcn have prolog
			framedata();
			~framedata() {}
};

framedata::framedata()
{
	for ( register int i = 0; i <= REG_EIP ; i++ ) 
	{
		accv[i] = REG_NO_INFO;
		saved_regs[i] = 0;
		cur_saved[i] = 0;
	}
	argaddr = (Iaddr)-1;
	noprolog = -1;
	prevpc = (Iaddr)-1;
	prevsp = (Iaddr)-1;
	prevframe = (Iaddr)-1;
	nargwds = -1;
}

// track depth, where 0 == bottom of stack
// this is only reliable as long as we do not return
// beyond the current frame
struct frameid {
	int depth;
};

FrameId::FrameId(Frame *frame)
{
	id = 0;
	if ( frame ) 
	{
		int	cnt = 0;
		Frame	*f = frame->caller();
		id = new frameid;
		for(; f; cnt++)
			f = f->caller();
		id->depth = cnt;
	}
}

FrameId::~FrameId()
{
	delete id;
}

void
FrameId::null()
{
	delete id;
	id = 0;
}

FrameId &
FrameId::operator=(const FrameId &other)
{
	if ( other.id == 0 && id == 0 )
		return *this;
	else if ( other.id == 0 )
	{
		delete id;
		id = 0;
		return *this;
	}
	else if ( id == 0 ) id = new frameid;
	*id = *other.id;
	return *this;
}

#if DEBUG
void
FrameId::print( char * s )
{
	if (s ) printf(s);
	if ( id == 0 )
		printf(" is null.\n");
	else
		printf(" depth is %d",id->depth);
}
#endif

int
#ifdef __cplusplus
FrameId::operator==(const FrameId& other) const
#else
FrameId::operator==(FrameId& other)
#endif
{
	if ( (id == 0) && ( other.id == 0 ) )
		return 1;
	else if ( id == 0 )
		return 0;
	else if ( other.id == 0 )
		return 0;
	else if ( id->depth != other.id->depth )
		return 0;
	else
		return 1;

}

int
#ifdef __cplusplus
FrameId::operator!=(const FrameId& other) const
#else
FrameId::operator!=(FrameId& other)
#endif
{
	return ! (*this == other);
}

Frame::Frame( ProcObj *npobj )
{
	DPRINT(DBG_FRAME,("new topframe() == %#x\n", this));

	Iaddr	pc, esp;
	data = new framedata;
	level  = 0;
	pobj = npobj;
	epoch = pobj->p_epoch();	// epoch never changes
	pc = getreg(REG_EIP);
	esp = getreg(REG_ESP);
	if (check_stack_bounds && !pobj->in_stack(esp))
	{
		printe(ERR_broken_frame_sp, E_ERROR);
	}
	else if (!pobj->in_text(pc))
	{
		printe(ERR_broken_frame_pc, E_ERROR);
		if (check_stack_bounds &&
			(fixup_stack(pc, esp) != 0))
		{
			data->accv[REG_EIP] = REG_ACTUAL;
			data->saved_regs[REG_EIP] = pc;
			data->accv[REG_ESP] = REG_ACTUAL;
			data->saved_regs[REG_ESP] = esp;
			printe(ERR_frame_fixed, E_WARNING);
		}
	}
	setup();
}

// create new top frame using supplied values for 
// pc and/or stack-pointer
Frame::Frame( Iaddr pc, Iaddr sp, ProcObj *npobj )
{
	data = new framedata;
	level  = 0;
	pobj = npobj;
	epoch = pobj->p_epoch();	// epoch never changes
	if ((pc != 0) && (pc != (Iaddr)-1))
	{
		data->accv[REG_EIP] = REG_ACTUAL;
		data->saved_regs[REG_EIP] = pc;
	}
	if ((sp != 0) && (sp != (Iaddr)-1))
	{
		data->accv[REG_ESP] = REG_ACTUAL;
		data->saved_regs[REG_ESP] = sp;
	}
	setup();
}

Frame::Frame( Frame *prev )
{
	data = new framedata;
	append( prev );
	pobj = prev->pobj;
	level = prev->level + 1;
	epoch = prev->epoch;
	DPRINT(DBG_FRAME,("new next frame(%#x) == %#x\n", prev, this));
}

Frame::~Frame()
{
	DPRINT(DBG_FRAME,("%#x.~Frame()\n", this));
	unlink();
	delete data;
}
int		
Frame::valid()
{
	return(this && data && epoch == pobj->p_epoch()); 
}

FrameId *
Frame::id()
{
	FrameId *fmid = new FrameId(this);
	return fmid;
}

Frame *
Frame::caller()
{
	Iaddr		ebp;
	Iaddr		base;
	Symbol		sym;

	DPRINT(DBG_FRAME,("%#x.caller()\n", this));

	Frame *p = (Frame *) next();
	if ( p ) 
		return p;
	// try to construct  a new frame
	DPRINT(DBG_FRAME,("no next, building it\n"));

	if (data->prevpc == (Iaddr)-1)
	{
		return 0;
	}
	p = new Frame(this);

	// set up new registers for new frame
	p->data->saved_regs[REG_EIP] = data->prevpc;
	p->data->accv[REG_EIP] = REG_ACTUAL;
	p->data->saved_regs[REG_ESP] = data->prevsp; 
	p->data->accv[REG_ESP] = REG_ACTUAL;
	if (data->prevframe)
	{
		// special case for the signal handling code
		// _sigreturn or _sigacthandler.
		// no prolog or saved registers; data->argaddr
		// was set by retaddr()
		// also used for functions with a frame pointer
		// but with no symbol information (find_return
		// was called by retaddr())
		p->data->saved_regs[REG_EBP] = data->prevframe; 
		p->data->accv[REG_EBP] = REG_ACTUAL;
		p->setup();
		return p;
	}
	else if (data->noprolog) 
	{
		base = data->prevsp - 4;
	}
	else 
	{
		// We have a function prolog and it has been executed
		ebp = getreg(REG_EBP);
		if (check_stack_bounds && !pobj->in_stack(ebp))
		{
			delete p;	// destructor does unlink
			return 0;
		}
		p->data->saved_regs[REG_EBP] = ebp;
		p->data->accv[REG_EBP] = REG_ADDR;
		base = ebp;
	}
	//
	// set saved registers
	//
	for(int i = 0; i <= REG_EIP; i++)
	{
		// cur_saved is setup by Frame::retaddr()
		if (data->cur_saved[i])
		{
			p->data->accv[i] = REG_ADDR;
			p->data->saved_regs[i] = base -
				data->cur_saved[i];
		}

	}
	p->setup();
	return p;
}

void
Frame::setup()
{
	Iaddr		ebp;

	DPRINT(DBG_FRAME,("%#x.setup()\n", this));

	// retaddr sets noprolog and saved_regs
	if (!retaddr(data->prevpc, data->prevsp, data->prevframe))
	{
		data->prevpc = (Iaddr)-1;
		return;
	}
	DPRINT(DBG_FRAME,("%#x.setup() prevpc = %#x\n", this, data->prevpc));
	if (data->prevframe)
		return;
	else if (data->noprolog) 
	{
		data->argaddr = data->prevsp;
	}
	else 
	{
		// We have a function prolog and it has been executed
		ebp = getreg(REG_EBP);
		if (check_stack_bounds && !pobj->in_stack(ebp))
		{
			return;
		}
		data->argaddr = ebp + 8;
	}
}

#if LONG_LONG
// read a register pair - must be integer
int
Frame::readreg( RegRef ref1, RegRef ref2, Stype what, Itype& dest )
{
	Itype	dest1;
	Itype	dest2;
	union {
		unsigned long long ull;
		unsigned long ul[2];
	} lul;

	if (ref1 > REG_EIP || ref2 > REG_EIP)
		return 0;
	if (!readreg(ref1, Saddr, dest1) || 
		!readreg(ref2, Saddr, dest2))
		return 0;
	lul.ul[0] = dest1.iaddr;
	lul.ul[1] = dest2.iaddr;
	switch (what)
	{
	case Schar:	dest.ichar = (char)lul.ull;		break;
	case Suchar:	dest.iuchar = (unsigned char)lul.ull;	break;
	case Sint1:	dest.iint1 = (char)lul.ull;		break;
	case Suint1:	dest.iuint1 = (unsigned char)lul.ull;	break;
	case Sint2:	dest.iint2 = (short)lul.ull;		break;
	case Suint2:	dest.iuint2 = (unsigned short)lul.ull;	break;
	case Sint4:	dest.iint4 = (int)lul.ull;		break;
	case Suint4:	dest.iuint4 = (unsigned int)lul.ull;	break;
	case Saddr:	dest.iaddr = (Iaddr)lul.ull;	break;
	case Sbase:	dest.ibase = (Ibase)lul.ull;		break;
	case Soffset:	dest.ioffset = (Ioffset)lul.ull;	break;
	case Sint8:	dest.iint8 = lul.ull; break;
	case Suint8:	dest.iuint8 = lul.ull; break;
	default:	return 0;
	}
	return 1;
}

// write a register pair - must be integer
int
Frame::writereg( RegRef ref1, RegRef ref2, Stype what, Itype& dest )
{
	Itype	dest1, dest2;
	union {
		unsigned long long ull;
		unsigned long ul[2];
	} lul;
	switch (what)
	{
	case Schar:	lul.ull = dest.ichar;		break;
	case Suchar:	lul.ull = dest.iuchar;		break;
	case Sint1:	lul.ull = dest.iint1;		break;
	case Suint1:	lul.ull = dest.iuint1;		break;
	case Sint2:	lul.ull = dest.iint2;		break;
	case Suint2:	lul.ull = dest.iuint2;		break;
	case Sint4:	lul.ull = dest.iint4;		break;
	case Suint4:	lul.ull = dest.iuint4;		break;
	case Suint8:	lul.ull = dest.iuint8;	break;
	case Sint8:	lul.ull = dest.iint8;	break;
	case Saddr:	lul.ull = dest.iaddr;		break;
	case Sbase:	lul.ull = dest.ibase;		break;
	default:	return 0;
	}
	dest1.iaddr = lul.ul[0];
	dest2.iaddr = lul.ul[1];
	return(writereg(ref1, Saddr, dest1) &&
		writereg(ref2, Saddr, dest2));
}
#else
int
Frame::readreg( RegRef, RegRef, Stype what, Itype& dest )
{
	return 0;
}
int
Frame::writereg( RegRef, RegRef, Stype what, Itype& dest )
{
	return 0;
}
#endif
// either read from pobj directly, or from saved locations
// on the stack, using the addresses saved in the access vector
int
Frame::readreg( RegRef which, Stype what, Itype& dest )
{
	if (which > REG_EIP)
		return pobj->readreg(which, what, dest);

	switch(data->accv[which])
	{
	case REG_NO_INFO:
		if ((level != 0) && (scratch_regs[which] == 0))
		{
			// callee-saved 
			// look ahead on stack for saved value
			Frame	*curr = (Frame *)prev();
			for(; curr; curr = (Frame *)curr->prev())
			{
				if (curr->data->accv[which] 
					!= REG_NO_INFO)
					break;
			}
			if (curr)
			{
				// we found one
				if (!curr->readreg(which, what, dest))
					return 0;
				// cache found value at all frames
				// between curr and this, not including
				// either curr or this (calls to
				// Frame::readreg will save them).
				curr = (Frame *)curr->next();
				while(curr && (curr != this))
				{
					curr->data->accv[which] = REG_ACTUAL;
					curr->data->saved_regs[which] =
						dest.iaddr;
					curr = (Frame *)curr->next();
				}
				break;
			}
			// FALLTHROUGH - no saved information
		}
		// level 0 or scratch reg or no saved info
		if (!pobj->readreg(which, what, dest))
			return 0;
		break;
	case REG_ACTUAL:
		dest.iaddr = data->saved_regs[which];
		return 1;
	case REG_ADDR:
		if (!pobj->read(data->saved_regs[which], what,
			dest))
			return 0;
		break;
	default:
		printe(ERR_internal, E_ERROR, "Frame:readreg",
			__LINE__);
		return 0;
	}
	// if here, we need to cache value;
	data->accv[which] = REG_ACTUAL;
	data->saved_regs[which] = dest.iaddr;
	return 1;

}

int
Frame::writereg( RegRef which, Stype what, Itype& dest )
{
	
	Frame	*fptr;
	if (which > REG_EIP)
		return pobj->writereg(which, what, dest);

	// we must find the frame closest to us, but
	// more recent on the stack that has saved the
	// register - we then write the saved location
	// so that when that frame is popped off, we receive
	// the new value; if no frame between us and the
	// top of the stack saves the register, just write
	// the process registers
	// As we go, we cache the new value, where appropriate.
	//
	for(fptr = callee(); fptr; fptr = callee())
	{
		if (fptr->data->accv[which] == REG_ACTUAL)
			fptr->data->saved_regs[which] = dest.iaddr;
		else if (fptr->data->accv[which] == REG_ADDR)
		{
			if (!pobj->write(data->saved_regs[which], what,
				dest))
				return 0;
			break;
		}
	}
	if (!fptr)
	{
		// reached top level
		if (!pobj->writereg(which, what, dest))
			return 0;
	}
	data->accv[which] = REG_ACTUAL;
	data->saved_regs[which] = dest.iaddr;
	return 1;
}

Iaddr
Frame::getreg( RegRef which )
{
	Itype itype;
	if ( !readreg( which, Saddr, itype ) ) 
	{
		if (!pobj->is_dead())
			printe(ERR_read_reg, E_ERROR, pobj->obj_name());
		return 0;
	}
	return itype.iaddr;
}

// return nth word of arguments
// assumes this->caller() has already been invoked to determine
// whether there is a prolog and save registers
Iint4
Frame::argword(int n)
{
	Itype itype;
	Iaddr base;

	if (data->argaddr == 0)
	{
		// have already determined we don't know
		return 0;
	}
	else if (data->argaddr != (Iaddr)-1)
	{
		base = data->argaddr;
	}
	else if (data->noprolog == 0)
	{
		// if prolog, arguments start at 8(%ebp)
		Iaddr	ebp;
		ebp = getreg(REG_EBP);
		base = ebp + 8;
		data->argaddr = base;
	}
	else
	{
		// otherwise, find place arguments start on stack;
		// first argument is just above return address on stack,
		// so we walk back looking for retaddr.

		Iaddr	esp;
		Iaddr	top;

		if (data->prevpc == (Iaddr)-1)
		{
			// don't know return address
			data->argaddr = 0;
			return 0;
		}
		esp = getreg(REG_ESP);
		itype.iaddr = 0;

		if (check_stack_bounds)
			top = pobj->end_stack();
		else
			top = (Iaddr)~0;
		while (esp < top)
		{
			pobj->read(esp, Sint4, itype);
			if (itype.iaddr == data->prevpc)
				break;
			esp += sizeof(int);
		}
		if (itype.iaddr != data->prevpc)
			return 0;
		base = itype.iaddr + 4;
		data->argaddr = base;
	}
	pobj->read( base + (sizeof(int)*n), Sint4, itype );
	return itype.iint4;
}

// Returns nth word of argument list
// assumes we are stopped at the entry point to the function,
// so the arguments start at 4(%esp).
// this is not valid for functions returning structs
// This is used by the thread code in the ProcObj class
// to read the arguments passed by the thread library
// notifier function.
Iint4 
Frame::quick_argword(int n)
{
	Itype	itype;
	Iaddr	esp;

	esp = getreg(REG_ESP);
	pobj->read(esp + 4 + (sizeof(int)*n), Sint4, itype );
	return itype.iint4;
}

// assumes this->caller() has already been called.
int
Frame::nargwds(int &assumed)
{
	// get return address and use instruction there
	// to calculate the number of argument words
	if ( data->nargwds < 0 ) 
	{
		if (data->prevpc == (Iaddr)-1)
		{
			return 0;
		}
		data->nargwds = 
			pobj->instruct()->nargbytes(data->prevpc) / 
				sizeof(int);
	}
	if (data->nargwds == 0)
	{
		// guessing - we can't tell the difference
		// between really having no arguments and not
		// being able to tell how many we have.
		assumed = 1;
		return 3;	 // 3 is arbitrary
	}
	assumed = 0;
	return data->nargwds;
}

Iaddr
Frame::pc_value()
{
	Iaddr	pc;

	// Return pc value for this frame. pc saved by
	// caller() is the return address of the
	// call to the next function in the stack sequence.
	// We adjust here to return the address of the call itself.
	//
	pc = getreg( REG_EIP );
	if ( level > 0 )
	// we have a return address
	{
		return(pc - pobj->instruct()->call_size(pc));
			// sizeof call instruction
	}
	else // top frame
		return(pc);
}

// get return address for current frame; if frame has a prolog,
// save register vars if necessary
// If frame has a prolog, six possible cases:
// 1) right after call; esp points to retaddr
// 2) in functions returning structs, after pop of return
//      address but before xchg - %eax holds retaddr
// 3) prev ebp pushed on stack; esp + 4 points to retaddr
// 4) current ebp set to point to prev ebp; ebp + 4 is retaddr
// 5) in middle of function; current ebp + 4 is retaddr
// 6) after leave instruction restores old frame pointer
// If no prolog, just look for what looks like a return addr.
// Returns previous pc, previous stack pointer, and previous
// frame pointer, if known.
 
#define STACK_BUF	256

int
Frame::retaddr(Iaddr &addr, Iaddr &stack, Iaddr &frame)
{
	Iaddr	ebp, esp, pc;
	Iaddr	orig_esp, orig_ebp;
	Iaddr	fn = 0;
	Itype	itype;
	Symbol	entry;
	int	prosize = 0;
	Iaddr	prostart = 0;

	frame = 0;
	pc = getreg(REG_EIP);
	orig_ebp = ebp = getreg(REG_EBP);
	orig_esp = esp = getreg(REG_ESP);
	entry = pobj->find_entry( pc );
	// get address of function start
	if ( !entry.isnull() ) 
	{
		fn = entry.pc(an_lopc); 
	}
	if ( fn != 0 ) 
	{
		const char	*name;

		name = pobj->symbol_name(entry);
		// check for special kernel signal handlers
		if (strcmp(name, "_sigreturn") == 0)
		{
			gregset_t	gregs;
			// old style signal handler
			// kernel pushes gregset_t after signal number
			if (pobj->read(esp + 4, sizeof(gregset_t),
				(char *)&gregs) != sizeof(gregset_t))
					return 0;
			addr = gregs.greg[EIP];
			stack = gregs.greg[ESP];
			frame = gregs.greg[EBP];
			data->noprolog = 1;
			data->argaddr = esp;
			return 1;
		}
		else if ((strcmp(name, "_sigacthandler") == 0) ||
			(strcmp(name, "_thr_sigacthandler") == 0))
		{
			// We have a new style signal handler.
			// For now, if we are actually stopped
			// in _sigacthandler, we will give up,
			// since we can't be sure where the arguments
			// are without doing a lot of code reading.
			// If we are at or beyond the user handler,
			// _sigacthandler has just pushed the args
			// to the user handler onto the stack.
			// The args to the user handler are:
			// 	int sig, siginfo_t *, ucontext_t *

			Iaddr		contextp = esp + 2*sizeof(int);
			ucontext_t	uap;

			if (level == 0)
				return 0;
			(void)pobj->read(contextp, Saddr, itype);
			if ((check_stack_bounds &&
				!pobj->in_stack(itype.iaddr)) ||
				(pobj->read(itype.iaddr,
				sizeof(ucontext_t), (char *)&uap) 
				!= sizeof(ucontext_t)))
				return 0;
			addr = uap.uc_mcontext.gregs.greg[EIP];
			stack = uap.uc_mcontext.gregs.greg[UESP];
			frame = uap.uc_mcontext.gregs.greg[EBP];
			data->argaddr = esp;
			data->noprolog = 1;
			return 1;
		}
		else if (strcmp(name, "_start") == 0)
		{
			// handle first frame - doesn't have a prolog
			return 0;
		}
#ifdef DEBUG_THREADS
		else if (strcmp(name, "_thr_start") == 0)
		{
			// we can't go back any further
			// the return frame for _thr_start
			// is thr_exit; but it is created
			// artificially and does not look
			// like a normal frame
			return 0;
		}
#endif
		// regular function
		// look for function prolog and save registers
		if (data->noprolog == -1)
		{
			if ((pobj->instruct()->fcn_prolog(fn, prosize,
				prostart, data->cur_saved) == fn) ||
				(check_stack_bounds &&
				!pobj->in_stack(ebp)))
				data->noprolog = 1;
			else
				data->noprolog = 0;
		}
	}
	else
		// can't determine beginning of function
		data->noprolog = 1;
	if (data->noprolog == 0)
	{
		Iaddr	pc_addr;
		int	diff = 0;
		if (pc != fn)
		{
			// might be a jump to prolog
			if (!prostart)
				prostart = pobj->instruct()->jmp_target(fn);
			if (prostart && prostart != fn)
			{
				fn = prostart;
				if (pc < fn)
					// beyond prolog already
					diff = 100; // arbitrary - big
						// enough
				else 
					diff = (int)(pc - fn);
			}
			else
				diff = (int)(pc - fn);
		}
		if (prosize == 8)
		{
			// function returning struct
			if (diff == 1)
			{
				// after pop of return addr
				stack = esp;
				addr = getreg(REG_EAX);
				data->noprolog = 1;
				for(int i = 0; i <= REG_EIP; i++)
					data->cur_saved[i] = 0;
				return 1;
			}
			else if (diff > 5)
				// synch up with regular prolog
				// so we can use common switch below
				diff -= 5;
			else
				diff = 0;
		}
		switch (diff)
		{
		case 0:
			// at beginning of prolog
			pc_addr = esp;
			stack = esp+4;
			data->noprolog = 1;
			break;
		case 1:
			// after ebp has been pushed, but not updated
			pc_addr = esp+4;
			stack = esp+8;
			data->noprolog = 1;
			break;
		default:
			// after ebp has been updated
			if (pobj->instruct()->isreturn(pc))
			{
				// leave instruction has reset esp and
				// popped ebp
				stack = esp+4;
				pc_addr = esp;
				data->noprolog = 1;
			}
			else
			{
				stack = ebp+8;
				pc_addr = ebp+4;
			}
			break;
		}
		pobj->read(pc_addr, Saddr, itype);
		addr = itype.iaddr;
		if (data->noprolog == 1)
		// haven't saved registers yet
			for(int i = 0; i <= REG_EIP; i++)
				data->cur_saved[i] = 0;
		return 1;
	}
	if (!entry.isnull())
	{
		if ((pc - fn) <= prosize)
		{
			// haven't saved registers yet
			for(int i = 0; i <= REG_EIP; i++)
				data->cur_saved[i] = 0;
		}
		// no prolog or after old frame pointer reset
		// look for a pc value on the stack


		Iaddr	sbuf[STACK_BUF];
		int	count;
		int	index;
		int	last = 0;
		Iaddr	top;

		esp = orig_esp;

		DPRINT(DBG_FRAME,("caller() no prolog\n"));
		if (!check_stack_bounds || !pobj->in_stack(esp))
			last = 1;
		else	
			top = pobj->end_stack();
		while(!last)
		{
			count = pobj->read(esp, 
				(STACK_BUF *sizeof(Iaddr)), (char *)sbuf);
			if (count < (STACK_BUF * sizeof(Iaddr)))
				last = 1;
			count /= sizeof(Iaddr);
			for(index = 0; index < count; index++)
			{
				Iaddr	stackword;
				int	i;
				if (top)
				{
					if (esp >= top)
					{
						last = 1;
						break;
					}
				}
				else
				{
					// top could be 0 for a core
					// file
					if (!pobj->in_stack(esp))
					{
						last = 1;
						break;
					}
				}
				stackword = sbuf[index];
				if ( pobj->in_text(stackword) &&
					((i = pobj->instruct()->iscall(stackword, fn))
						== 1))
				{
					addr = stackword;
					stack = esp+4;
					return 1;
				}
				else if (i == -1)
				// stop looking - we have hit a call
				// but we can't figure out the callee address
				{
					last = 1;
					break;
				}
				esp += sizeof(int);
			} 
		}
	}

	// at this point, we probably have a function called
	// via an indirect call through a register; there
	// is nothing we can do but disassemble forward
	// until we find a return

	esp = orig_esp;
	if (!pobj->instruct()->find_return(pc, esp, orig_ebp))
		return 0;
	// find return returns the contents of the stack pointer
	// at the function's return - this should point to the return
	// address
	if ((pobj->read(esp, Saddr, itype) != sizeof(Saddr)) ||
		(!pobj->in_text(itype.iaddr)))
		return 0;
	addr = itype.iaddr;
	stack = esp+4;
	if (orig_ebp != (Iaddr)-1)
	{
		frame = orig_ebp;
		data->argaddr = stack;
	}
	return 1;
}

// try to deal with broken frame (pc not in text)
// look back on stack for valid text address; if found,
// check that previous instruction is a call
int
Frame::fixup_stack(Iaddr &addr, Iaddr &stack)
{
	Iaddr	sbuf[STACK_BUF];
	int	count;
	int	index;
	int	last = 0;
	Iaddr	top, esp;

	esp = stack;
	top = pobj->end_stack();
			// top could be 0 for a core file

	while(!last)
	{
		count = pobj->read(esp, 
			(STACK_BUF * sizeof(Iaddr)), (char *)sbuf);
		if (count < (STACK_BUF * sizeof(Iaddr)))
			last = 1;
		count /= sizeof(Iaddr);
		for(index = 0; index < count; 
			index++, esp += sizeof(int))
		{
			Iaddr	stackword;
			if ((top && (esp >= top)) ||
				!pobj->in_stack(esp))
				return 0;
				
			stackword = sbuf[index];
			if (pobj->in_text(stackword) &&
				(pobj->instruct()->call_size(stackword)
					!= 0))
			{
				addr = stackword;
				stack = esp+4;
				return 1;
			}
		} 
	}
	return 0;
}

// never true on 386
int
Frame::incomplete()
{
	return 0;
}
