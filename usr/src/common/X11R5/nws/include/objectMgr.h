#ident	"@(#)objectMgr.h	1.2"
/////////////////////////////////////////////////////////
// ObjectManager.h:
/////////////////////////////////////////////////////////
#ifndef OBJECTMANAGER_H
#define OBJECTMANAGER_H

#include <Xm/Xm.h>

class LinkObject;

class ObjectManager {
public:                         // Constructors/Desctuctors
                                ObjectManager ();
    virtual                     ~ObjectManager ();

protected:                      // Protected Data
    LinkObject					*_objectlist;

protected:                      // Protected List Methods
    void                        addObject (LinkObject*);
    void                        removeObject (LinkObject*);
    Boolean                     raiseObject (const char *name);
    Boolean                     raiseObject (int num);
    LinkObject                  *IsObject (const char *name);
    LinkObject                  *IsObject (int num);
    LinkObject                  *GetObject (int num);
	void						collectGarbage();

protected:                      // Private Callback Methods

};
#endif      // OBJECTMANAGER_H
