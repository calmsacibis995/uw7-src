#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/client.h	1.1"
#endif

	/* File : client.h 	
	 * To be included by the user application and the
	 * lookup table library
    	 */
typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;


typedef struct {
  Widget   text;        /* Text widget handle where the system name is to be
                           set */
  char     *prevVal; /* previous text string in the text */
  Boolean  hostSelected;
  HelpText *help;       /* help info to be passed */
} UserData;


