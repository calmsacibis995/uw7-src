#ident	"@(#)debugger:gui.d/common/Eventlist.C	1.20"

// GUI headers
#include "Dispatcher.h"
#include "Eventlist.h"
#include "Events.h"
#include "Proclist.h"
#include "UI.h"
#include "Windows.h"
#include "gui_msg.h"
#include "config.h"
#include "gui_label.h"
#include "Label.h"
#include "Path.h"

// Debug headers
#include "Message.h"
#include "str.h"
#include "Vector.h"
#include "Buffer.h"

#include <string.h>

Event_list	event_list;

static	Vector	break_list;
static	int	breakpoints;

enum Event_action {  Event_add = 1, Event_delete, Event_change };

// Walk the list of window sets and update their event windows
// for Event_change, p may be 0 - there is only one call to this function
// event if the event applies to more than one procobj, so if p == 0,
// call change_event for each event window
static void
updateAllEventWindow(Event *e, ProcObj *p, Event_action a)
{
	Window_set	*ws = (Window_set *)windows.first();

	for (; ws; ws = (Window_set *)windows.next())
	{
		Base_window	**win = ws->get_base_windows();
		for(int i = 0; i < windows_per_set; i++, win++)
		{
			Event_pane	*ew;

			if (!*win)
				continue;
			ew = (Event_pane *)(*win)->get_pane(PT_event);

			// Event window needs to be updated only if 
			// the event applies to
			// the window set's current procobj
			if (!ew || !(*win)->is_open() || 
				!ws->current_obj())
			continue;

			switch(a)
			{
			case Event_add:
				if (ws->current_obj() != p)
				continue;
				ew->add_event(e);
				break;

			case Event_delete:
				if (ws->current_obj() != p)
					continue;
				ew->delete_event(e);
				break;

			case Event_change:
				if (p && ws->current_obj() != p)
					continue;
				ew->change_event(e);
				break;

			default :
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				return;
							
			}
		}
	}
}

void
Event_list::breakpt_set(Message *m)
{
	Msg_id		mtype = m->get_msg_id();

	Breakpoint	bkpt;
	Word		line, addr;
	char		*file;

	if (mtype == MSG_bkpt_set)
	{
		m->unbundle(addr, line, file);
		bkpt.line = (int)line;
		bkpt.file = pathcanon(file);
	}
	else // a break point without file name and line number (MSG_break_set_addr)
	{
		m->unbundle(addr);
		bkpt.line = 0;
		bkpt.file = 0;
	}

	bkpt.addr = (Iaddr)addr;
	break_list.add(&bkpt, sizeof(Breakpoint));
	breakpoints++;
}

Event *
Event_list::findEvent(int eId)
{
	Event *e = (Event *)events.first();
	while (e)
	{
		if (e->get_id() == eId)
			break;

		e =	(Event *)events.next();
	}
	return e;
}

