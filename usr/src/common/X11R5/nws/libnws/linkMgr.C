#ident	"@(#)linkMgr.C	1.2"
#include <iostream.h>

#include "linkMgr.h"

/******************************************************************************
 * Constructor for the LinkManager class.  
 *****************************************************************************/
LinkManager::LinkManager()
{
#ifdef DEBUG
cout << "ctor for LinkManager" << endl;
#endif
	/* Initialize the variables
	 */
	_prev = _next = 0;
}

/******************************************************************************
 * Destructor for the LinkManager class.
 *****************************************************************************/
LinkManager::~LinkManager()
{
#ifdef DEBUG
cout << "dtor for LinkManager" << endl;
#endif
	_prev = _next = 0;
}

