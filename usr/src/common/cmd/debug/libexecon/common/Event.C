#ident	"@(#)debugger:libexecon/common/Event.C	1.28"

#include "Event.h"
#include "List.h"
#include "Parser.h"
#include "Buffer.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proctypes.h"
#include "Proglist.h"
#include "Interface.h"
#include "Machine.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#include "TYPE.h"
#include "utility.h"
#endif
#include <string.h>
#include <signal.h>

EventManager	m_event;

// EventManager maintains a doubly-linked list of events.
// The events are kept in numeric order by event id.

Event *
EventManager::add( Event *event )
{
	int	eid;
	Event	*eptr = first;
	Event	*prev = 0;

	if (!event)
	{
		printe(ERR_internal, E_ERROR, "EventManager::add",
			__LINE__);
		return 0;
	}

	eid = event->id;
	for(; eptr; prev = eptr, eptr = eptr->next())
	{
		if (eptr->id > eid)
			break;
	}
	if (!prev)
	{
		if (eptr)
			event->prepend(eptr);
		first = event;
	}
	else
		event->append(prev);
	return event;
}

// Copy an event, using a new new ProcObj.
int
EventManager::copy(Event *eptr, ProcObj *pobj, int fork)
{
	Event		*e;
	int		eid;
	int		level, quiet;
	Node		*cmd;

	if (!eptr || !pobj)
	{
		printe(ERR_internal, E_ERROR, "EventManager::copy",
			__LINE__);
		return 0;
	}
	if (eptr->state == E_DELETED)
		return 1;

	eid = eptr->id;
	level = eptr->level;
	cmd = eptr->cmd;
	quiet = eptr->quiet;

	switch(eptr->get_type())
	{
	case E_ONSTOP:
		e = (Event *)new Onstop_e(eid, level, cmd, pobj);
		break;
	case E_STOP:
		e = (Event *)new Stop_e(eid, level, quiet,
			((Stop_e *)eptr)->get_count(), cmd, pobj);
		((Stop_e *)e)->copy((Stop_e *)eptr, fork);
		break;
	case E_SIGNAL:
		e = (Event *)new Sig_e(*((Sig_e *)eptr)->get_sigs(), 
			eid, level, quiet, cmd, pobj);
		break;
#ifdef HAS_SYSCALL_TRACING
	case E_SCALL:
	{
		e = (Event *)new Sys_e(
			((Sys_e *)eptr)->get_calls(),
			((Sys_e *)eptr)->get_stype(),
			eid, level, quiet, ((Sys_e *)eptr)->get_count(), 
			cmd, pobj);
		break;
	}
#endif
#if EXCEPTION_HANDLING
	case E_EH_EVENT:
	{
		TYPE	*ntype = ((EH_e *)eptr)->get_eh_type();
		e = (Event *)new EH_e(((EH_e *)eptr)->get_flags(),
					eid, level, quiet,
					((EH_e *)eptr)->get_type_name(),
					ntype ? ntype->clone() : 0,
					cmd, pobj);
		break;
	}
#endif // EXCEPTION_HANDLING
	}
	if (e->get_state() == E_DELETED)
	{
		printe(ERR_event_copy, E_WARNING, eid, pobj->obj_name());
		delete e;
		return 0;
	}
	pobj->add_event(e);
	add(e);
	return 1;
}


#define MAX_LEN	20	// length of associated cmd string

