#ident	"@(#)MultiPList.h	1.2"
#ifndef MULTIPLIST_H
#define MULTIPLIST_H
#include <Xm/Xm.h>
#include "PList.h"

//
// Callback usage notes.
//
// 1.  The "itemClientData" argument to member functions AddListItem  
//     and AddLIstItems is returned in the itemSelectedCallback 
//     function that was given during object construction. See
//     the ItemSelCallback typedef.  Also the callback set by member 
//     function SetClearItemCallback will return the same 
//     "itemClientData".  See the ClearCallback typedef.
//
// 2.  The "clientData" argument to member function SetListClientData 
//     is returned to the function specified by the member function
//     SetClearCallback.
// 
// 3.  The first argument to callbacks "ItemSelCallback", 
//     "SetClearItemCallback" and "SetClearCallback" will 
//     contain the object pointer "this".
// 

typedef void (*ItemSelCallback)(XtPointer obj, char * itemString, 
                                 int vlistNum, XtPointer clientData);
typedef void (*ItemOpenCallback)(XtPointer obj, char * itemString, 
                                 int vlistNum, XtPointer clientData);
typedef void (*ClearCallback) (XtPointer obj, int, XtPointer clientData);
typedef void (*UnSelectCallback) (XtPointer obj, int listNum);


//
// This structure maintains the state of a virtual list
//
typedef struct _vir{
	XmString *listItems;         // XmStrings array
	int *pixmapRelate;           // pixmap relationship array
	XtPointer *clientItemData;   // per item client data array
	XtPointer clientData;        // per VList client data
	int arraySize;			     // size of item relationship arrays
	int count;				     // item count in v list
    int selectedPos;		     // selected item in v list
    int topItem;			     // top most visible item in v list
	int	listLoadedIn;		     // physical list v list loaded in
}vir;


class MultiPList {

  public:
    
    MultiPList(Widget, int, int, ItemSelCallback,
			   XtPointer perObjClientData, int spacing = 0, int rowcount = 0,
				int numcol = 0);
    MultiPList(Widget, int, ItemSelCallback, XtPointer perObjClientData,
			   int spacing = 0, int rowcount = 0, int numcol = 0,
			   int visibleItemCount = 7); 
    virtual ~MultiPList ();
	void   NextList(void);

	void   AddListItem(XmString  xmstr,int pixmapIndex, 
                            XtPointer itemClientData);
	void   AddListItems(XmString  *xmstr,int *pixmapIndex,
                            XtPointer *itemClientData, int count);

    int    AddPixmap(char *pixmapName );
	
    void   SetOpenCallback(ItemOpenCallback);
    void   SetClearCallback(ClearCallback);
    void   SetUnSelectCallback(UnSelectCallback);
    void   SetListClientData(XtPointer clientData);
    void   SetClearItemCallback(ClearCallback);

	void   ClearLists(int pos = 0);
	Widget GetListWidget(int);
	void   SelectItem(int i, int position);
	void   SelectItem(int i, XmString xmstr);
	void   SelectTop(int i, int position);
	int    GetTop(int i);
	int    GetListCount(int i);
	int    GetVisibleCount();
    void   DisplayLists();

	int    NumOfLists() { return (amountOfLists); }
	int	   GetCurrentListNumber() { return (currentLoadVList); }
    Widget GetTopWidget() { return (form); }
	XtPointer  GetObjClientData(void);
    
  protected:

    ClearCallback clientItemClearCallback;  // Function to be called when 
	                                        //  an item is cleared
    UnSelectCallback clientUnSelectCallback;// Function to be called when 
	                                        //  an item is unselected
	ClearCallback clientClearCallback;      // Function to be called when 
	                                        //  a vlist is cleared 
    ItemSelCallback clientCallback;         // Function to be called when
	                                        //  an item is selected
    ItemOpenCallback clientOpenCallback;    // Function to be called when
	                                        //  an item is "opened"
    XtPointer clientData;                   // Per object clientData 

    virtual void ItemSelected (char * ,int listNum);
    virtual void ItemOpened (char * ,int listNum);
    virtual void Destroy (void);
    virtual void DisplayAt(int value, int savePos);
	virtual void LoadList(int i, vir *lip);
	virtual void PositionScrollBar(void);

  private:

	void   AddMoreSpace(void);	
	static void ScrollBarCallback ( Widget, XtPointer, XtPointer );
	static void ListDblCallback ( Widget, XtPointer, XtPointer );
	static void ListCallback ( Widget, XtPointer, XtPointer );
	static void DestroyCallback ( Widget, XtPointer, XtPointer );

	enum { INVALID = -1 };            // Invalid condition emun
	const int initialVirtualLists;    // Num of vlist to allocate space for
	const int initialArraySize;       // Array size to allocate on creation
	const int EXTRA_SPACE;            // malloc this much extra space when 
                                      // allocating blocks

	Widget parentWidget;			  // parent widget of MultiPList object
    Widget form;                      // mother of MultiPList widgets
    Widget scrollbar;        
	enum { maxVisibleLists = 10 };    // maximum PLists allowed
    Widget plistForm[maxVisibleLists];
	PList *plist[maxVisibleLists];

	int	   widgetsDestroyed;          // Boolean Xt destroyed me already
	int    listStart;                 // first visible virtual list
	int    amountOfLists;             // number of physical lists
	int    numOfVirtualLists;       
	int	   currentLoadVList;          // virtual list number to load items into
	vir    *vlp;                      // virtual lists storge area pointer

	//
	// Both the initialize( copy constructor) and assignment member functions 
	// are illegal to use so they are declared private ( not defined anywhere )
	// ( Link time error if used by me, compile time by anyone else )
	//
	MultiPList( const MultiPList &);
	MultiPList& operator=(const MultiPList & ); 
};
#endif
