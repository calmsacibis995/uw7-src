/* $XFree86: xc/programs/Xserver/hw/xfree86/reconfig/strlist.h,v 3.1 1995/01/28 16:07:37 dawes Exp $ */





/* $XConsortium: strlist.h /main/3 1995/11/13 06:29:04 kaleb $ */

/* Used in the %union, therefore to be included in the scanner. */
typedef struct {
	int count;
	char **datap;
} string_list ;

typedef struct {
	int count;
	string_list **datap;
} string_list_list;
