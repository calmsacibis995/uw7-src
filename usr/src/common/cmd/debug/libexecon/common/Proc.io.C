#ident	"@(#)debugger:libexecon/common/Proc.io.C	1.4"

#include "ProcObj.h"
#include "Process.h"
#include "Procctl.h"
#include "Proccore.h"
#include "Seglist.h"
#include "sys/regset.h"

gregset_t *
Process::read_greg()
{
	if (state == es_corefile && core)
		return(core->read_greg());
	else if (state != es_dead && pctl)
		return(pctl->read_greg());
	return 0;
}

fpregset_t *
Process::read_fpreg()
{
	if (state == es_corefile && core)
		return(core->read_fpreg());
	else if (state != es_dead && pctl)
		return(pctl->read_fpreg());
	return 0;
}

int
Process::write_greg(gregset_t *greg)
{
	if (state == es_corefile)
		return 0;
	else if (state != es_dead && pctl)
		return(pctl->write_greg(greg));
	return 0;
}

int
Process::write_fpreg(fpregset_t *fpreg)
{
	if (state == es_corefile)
		return 0;
	else if (state != es_dead && pctl)
		return(pctl->write_fpreg(fpreg));
	return 0;
}

// process versions of ProcObj virtual access functions

int
Process::in_stack( Iaddr addr)
{
        return seglist->in_stack(addr);
}

Iaddr
Process::end_stack()
{
        return seglist->end_stack();
}

Process *
Process::process()
{
	return (Process *)this;
}