// display a single event - the mode controls the
// fullness of the display
//
// uses 3 global buffers - one for the event display itself,
// one for the list of processes, and one, indirectly, in list_cmd()
void
EventManager::display(Event *event, int mode, char *proclist)
{
	Node		*cmd;
	const char	*dis;
	char		cmd_str[MAX_LEN];
	int		first = 1;
	const char	*cmdptr;
	const char	*quiet = " ";
	Buffer		*buf1 = buf_pool.get();
	Buffer		*buf2 = 0;

	if (!event)
	{
		printe(ERR_internal, E_ERROR, "EventManager::display",
			__LINE__);
	}

	cmd = event->cmd;

        if (proclist == 0) 
		proclist = "";

	if (cmd)
	{
		int	len;

		// print out command
		buf2 = buf_pool.get();
		list_cmd(cmd, buf2);
		cmdptr = (char *)*buf2;
		if (!mode)  // short print
		{
			len = strlen(cmdptr);
			if (len+1 >= MAX_LEN)
			{
				len = MAX_LEN - sizeof("...");
				strncpy(cmd_str, cmdptr, len);
				strcpy(cmd_str + len, "...");
				cmdptr = cmd_str;
			}
		}
	}
	else
		cmdptr = "";

	switch(event->state)
	{
		default:
			dis = "  ";
			break;
		case E_INVALID:
			dis = "I ";
			break;
		case E_DISABLED_INV:
			dis = "DI";
			break;
		case E_DISABLED:
			dis = "D ";
			break;
	}

	if (event->quiet)
		quiet = "Q";

	switch(event->get_type())
	{
#ifdef HAS_SYSCALL_TRACING
	case E_SCALL:
	{
		int		*p;
		const char	*sys_type;
		const char	*sysp;

		p = ((Sys_e *)event)->get_calls();
		if (!p || !*p)
		{
			printe(ERR_internal, E_ERROR,
				"EventManager::display" , __LINE__);
			buf_pool.put(buf1);
			if (buf2)
				buf_pool.put(buf2);
			return;
		}

		switch(((Sys_e *)event)->get_stype())
		{
		case Entry:		sys_type = "E";  break;
		case Exit:		sys_type = "X";  break;
		case Entry_exit:	sys_type = "EX"; break;
		case NoSType:
		default:		sys_type = "?";  break;
		}

		buf1->clear();
		do
		{
			if ((sysp = sysname(*p++)) != 0)
			{
				if (first)
					first = 0;
				else
					buf1->add(", ");
				buf1->add(sysp);
			}
		} while(*p);
		if (mode)
			printm(MSG_syscall_event_f, event->id, quiet, dis,
				sys_type, ((Sys_e *)event)->get_count(), 
				(char *)*buf1, proclist, cmdptr);
		else
			printm(MSG_syscall_event, event->id, quiet, dis,
				sys_type, ((Sys_e *)event)->get_count(), 
				(char *)*buf1, cmdptr);
		break;
	}
#endif
	case E_ONSTOP:
		if (mode)
			printm(MSG_onstop_event_f, event->id, dis, 
				proclist, cmdptr);
		else
			printm(MSG_onstop_event, event->id, dis, cmdptr);
		break;

	case E_STOP:
		if (mode)
			printm(MSG_stop_event_f, event->id, quiet, dis,
				((Stop_e *)event)->get_count(), 
				((Stop_e *)event)->get_expr(), proclist,
				cmdptr);
		else
			printm(MSG_stop_event, event->id, quiet, dis,
				((Stop_e *)event)->get_count(), 
				((Stop_e *)event)->get_expr(), cmdptr);
		break;

	case E_SIGNAL:
	{
		sigset_t	*sigs;

		buf1->clear();
		sigs = ((Sig_e *)event)->get_sigs();
		for(int i = 1; i <= NUMBER_OF_SIGS; i++)
		{
			if (prismember(sigs, i))
			{
				if (first)
					first = 0;
				else
					buf1->add(", ");
				buf1->add(signame(i));
			}
		}
		if (mode)
			printm(MSG_signal_event_f, event->id, quiet, dis,
				(char *)*buf1, proclist, cmdptr);
		else
			printm(MSG_signal_event, event->id, quiet, dis,
				(char *)*buf1, cmdptr);
		break;
	}
#if EXCEPTION_HANDLING
	case E_EH_EVENT:
	{
		const char	*fval = "";
		EH_e		*ev = (EH_e *)event;

		switch(ev->get_flags() & (E_THROW|E_CATCH))
		{
		case E_THROW:		fval = "T";  break;
		case E_CATCH:		fval = "C";  break;
		case E_THROW|E_CATCH:	fval = "TC"; break;
		default:		fval = "?";  break;
		}

		if (mode)
			printm(MSG_eh_event_f, event->id, quiet, dis,
				fval, ev->get_type_name(), proclist, cmdptr);
		else
			printm(MSG_eh_event, event->id, quiet, dis,
				fval, ev->get_type_name(), cmdptr);
		break;
	}
#endif // EXCEPTION_HANDLING		
	}
	buf_pool.put(buf1);
	if (buf2)
		buf_pool.put(buf2);
}

