#ident	"@(#)linkObj.C	1.2"
/******************************************************************************
 * Link Object = 	Generic Link object class that provides a template for	a
					acts as a link between the LinkManager and the derived obj
 *****************************************************************************/
#include <iostream.h>

#include "linkObj.h"

/******************************************************************************
 * Constructor for the LinkObject class.  New the LinkManager from which the
 * LinkObject is inherited.
 *****************************************************************************/
LinkObject::LinkObject() : LinkManager ()
{
#ifdef DEBUG
cout << "ctor for LinkObject" << endl;
#endif
	_ObjectIndex = 0;
	_ObjectName = NULL;
	_deleteObject = False;
}

/******************************************************************************
 * Destructor for the LinkObject class.
 *****************************************************************************/
LinkObject::~LinkObject()
{
#ifdef DEBUG
cout << "dtor for LinkObject" << endl;
#endif
}

/*****************************************************************
 Compare the object by number.
 *****************************************************************/
Boolean
LinkObject::CompareObject (const char *name)
{
	if (!name)
		return False;

	if (!strcmp (name, _ObjectName))
		return True;
	else
		return False;
}

/*****************************************************************
 Compare the object by number.
 *****************************************************************/
Boolean
LinkObject::CompareObject (int index)
{
	if (!index)
		return False;

	if (index == _ObjectIndex)
		return True;
	else
		return False;	
}

/*****************************************************************
 * Get the object by number.
 *****************************************************************/
void
LinkObject::GetObject (int &index)
{
	index = _ObjectIndex;	
}

/*****************************************************************
 * Get the object by name.
 *****************************************************************/
void
LinkObject::GetObject (char *name)
{
	name = _ObjectName;
	cout << "name is " << name << endl;
}

/*****************************************************************
 * Raise the object . Dummy function for non-gui instances. To be derived
 * for gui instances. Raise dialog window in gui object.
 *****************************************************************/
void
LinkObject::RaiseObject ()
{
}
