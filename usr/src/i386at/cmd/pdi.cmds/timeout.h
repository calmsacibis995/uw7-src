#ident	"@(#)pdi.cmds:timeout.h	1.1.1.1"

#ifndef _timeout_h
#define _timeout_h

#include	<stdio.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<sys/sdi_edt.h>
#include	<sys/scsi.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<langinfo.h>

typedef char Boolean;
#define False	0
#define True	1

#define UNDEF	-1

#ifndef HBA_TIMEOUT_RESET
#define HBA_TIMEOUT_RESET	8
#endif

#ifndef B_CAPABILITIES
#define B_CAPABILITIES	0x4211
#endif

#ifndef B_NEW_TIMEOUT_VALUES
#define B_NEW_TIMEOUT_VALUES	0x4210
#endif

#ifndef B_TIMEOUT_SUPPORT
#define B_TIMEOUT_SUPPORT	0x4212
#endif

/* values used in the parsing of the command line arguments */
#define START	0
#define DEVICE	1
#define COMMAND	2
#define VALUE	3

/* values use in the CommandTable to indicate which commands are for
	which device types */
#define PT_DISK	1
#define PT_TAPE	2
#define PT_CDROM	4
#define PT_WORM	8
#define PT_CHANGER	16
#define PT_ALL	PT_DISK|PT_TAPE|PT_CDROM|PT_WORM|PT_CHANGER

typedef struct {
	struct	scsi_adr	sap;
	int			length;
	unsigned short		table[256];
} ptTimeoutTable;

typedef struct fentry {
	int		device_type;
	char * 		inquiry;
	int		command;
	unsigned short	value;
	struct fentry *	next;
} ptFileEntry;

extern Boolean	ptCheckDevice(char *, struct scsi_xedt *, int, char **, int *);
extern Boolean	ptCommandToInt(char *, char *, int, Boolean, int **, int *);
extern void	ptFreeFileEntry(ptFileEntry *);
extern void	ptInsertEntry(ptFileEntry **, char *, int, int, unsigned short);
extern void	ptInsertEntryValue(ptFileEntry **, char *, int, int, unsigned short, struct scsi_xedt *, int);
extern Boolean	ptIsCommand(char *, int, char *, Boolean);
extern Boolean	ptPrintCommands(char *);
extern void	ptPrintInfo(struct scsi_xedt *, int);
extern void	ptPrintQuery(ptFileEntry *, ptTimeoutTable *, struct scsi_xedt *, int);
extern void	ptQueryAll(ptFileEntry **, struct scsi_xedt *, int);
extern struct scsi_xedt * ptReadEdt(int *);
extern ptFileEntry * ptReadFile(char *, struct scsi_xedt *, int, ptTimeoutTable **);
extern Boolean	ptResetToFactory(Boolean, ptFileEntry *, ptFileEntry **, struct scsi_xedt *, int, ptTimeoutTable *);
extern Boolean ptSendTable(ptTimeoutTable *, int);
extern Boolean	ptSetTimeout(int);
extern void	ptUpdateTable(ptTimeoutTable *, ptFileEntry *, struct scsi_xedt *, int);
extern Boolean	ptValidateNodeDescriptor(char *, struct scsi_xedt *, int, char **, int *, int *);
extern Boolean ptWriteFile(ptFileEntry *);
extern Boolean ptYesP(void);

#endif /* _timeout_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