// display current signal status for all signals
// signal events are displayed separately
static void
display_sigs(ProcObj *pobj)
{
	sigset_t		*sigs;
	Iaddr			addr;
	int			print_action = 1;
	struct sigaction	*sact;
	int			num_sigs = NUMBER_OF_SIGS;

	// pobj checked in caller

	if (!pobj->get_sig_disp(sact, num_sigs))
	{
		print_action = 0;
	}
	printm(MSG_signal_header, pobj->obj_name(), pobj->prog_name());
	sigs = pobj->sig_mask();
	for (int i = 1; i <= num_sigs; i++)
	{
		const char	*disp;
		if (print_action)
		{
			struct	sigaction	*act;
			act = sact + (i - 1);
			addr = (Iaddr)act->sa_handler;
			if (addr == (Iaddr)SIG_DFL)
				disp = "SIG_DFL";
			else if (addr == (Iaddr)SIG_IGN)
				disp = "SIG_IGN";
			else
			{
				Symbol	sym;
				sym = pobj->find_symbol(addr);
				if (!sym.isnull())
					disp = pobj->symbol_name(sym);
				else
					disp = 0;
			}
		}
		else
			disp = "";
		if (disp != 0)
		{
			printm(prismember(sigs, i) ?
			MSG_sig_caught : MSG_sig_ignored,
			i, signame(i), disp);
		}
		else
		{
			printm(prismember(sigs, i) ?
			MSG_sig_caught_addr : MSG_sig_ignored_addr,
			i, signame(i), addr);
		}
	}
}

// perform designated operation on a single event
int
EventManager::event_op( int eid , Event_op op)
{
	Event	*eptr;
	Event	*saveptr = 0;
	Event	*next;
	int	found = 0;
	int	ret = 1;
	Buffer	*buf = buf_pool.get();

	eptr = first;
	buf->clear();
	for(; eptr; eptr = next)
	{
		next = eptr->next();
		if (eptr->id > eid)
			// events are sorted by id
			break;
		if (eptr->id == eid && 
			(eptr->state != E_DELETED && eptr->pobj))
		{
			Process	*proc; 
			found = 1;
			if (op == M_Display)
			{
				if (!saveptr)
					saveptr = eptr;
				else
					buf->add(", ");
				buf->add(eptr->pobj->obj_name());
				continue;
			}
			else
			{
				proc = eptr->pobj->process();
				if (!eptr->pobj->state_check(E_RUNNING) ||
					!proc->stop_all())
				{
					ret = 0;
					continue;
				}
			}
			switch(op)
			{
			case M_Delete:
				if (!eptr->remove(0))
					ret = 0;
				drop_event(eptr);
				break;
			// Disabling and enabling events
			// is done by setting a flag;
			// the events still happen, but
			// the user doesn't find out if
			// the disabled flag is set.
			case M_Enable:
				eptr->enable();
				break;
			case M_Disable:
				eptr->disable();
				break;
			default:
				break;
			}
			if (!proc->restart_all())
				ret = 0;
		}
	}
	if (found)
	{
		if (op == M_Display)
			display(saveptr, 1, (char *)*buf);
	}
	else
	{
		printe(ERR_no_event_id, E_ERROR, eid);
		ret = 0;
	}
	buf_pool.put(buf);
	return ret;
}

// perform designated operation on
// all events of a given type for a given proclist;
// if etype is 0, do operation on all events for that proclist
int
EventManager::event_op( Proclist *procl, int etype , Event_op op)
{
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head;
	int	ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else if (op == M_Display)
	{
		list = proglist.proc_list(proglist.current_program());
		pobj = list++->p_pobj;
	}
	else
	{
		list = 0;
		pobj = proglist.current_object();
	}
	list_head = list;
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	do
	{

		Process	*proc;
		if (op != M_Display)
		{
			proc = pobj->process();
			if (!pobj->state_check(E_RUNNING)
				|| !proc->stop_all())
			{
				ret = 0;
				continue;
			}
		}
		if (!event_op(pobj, etype, op))
			ret = 0;
		if (op != M_Display)
			proc->restart_all();
	} while (list && (pobj = list++->p_pobj));
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}


