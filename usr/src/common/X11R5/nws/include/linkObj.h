#ident	"@(#)linkObj.h	1.2"
////////////////////////////////////////////////////////
// LinkObject.h:
/////////////////////////////////////////////////////////
#ifndef LINKOBJ_H
#define LINKOBJ_H

#include "linkMgr.h"

class LinkObject  : public LinkManager {
    
public:
   						LinkObject();	/* CTOR for LinkObject */
   	virtual 			~LinkObject(); 	/* DTOR for LinkObject */

private:

protected:									/* Inherited protected variables. */
	Boolean				_deleteObject;		/* Flag for garbage collection */
	char				*_ObjectName;
	int					_ObjectIndex;

public:			/* Public member methods accessed from DialogManager class  */
	virtual void		RaiseObject();
	Boolean				CompareObject(const char *);
	Boolean				CompareObject(int);
	void 				GetObject(char *);
	void 				GetObject(int&);
	Boolean				DeleteObject() const { return _deleteObject; }
};
#endif
