#ident	"@(#)proclist.C	1.2"
/*****************************************************************************
 *		ProcList class - linked list access.
 *****************************************************************************/
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "proclist.h"
#include "procitem.h"
#include "process.h"

/*****************************************************************************
 *  FUNCTION:
 *	ctor
 *  DESCRIPTION:
 *  The ctor for process list initializes the link list ProcItem
 *  and sets up the device data in the Device class
 *  RETURN:
 *  nothing
 *****************************************************************************/
ProcList::ProcList()
{
#ifdef DEBUG
cout << "ctor for ProcList" << endl;
#endif
	_procitem = _enditem = 0;
}

/*****************************************************************************
 *  FUNCTION:
 *	dtor
 *  DESCRIPTION:
 *  The dtor deletes the Device class (array) that was set up and
 *  removes the linked list of ProcItems.
 *  RETURN:
 *  nothing
 *****************************************************************************/
ProcList::~ProcList()
{
#ifdef DEBUG
cout << "dtor for ProcList" << endl;
#endif
	remove();
}

/*****************************************************************************
 *  FUNCTION:
 *	insert (psinfo_t&)
 *  DESCRIPTION:
 *  Insert a process item into the linked list
 *  RETURN:
 *  nothing
 *****************************************************************************/
void 
ProcList::insert (psinfo_t& eachps)
{
	ProcItem 	*newprocitem = new ProcItem (eachps);

	if (_procitem == 0)
		_enditem = newprocitem;

	_procitem = newprocitem;
}

/*****************************************************************************
 *  FUNCTION:
 *	append (psinfo_t&)
 *  DESCRIPTION:
 *  Append a process item at the end of the linked list
 *  RETURN:
 *  nothing
 *****************************************************************************/
void 
ProcList::append (psinfo_t &eachps)
{
	ProcItem	*newprocitem = new ProcItem(eachps);

	if (_procitem == 0)
		_procitem = newprocitem;	
	else
		_enditem->_next = newprocitem;	
	_enditem = newprocitem;
}

/*****************************************************************************
 *  FUNCTION:
 *	remove ()
 *  DESCRIPTION:
 *  Remove all linked list items. Empty the list.
 *  RETURN:
 *  nothing
 *****************************************************************************/
void 
ProcList::remove ()
{
	ProcItem	*eachprocitem = _procitem;

	while (eachprocitem) {
		ProcItem *tmp = eachprocitem;
		eachprocitem = eachprocitem->_next;
		delete tmp;
	}
	_procitem = _enditem = 0;
}

/*****************************************************************************
 *  FUNCTION:
 *	remove (pid_t)
 *  DESCRIPTION:
 *  Find the item in the linked list using the pid
 *  RETURN:
 *  ProcItem * or NULL if not found
 *****************************************************************************/
ProcItem *
ProcList::FindProcItem (pid_t pid)
{
	ProcItem	*tmp;

	for (tmp = _procitem; tmp != NULL; tmp = tmp->_next) {
		if (tmp->_psinfostruct->pr_pid == pid)
			return tmp;
	}
	return NULL;
}

/*****************************************************************************
 *  FUNCTION:
 *	remove (pid_t)
 *  DESCRIPTION:
 *  Find the item in the linked list using the position
 *  RETURN:
 *  ProcItem * or NULL if not found
 *****************************************************************************/
ProcItem *
ProcList::FindProcItem (int position)
{
	int			i;
	ProcItem	*tmp;

	for (i = 0,tmp = _procitem; tmp != NULL; i++, tmp = tmp->_next) {
		if (i == (position - 1)) 
			return tmp;
	}

	return NULL;
}

/*****************************************************************************
 *  FUNCTION:
 *	sort (Process *, int)
 *  DESCRIPTION:
 *  Sort all linked list items by pid, username etc.
 *  RETURN:
 *  nothing
 *****************************************************************************/
void 
ProcList::sort(Process *obj, int sortbynum)
{
	ProcItem 	*prv, *tmpitem;
	int			changed = 0, i = 0;

	do {
		changed = 0; i = 0;
		prv = _procitem;
		for (tmpitem = _procitem; tmpitem->_next != NULL; 
						tmpitem = tmpitem->_next){

			if (IsLessThan (obj, tmpitem, sortbynum)){

				ProcItem *tmp = tmpitem;
				ProcItem *nextone = tmpitem->_next->_next;

				tmpitem = tmpitem->_next;
				tmpitem->_next = tmp;
				tmp->_next =  nextone;
				if (!i) 				/* First item in the linked list*/ 
					_procitem = tmpitem; 	
				else 
					prv->_next = tmpitem; /* Make sure prv one is current*/
				changed = 1;
			}
			i++;
			prv = tmpitem;
		}
		_enditem = tmpitem;
	} while (changed);

}

/*****************************************************************************
 *  FUNCTION:
 *	IsLessThan (Process *, ProcItem *, int)
 *  DESCRIPTION:
 *  Determine if the next value is less than the current one based
 *  on the sorting type.
 *  RETURN:
 *  if less than or not.
 **************************************************************/
int
ProcList::IsLessThan(Process *processobj, ProcItem *tmpitem, int sortbynum)
{
	int			retcode = 0;  

	switch (sortbynum) {
		case PID: 	if (tmpitem->_next->_psinfostruct->pr_pid <  
						tmpitem->_psinfostruct->pr_pid)
						retcode = 1;
					break;
			
		case USER: 	char *username1 = processobj->GetUserName(
										tmpitem->_next->_psinfostruct->pr_uid);
					char *username2 = processobj->GetUserName(
										tmpitem->_psinfostruct->pr_uid);
					
					if (strcmp (username1, username2) < 0)
						retcode = 1;
					free (username1);
					free (username2);
					break;
	}
	return retcode;
}