int
EventManager::event_op(ProcObj *pobj, int etype, Event_op op)
{
	Event	*eptr;
	Event	*next;
	int	ret = 1;
		
	if (!pobj)
	{
		printe(ERR_internal, E_ERROR, "EventManager::event_op",
			__LINE__);
		return 0;
	}
	eptr = first;

	if (op == M_Display)
	{
		if (etype == E_SIGNAL)
		{
			display_sigs(pobj);
			if (!eptr)
				return 1;
		}
#if EXCEPTION_HANDLING
		else if (etype == E_EH_EVENT)
		{
			if (!pobj->get_eh_info())
			{
				printe(ERR_no_exceptions, E_ERROR, pobj->prog_name());
				return 0;
			}
			display_default_eh_setting(pobj);
			if (!eptr)
				return 1;
		}
#endif

		printm(MSG_events, pobj->obj_name(), 
			pobj->prog_name());
		printm(MSG_event_header);
	}
	for(; eptr; eptr = next)
	{
		next = eptr->next();
		if ((!etype || eptr->get_type() == etype) 
			&& (eptr->state != E_DELETED))
		{
			if (eptr->pobj != pobj)
			{
				continue;
			}
			switch(op)
			{
			case M_Delete:
				if (!eptr->remove(0))
					ret = 0;
				drop_event(eptr);
				break;
			case M_Enable:
				eptr->enable();
				break;
			case M_Disable:
				eptr->disable();
				break;
			case M_Display:
				display(eptr, 0);
				break;
			case M_Nop:
			default:
				break;
			}
		}
	}
	return ret;
}


// find first active (not deleted) event with given id
Event *
EventManager::find_event(int eid)
{
	register Event	*eptr;

	eptr = first;

	for( ; eptr; eptr = eptr->next())
	{
		if (eptr->id > eid)
			// events are sorted by id
			return 0;
		if (eptr->id == eid && 
			(eptr->state != E_DELETED))
		{
			return eptr;
		}
	}
	return 0;
}

// remove event, but don't bother going to ProcObj to
// disable primitive event
void
EventManager::drop_event(Event *eptr)
{
	if (eptr == first)
		first = eptr->next();
	eptr->unlink();
	delete(eptr);
}

// change an Event's id - we must reposition event in list to
// maintain sorted order
void
EventManager::set_id(int oldid, int newid)
{
	Event	*eptr;
	while((eptr = find_event(oldid)) != 0)
	{
		if (eptr == first)
			first = eptr->next();
		eptr->unlink();
		eptr->id = newid;
		add(eptr);
	}
}


Event::Event(int Id, int Lvl, int Quiet, Node *Cmd, ProcObj *Pobj)
{
	id = Id;
	level = Lvl;
	quiet = Quiet;
	cmd = Cmd;
	pobj = Pobj;
	state = E_DELETED;
	_enext = _eprev = 0;
}

void
Event::eprepend(Event *elem)
{
	if (_eprev = elem->_eprev)
		_eprev->_enext = this;
	elem->_eprev = this;
	_enext = elem;
}

void
Event::eappend(Event *elem)
{
	if (_enext = elem->_enext)
		_enext->_eprev = this;
	_eprev = elem;
	elem->_enext = this;
}

void
Event::eunlink()
{
	if (_enext)
		_enext->_eprev = _eprev;
	if (_eprev)
		_eprev->_enext = _enext;
	_enext = 0;
	_eprev = 0;
}

int
Event::disable()
{
	if (state == E_ENABLED)
	{
		state = E_DISABLED;
	}
	else if (state == E_INVALID)
	{
		state = E_DISABLED_INV;
	}
	else
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_disabled, id,
			(unsigned long)pobj);
	return 1;
}

int
Event::enable()
{
	if (state == E_DISABLED)
	{
		state = E_ENABLED;
	}
	else if (state == E_DISABLED_INV)
	{
		state = E_INVALID;
	}
	else
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_enabled, id, 
			(unsigned long)pobj);
	return 1;
}

int
Event::trigger()
{
	if (state != E_ENABLED)
		return NO_TRIGGER;

	return common_trigger();
}

int
Event::common_trigger()
{
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	if (!quiet)
		return TRIGGER_VERBOSE;
	else
		return TRIGGER_QUIET;
}

// Null base class version
int
Event::remove(ProcObj *)
{
	return 0;
}

void
Event::cleanup()
{
}

int
Event::re_init(ProcObj *)
{
	return 0;
}


// Null base class version
int
Event::get_type()
{
	return 0;
}