void
Event_list::enable_event(Message *m)
{
	Word eId, pId;
	m->unbundle(eId, pId);

	// msg type is MSG_event_enabled
	Event	*e = findEvent((int)eId);
	
	if (e != 0)
	{
		switch(e->get_state())
		{
		case ES_disabled_inv:
		case ES_invalid:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
		case ES_disabled:
			e->set_state(ES_valid);
		case ES_valid:
			break;
		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	
		if (e->get_type() == ET_stop)
			event_list.change_list.notify(BK_add, e);
		
		updateAllEventWindow(e, proclist.find_obj(m), Event_change);
	}
}

void
Event_list::disable_event(Message *m)
{
	Word eId, pId;
	m->unbundle(eId, pId);

	// msg type is MSG_event_disabled
	Event	*e = findEvent((int)eId);
	
	if (e != 0)
	{
		switch(e->get_state())
		{
		case ES_valid:
			e->set_state(ES_disabled); break;
		case ES_invalid:
			e->set_state(ES_disabled_inv); break;
		case ES_disabled_inv:
		case ES_disabled:
			break;
		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}

		if (e->get_type() == ET_stop)
			event_list.change_list.notify(BK_delete, e);

		updateAllEventWindow(e, proclist.find_obj(m), Event_change);
	}
}

void
Event_list::delete_event(Message *m, ProcObj *p)
{
	Word eId, pId;
	m->unbundle(eId, pId);

	// msg type is MSG_event_deleted
	Event *	e = findEvent((int)eId);
	
	if (e != 0)
	{
		// notify must be done before delete_obj since Source and Dis
		// windows check event's list of procobjs against their current procobj
		if (e->get_type() == ET_stop)
			event_list.change_list.notify(BK_delete, e);

		p->delete_event(e);
		e->delete_obj(p);

		updateAllEventWindow(e, p, Event_delete);

		//if this procobj is the only one using this event, delete the event 
		//else only delete this procobj from procobj list of this event 
		if (e->has_no_obj())
		{
			events.remove(e);
			
			// dtor of Stop event will clean up the file_list.
			delete e;
		}
	}
	else
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
}

File_list  *
Event_list::find_file(const char *fname, int add)
{
	File_list	*fptr = fhead;

	for (; fptr; fptr = fptr->next())
	{
		int val = strcmp(fname, fptr->get_name());
		if (val == 0)
			return fptr;
		else if (val < 0)
			break;
	}

	if (add)
	{
		File_list *new_file = new File_list(fname);

		if (!fhead)
			fhead = ftail = new_file;
		else if (fptr != 0)
		{
			new_file->prepend(fptr);
			if (fhead == fptr)
				fhead = new_file;
		}
		else
		{
			new_file->append(ftail);
			ftail = new_file;
		}
		return new_file;
	}
	return 0;
}

void
Event_list::change_event(Message *m, ProcObj *proc)
{
	if (!proc)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Word	id;
	Event	*event;

	m->unbundle(id);
	if ((event = findEvent((int)id)) == 0)
	{
		new_event(m, proc);
		return;
	}
	
	Message	*msg;
	Word	count = 0;
	char	*qflag = 0;
	char	*state = 0;
	char	*condition = 0;
	char	*systype = 0;
	char	*plist = 0;
	char	*cmd = 0;

	// Note : change event will NEVER change the plist, since change
	//	  of plist will delete the event and create new one
	dispatcher.query(0, proc->get_id(), "events %d\n", (int)id);
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch(msg->get_msg_id())
		{
		case MSG_stop_event_f:
			msg->unbundle(id, qflag, state, count, condition, plist, cmd);
			((Stop_event *)event)->set_count((int)count);
			break;

		case MSG_syscall_event_f:
			msg->unbundle(id, qflag, state, systype, count, condition,
				plist, cmd);
			((Syscall_event *)event)->set_count((int)count);
			break;

		case MSG_signal_event_f:
			msg->unbundle(id, qflag, state, condition, plist, cmd);
			break;

		case MSG_onstop_event_f:
			msg->unbundle(id, state, plist, cmd);
			break;

		case MSG_events:
		case MSG_event_header:
		case MSG_event_header_f:
			break;

		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}

		event->set_state(state);
		event->set_condition(condition);
		event->set_commands(cmd);
	}

	updateAllEventWindow(event, 0, Event_change);
}

// new_event is called for both brand-new events, and for changes to events
// that change the stop expression or the procobj list - essentially, 
// debug deletes the old event and creates a new one with the same number
void
Event_list::new_event(Message *m, ProcObj *proc)
{
	if (!proc)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Word	id;
	Message	*msg;
	Event	*e = 0;
	Word	count = 0;
	char	*qflag = 0;
	char	*state = 0;
	char	*condition = 0;
	char	*systype = 0;
	char	*ehtype = 0;
	char	*plist = 0;
	char	*cmd = 0;

	m->unbundle(id);

	dispatcher.query(0, proc->get_id(), "events %d\n", (int)id);
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch(msg->get_msg_id())
		{
		case MSG_stop_event_f:
			msg->unbundle(id, qflag, state, count, condition, plist,
				cmd);
			e = new Stop_event((int)id, state, condition, plist,
				cmd, (int)count, breakpoints,
				(Breakpoint *)break_list.ptr());
			// one or more breakpoint_set msg will come before
			// MSG_stop_event_f, and fill the break_list. Clean it
			// after use.
			breakpoints = 0;
			break_list.clear();

			// The user may have edited a disabled event
			if (e->get_state() == ES_valid)
				event_list.change_list.notify(BK_add, e);
			break;

		case MSG_syscall_event_f:
			msg->unbundle(id, qflag, state, systype, count, condition,
				plist, cmd);
			e = new Syscall_event((int)id, state, systype, condition,
				plist, cmd, (int)count);
			break;

		case MSG_signal_event_f:
			msg->unbundle(id, qflag, state, condition, plist, cmd);
			e = new Signal_event((int)id, state, condition,
				plist, cmd);
			break;

		case MSG_onstop_event_f:
			msg->unbundle(id, state, plist, cmd);
			e = new Onstop_event((int)id, state, plist, cmd);
			break;

#if EXCEPTION_HANDLING
		case MSG_eh_event_f:
			msg->unbundle(id, qflag, state, ehtype, condition, plist, cmd);
			e = new Exception_event((int)id, state, ehtype, condition,
				plist, cmd);
			break;
#endif

		case MSG_events:
		case MSG_event_header:
		case MSG_event_header_f:
			break;

		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
		}
	}

	if (!e)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	events.add(e);

}

