#ident	"@(#)PList.h	1.2"
#ifndef PLIST_H
#define PLIST_H
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/List.h>

typedef void (*clientCallback) (Widget,XtPointer,XtPointer);

struct pixmapStruct {
	char *name;
	Pixmap pixmap;
	Pixmap mask;
};

class PList {
	
public:
	
	PList (Widget parent, char *name, int spacing, int rowcount, 
				int numcol, int pixmapGeom = 16,
				XtTranslations parsed_xlations = NULL, int visibleItemCount = 7);
	~PList();

	// Item selection/manipulation routines

	Boolean  UW_ListItemExists (XmString);
	Widget   UW_GetListWidget(void) { return (listWidget); }
	void     UW_SetSelectedItem(int);
	void     UW_SetSelectedItem(XmString);
	void     UW_SetTopItem(int);
	XmString UW_GetSelectedItem(void);

	//  Callback registration routines

	void   UW_RegisterDblCallback(clientCallback, XtPointer userData);
	void   UW_RegisterSingleCallback(clientCallback, XtPointer userData);

	// Functions to delete a list item

	virtual void   UW_ListDeleteAllItems(void);
	virtual void   UW_ListDeleteItem(XmString item);
	virtual void   UW_ListDeleteItems(XmString *item,int item_count);
	virtual void   UW_ListDeleteItemsPos(int item_count, int position);
	virtual void   UW_ListDeletePos(int position);

	// Functions to add item/items

	virtual void   UW_ListAppendItem(XmString xmstr, int pixmapIndex);
	virtual void   UW_ListAddItem(XmString item,int pos,int pixmapIndex = -1);
	virtual void   UW_ListAddItemUnselected(XmString item,int pos,
								int pixmapIndex = -1);
	virtual void   UW_ListAddItems(XmString * xmstr,int count, int pos,
								int pixmapIndex = -1);
	virtual void   UW_ListAddItems(XmString * xmstr,int count, int pos,
								int *pixmapIndexs = NULL);

	// Functions to replace items

	virtual void   UW_ListReplaceItems(XmString *old_items, int item_count,
								XmString *new_items);
	virtual void   UW_ListReplaceItemsPos(XmString *new_items, 
								int item_count, int position);


	// Pixmap related functions
	
	virtual int    UW_AddPixmap(char *pixmapName, Pixmap * = NULL);
	virtual int    UW_AddPixmap(Pixmap pixmap, Pixmap mask = 0);
	virtual void   UW_DelAllPixmaps(int reloadFlag = False);
	virtual int    UW_ChangePixmap(int pixmapIndex,char *newPixmapName);
	virtual void   UW_ChangePixmapRelation(int itemPostion,int pixmapIndex);
	virtual void   UW_ChangePixmapRelation(int itemPosition,char *pixmapName);
	virtual int    UW_GetPixmapName(int pixmapIndex,char *pixmapName );
	virtual Pixmap UW_GetPixmapAtIndex(int pixmapIndex);
	virtual int    UW_GetPixmapIndex(char *pixmapName);
	virtual int    UW_GetPixmapIndex(Pixmap pixmap);
	virtual unsigned long UW_GetPixmapBackground(void);

protected:

	clientCallback callbackDbl;   // Function to be called
	clientCallback callbackSingle;
	virtual void List ( Widget w, XtPointer clientData,
	                  XtPointer callData );

private:

	void   GetMoreSpace(int incBy);	
	static void ListWindowCallback ( Widget, XtPointer, XtPointer );
	static void ItemInitProc( Widget , XtPointer , XtPointer );
	void GetPixmapMask( Widget w, struct pixmapStruct * pix);

	const int initialArraySize;      // Array size to allocate on creation
	const int EXTRA_SPACE;           // malloc this much extra space when 
    	                             // allocating blocks
	enum { INVALID = -1 };           // Invalid condition emun
	enum { APPEND = 0 };             // End of list append enum
	Widget parentWidget;
	Widget listWidget;
	Widget scrollWindowWidget;
	XtPointer callbackSingleData;
	XtPointer callbackDblData;

	int		pixmapHeight;
	int		pixmapWidth;
	unsigned long 	pixmapBackground;
	XmString	selectedItem;
	enum { maxPixmapFiles = 32 };    // maximum pixmaps allowed
	struct pixmapStruct pix[maxPixmapFiles];
	int		*pixmapRelate;
	int     pixmapRelateSize;
	int		pixmapCount;

	XmString *listItems;
	int     listItemsSize;
	int		listItemsCount;

	int		itemCount;

	//
	// Both the initialize( copy constructor) and assignment member functions 
	// are illegal to use so they are declared private ( not defined anywhere )
	// ( Link time error if used by me, compile time by anyone else )
	//
	PList( const PList &);
	PList& operator=(const PList & ); 
};

#endif