Sig_e::Sig_e(sigset_t Sigs, int Id, int Lvl, int Quiet, 
	Node *Cmd, ProcObj *Pobj) : EVENT(Id, Lvl, Quiet, Cmd, Pobj)
{
	if (!Pobj)
	{
		printe(ERR_internal, E_ERROR, "Sig_e::Sig_e",
			__LINE__);
		return;
	}
	signals = Sigs;
	if (Pobj->set_sig_event(&signals, 
		(Notifier)notify_sig_e_trigger, this, E_SET_YES))
	{
		state = E_ENABLED;
	}
}

int
Sig_e::re_init(ProcObj *p)
{
	if (!p)
	{
		printe(ERR_internal, E_ERROR, "Sig_e::re_init",
			__LINE__);
		return 0;
	}
	pobj = p;
	if (!pobj->set_sig_event(&signals, 
		(Notifier)notify_sig_e_trigger, this, E_SET_NO))
	{
		state = E_DELETED;
		pobj->remove_event(this);
		return 0;
	}
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	return 1;
}

// cleanup is used when a process or thread exits and the
// event is to be saved and possibly re-assigned to a different
// ProcObj; remove completely removes the event
void
Sig_e::cleanup()
{
	switch(state)
	{
		case E_DELETED:
		case E_INVALID:
		case E_DISABLED_INV:
			return;
		default:
			break;
	}
	pobj->remove_sig_event(&signals, 
		(Notifier)notify_sig_e_trigger, this, E_DELETE_NO);
	pobj = 0;
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

// If pobj is 0, cleanup was already called on this event;
// this can happen when a thread exits, but the process is
// still around - if the process tries to remove the event,
// it must pass in its own ProcObj ptr.
int
Sig_e::remove(ProcObj *p)
{
	if (state == E_DELETED)
		return 1;

	if (!pobj && !p)
		return 0;

	int	ret = 1;

	if (pobj)
	{
		pobj->remove_event(this);
		if (!pobj->remove_sig_event(&signals, 
			(Notifier)notify_sig_e_trigger, this, E_DELETE_YES))
		{
			ret = 0;
		}
	}
	else
	{
		p->remove_event(this);
	}
	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, (unsigned long)pobj);
	return ret;
}

int
Sig_e::get_type()
{
	return E_SIGNAL;
}

#ifdef HAS_SYSCALL_TRACING
Sys_e::Sys_e(int *sys, Systype stype, int Id, int Lvl, int Quiet, 
	int count, Node *Cmd, ProcObj *Pobj) : EVENT(Id, Lvl, Quiet, Cmd, Pobj)
{
	int	fail = 0;
	int	snum;
	int	ncalls = 0;

	if (!Pobj || !sys || !*sys)
	{
		printe(ERR_internal, E_ERROR, "Sys_e::Sys_e",
			__LINE__);
		return;
	}
	systype = stype;
	syscalls = sys;
	orig_count = count;
	cur_count = 0;

	do
	{
		snum = *syscalls++;
		if (!pobj->set_sys_trace(snum, stype, 
			(Notifier)notify_sys_e_trigger, this, E_SET_YES))
		{
			fail++;
			break;
		}
		ncalls++;
	} while(*syscalls);

	if (!fail)
	{
		state = E_ENABLED;
		ncalls++;
		syscalls = new int[ncalls];
		memcpy((char *)syscalls, (char *)sys, ncalls*sizeof(int));
	}
	else
	{
		// failure - remove syscalls already set
		syscalls = sys;
		while(snum != *syscalls)
		{
			pobj->remove_sys_trace(*syscalls++, stype,
				(Notifier)notify_sys_e_trigger,
					this, E_DELETE_YES);
			syscalls++;
		}
		syscalls = 0;
	}
}

Sys_e::~Sys_e()
{
	delete syscalls;
}

int
Sys_e::trigger()
{

	if (state != E_ENABLED)
		return NO_TRIGGER;
	cur_count++;
	if (cur_count < orig_count)
		return NO_TRIGGER;
	
	return common_trigger();
}