Event::Event(int eid, const char *s, const char *c,
	char *pIn, const char *cmds)
{
	head = tail = 0;
	id = eid;

	if (c != 0 && *c != 0)
		condition = makestr(c);
	else
		condition = 0;

	if (cmds != 0 && *cmds != 0)
		commands = makestr(cmds);
	else
		commands = 0;

	set_state(s);
	set_plist(pIn);
}

Event::~Event()
{
	delete (char *)commands;
	delete (char *)condition;
	
	for (Plink *walker = head; walker;)
	{
		Plink	*tmp = walker;
		ProcObj	*p = walker->procobj();

		p->delete_event(this);

		walker = walker->next();
		delete tmp;
	} 
}

int
Event::has_obj(ProcObj *p)
{
	for (Plink *walker = head; walker; walker = walker->next())
	{
		if (walker->procobj() == p)
			return 1;
	}

	return 0;
}

void
Event::add_obj(ProcObj *p)
{
	if (has_obj(p))
		return;

	Plink	*item = new Plink(p);

	if (!head)
	{
		head = tail = item;
		return;
	}
	else
	{
		item->append(tail);
		tail = item;
	}
}

void
Event::delete_obj(ProcObj *p)
{
	if (p == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Plink	*ptr;
	
	for (ptr = head; ptr; ptr = ptr->next())
		if (ptr->procobj() ==  p)
			break;

	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	else if (ptr == head)
		head = head->next();

	if (ptr == tail)
		tail = tail->prev();

	ptr->unlink();
	delete ptr;
}

void
Event::set_plist(char *pIn)
{
	// clear the old procobj 
	for(Plink *walker = head; walker;)
	{
		Plink	*tmp = walker;
		ProcObj	*p = walker->procobj();

		p->delete_event(this);

		walker = walker->next();
		delete tmp;
	} 

	head = tail = 0;

	//get the list of procobj applicable
	char	*p = strtok(pIn, " ,");
	ProcObj	*pProc = 0;

	while (p!= NULL)
	{
		if ((*p) != 0)
		{
			pProc = proclist.find_obj(p);

			if (pProc == 0)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				return;
			}
			pProc->add_event(this);
			add_obj(pProc);
		}
		p = strtok(NULL, " ,");
	}
}

void
Event::get_plist(Buffer *buffer)
{
	buffer->clear();
	for (Plink *plink = head; plink; plink = plink->next())
	{
		if (plink != head)
			buffer->add(", ");

		buffer->add(plink->procobj()->get_name());
	}
}

