#ident	"@(#)objectMgr.C	1.2"
/////////////////////////////////////////////////////////
// ObjectManager object class = manages the object.
/////////////////////////////////////////////////////////
#include <iostream.h>

#include "objectMgr.h"
#include "linkObj.h"

/******************************************************************************
 * Constructor for the Object Manager
 *****************************************************************************/
ObjectManager::ObjectManager ()
{
#ifdef DEBUG
    cerr << "Construct - ObjectManager (" << this << ")" << endl;
#endif
    _objectlist = 0;
}

/******************************************************************************
 * Destructor for the Object Manager
 *****************************************************************************/
ObjectManager::~ObjectManager ()
{
#ifdef DEBUG
    cerr << "Destroy - ObjectManager (" << this << ")" << endl;
#endif
	collectGarbage();

    LinkObject*                   pt;

	/* Delete all objects from the linked list
	 */
    while (_objectlist) {	
        pt = _objectlist;
        _objectlist = (LinkObject *)pt->next ();
        delete (pt);                            // ??????????????????? TEST THIS
    }
}

/******************************************************************************
 * Add an object to the top of the linked list. Connect the next pointer
 * of the new object to the top of the list.  Connect the previous of the top
 * of the list to the new object. 
 *****************************************************************************/
void
ObjectManager::addObject (LinkObject* object)
{
#ifdef DEBUG
	cout << "Adding object" << endl;
#endif
	collectGarbage();

    if (object) {

		/* Set next pointer of the incoming object to _objectlist (first one).
		 * In case of the first object the next pointer is null.
	 	 */
        object->next (_objectlist);

		/* If objectlist was already created then set the previous object in
		 * the link to the incoming object. Basically creating new object on
		 * top of the linked list each time it comes in. 
		 */
        if (_objectlist) {		
            _objectlist->prev (object);
        }

		/* Set the objectlist to incoming object
		 */
        _objectlist = object;
    }
}

/******************************************************************************
 * Remove object from the link list.  Connect the previous of the next object
 * to the previous of the deleted object.  Connect the next of the previous 
 * object to the next of the deleted object.
 *****************************************************************************/
void
ObjectManager::removeObject (LinkObject* object)
{
#ifdef DEBUG
    cerr << "ObjectManager::removeObject (" << object << ")" << endl;
#endif

	collectGarbage();

    LinkObject*                   pt = _objectlist;

	/* If null object was sent then return 
	 */
    if (!object) 
        return;

    while (pt) {
        if (pt == object) {

			/* If there is a next object point the previous of the next object
			 * to the previous of current object (i.e pt since it will be gone)
			 */
            if (pt->next ()) {
                (pt->next ())->prev (pt->prev ());
            }
			/* If the previous of current object is NULL then the next object
			 * becomes the first object in linked list
		 	 */
            if (!pt->prev ()) {
                _objectlist = (LinkObject *)pt->next ();
            }
            else {
				/* Otherwise, the next of the previous object should now point
				 * to the next of our current object since we are deleting the 
				 * current object
				 */
                (pt->prev ())->next (pt->next ());
            }
        }
        pt = (LinkObject *)pt->next ();	/* Loop all object in list */
    }						/* For all object in a linked list */
}

/******************************************************************************
 * Remove the object object from the linked list whose delete flag was set 
 * to true. Then delete the object object. 
 *****************************************************************************/
void
ObjectManager::collectGarbage ()
{
#ifdef DEBUG
	cout << "collecting garbage " << endl;
#endif
    LinkObject* pt = _objectlist;
    LinkObject* tmp;

    while (pt) {

		tmp = (LinkObject *)pt->next();

		/* If the object was marked delete then remove object and delete object
		 */
        if (pt->DeleteObject())  {
#ifdef DEBUG
			int index;
			pt->GetObject(index);
			cout << "before delete pt is needed = " << index << endl;
#endif
			/* If there is a next object point the previous of the next object
			 * to the previous of current object (i.e pt since it will be gone)
			 */
            if (pt->next ()) 
                (pt->next ())->prev (pt->prev ());

			/* If the previous of current object is NULL then the next object
			 * becomes the first object in linked list
		 	 */
            if (!pt->prev ()) 
                _objectlist = (LinkObject *)pt->next ();
            else 
				/* Otherwise, the next of the previous object should now point
				 * to the next of our current object since we are deleting the 
				 * current object
				 */
                (pt->prev ())->next (pt->next ());
#ifdef DEBUG
			pt->GetObject(index);
			cout << "deleting name from pt = " << index << endl;
#endif
			delete pt;
        }	/* If the delete flag was set to true */
      		
		pt = tmp;	/* Loop all object */
    }						/* For all object in a linked list */

}

/******************************************************************************
 *  Raise the object window of the object whose name matches with incoming
 *  name parameter.
 *****************************************************************************/
Boolean
ObjectManager::raiseObject (const char *name)
{
    LinkObject		*obj  = IsObject(name);

	if (obj){
		obj->RaiseObject();
		return True;
	}
	return False;
}

/******************************************************************************
 *  Raise the object window of the object whose name matches with incoming
 *  name parameter.
 *****************************************************************************/
Boolean
ObjectManager::raiseObject (int num)
{
    LinkObject		*obj  = IsObject(num);

	if (obj){
		obj->RaiseObject();
		return True;
	}
	return False;
}

/******************************************************************************
 * Check to see if object exists, by name.
 *****************************************************************************/
LinkObject *
ObjectManager::IsObject (const char *name)
{
#ifdef DEBUG
	cout << "is  object there by name" << endl;
#endif
	collectGarbage();

    LinkObject		*pt = _objectlist;

    if (!name) 
        return (NULL);
	
    while (pt) {
        if (pt->CompareObject (name)) 
            return (pt);
        pt = (LinkObject *)pt->next ();
    }
    return (NULL);
}

/******************************************************************************
 * Check to see if object exists, by integer value (usually index used by app).
 *****************************************************************************/
LinkObject *
ObjectManager::IsObject (int num)
{
#ifdef DEBUG
	cout << "Is object there by num" << endl;
#endif
	collectGarbage();

    LinkObject		*pt = _objectlist;

    if (!num) 
        return (NULL);
	
    while (pt) {
        if (pt->CompareObject (num))
            return (pt);
        pt = (LinkObject *)pt->next ();
    }
    return (NULL);
}

/******************************************************************************
 * Get each object by index that is passed, sequential access.
 *****************************************************************************/
LinkObject *
ObjectManager::GetObject (int index)
{
#ifdef DEBUG
	cout << "Get object by index" << endl;
#endif
	collectGarbage();

    LinkObject		*pt = _objectlist;

	int i = 0;
	while (pt) {
		if (i == index)
			return pt;
		else
			pt = (LinkObject *)pt->next();
		i++;
	}
	return NULL;
}