void
Sys_e::cleanup()
{
	int	*sys;
	switch(state)
	{
		case E_DELETED:
		case E_INVALID:
		case E_DISABLED_INV:
			return;
		default:
			break;
	}
	sys = syscalls;
	do
	{
		pobj->remove_sys_trace(*sys++, systype,
			(Notifier)notify_sys_e_trigger, this,
				E_DELETE_NO);
	} while(*sys);
	pobj = 0;
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

int
Sys_e::remove(ProcObj *p)
{
	if (state == E_DELETED)
		return 1;

	if (!pobj && !p)
		return 0;

	int	ret = 1;

	if (pobj)
	{
		int	*sys = syscalls;
		pobj->remove_event(this);
		do
		{
			if (!pobj->remove_sys_trace(*sys++,
				systype, (Notifier)notify_sys_e_trigger,
					this, E_DELETE_YES))
				ret = 0;
		} while(*sys);
	}
	else
	{
		p->remove_event(this);
	}
	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, (unsigned long)pobj);
	return ret;
}

int
Sys_e::get_type()
{
	return E_SCALL;
}

int
Sys_e::re_init(ProcObj *p)
{
	int	snum;
	int	fail = 0;

	if (!p)
	{
		printe(ERR_internal, E_ERROR, "Sys_e::re_init",
			__LINE__);
		return 0;
	}

	pobj = p;

	int	*sys = syscalls;
	do
	{
		snum = *sys++;
		if (!pobj->set_sys_trace(snum, systype, 
			(Notifier)notify_sys_e_trigger, this, E_SET_NO))
		{
			fail++;
			break;
		}
	} while(*sys);

	if (fail)
	{
		// failure - remove syscalls already set
		sys = syscalls;
		while(snum != *sys)
		{
			pobj->remove_sys_trace(*sys++, systype,
				(Notifier)notify_sys_e_trigger,
					this, E_DELETE_NO);
		}
		state = E_DELETED;
		pobj->remove_event(this);
		return 0;
	}
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	cur_count = 0;
	return 1;
}
#endif // HAS_SYSCALL_TRACING

Onstop_e::Onstop_e(int Id, int Lvl,
	Node *Cmd, ProcObj *Pobj) : EVENT(Id, Lvl, 0, Cmd, Pobj)
{
	if (!Pobj)
	{
		printe(ERR_internal, E_ERROR, "Onstop_e::Onstop_e",
			__LINE__);
		return;
	}
	Pobj->set_onstop((Notifier)notify_onstop_e_trigger, this);
	state = E_ENABLED;
}

void
Onstop_e::cleanup()
{
	switch(state)
	{
		case E_DELETED:
		case E_INVALID:
		case E_DISABLED_INV:
			return;
		default:
			break;
	}

	pobj->remove_onstop((Notifier)notify_onstop_e_trigger, this);
	pobj = 0;
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

int
Onstop_e::re_init(ProcObj *p)
{
	if (!p)
	{
		printe(ERR_internal, E_ERROR, "Onstop_e::Onstop_e",
			__LINE__);
		return 0;
	}
	pobj = p;
	pobj->set_onstop((Notifier)notify_onstop_e_trigger, this);
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	return 1;
}

int
Onstop_e::remove(ProcObj *p)
{
	if (state == E_DELETED)
		return 1;

	if (!pobj && !p)
		return 0;

	int	ret = 1;

	if (pobj)
	{
		pobj->remove_event(this);
		if (!pobj->remove_onstop((Notifier)notify_onstop_e_trigger, this))
		{
			ret = 0;
		}
	}
	else
	{
		p->remove_event(this);
	}
	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, (unsigned long)pobj);
	return ret;
}

int
Onstop_e::get_type()
{
	return E_ONSTOP;
}

Stop_e::Stop_e(StopEvent *Stop, int Id, int Lvl, 
	int Quiet, int count, Node *Cmd, ProcObj *Pobj) :
		EVENT(Id, Lvl, Quiet, Cmd, Pobj)
{
	int			i;
	register StopEvent	*se;

	if (!Pobj || !Stop)
	{
		printe(ERR_internal, E_ERROR, "Stop_e::Stop_e",
			__LINE__);
		return;
	}
	stop = Stop;
	orig_count = count;
	cur_count = 0;
	event_expr = print_stop(stop);

	// for now we set all parts of a stop event.
	// optimizations may be possible
	for(se = stop; se; se = se->next())
	{
		if ((i = se->stop_set(Pobj, this)) == SET_FAIL)
			break;
		else if (i == SET_INVALID)
			state = E_INVALID;
	}
	if (i == SET_FAIL)
	{
		// might have set part of stop expression
		for(StopEvent *nse = stop; nse != se; nse = nse->next())
		{
			// StopEvents deleted in Stop_e destructor
			nse->remove();
		}
	}
	else if (state != E_INVALID)
		state = E_ENABLED;
}

