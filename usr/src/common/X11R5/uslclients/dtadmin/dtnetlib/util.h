#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:util.h	1.5"
#endif


#include <Xm/Label.h> /* for Boolean definition -- should be get rid of --*/
/*
typedef struct _ActionAreaItems{
    char *label;
    void (*callback)();
    caddr_t data;
    Widget      widget;
} ActionAreaItems;
*/

enum enumInetAddr {
        ADDR1,
        ADDR2,
        ADDR3,
        ADDR4,
        NUMADDR
};

typedef struct _inetAddr {
	Widget	addr[NUMADDR];
} inetAddr;

typedef struct _radioItem {
	char	*label;
	void 	(*callback)();
	Widget	widget;
} radioItem;

typedef struct _radioList {
	radioItem		*list;
	int			count;
	int			orientation;	
	int			curSelection; /* set the default value */
} radioList;

typedef enum {
	ENTIRE_SYSTEM,
	PERSONAL_ACCOUNT_ONLY,
	NUM_ACCESS_RIGHT
} accessRight;

typedef enum {
	NO_ONE,
	SELF,
	ALL_USERS,
	SPECIFIC_USERS,
	NUM_USER_ACCESS
} userAccess;

enum subnet {
        CLASS_A,
        CLASS_B,
        CLASS_C,
        OTHER,
        NUM_SUBNET
};

#define DIRECTORY(f, s)  ((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define  DMODE           (S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define MODE            (S_IRWXU | S_IRGRP | S_IROTH) /* 0744 */

/* mini-help data type */

typedef struct _mini_help {
	String	msg;
	Widget	widget;
} MiniHelp_t;

/* macros to help w/ pushbutton mnemonics */

#define Xm_MNE(M, MNE_INFO, OP)\
	if (M) {\
		mne = (unsigned char *)mygettxt(M);\
		mneks = XStringToKeysym((char *)mne);\
		(MNE_INFO).mne = (unsigned char *)strdup((char *)mne);\
		(MNE_INFO).mne_len = strlen((char *)mne);\
		(MNE_INFO).op = OP;\
	} else {\
		mne = (unsigned char *)NULL;\
		mneks = NoSymbol;\
	}

#define XmSTR_N_MNE(S, M, M_INFO, OP)\
	mneString = XmStringCreateSimple(S);\
	Xm_MNE(M, M_INFO, OP)

#define REG_MNE(W, MNE_INFO, N_MNE)\
	DmRegisterMnemonic(W, MNE_INFO, N_MNE);\
	for (mm = 0; mm < N_MNE; mm++) {\
		XtFree((char *)MNE_INFO[mm].mne);\
	}

#define MI_TOTAL	10