void
Event::set_state(const char *s)
{
	if (!s || *s == ' ')
		state = ES_valid;
	else if (*s == 'I' && *(s+1) == '\0')
		state = ES_invalid;
	else if (*s == 'D' && *(s+1) == 'I')
		state = ES_disabled_inv;
	else if (*s == 'D')		//yes, leading D with anything except
		state = ES_disabled;	// I following, means disabled
	else
	{
		state = ES_invalid;
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

Event_type
Event::get_type()
{
	return ET_none;
}

const char *
Event::get_type_string()
{
	return "";
}

Stop_event::Stop_event(int eid, const char *s, const char *stop, char *p,
	const char *cmds, int c, int b, Breakpoint *bVec)
	: Event(eid, s, stop, p, cmds)
{
	count = c;
	nbreakpts = b;
	breakpts = new Breakpoint[nbreakpts];
	memcpy(breakpts, bVec, sizeof(Breakpoint) *nbreakpts);

	for (int i = 0; i < nbreakpts; i++, bVec++)
	{
		if (!bVec->file)
			// don't add breakpts without symbolic info
			// to file list
			continue;
		File_list *fptr = event_list.find_file(bVec->file, 1);
		fptr->add_event(bVec->line, this);
	}

	for (Plink *procP = head; procP; procP = procP->next())
	{
		updateAllEventWindow(this, procP->procobj(), Event_add);
	}
}

Stop_event::~Stop_event()
{

	for(int i = 0; i < nbreakpts; i++)
	{
		if (!breakpts[i].file)
			continue;
		File_list *fptr = event_list.find_file(breakpts[i].file, 0);
		fptr->delete_event(breakpts[i].line, this);
		delete breakpts[i].file;
	}

	delete breakpts;
}

Event_type
Stop_event::get_type()
{
	return ET_stop;
}

const char *
Stop_event::get_type_string()
{
	return labeltab.get_label(LAB_STOP);
}

Signal_event::Signal_event(int eid, const char *s, const char *sig,
	char *p, const char *cmds) : Event(eid, s, sig, p, cmds)
{
	for (Plink *procP = head; procP; procP = procP->next())
	{
		updateAllEventWindow(this, procP->procobj(), Event_add);
	}
}

Event_type
Signal_event::get_type()
{
	return ET_signal;
}

const char *
Signal_event::get_type_string()
{
	return labeltab.get_label(LAB_SIGNAL);
}

Syscall_event::Syscall_event(int eid, const char *s, const char *stype,
	const char *sys, char *p, const char *cmds, int c)
	: Event(eid, s, sys, p, cmds)
{
	const char *l = labeltab.get_label(LAB_SYSCALL);
	type = makesf(strlen(l) + strlen(stype) + 1, "%s %s", l, stype);
	count = c;

	for (Plink *procP = head; procP; procP = procP->next())
	{
		updateAllEventWindow(this, procP->procobj(), Event_add);
	}
}

Event_type
Syscall_event::get_type()
{
	return ET_syscall;
}

const char *
Syscall_event::get_type_string()
{
	return type;
}

Onstop_event::Onstop_event(int eid, const char *s, char *p,
	const char *cmds) : Event(eid, s, 0, p, cmds)
{
	for (Plink *procP = head; procP; procP = procP->next())
	{
		updateAllEventWindow(this, procP->procobj(), Event_add);
	}
}

Event_type
Onstop_event::get_type()
{
	return ET_onstop;
}

const char *
Onstop_event::get_type_string()
{
	return labeltab.get_label(LAB_ONSTOP);
}

#if EXCEPTION_HANDLING
Exception_event::Exception_event(int eid, const char *s, const char *eht,
	const char *user_type, char *p, const char *cmds)
	: Event(eid, s, user_type, p, cmds)
{
	const char *l = labeltab.get_label(LAB_EXCEPTION);
	eh_type = makesf(strlen(l) + strlen(eht) + 1, "%s %s", l, eht);

	for (Plink *procP = head; procP; procP = procP->next())
	{
		updateAllEventWindow(this, procP->procobj(), Event_add);
	}
}

Event_type
Exception_event::get_type()
{
	return ET_exception;
}

const char *
Exception_event::get_type_string()
{
	return eh_type;
}
#endif

File_list::File_list(const char *f)
{
	fname = makestr(f);
	head = 0;
	tail = 0;
}

File_list::~File_list()
{
	for (Flink *walker = head; walker;)
	{
		Flink *tmp = walker;

		walker = walker->next();
		delete tmp;
	} 

	delete fname;
}

void
File_list::delete_event(int line, Stop_event *event)
{
	if (!head)
		return;
	
	Flink	*ptr;
	
	for (ptr = head; ptr; ptr = ptr->next())
		if (line ==  ptr->get_line() && ptr->get_event() == event)
			break;

	if (!ptr)
		return;
	else if (ptr == head)
		head = head->next();

	if (ptr == tail)
		tail = tail->prev();

	ptr->unlink();
	delete ptr;
}

void
File_list::add_event(int line, Stop_event *event)
{
	Flink	*item = new Flink(line, event);
	Flink	*ptr;

	if (!head)
	{
		head = tail = item;
		return;
	}

	for (ptr = head; ptr; ptr = ptr->next())
		if (line <= ptr->get_line())
			break;

	if (ptr)
	{
		if (line == ptr->get_line() && event == ptr->get_event())
		{
			// breakpoint on line in header file set in
			// multiple .o's - should be in the list only once
			delete item;
			return;
		}

		item->prepend(ptr);
		if (ptr == head)
			head = item;
	}
	else
	{
		item->append(tail);
		tail = item;
	}
}

void
File_list::get_break_list(ProcObj *p, Vector *v)
{
	Flink	*ptr;
	int	line = 0;
	int	num = 0;

	v->clear();
	for (ptr = head; ptr; ptr = ptr->next())
	{
		if (! ptr->get_event()->has_obj(p))
			continue;

		line = ptr->get_line();
		v->add(&line, sizeof(int));
		num++;
	}
	line = 0;
	v->add(&line, sizeof(int));
}