Stop_e::~Stop_e()
{ 
	dispose_event(stop); 
	delete event_expr;
}

// reset event expr; used when an overloaded function
// results in more StopLocs being added
void
Stop_e::reset_expr()
{
	delete event_expr;
	event_expr = print_stop(stop);
}

int
Stop_e::trigger()
{

	if ((state != E_ENABLED) || (!stop_eval(stop)))
		return NO_TRIGGER;
	cur_count++;
	if (cur_count < orig_count)
		return NO_TRIGGER;
	pobj->set_expr(event_expr);
	return common_trigger();
}

// Event triggered by ProcObj that was not ProcObj in whose context
// event was set.  Only stop latter ProcObj.
int
Stop_e::trigger_foreign()
{
	int	ret = trigger();

	if (ret == NO_TRIGGER)
		return NO_TRIGGER;

	pobj->stop_for_event(ret);
	return TRIGGER_FOREIGN;
}

int
Stop_e::remove(ProcObj *p)
{
	if (state == E_DELETED)
		return 1;

	if (!pobj && !p)
		return 0;

	int	ret = 1;

	if (pobj)
	{
		register StopEvent	*se = stop;
		pobj->remove_event(this);
		for(se = stop; se; se = se->next())
		{
			// StopEvents deleted in Stop_e destructor
			if (!se->remove())
				ret = 0;
		}
	}
	else
	{
		p->remove_event(this);
	}
	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, (unsigned long)pobj);
	return ret;
}

int
Stop_e::get_type()
{
	return E_STOP;
}

