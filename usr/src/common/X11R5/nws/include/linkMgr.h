#ident	"@(#)linkMgr.h	1.2"
////////////////////////////////////////////////////////
// LinkManager.h:
/////////////////////////////////////////////////////////
#ifndef LINKMANAGER_H
#define LINKMANAGER_H

#include <Xm/Xm.h>

class LinkManager  {
    
public:
   					LinkManager();		/* CTOR for LinkManager */
   	virtual 		~LinkManager(); 	/* DTOR for LinkManager */

private:

protected: 										/* Protected inline methods */

	LinkManager		*_next;
	LinkManager		*_prev;

public: 										/* Public inline methods */

    virtual inline void                 next (LinkManager* next);
    virtual inline LinkManager*         next ();
    virtual inline void                 prev (LinkManager* prev);
    virtual inline LinkManager*         prev ();
};

LinkManager*
LinkManager::next ()
{
    return (_next);
}

void
LinkManager::next (LinkManager* next)
{
    _next = next;
}

LinkManager*
LinkManager::prev ()
{
    return (_prev);
}

void
LinkManager::prev (LinkManager* prev)
{
    _prev = prev;
}
#endif 		//LINKOBJECT_H
