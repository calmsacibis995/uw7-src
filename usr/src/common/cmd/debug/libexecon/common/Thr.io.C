#ident	"@(#)debugger:libexecon/common/Thr.io.C	1.9"

#ifdef DEBUG_THREADS

#include "ProcObj.h"
#include "Thread.h"
#include "Procctl.h"
#include "Proccore.h"
#include "Interface.h"
#include "Seglist.h"
#include <ucontext.h>
#include "sys/regset.h"

// read/write registers - if thread is idle, we read from
// or write to descriptor

gregset_t *
Thread::read_greg()
{
	if (is_core())
	{
		if (core)
			return(core->read_greg());
		else
		{
			if (flags & L_INCONSISTENT)
				printe(ERR_reg_read_off_lwp, E_WARNING,
					pobj_name);
			return &map.thr_ucontext.uc_mcontext.gregs;
		}
	}
	else if (state == es_off_lwp)
	{
		if (!read_state())
			return 0;
		if (flags & L_INCONSISTENT)
			printe(ERR_reg_read_off_lwp, E_WARNING,
				pobj_name);
		return &map.thr_ucontext.uc_mcontext.gregs;
	}
	else if (state != es_dead && pctl)
	{
		return(pctl->read_greg());
	}
	return 0;
}

fpregset_t *
Thread::read_fpreg()
{
	if (is_core())
	{
		if (core)
			return(core->read_fpreg());
		else
			// no warning for inconsistent for core
			// files; msg will already have been
			// printed for general regs
			return &map.thr_ucontext.uc_mcontext.fpregs;
	}
	else if (state == es_off_lwp)
	{
		if (!read_state())
			return 0;
		if (flags & L_INCONSISTENT)
			printe(ERR_reg_read_off_lwp, E_WARNING,
				pobj_name);
		return &map.thr_ucontext.uc_mcontext.fpregs;
	}
	else if (state != es_dead && pctl)
	{
		return(pctl->read_fpreg());
	}
	return 0;
}

int
Thread::write_greg(gregset_t *greg)
{
	if (is_core())
		return 0;
	if (state == es_off_lwp)
	{
		// no need to copy - read_greg returned ptr
		// to context registers - they were changed in place
		if (flags & L_INCONSISTENT)
		{
			printe(ERR_reg_write_off_lwp, E_ERROR,
				pobj_name);
			return 0;
		}
		return write_state();
	}
	else if (state != es_dead && pctl)
		return(pctl->write_greg(greg));
	return 0;
}

int
Thread::write_fpreg(fpregset_t *fpreg)
{
	if (is_core())
		return 0;
	if (state == es_off_lwp)
	{
		// no need to copy - read_fgreg returned ptr
		// to context registers - they were changed in place
		if (flags & L_INCONSISTENT)
		{
			printe(ERR_reg_write_off_lwp, E_ERROR,
				pobj_name);
			return 0;
		}
		return write_state();
	}
	else if (state != es_dead && pctl)
		return(pctl->write_fpreg(fpreg));
	return 0;
}

int
Thread::read_state()
{
	if (read(map_addr, sizeof(struct thread_map), 
		(char *)&map) != sizeof(struct thread_map))
	{
		printe(ERR_proc_read, E_ERROR, pobj_name, map_addr);
		return 0;
	}
	return 1;
}

int
Thread::write_state()
{
	if (write(map_addr, (char *)&map, sizeof(struct thread_map)) 
		!= sizeof(struct thread_map))
	{
		printe(ERR_proc_write, E_ERROR, pobj_name, map_addr);
		return 0;
	}
	return 1;
}

// thread versions of ProcObj virtual access functions

// stack bounds remain constant for threads
int
Thread::in_stack( Iaddr addr)
{
	stack_t	*stack;
        if (addr == 0)
		return 0;
	stack = &map.thr_ucontext.uc_stack;

	// initial thread has 0 for stack size
	// it runs on bottom-most portion of stack
	// and so can use the process stack boundaries
	if (stack->ss_size == 0)
		return(seglist->in_stack(addr));

	return((addr >= (Iaddr)stack->ss_sp) && (addr <
		((Iaddr)stack->ss_sp + stack->ss_size)));
}

Iaddr
Thread::end_stack()
{
	stack_t	*stack;
	stack = &map.thr_ucontext.uc_stack;
	// initial thread has 0 for stack size
	// it runs on bottom-most portion of stack
	// and so can use the process stack boundaries
	if (stack->ss_size == 0)
		return(seglist->end_stack());
	return((Iaddr)stack->ss_sp + stack->ss_size);
}
Process *
Thread::process()
{
	return parent;
}

#endif