void
Stop_e::cleanup()
{
	register StopEvent	*se;
	switch(state)
	{
		case E_DELETED:
			return;
		case E_INVALID:
		case E_DISABLED_INV:
			if (!pobj)
				return;
			break;
		default:
			break;
	}
	for(se = stop; se; se = se->next())
	{
		se->cleanup();
	}
	pobj = 0;
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

int
Stop_e::re_init(ProcObj *Pobj)
{
	int			i, invalid = 0;
	register StopEvent	*se, *se2;

	if (!Pobj)
	{
		printe(ERR_internal, E_ERROR, "Stop_e::re_init",
			__LINE__);
		return 0;
	}
	pobj = Pobj;
	for(se = stop; se; se = se->next())
	{
		if ((i = se->re_init(pobj)) == SET_FAIL)
		{
			// remove events already reset
			for(se2 = stop; se2 != se; se2 = se2->next())
			{
				se2->remove();
			}
			state = E_DELETED;
			Pobj->remove_event(this);
			return 0;
		}
		else if (i == SET_INVALID)
			invalid = 1;
	}
	if (!invalid)
	{
		if (state == E_DISABLED_INV)
			state = E_DISABLED;
		else if (state == E_INVALID)
			state = E_ENABLED;
	}
	cur_count = 0;
	return 1;
}

void
Stop_e::invalidate()
{
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

void
Stop_e::validate()
{
	register StopEvent	*se;

	for(se = stop; se; se = se->next())
	{
		if (!(se->get_flags() & E_VALID))
			return;
	}
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
}

Stop_e::Stop_e(int Id, int Lvl, 
	int Quiet, int count, Node *Cmd, ProcObj *Pobj) :
		EVENT(Id, Lvl, Quiet, Cmd, Pobj)
{
	orig_count = count;
	cur_count = 0;
}

// if fork is non-zero we copy event, assuming same process or
// thread context; if fork is 0, we set the event as for a
// new thread - this is used for thread_create
void
Stop_e::copy(Stop_e *oldevent, int fork)
{
	int		i;
	StopEvent	*nse, *ose;
	char		*new_expr;

	if (!oldevent)
	{
		printe(ERR_internal, E_ERROR, "Stop_e::copy",
			__LINE__);
		return;
	}
	stop = copy_tree(oldevent->stop);
	new_expr = oldevent->get_expr();
	event_expr = new(char[strlen(new_expr) + 1]);
	strcpy(event_expr, new_expr);
	for(nse = stop, ose = oldevent->stop; nse && ose; 
		nse = nse->next(), ose = ose->next())
	{
		if ((i = nse->stop_copy(pobj, this, ose, fork)) 
			== SET_FAIL)
			break;
		else if (i == SET_INVALID)
			state = E_INVALID;
	}
	if (i == SET_FAIL)
	{
		// might have set part of stop expression
		StopEvent	*tnse;
		for(tnse = stop; tnse != nse; tnse = tnse->next())
		{
			// StopEvents deleted in Stop_e destructor
			tnse->remove();
		}
		return;
	}
	else if (state != E_INVALID)
		state = E_ENABLED;
}

int
Stop_e::disable()
{
	register StopEvent	*se;

	for(se = stop; se; se = se->next())
	{
		se->disable();
	}
	return Event::disable();
}

int
Stop_e::enable()
{
	register StopEvent	*se;

	for(se = stop; se; se = se->next())
	{
		se->enable();
	}
	return Event::enable();
}

// The StopEvents for a single Stop_e are connected as a list; 
// each node is either a leaf - the end of the list - 
// or one operand of an "and" or "or"
// operator.  Many functions that deal with StopEvents recurse
// down the list.
//
// The list for "a && b || c", for example, looks like
//		c  OR
//		|
//		b  AND
//		|
//		a  LEAF

// Is event true or false?
int 
Stop_e::stop_eval(StopEvent *node)
{
	if (!node)
		return 0;

	if (node->get_flags() & E_LEAF)
	{
		return(node->stop_true());
	}
	else if (node->get_flags() & E_AND)
	{
		if (stop_eval(node->next()))
		{
			return(node->stop_true());
		}
		else
		{
			return 0;
		}
	}
	else // or
	{
		if (stop_eval(node->next()))
		{
			return 1;
		}
		else
		{
			return(node->stop_true());
		}
	}
	/*NOTREACHED*/
}

#if EXCEPTION_HANDLING

int default_eh_setting = E_THROW|E_CATCH;

EH_e::EH_e(int f, int Id, int Lvl, int Quiet, 
	const char *exp, TYPE *type, Node *Cmd, ProcObj *Pobj)
	: EVENT(Id, Lvl, Quiet, Cmd, Pobj)
{
	if (!Pobj)
	{
		printe(ERR_internal, E_ERROR, "EH_e::EH_e",
			__LINE__);
		return;
	}
	flags = f;
	type_name = exp;
	eh_type = type;
	if (Pobj->set_eh_event((Notifier)notify_eh_trigger, this))
		state = E_ENABLED;
}

EH_e::~EH_e()
{
	delete eh_type;
}

int
EH_e::re_init(ProcObj *p)
{
	if (!p)
	{
		printe(ERR_internal, E_ERROR, "EH_e::re_init",
			__LINE__);
		return 0;
	}
	pobj = p;
	if (!pobj->set_eh_event((Notifier)notify_eh_trigger, this))
	{
		state = E_DELETED;
		pobj->remove_event(this);
		return 0;
	}
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	return 1;
}

// cleanup is used when a process or thread exits and the
// event is to be saved and possibly re-assigned to a different
// ProcObj; remove completely removes the event
void
EH_e::cleanup()
{
	switch(state)
	{
		case E_DELETED:
		case E_INVALID:
		case E_DISABLED_INV:
			return;
		default:
			break;
	}

	pobj->remove_eh_event((Notifier)notify_eh_trigger, this);
	pobj = 0;
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

// If pobj is 0, cleanup was already called on this event;
// this can happen when a thread exits, but the process is
// still around - if the process tries to remove the event,
// it must pass in its own ProcObj ptr.
int
EH_e::remove(ProcObj *p)
{
	if (state == E_DELETED)
		return 1;

	if (!pobj && !p)
		return 0;

	int	ret = 1;

	if (pobj)
	{
		pobj->remove_event(this);
		if (!pobj->remove_eh_event((Notifier)notify_eh_trigger, this))
		{
			ret = 0;
		}
	}
	else
	{
		p->remove_event(this);
	}
	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, (unsigned long)pobj);
	return ret;
}

int
EH_e::get_type()
{
	return E_EH_EVENT;
}

int
EH_e::trigger()
{
	TYPE	*etype;	// type of current exception

	if (!pobj || !pobj->get_eh_info()
		|| (etype = pobj->get_eh_info()->get_eh_type()) == 0
		|| state != E_ENABLED
		|| (eh_type && !etype->is_assignable(eh_type)))
		return NO_TRIGGER;
	return common_trigger();
}

#endif // EXCEPTION_HANDLING
