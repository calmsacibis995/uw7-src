#ident	"@(#)debugger:libexecon/i386/HW_Watch.C	1.7"


#include "Watchlist.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "Event.h"
#include "Iaddr.h"
#include "ProcObj.h"
#include "Rvalue.h"
#include "TYPE.h"
#include <string.h>
#include <sys/procfs.h>
#include <sys/regset.h>
#include <sys/debugreg.h>

#define MAXSIZE	16	// maximum size of data to watch


// We maintain our own internal copy of the debug register set.
// We set the LWP or Process debug registers to match our copy.
// When a thread is off an LWP, we do not attempt set its debug
// registers until picked up by another LWP.

class HW_Wdata {
	int		initialized;
	dbregset_t	dbreg;
	Iaddr		db_addr[DR_LASTADDR+1];
	void		*db_expr[DR_LASTADDR+1];
	Iaddr 		get_address(int reg) {
				return (Iaddr)dbreg.debugreg[reg]; }
	void		set_address(int reg, Iaddr addr) { 
				dbreg.debugreg[reg] = (unsigned int)addr; }
	void		set_control(unsigned int ctl) { 
				dbreg.debugreg[DR_CONTROL]=ctl; }
	unsigned int	get_control() {
				return dbreg.debugreg[DR_CONTROL]; }
	void		clear_status() { dbreg.debugreg[DR_STATUS] = 0;}
	unsigned int	get_status() { 
				return dbreg.debugreg[DR_STATUS]; }
public:
			HW_Wdata();
			~HW_Wdata() {}
	int		set_wpt(Iaddr, unsigned long, Proclive *, void *);
	int 		remove_wpt(Iaddr, Proclive *, void *);
	int 		triggered(Proclive *);
	int		set_state(Proclive *pctl);
	int		clear_state(Proclive *);
};

HW_Wdata::HW_Wdata()
{
	memset((char *)this, 0, sizeof(*this));
}

HW_Watch::HW_Watch()
{
	hwdata = new HW_Wdata();
}

HW_Watch::~HW_Watch()
{
	delete hwdata;
}

int
HW_Watch::hw_fired(ProcObj *p)
{
	if (!hwdata)
		return 0;
	return hwdata->triggered(p->proc_ctl());
}

int
HW_Watch::add(Iaddr pl, Rvalue *rval, ProcObj *p, void *watchexpr)
{
	unsigned long	sz;
	TYPE		*t;

	t = rval->type();	// previous value for watchpoint
	if ((t->isnull()) // can't get type info
		|| ((sz = t->size()) > MAXSIZE)) // can't watch 
						// item this big
		return 0;
	return(hwdata->set_wpt(pl, sz, p->proc_ctl(), watchexpr));
}

int
HW_Watch::remove(Iaddr place, ProcObj *p, void *watchexpr)
{
	if (!hwdata)
	{
		return 0;
	}
	return(hwdata->remove_wpt(place, p->proc_ctl(), watchexpr));
}

int 
HW_Watch::clear_state(Proclive *pctl)
{
	if (!hwdata)
		return 1;
	return(hwdata->clear_state(pctl));
}

int
HW_Watch::set_state(Proclive *pctl)
{
	if (!hwdata)
		return 1;
	return hwdata->set_state(pctl);
}


int
HW_Wdata::set_wpt(Iaddr addr, unsigned long size, Proclive *pctl, void *watchexpr)
{
	unsigned int	ctl, octl;
	int		i;
	int		reg[DR_LASTADDR+1];
	Iaddr		oaddr, naddr;

	oaddr = addr;

	ctl = octl = get_control();

	// reg keeps track of which registers we modified, so
	// we can fix things up in case of error
	for (i = 0; i <= DR_LASTADDR; i++)
	{
		reg[i] = 0;
	}

	i = DR_FIRSTADDR;
	while(size > 0)
	{
		int	len;
		
		if ((size >= sizeof(int)) && !(addr & (sizeof(int)-1)))
		{
			len = DR_LEN_4;
			size -= sizeof(int);
			naddr = addr + sizeof(int);
		}
		else if ((size >= sizeof(short)) && 
			!(addr & (sizeof(short)-1)))
		{
			len = DR_LEN_2;
			size -= sizeof(short);
			naddr = addr + sizeof(short);
		}
		else
		{
			len = DR_LEN_1;
			size -= 1;
			naddr = addr + 1;
		}
		// find first avail
		while(i <= DR_LASTADDR)
		{
			if (get_address(i) == 0)
				break;
			i++;
		}
		if (i > DR_LASTADDR)
			goto error;
		
		// set up ctl register
		ctl |= (DR_LOCAL_SLOWDOWN | (1<<(i * DR_ENABLE_SIZE)));
		ctl |= (((DR_RW_WRITE | len) << 
			(i * DR_CONTROL_SIZE)) << DR_CONTROL_SHIFT);
		
		set_address(i, addr);
		reg[i] = 1;
		db_addr[i] = oaddr;
		db_expr[i] = watchexpr;
		addr = naddr;
	}
	set_control(ctl);
	if (!pctl || (set_state(pctl) != 0))
		// don't attempt to write if off LWP
		return 1;
error:
	for(i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if (reg[i])
		{
			set_address(i, 0);
			db_addr[i] = 0;
			db_expr[i] = 0;
		}
	}
	set_control(octl);
	return 0;
}

int
HW_Wdata::remove_wpt(Iaddr addr, Proclive *pctl, void *watchexpr)
{
	unsigned int	ctl;
	int		i;
	int		found = 0;

	ctl = get_control();

	for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if ((db_addr[i] != addr) || (db_expr[i] != watchexpr))
			continue;
		// set up ctl register
		ctl &= ~(1 << (i * DR_ENABLE_SIZE));
		ctl &= ~(((0xF) << (i * DR_CONTROL_SIZE)) << 
			DR_CONTROL_SHIFT);

		set_address(i, 0);
		db_expr[i] = 0;
		db_addr[i] = 0;
		found++;
	}

	if (!found)
		return 0;
	set_control(ctl);
	if (pctl && !set_state(pctl))
		// don't attempt to set if off LWP
		return 0;
	return 1;
}

int 
HW_Wdata::triggered(Proclive *p)
{
	unsigned int	status;
	dbregset_t	*dregs;

	if (!get_control() || !p)
		return 0;
	if ((dregs = p->read_dbreg()) == 0)
		return 0;
	status = dregs->debugreg[DR_STATUS];
	for(int i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if (status & (1 << i))
		{
			clear_status();
			set_state(p);
			return 1;
		}
	}
	return 0;
}

// clear control register to disable watchpoints
// but do not lose saved control settings
int
HW_Wdata::clear_state(Proclive *pctl)
{
	dbregset_t	dregs;
	dregs.debugreg[DR_CONTROL] = 0;
	if (pctl && pctl->write_dbreg(&dregs))
	{
		initialized = 1;
		return 1;
	}
	return 0;
}

int
HW_Wdata::set_state(Proclive *pctl)
{
	if (!pctl)
		return 0;
	if (!initialized && !clear_state(pctl))
		// reset first time through
		return 0;
	return(pctl->write_dbreg(&this->dbreg));
}
