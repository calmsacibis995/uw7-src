#ident	"@(#)pdi.cmds:ptFuncs.c	1.1.1.2"

#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sysi86.h>
#include <limits.h>
#include <sys/wait.h>
#include "timeout.h"

/*  These coordinate with the ones in timeout.c */
#define MSG17	":17:Cannot open %s\n"
#define MSG18	":18:Cannot create %s\n"
#define MSG19	":19:Write error\n"
#define MSG20	":20:State of Timeout Reset feature cannot be determined\n"
#define MSG21	":21:Timeout Reset feature is enabled.\n"
#define MSG22	":22:Timeout Reset feature is disabled.\n"
#define MSG23	":23:\t%s supports Timeout Reset\n"
#define MSG24	":24:\t%s does not support Timeout Reset\n"
#define MSG25	":25:No devices of type %s on system\n"
#define MSG26	":26:No match for inquiry string: %s\n"
#define MSG27	":27:Invalid edt\n"
#define MSG28	":28:Insufficient privileges to run command"
#define MSG29	":29:Invalid command in pditimetab: %x\n"
#define MSG30	":30:Reset timeout values to defaults? [%s | %s]: "
#define MSG31	":31:Cannot determine Timeout Reset support for: %s\n"
#define MSG35	":35:ioctl(B_CAPABILITIES) failed\n"
#define MSG36	":36:ioctl(B_NEW_TIMEOUT_VALUES) failed\n"
#define MSG37	":37:ioctl(B_TIMEOUT_SUPPORT) failed\n"
#define MSG38	":38:mknod failed for sdi temp device\n"
#define MSG39	":39:Cannot open sdi device: %s\n"
#define MSG40	":40:RE failure: %s\n"
#define MSG41	":41:ioctl(SI86SDIDEV) failed.\n"

#define DEVICE_TYPE_NAME(t)	(t==PT_DISK ? "DISK" : (t==PT_TAPE ? "TAPE" : (t==PT_CDROM ? "CDROM" : (t==PT_WORM ? "WORM" : "CHANGER"))))

#define XTYPE_TO_PT(t) (t==ID_RANDOM ? PT_DISK : (t==ID_TAPE ? PT_TAPE : (t==ID_ROM ? PT_CDROM : (t==ID_WORM ? PT_WORM : (t==ID_CHANGER ?PT_CHANGER : 0)))))
#define MATCH_TYPES(id, pt)	((id == ID_RANDOM && pt & PT_DISK) || \
				(id == ID_TAPE && pt & PT_TAPE) || \
				(id == ID_WORM && pt & PT_WORM) || \
				(id == ID_ROM && pt & PT_CDROM) || \
				(id == ID_CHANGER && pt & PT_CHANGER))

#define PDITIMETAB	"/etc/scsi/pditimetab"
#define PDITIMETAB_ORIG	"/etc/scsi/pditimetab.orig"
#define IDTUNE_CMD	"/etc/conf/bin/idtune -f PDI_TIMEOUT 0"
#define IDBUILD_CMD	"/etc/conf/bin/idbuild"

static Boolean	closeSDI(int, char *);
static int	compareEntry(ptFileEntry *, ptFileEntry *);
static char *	intToCommand(int, int);
static int	openSDI(char *);
static int	patternMatch(char *, char *);
static ptFileEntry *	printValue(ptFileEntry *, ptTimeoutTable *, int);
static int	sendCapabilities(int);
static Boolean	sendNewTimeoutValues(int, ptTimeoutTable *);
static Boolean	sendTimeoutSupport(int *);
static int	validateDevice(char *);
static Boolean	validateInquiry(char *, struct scsi_xedt *, int, int *);

extern struct scsi_xedt * readxedt(int *edtcnt);

Boolean	ptCheckDevice(char *, struct scsi_xedt *, int, char **, int *);
Boolean	ptCommandToInt(char *, char *, int, Boolean, int **, int *);
void	ptFreeFileEntry(ptFileEntry *);
void	ptInsertEntry(ptFileEntry **, char *, int, int, unsigned short);
void	ptInsertEntryValue(ptFileEntry **, char *, int, int, unsigned short, struct scsi_xedt *, int);
Boolean	ptIsCommand(char *, int, char *, Boolean);
Boolean	ptPrintCommands(char *);
void	ptPrintInfo(struct scsi_xedt *, int);
void	ptPrintQuery(ptFileEntry *, ptTimeoutTable *, struct scsi_xedt *, int);
void	ptQueryAll(ptFileEntry **, struct scsi_xedt *, int);
struct scsi_xedt * ptReadEdt(int *);
ptFileEntry * ptReadFile(char *, struct scsi_xedt *, int, ptTimeoutTable **);
Boolean ptResetToFactory(Boolean, ptFileEntry *, ptFileEntry **, struct scsi_xedt *, int, ptTimeoutTable *);
Boolean ptSendTable(ptTimeoutTable *, int);
Boolean	ptSetTimeout(int);
void	ptUpdateTable(ptTimeoutTable *, ptFileEntry *, struct scsi_xedt *, int);
Boolean	ptValidateNodeDescriptor(char *, struct scsi_xedt *, int, char **, int *, int *);
Boolean ptWriteFile(ptFileEntry *);
Boolean ptYesP(void);

/* The CommandMap structure is used to create a static table of SCSI
	command information.  This includes the name of the SCSI command,
	the hex command, and flags to indicate which device types the
	command applies to.

	Notice that a given hex command can have different names for
	different devices.  This table was created from the information
	in the SCSI-2 specification.

	If changes are made (for future SCSI specs) make sure you update
	two things: the cmdTableLen int, and MAX_CMD_LEN.
	MAX_CMD_LEN is the length of the string plus 3 for "_nn" plus
	1 for the null.

	The PT_ALL is used only for commands that the SCSI spec identifies
	specifically as common commands.  Some entries in the table OR
	in all of the currently supported devices - but this may not be
	equivalent to PT_ALL in the future when other device types are
	supported.

	This list MUST be kept in alphabetical order so that the commands
	are printed to the user in order.
*/
typedef struct {
	char * name;
	int command;
	int device;
} CommandMap;

#define MAX_CMD_PREFIX	15
#define MAX_CMD_LEN	34
static int cmdTableLen = 91;
static const CommandMap cmdTable[] = {
	{"CHANGE_DEFINITION", 0x40, PT_ALL},
	{"COMPARE", 0x39, PT_ALL},
	{"COPY", 0x18, PT_ALL},
	{"COPY_AND_VERIFY", 0x3A, PT_ALL},
	{"ERASE", 0x19, PT_TAPE},
	{"EXCHANGE_MEDIUM", 0xA6, PT_CHANGER},
	{"FORMAT_UNIT", 0x04, PT_DISK},
	{"INQUIRY", 0x12, PT_ALL},
	{"INITIALIZE_ELEMENT_STATUS", 0x07, PT_CHANGER},
	{"LOAD_UNLOAD", 0x1B, PT_TAPE},
	{"LOCATE", 0x2B, PT_TAPE},
	{"LOCK_UNLOCK_CACHE", 0x36, PT_DISK|PT_WORM|PT_CDROM},
	{"LOG_SELECT", 0x4C, PT_ALL},
	{"LOG_SENSE", 0x4D, PT_ALL},
	{"MEDIUM_SCAN", 0x38, PT_WORM},
	{"MODE_SELECT_10", 0x55, PT_ALL},
	{"MODE_SELECT_6", 0x15, PT_ALL},
	{"MODE_SENSE_10", 0x5A, PT_ALL},
	{"MODE_SENSE_6", 0x1A, PT_ALL},
	{"MOVE_MEDIUM", 0xA5, PT_CHANGER},
	{"PAUSE_RESUME", 0x4B, PT_CDROM},
	{"PLAY_AUDIO_10", 0x45, PT_CDROM},
	{"PLAY_AUDIO_12", 0xA5, PT_CDROM},
	{"PLAY_AUDIO_MSF", 0x47, PT_CDROM},
	{"PLAY_AUDIO_TRACK_INDEX", 0x48, PT_CDROM},
	{"PLAY_AUDIO_TRACK_RELATIVE_10", 0x49, PT_CDROM},
	{"PLAY_AUDIO_TRACK_RELATIVE_12", 0xA9, PT_CDROM},
	{"POSITION_TO_ELEMENT", 0x2B, PT_CHANGER},
	{"PREVENT_ALLOW_MEDIUM_REMOVAL", 0x1E, PT_DISK|PT_WORM|PT_CDROM|PT_TAPE|PT_CHANGER},
	{"PRE_FETCH", 0x34, PT_DISK|PT_WORM|PT_CDROM},
	{"READ", 0x08, PT_TAPE},
	{"READ_10", 0x28, PT_DISK|PT_WORM|PT_CDROM},
	{"READ_12", 0xA8, PT_WORM|PT_CDROM},
	{"READ_6", 0x08, PT_DISK|PT_WORM|PT_CDROM},
	{"READ_BLOCK_LIMITS", 0x05, PT_TAPE},
	{"READ_BUFFER", 0x3C, PT_ALL},
	{"READ_CAPACITY", 0x25, PT_DISK|PT_WORM},
	{"READ_CDROM_CAPACITY", 0x25, PT_CDROM},
	{"READ_DEFECT_DATA", 0x37, PT_DISK},
	{"READ_ELEMENT_STATUS", 0xB8, PT_CHANGER},
	{"READ_HEADER", 0x44, PT_CDROM},
	{"READ_LONG", 0x3E, PT_DISK|PT_WORM|PT_CDROM},
	{"READ_POSITION", 0x34, PT_TAPE},
	{"READ_REVERSE", 0x0F, PT_TAPE},
	{"READ_SUB_CHANNEL", 0x42, PT_CDROM},
	{"READ_TOC", 0x43, PT_CDROM},
	{"REASSIGN_BLOCKS", 0x07, PT_DISK|PT_WORM},
	{"RECEIVE_DIAGNOSTIC_RESULTS", 0x1C, PT_ALL},
	{"RECOVER_BUFFERED_DATA", 0x14, PT_TAPE},
	{"RELEASE", 0x17, PT_DISK|PT_WORM|PT_CDROM|PT_CHANGER},
	{"RELEASE_UNIT", 0x17, PT_TAPE},
	{"REQUEST_SENSE", 0x03, PT_ALL},
	{"REQUEST_VOLUME_ELEMENT_ADDRESS", 0xB5, PT_CHANGER},
	{"RESERVE", 0x16, PT_DISK|PT_WORM|PT_CDROM|PT_CHANGER},
	{"RESERVE_UNIT", 0x16, PT_TAPE|PT_CHANGER},
	{"REWIND", 0x01, PT_TAPE},
	{"REZERO_UNIT", 0x01, PT_DISK|PT_WORM|PT_CDROM},
	{"SEARCH_DATA_EQUAL", 0x31, PT_DISK},
	{"SEARCH_DATA_EQUAL_10", 0x31, PT_WORM|PT_CDROM},
	{"SEARCH_DATA_EQUAL_12", 0xB1, PT_WORM|PT_CDROM},
	{"SEARCH_DATA_HIGH", 0x30, PT_DISK},
	{"SEARCH_DATA_HIGH_10", 0x30, PT_WORM|PT_CDROM},
	{"SEARCH_DATA_HIGH_12", 0xB0, PT_WORM|PT_CDROM},
	{"SEARCH_DATA_LOW", 0x32, PT_DISK},
	{"SEARCH_DATA_LOW_10", 0x32, PT_WORM|PT_CDROM},
	{"SEARCH_DATA_LOW_12", 0xB2, PT_WORM|PT_CDROM},
	{"SEEK_10", 0x2B, PT_DISK|PT_WORM|PT_CDROM},
	{"SEEK_6", 0x0B, PT_DISK|PT_WORM|PT_CDROM},
	{"SEND_DIAGNOSTIC", 0x1D, PT_ALL},
	{"SEND_VOLUME_TAG", 0xB6, PT_CHANGER},
	{"SET_LIMITS", 0x33, PT_DISK},
	{"SET_LIMITS_10", 0x33, PT_WORM|PT_CDROM},
	{"SET_LIMITS_12", 0xB3, PT_WORM|PT_CDROM},
	{"SPACE", 0x11, PT_TAPE},
	{"START_STOP_UNIT", 0x1B, PT_DISK|PT_WORM|PT_CDROM},
	{"SYNCHRONIZE_CACHE", 0x35, PT_DISK|PT_WORM|PT_CDROM},
	{"TEST_UNIT_READY", 0x00, PT_ALL},
	{"VERIFY", 0x13, PT_DISK|PT_TAPE},
	{"VERIFY_10", 0x2F, PT_WORM|PT_CDROM},
	{"VERIFY_12", 0xAF, PT_WORM|PT_CDROM},
	{"WRITE", 0x0A, PT_TAPE},
	{"WRITE_10", 0x2A, PT_DISK|PT_WORM},
	{"WRITE_12", 0xAA, PT_WORM},
	{"WRITE_6", 0x0A, PT_DISK|PT_WORM},
	{"WRITE_AND_VERIFY", 0x2E, PT_DISK},
	{"WRITE_AND_VERIFY_10", 0x2E, PT_WORM},
	{"WRITE_AND_VERIFY_12", 0xAE, PT_WORM},
	{"WRITE_BUFFER", 0x3B, PT_ALL},
	{"WRITE_FILEMARKS", 0x10, PT_TAPE},
	{"WRITE_LONG", 0x3F, PT_DISK|PT_WORM},
	{"WRITE_SAME", 0x41, PT_DISK}
};


/*
    closeSDI - called from functions that had previously opened SDI. It
	closes the file descriptor, and removes the temporary file.
*/
static Boolean
closeSDI(int sdi_fd, char * sditempnode)
{
	Boolean rc = True;

	if (close(sdi_fd) != 0)
		rc = False;
	if (unlink(sditempnode) != 0)
		rc = False;

	return(rc);
}  /* end of closeSDI() */

/*
    compareEntry - given two ptFileEntries returns -1, 0, or 1 when
	the first is less than, equal, or greater than the second,
	respectively.  This is used to sort the new entries into the
	timeout table, and to order the queries.
*/
static int
compareEntry(ptFileEntry * e1, ptFileEntry * e2)
{
	int d1 = e1->device_type;
	int d2 = e2->device_type;
	char * i1 = e1->inquiry;
	char * i2 = e2->inquiry;
	int o;

	/* if both are generic, then they are equal */
	if (!i1 && !d1 && !i2 && !d2)
		return(0);
	/* if one is generic then it is less */
	if (!i1 && !d1)
		return(-1);
	if (!i2 && !d2)
		return(1);
	/* if both are device types then order by device type and command */
	if (!i1 && d1 && !i2 && d2)  {
		if (d1 == d2) {
			if (e1->command == e2->command) return(0);
			if (e1->command < e2->command) return(-1);
			if (e1->command > e2->command) return(1);
		}
		if (d1 < d2) return(-1);
		if (d1 > d2) return(1);
	}
	/* if one is a device type then it is less */
	if (!i1 && d1)
		return(-1);
	if (!i2 && d2)
		return(1);
	/* then both are inquiries - order alphebetically */
	o = strcoll(i1, i2);
	if (o == 0)  {
		if (e1->command == e2->command) return(0);
		if (e1->command < e2->command) return(-1);
		if (e1->command > e2->command) return(1);
	}
	else return(o);

}  /* end of compareEntry() */

/*
    findMatch - is used in resetting values to the factory defaults.  It
	handles two operations: adding the matching entries to another list,
	and removing the matching entries from the list.  This function
	does not validate its arguments.
*/
#define ADD	1
#define DEL	2

static void
findMatch(ptFileEntry **list, ptFileEntry ** mlist, int op, ptFileEntry * query)
{
	ptFileEntry * l = *list;
	ptFileEntry * last;
	char * inq = query->inquiry;
	int dt = query->device_type;
	int cmd = query->command;

	/* move through the generic entries in the list */
	while (l && l->device_type == NULL && l->inquiry == NULL) {
		last = l;
		l = l->next;
	}

	/* look for device type matches */
	while (l && l->device_type && l->inquiry == NULL)  {
		if (l->device_type&dt && (cmd == UNDEF || cmd == l->command))  {
			/* Found a matching device_type and command */
			if (op == ADD && mlist)  {
				ptInsertEntry(mlist, l->inquiry, l->device_type, l->command, l->value);
			}
			else if (op == DEL)  { /* remove this entry */
				last->next = l->next;
				if (l->inquiry) free(l->inquiry);
				free(l);
				/* Reset l for next loop; last stays the same */
				l = last->next;
				continue;
			}
		}
		last = l;
		l = l->next;
	}
	/* look for inquiry matches */
	while (l && l->device_type && l->inquiry)  {
		if ( l->device_type&dt &&
			(!inq || patternMatch(inq, l->inquiry) == 0) &&
			(cmd == UNDEF || cmd == l->command) )  {
			/* Found a matching device, inquiry, and command */
			if (op == ADD && mlist)  {
				ptInsertEntry(mlist, l->inquiry, l->device_type, l->command, l->value);
			}
			else if (op == DEL)  { /* remove this entry */
				last->next = l->next;
				if (l->inquiry) free(l->inquiry);
				free(l);
				/* Reset l for next loop; last stays the same */
				l = last->next;
				continue;
			}
		}
		last = l;
		l = l->next;
	}
}  /* end of findMatch() */

/*
    intToCommand - returns the first command string in the cmdTable that has
	the same command int and matches the given device_type.  Returns
	NULL when it cannot be found.  Note - it does not validate the 
	arguments.
*/
static char *
intToCommand(int command, int device_type)
{
	int i;

	for (i = 0; i < cmdTableLen; i++)  {
		if (cmdTable[i].command == command &&
			cmdTable[i].device & device_type)
			return(cmdTable[i].name);
	}
	return(NULL);
}  /* end of intToCommand() */

/*
    openSDI - returns a file descriptor for the SDI minor number that
	is used to send ioctls to SDI.  Use closeSDI to clean up after
	calling openSDI.  This function returns 0 if it failed to open
	SDI.
*/
static int
openSDI(char * sditempnode)
{
	int	sdi_fd;
	dev_t	sdi_dev;

	if (setuid(0) != 0) {
		pfmt(stderr, MM_ERROR, MSG28);
		return(-1);
	}

	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		pfmt(stderr, MM_ERROR, MSG41);
		return(-1);
	}

	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		pfmt(stderr, MM_ERROR, MSG38);
		return(-1);
	}

	errno = 0;
	if ((sdi_fd = open(sditempnode, O_RDONLY)) < 0) {
		unlink(sditempnode);
		pfmt(stderr, MM_ERROR, MSG39, sditempnode);
		return(-1);
	}

	return(sdi_fd);
}  /* end of openSDI() */

/*
    patternMatch - the function that is used to determine if a string
	entered in by the user on the command line as an inquiry matches
	an inquiry string of a device on the system.  This algorithm
	is simple - the str1 must match exactly as a prefix to str2.
	The function returns 0, -1, or 1 when str1 matches str2, or str1
	is less than or greater than str2, respectively. */
static int
patternMatch(char * str1, char * str2)
{
	return(strncmp(str1, str2, strlen(str1)));
}  /* end of patternMatch() */

/*
    printValue - is called when the user has done a query to print out
	the values of a particular command.  It is passed the query,
	the timeout table, and the index into the timeout table for
	this query.  This function continues through the list of queries
	until it no longer finds that the query is for the same device.
	This is so the device is printed once, and then all the commands
	and values are printed as a group.  Since it handles more than
	one query, it returns the pointer to the next query that needs to
	be printed.
*/
static ptFileEntry *
printValue(ptFileEntry * query, ptTimeoutTable * table, int d)
{
	int i;
	char * inquiry = query->inquiry;
	int device_type = query->device_type;

	/* while we are querying the same device ... */
	while (query && (strcmp(query->inquiry, inquiry) == 0) && (query->device_type == device_type))  {
		if (query->command == UNDEF)  {
			/* print all the values of the commands */
			for (i = 0; i < cmdTableLen; i++)  {
				if (cmdTable[i].device&query->device_type) {
					printf("\t%s\t%d\n",
						cmdTable[i].name,
						(int)table[d].table[cmdTable[i].command]);
				}
			}
		}
		else  {
			Boolean printed = False;
			/* print the specific command and value */
			for (i = 0; i < cmdTableLen; i++)  {
				if (cmdTable[i].command == query->command &&
					cmdTable[i].device & query->device_type)  {
					printed = True;
					printf("\t%s\t%d\n",
						cmdTable[i].name,
						(int)table[d].table[cmdTable[i].command]);
					break;
				}
			}
			if (!printed)
				printf("\t0x%x\t%d\n", query->command,
					(int)table[d].table[query->command]);
		}
		query = query->next;
	}
	return(query);
}  /* end of printValue() */

/*
    sendCapabilities - does an ioctl to get the hba capabilities from
	sdi.  It returns True if the hba supports Timeout/Reset,
	False if it does not support Timeout/Reset, and -1 if an error
	prevented it from determining the state.
*/
static int
sendCapabilities(int ctl)
{
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	int	sdi_fd;
	unsigned int info;

	if ((sdi_fd = openSDI(sditempnode)) < 0)
		return(-1);

	/* initialize the info to the hba controller number */
	info = (unsigned int) ctl;

	if (ioctl(sdi_fd, B_CAPABILITIES, &info) < 0)  {
		pfmt(stderr, MM_ERROR, MSG35);
		closeSDI(sdi_fd, sditempnode);
		return(-1);
	}

	if (!closeSDI(sdi_fd, sditempnode)) return(-1);
	return((info&HBA_TIMEOUT_RESET) != 0);
}  /* end of sendCapabilities() */

/*
    sendNewTimeoutValues - does an ioctl to send the timeout values to
	sdi.  It returns True if the hba values were sent successfully and
	False otherwise.
*/
static Boolean
sendNewTimeoutValues(int sdi_fd, ptTimeoutTable * table)
{
	if (ioctl(sdi_fd, B_NEW_TIMEOUT_VALUES, table) < 0)  {
		pfmt(stderr, MM_ERROR, MSG36);
		return(False);
	}

	return(True);
}  /* end of sendNewTimeoutValues() */

/*
    sendTimeoutSupport - does an ioctl to turn timeout/reset support on
	and off or to query the current value.  It is passed a pointer to
	an integer.  If it points to 1 or 0, then the feature is turned
	on or off.  If it points to 2, then the ioctl returns a pointer
	to 1 or 0 to indicate the current state.

	The function returns True if the timeout support was sent
	successfully and False otherwise.
*/
static Boolean
sendTimeoutSupport(int * timeout)
{
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	int	sdi_fd;

	if ((sdi_fd = openSDI(sditempnode)) < 0)
		return(False);
		
	if (ioctl(sdi_fd, B_TIMEOUT_SUPPORT, timeout) < 0)  {
		pfmt(stderr, MM_ERROR, MSG37);
		closeSDI(sdi_fd, sditempnode);
		return(False);
	}

	if (!closeSDI(sdi_fd, sditempnode)) return(False);
	return(True);
}  /* end of sendTimeoutSupport() */

/*
    validateDevice - given an upper case string, determines if the string is one
	of the predefined device types, and if so, returns the corresponding
	device_type integer value.  It returns 0 if it is not a match.

	This function assumes that the device type strings are not going
	to be internationalized.
*/
static int
validateDevice(char * device)
{
	int device_type;
	/* Validate the device type */
	if (strcmp (device, "DISK") == 0)
		device_type = PT_DISK;
	else if (strcmp (device, "TAPE") == 0)
		device_type = PT_TAPE;
	else if (strcmp (device, "CDROM") == 0)
		device_type = PT_CDROM;
	else if (strcmp (device, "WORM") == 0)
		device_type = PT_WORM;
	else if (strcmp (device, "CHANGER") == 0)
		device_type = PT_CHANGER;
	else
		return(0);
	return(device_type);
}  /* end of validateDevice() */

/*
    validateInquiry - given an uppercase string, determines if it matches
	any of the inquiry strings for the devices on the system.  A match
	is a simple prefix match on the full inquiry for the device.

	The function returns True when the inquiry passed in matches, and
	false otherwise.

	When a match is found, it returns the info needed to create a
	ptFileEntry: the inquiry string and the device type.
*/
static Boolean
validateInquiry(char * device, struct scsi_xedt * edt, int edt_cnt, int * device_type)
{
	int i;
	int len = strlen(device);

	*device_type = 0;
	for (i = 0; i < edt_cnt; i++)  {
		if (patternMatch(device, (char *) edt[i].xedt_tcinquiry) == 0) {	
			switch (edt[i].xedt_pdtype)  {
			case ID_RANDOM:
				*device_type |= PT_DISK;
				break;
			case ID_TAPE:
				*device_type |= PT_TAPE;
				break;
			case ID_WORM:
				*device_type |= PT_WORM;
				break;
			case ID_ROM:
				*device_type |= PT_CDROM;
				break;
			}
			return(True);
		}
	}

	return(*device_type != 0);
}  /* end of validateInquiry() */

/*
    ptCheckDevice - the device can be either "disk", "tape", "CDROM",
	or "WORM", OR a cNbNtNdN, OR part of an inquiry string.  This
	function validates the returns the info needed to create a
	ptFileEntry for the device: inquiry and device_type.
	
	The function returns True if the device is valid and False otherwise.
*/
Boolean
ptCheckDevice(char * device, struct scsi_xedt * edt, int num_edt, char ** inquiry_p, int * device_type)
{
	int i;

	/* Convert to upper case */
	for (i = 0; device[i]; i++)
		device[i] = (char)toupper(device[i]);
	
	/* Check if it is one of disk, tape, CDROM, or WORM */
	if (*device_type = validateDevice(device))  {
		*inquiry_p = NULL;
		return(True);
	}
	/* Check if device is of form cCbBtTdD */
	else if (ptValidateNodeDescriptor(device, edt, num_edt, inquiry_p, device_type, NULL))  {
		return(True);
	}
	/* Check if device is a prefix of the INQUIRY string */
	else if (validateInquiry(device, edt, num_edt, device_type)) {
		*inquiry_p = device;
		return(True);
	}
	else {
		*inquiry_p = NULL;
		*device_type = 0;
		return(False);
	}
}  /*  end of ptCheckDevice() */

/*
    ptCommandToInt - converts the ACSII SCSI command names to their
	integer values.  The second argument is the value of the
	length set by the -l option.  It returns an array of integer
	commands that the caller is responsible for freeing.

	The function returns True when it matchs a command and False
	otherwise.
*/
Boolean
ptCommandToInt(char * command, char * length, int device_type, Boolean isHex, int ** command_list, int * num_commands)
{
	int i;
	int len;
	int tmp_cmds[MAX_CMD_PREFIX];
	Boolean match = False;
	
	/* Convert command to upper case */
	for (i = 0; command[i]; i++)
		command[i] = (char)toupper(command[i]);

	len = strlen(command);
	*num_commands = 0;
	if (isHex)  {
		if (sscanf(command, "0X%x", &(tmp_cmds[*num_commands])) == 1) {
			if (tmp_cmds[*num_commands] >= 0 &&
				tmp_cmds[*num_commands] <= 256)
				*num_commands += 1;
		}
	}
	else  {
	for (i = 0; i < cmdTableLen; i++)  {
		/* Loop through table until the given command matchs the
		   prefix of a command */
		if (patternMatch(command, cmdTable[i].name) == 0)  {
			match = True;
			/* Make sure the device_type matches */
			if (!(cmdTable[i].device&device_type))
				continue;
			if (length[0])  {
				/*  If length is set, then the command may
					match "<command>_<length>"; it may not
					match because only some commands use
					the "_<length>"  */
				if (len < (int) strlen(cmdTable[i].name) &&
					cmdTable[i].name[len] == '_' &&
					strcmp((char *)&(cmdTable[i].name[len+1]),
						length) == 0)  {
					tmp_cmds[*num_commands] = cmdTable[i].command;
					*num_commands += 1;
					break; /* this is the only command */
				}
			}
			if (cmdTable[i].name[len] == '_') {
				int l;
				/* If it is "<command>_<length>" add to list */
				if (sscanf((char *)&(cmdTable[i].name[len+1]), "%d", &l) == 1 &&
					(l == 6 || l == 10 || l == 12))  {
					tmp_cmds[*num_commands] = cmdTable[i].command;
					*num_commands += 1;
				}
			}
			else if (len == strlen(cmdTable[i].name)) {
				/* Exact match - add to list. */
				tmp_cmds[*num_commands] = cmdTable[i].command;
				*num_commands += 1;
			}
		} else if (match) {
			/* This breaks the loop after all matches on the
				command. Assumes the list is sorted.  */
			break;
		}
	}
	} /* end else */

	if (*num_commands > 0)  {
		int * list;
		if (!(list = (int *) malloc(sizeof(int) * (*num_commands))))
			return(False);
		for (i = 0; i < *num_commands; i++)
			list[i] = tmp_cmds[i];
		*command_list = list;
	}

	return(*num_commands > 0);
}  /* end of ptCommandToInt() */

/*
    ptFreeFileEntry - walks the linked list of ptFileEntries and frees
	them.  Use this function to clean up the values returned from
	ptReadFile and ptInsertEntry.
*/
void
ptFreeFileEntry(ptFileEntry * file)
{
	ptFileEntry * tmp;

	while (file)  {
		tmp = file->next;
		if (file->inquiry)
			(void)free(file->inquiry);
		(void)free(file);
		file = tmp;
	}
}  /* end of ptFreeFileEntry() */


/*
    ptInsertEntry - given all the information for a PtFileEntry, this function
	creates a new entry and inserts it into the given list in the
	right order. It does not return a value, but it modifies the pointer
	to the list that is passed in.  The caller is responsible for 
	freeing the list.
*/
void
ptInsertEntry(ptFileEntry ** list, char * inquiry, int device_type, int command, unsigned short value)
{
	ptFileEntry * new_entry = (ptFileEntry *) malloc (sizeof(ptFileEntry));
	ptFileEntry * l = *list;
	ptFileEntry * last;
	int r = 1;

	if (!new_entry) return; /* malloc failed */
	if (inquiry)
		new_entry->inquiry = strdup(inquiry);
	else
		new_entry->inquiry = NULL;
	new_entry->device_type = device_type;
	new_entry->command = command;
	new_entry->value = value;
	new_entry->next = NULL;

	if (l == NULL)  {
		*list = new_entry;
		return;
	}

	if (compareEntry(new_entry, l) <= 0) {
		/* make it the start of the list */
		new_entry->next = l;
		*list = new_entry;
		return;
	}
	last = l;
	while (l && ((r = compareEntry(new_entry, l)) > 0))  {
		last = l;
		l = l->next;
	}
	if (l && !r)  {
		/* check for duplicate command and replace if needed */
		if (l->command == new_entry->command)  {
			last->next = new_entry;
			new_entry->next = l->next;
			if (l->inquiry) free(l->inquiry);
			free(l);
		}
		else  {
			last->next = new_entry;
			new_entry->next = l;
			return;
		}
	}
	else if (l)  {
		last->next = new_entry;
		new_entry->next = l;
		return;
	}
	else {  /* reached the end of the list */
		last->next = new_entry;
		/* new_entry->next is already NULL */
		return;
	}
}  /* end of ptInsertEntry() */

/*
    ptInsertEntryValue - This function changes the pditimetab file as
	represented by list.  It converts the inquiry and device_type
	information into specific fully specified inquiry strings for
	the devices that match.  Then it call ptInsertEntry to add them
	to the list.  */
void
ptInsertEntryValue(ptFileEntry ** list, char * inquiry, int device_type, int command, unsigned short value, struct scsi_xedt * edt, int edt_cnt)
{
	int i;

	/* If this is a device_type entry then add that to the file for
	   future devices of this type to get the value. */
	if (!inquiry)
		ptInsertEntry(list, inquiry, device_type, command, value);

	/* Look for edt entries that match exactly */
	for (i = 0; i < edt_cnt; i++)  {
		if (!(MATCH_TYPES(edt[i].xedt_pdtype, device_type)))
			continue;
		if (!inquiry ||
			(inquiry && patternMatch(inquiry, (char *)edt[i].xedt_tcinquiry) == 0))
			ptInsertEntry(list, (char *)edt[i].xedt_tcinquiry, device_type, command, value);
	}
} /* end of ptInsertEntryValue() */

/*
    ptIsCommand - validates a given command and length combination
	for a particular device_type.  It returns True if the command
	is a match in the cmdTable and false otherwise.
*/
Boolean
ptIsCommand(char * command, int device, char * length, Boolean isHex)
{
	char up_cmd[MAX_CMD_LEN];
	int i = 0;
	int len;
	int orig_len;
	Boolean match = False;

	/* convert to upper case */
	for (i = 0; command[i] && i < MAX_CMD_LEN; up_cmd[i] = (char)toupper(command[i]), i++);
	up_cmd[i] = (char) 0;
	orig_len = i;

	/* See if it is a hex command */
	if (isHex)  {
		int tmp_cmd;
		if (sscanf(command, "0X%x", &tmp_cmd) == 1)
			if (tmp_cmd >= 0 && tmp_cmd <= 256)
				return(True);
		return(False);
	}

	/* add the length to the upper case command (if needed) */
	if (length[0])  {
		up_cmd[i++] = '_';
		strncpy(&(up_cmd[i]), length, MAX_CMD_LEN - i);
	}
	len = strlen(up_cmd);

	/*  look for the command in the cmdTable */
	for (i = 0; i < cmdTableLen; i++)  {
		/* First look for the original length match */
		if (strncmp(cmdTable[i].name, up_cmd, orig_len) == 0)  {
			int l;
			match = True;

			/* all matches must have the same device_type */
			if (!cmdTable[i].device & device)
				continue;
			/* If the command is the same as the original
			   command (with or without length, it is a
			   match.  */
			if (orig_len == strlen(cmdTable[i].name))
				return(True);
			/*  If there is a length, see if it is an
			    exact match */
			if (len != orig_len &&
				strncmp(cmdTable[i].name, up_cmd, len) == 0)
				return(True);
			/*  If there isn't a length, match it with
			    commands that do have a length.  */
			if (len == orig_len && 
				sscanf((char *)&(cmdTable[i].name[len+1]), "%d", &l) == 1)
				return(True);
		}
		else if (match)
			break;
	}
	return(False);
}  /* end of ptIsCommand() */

/*
    ptPrintCommands - Prints to stdout the SCSI commands applicable to 
	the device passed as an argument.  The valid device types are:
	"disk", "tape", "cdrom", and "worm".
*/
Boolean
ptPrintCommands(char * device)
{
	int device_type;
	int i;

	/* Convert to upper case */
	for (i = 0; device[i]; i++)
		device[i] = (char)toupper(device[i]);

	if (!(device_type = validateDevice(device)))
		return(False);

	for (i = 0; i < cmdTableLen; i++)  {
		if (cmdTable[i].device & device_type)
			printf("%s\n", cmdTable[i].name);
	}
	return(True);
}  /* end of ptPrintCommands() */

/*
    ptPrintInfo -  is called when the user specifies the -q option on
	the pdi_timeout command line.  This prints to stdout, the
	current state of the timeout/reset feature, and then for each
	hba on the system it prints whether the hba support timeout/reset
	according to the return from the CAPABILITIES ioctl.
*/
void
ptPrintInfo(struct scsi_xedt * edt, int edt_cnt)
{
	int current = 2;
	int i;

	/* query the state of the PDI_TIMEOUT tunable */
	if (!sendTimeoutSupport(&current))
		pfmt(stdout, MM_NOSTD, MSG20); /* cannot be determined */
	else if (current == 1)
		pfmt(stdout, MM_NOSTD, MSG21); /* enabled */
	else if (current == 0)
		pfmt(stdout, MM_NOSTD, MSG22); /* disabled */
	else
		pfmt(stdout, MM_NOSTD, MSG20); /* cannot be determined */

	/* For each hba determine if timeout/reset is supported. */
	for (i = 0; i < edt_cnt; i++)  {
		if (edt[i].xedt_pdtype == ID_PROCESOR)  {
			int r = sendCapabilities(edt[i].xedt_ctl);
			if (r == 1)
				pfmt(stdout, MM_NOSTD, MSG23, edt[i].xedt_tcinquiry);
			else if (r == 0)
				pfmt(stdout, MM_NOSTD, MSG24, edt[i].xedt_tcinquiry);
			else
				pfmt(stdout, MM_NOSTD, MSG31, edt[i].xedt_tcinquiry);
		}
	}
}  /* end of ptPrintInfo() */

/*
    ptPrintQuery - given a list of queries and the current timeout table,
	this function prints a formated table of the query results.
	For each device that is queried, it prints the device type,
	the node descriptor, and the full inquiry string.  On successive
	lines it prints the command and timeout value.
*/
void
ptPrintQuery(ptFileEntry * query, ptTimeoutTable * table, struct scsi_xedt * edt, int edt_cnt)
{
	int i;
	Boolean match = False;

	if (!query || !table)
		return;

	while(query)  {
		if (query->inquiry == NULL)  {
			ptFileEntry * next = NULL;
			/* This is a generic query for all disks, tapes, or etc. */
			match = False;
			for (i = 0; i < edt_cnt; i++)  {
				if (MATCH_TYPES(edt[i].xedt_pdtype, query->device_type))  {
					/* This is a match - print Device info */
					match = True;
					printf("%s\tc%db%dt%dd%d\t%s\n", 
						DEVICE_TYPE_NAME(query->device_type),
						edt[i].xedt_ctl,
						edt[i].xedt_bus,
						edt[i].xedt_target,
						edt[i].xedt_lun,
						edt[i].xedt_tcinquiry);
					/* Print command value pairs */
					next = printValue(query, table, i);
				}
			}
			if (!match)
				pfmt(stderr, MM_ERROR, MSG25, DEVICE_TYPE_NAME(query->device_type));
			query = next;
			continue;
		}
		else  {
			ptFileEntry * next = NULL;
			/* Search edt for matching inquiry string */
			match = False;

			for (i = 0; i < edt_cnt; i++)  {
				if (query && patternMatch(query->inquiry, (char *)edt[i].xedt_tcinquiry) == 0)  {
					/* This is a match - print Device info */
					match = True;
					printf("%s\tc%db%dt%dd%d\t%s\n", 
						DEVICE_TYPE_NAME(query->device_type),
						edt[i].xedt_ctl,
						edt[i].xedt_bus,
						edt[i].xedt_target,
						edt[i].xedt_lun,
						edt[i].xedt_tcinquiry);
					/* Print command value pairs */
					next = printValue(query, table, i);
					continue;
				}
			}
			if (!match)
				pfmt(stderr, MM_ERROR, MSG26, query->inquiry);
			query = next;
		}
	}
}  /* end of ptPrintQuery() */


/*
    ptQueryAll - build a ptFileEntry list for all the devices and all
	the commands.
*/
void
ptQueryAll(ptFileEntry ** query, struct scsi_xedt * edt, int edt_cnt)
{
	int i;
	int dev = 0; /* flag used to insert only one query per device type */

	for (i = 0; i < edt_cnt; i++)  {
		switch (edt[i].xedt_pdtype)  {
		case ID_RANDOM:
			if (dev & PT_DISK)
				break;
			ptInsertEntry(query, NULL, PT_DISK, UNDEF, UNDEF);
			dev |= PT_DISK;
			break;
		case ID_TAPE:
			if (dev & PT_TAPE)
				break;
			ptInsertEntry(query, NULL, PT_TAPE, UNDEF, UNDEF);
			dev |= PT_TAPE;
			break;
		case ID_WORM:
			if (dev & PT_WORM)
				break;
			ptInsertEntry(query, NULL, PT_WORM, UNDEF, UNDEF);
			dev |= PT_WORM;
			break;
		case ID_ROM:
			if (dev & PT_CDROM)
				break;
			ptInsertEntry(query, NULL, PT_CDROM, UNDEF, UNDEF);
			dev |= PT_CDROM;
			break;
		}
	}
}  /* end of ptQueryAll() */

/*
    ptReadEdt - this is an external version of the function readxedt
	used by many of the pdi commands.
*/
struct scsi_xedt *
ptReadEdt(int * edt_cnt)
{
	return(readxedt(edt_cnt));
}  /* end of ptReadEdt() */

/*
    ptReadFile - reads the given file and creates a
	timeout table and ptFileEntry list that corresponds.

	It returns NULL if an error occurred, and a pointer to the
	ptFileEntry list otherwise.  This needs to be freed by calling
	ptFreeFileEntry(). The timetab also must be freed by the caller.
*/
ptFileEntry *
ptReadFile(char * file_name, struct scsi_xedt *edt, int num_edt, ptTimeoutTable ** timetab)
{
	int i, j;
	FILE * ptfile;
	char	device[256];
	int command;
	int read_value;
	unsigned short value;
	ptFileEntry * start_file;
	ptFileEntry * file;
	int device_type;
	ptTimeoutTable * tab;

	if (edt == NULL || num_edt < 1)  {
		pfmt(stderr, MM_ERROR, MSG27);
		return(NULL);
	}

	/* Allocate ptTimeoutTable for each entry in the edt */
	tab = (ptTimeoutTable *) calloc(num_edt, sizeof(ptTimeoutTable));
	*timetab = tab;
	if (!tab)
		return(NULL);

	/* Fill in the scsi_adr and length */
	for (i = 0; i < num_edt; i++)  {
		tab[i].sap.scsi_ctl = edt[i].xedt_ctl;
		tab[i].sap.scsi_bus = edt[i].xedt_bus;
		tab[i].sap.scsi_target = edt[i].xedt_target;
		tab[i].sap.scsi_lun = edt[i].xedt_lun;

		tab[i].length = 256;
	}

	/* Open the file */
	if ((ptfile = fopen(file_name, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, MSG17, file_name);
		*timetab = NULL;
		free(tab);
		return(NULL);
	}

	start_file = file = (ptFileEntry *)NULL;

	/* Read each line and set the appropriate command value */
	for (;;)  {
		char line[256];
		char * next_tok;
		if (fgets(line, 256, ptfile) == NULL)
			break;
	
		if ((next_tok = strtok(line,":")) == NULL)
			break;
		strncpy(device, next_tok, 256);
		if ((next_tok = strtok(NULL, ":")) == NULL)
			break;
		command = (int) strtol(next_tok, NULL, 16);
		if ((next_tok = strtok(NULL, ":")) == NULL)
			break;
		read_value = (int) atoi(next_tok);
		value = (unsigned short) read_value;

		if (!start_file)  {
			if (!(start_file = file = (ptFileEntry *)calloc(1, sizeof(ptFileEntry))))  {
				free(tab);
				goto err_return;
			}
		}
		else  {
			if (!(file->next = (ptFileEntry *) calloc(1, sizeof(ptFileEntry))))  {
				free(tab);
				goto err_return;
			}
			file = file->next;
		}
		file->value = value;
			
		/* Validate command */
		if (command < 0 || command > 256)  {
			pfmt(stderr, MM_ERROR, MSG29, command);
			(void)free(tab);
			fclose(ptfile);
			*timetab = NULL;
			ptFreeFileEntry(start_file);
			return(NULL);
		}
		file->command = command;

		/* Convert device to upper case */
		for (j = 0; device[j]; j++)
			device[j] = (char)toupper(device[j]);
		if (strcmp(device, "*") == 0)  {
			file->inquiry = NULL;
			file->device_type = 0;
			/* A generic entry - put in every table */
			for (i = 0; i < num_edt; i++)
				tab[i].table[command] = value;
		}
		else if ((device_type = validateDevice(device)) != 0)  {
			file->inquiry = NULL;
			file->device_type = device_type;
			/* Enter device type values for matching device types */
			for (i = 0; i < num_edt; i++)  {
				if (MATCH_TYPES(edt[i].xedt_pdtype, device_type)) 
					tab[i].table[command] = value;
			}
		}
		else  {
			/* Must be INQUIRY string - enter value for matches */
			if (device)
				file->inquiry = strdup(device);
			else
				file->inquiry = NULL;
			file->device_type = NULL;
			for (i = 0; i < num_edt; i++)
				if (patternMatch(device, (char *)edt[i].xedt_tcinquiry) == 0)  {
					tab[i].table[command] = value;
					file->device_type |= XTYPE_TO_PT(edt[i].xedt_pdtype);
					
				}
		}
	}
err_return:
	fclose(ptfile);
	return(start_file);
}  /* end of ptReadFile() */

/*
    ptResetToFactory - is used to reset all or some of the timeout values to
	the defaults specified when the system was shipped.  When the -s
	option is not used, the user must answer an "are you sure?" question.

	Resetting all of the fields works by reading the pditimetab.orig
	file, and then updating the sdi tables with those values, and
	finally calling the function to write the file out passing it the
	original FileEntry list instead of the one read from pditimetab.

	Resetting a particular device is tricky.  The problem is that
	there can be inquiry and device type entries in pditimetab that
	influence that device.  The method is to search the list of 
	FileEntries from pditimetab and remove all that contribute to the
	times for the given device.  Then we search the FileEntries from
	the pditimetab.orig file for any entries that contribute to the
	times for the give device, and insert them into the current
	FileEntries list.

	This method has an unavoidable side effect.  Say there are entries
	for DISK INQUIRY and CONNER TEST_UNIT_READY in the current file.
	Restoring devices of type CONNER will have to remove the DISK INQUIRY
	entry from the current file.  So, the side effect is that other
	disks will have the original inquiry value too.
*/
Boolean
ptResetToFactory(Boolean silent, ptFileEntry * entry, ptFileEntry ** cur_list, struct scsi_xedt * edt, int edt_cnt, ptTimeoutTable * table_p)
{
	ptFileEntry * orig_list;
	ptTimeoutTable * orig_table;
	int rc = True;
	Boolean use_orig = (entry == NULL);
	int i;

	orig_list = ptReadFile(PDITIMETAB_ORIG, edt, edt_cnt, &orig_table);

	if (!orig_list)
		return(False);  /* ptReadFile cleans up on error */

	while (entry)  {

		if (!entry->inquiry) {
		/* Remove all matching inquiries and device_type entries
		   from the current list-
		   if there is no inquiry in the entry, then remove all
		   the inquiries from the current list. */
			findMatch(cur_list, NULL, DEL, entry);
		/* Look in the original file for matching inquiries or
		   device_type entries.
		   If there are any, insert them into the current list.  If
		   there is no inquiry, then insert all matching inquiries
		   from the original that match the device type.  */
			findMatch(&orig_list, cur_list, ADD, entry);
		}
		else  {
			for (i = 0; i < edt_cnt; i++)  {
				if (MATCH_TYPES(edt[i].xedt_pdtype, entry->device_type) &&
					patternMatch(entry->inquiry, (char *)edt[i].xedt_tcinquiry) == 0)  {
					if (entry->command != UNDEF)
						ptInsertEntry(cur_list,
							(char *)edt[i].xedt_tcinquiry,
							entry->device_type,
							entry->command,
							orig_table[i].table[entry->command]);
					else  {
						int j;
						for (j=0;j<cmdTableLen;j++)  {
							if (!(cmdTable[j].device& entry->device_type))
								continue;
							ptInsertEntry(cur_list,
								(char *)edt[i].xedt_tcinquiry,
								entry->device_type,
								cmdTable[j].command,
								orig_table[i].table[entry->command]);
						}
					}
				}
			}
		}

		entry = entry->next;
	}
	if (!silent)  {
		pfmt(stdout, MM_NOSTD, MSG30, nl_langinfo(YESSTR), nl_langinfo(NOSTR));
		if (!ptYesP())  {
			rc = False;
			goto rc_return;
		}
	}
	/* Copy pditimetab.orig to pditimetab. */
	if (use_orig)  {
		if (!ptWriteFile(orig_list))
			rc = False;
		ptFreeFileEntry(*cur_list);
		*cur_list = orig_list;
		rc = ptSendTable(orig_table, edt_cnt);
		if (orig_table) free(orig_table);
		return(rc);
	}
	else  {
		if (!ptWriteFile(*cur_list))  {
			rc = False;
			goto rc_return;
		}
		ptUpdateTable(table_p, *cur_list, edt, edt_cnt);
		rc = ptSendTable(table_p, edt_cnt);
	}
rc_return:
	ptFreeFileEntry(orig_list);
	if (orig_table) free (orig_table);
	return (rc);
}  /* end of ptResetToFactory() */

/*
    ptSendTable - given a timeout table, it will send the timeout values
	for num_table entries to sdi.  It returns False if the values
	cannot be sent for some reason.
*/
Boolean
ptSendTable(ptTimeoutTable * table, int num_table)
{
	int i;
	char 	sditempnode[]="/tmp/scsiXXXXXX";
	int	sdi_fd;

	if ((sdi_fd = openSDI(sditempnode)) < 0)
		return(False);
		

	for (i = 0; i < num_table; i++)  {
		if (table[i].length)  { /* only disk, tape, ... have length */
			if (!sendNewTimeoutValues(sdi_fd, &(table[i])))
				return(False);
		}
	}

	if (!closeSDI(sdi_fd, sditempnode)) return(False);
	return(True);
}  /* end of ptSendTable() */

/*
    ptSetTimeout - Disables or enables the timeout and reset feature by
	sending an ioctl to sdi and changing the value of the
	PDI_TIMEOUT tunable.
*/
Boolean
ptSetTimeout(int on_off)
{
	char cmd[] = IDTUNE_CMD;
	int query = 2;
	int status;
	/*  First check if the state will actually change and return if not */
	if (sendTimeoutSupport(&query)) {
		if (query == on_off)
			return(True);
	}
	else
		return(False);

	/*  Now set the state to the new value, and do the idtune and idbuild */
	if (sendTimeoutSupport(&on_off)) {
		if (on_off)
			cmd[strlen(cmd)-1]= '1';
		if ((status = system(cmd)) != 0)
			return(False);
		else  {
			if (!(WIFEXITED(status)))
				return(False);
			if (WEXITSTATUS(status) != 0)
				return(False);
		}
		if ((status = system(IDBUILD_CMD)) != 0)
			return(False);
		else  {
			if (!(WIFEXITED(status)))
				return(False);
			if (WEXITSTATUS(status) != 0)
				return(False);
		}
	}
	else
		return(False);
	return(True);
}  /* end of ptSetTimeout() */

/*
    ptUpdateTable - changes the timeout table to reflect the current 
	ptFileEntry list.  It recreates the timeout values from scratch
	into the existing ptTimeoutTable structure.  New entries that
	were inserted into the ptFileEntry list will be reflected in
	this recreation of the timeout values.
*/
void
ptUpdateTable(ptTimeoutTable * pt, ptFileEntry * file, struct scsi_xedt * edt, int edt_cnt)
{
	int i;

	/* Zero out the current table so we start from scratch */
	for (i = 0; i < edt_cnt; i++)  {
		int j;
		for (j = 0; j < pt[i].length; j++)
			pt[i].table[j] = 0;
	}

	/* Go through the sorted file_entries and enter values in the tables */
	while (file)  {
		if (file->inquiry == NULL && file->device_type == NULL)  {
			/* Generic entry - apply to all tables */
			for (i = 0; i < edt_cnt; i++)  {
				pt[i].table[file->command] = file->value;
			}
		}
		else if (file->device_type)  {
			/* Device specific entry - apply to matching devices */
			for (i = 0; i < edt_cnt; i++)  {
				if (MATCH_TYPES(edt[i].xedt_pdtype, file->device_type))
					pt[i].table[file->command] = file->value;
			}
		}
		else if (file->inquiry)  {
			/* Apply to matching inquiry strings */
			for (i = 0; i < edt_cnt; i++)  {
				if (patternMatch(file->inquiry, (char *)edt[i].xedt_tcinquiry) == 0)
					pt[i].table[file->command] = file->value;
			}
		}
		file = file->next;
	}
} /* end of ptUpdateTable() */

/*
    ptValidateNodeDescriptor - this function is called from ptCheckDevice
	and from the main pdi_timeout command to verify the node descriptor
	passed to the -b option.

	It returns True when the device name passed in matches a node
	descriptor in the edt.  It returns the information needed to
	build a ptFileEntry structure for the device: inquiry and
	device type.  It also returns the index into the edt where the
	match was found.
*/
Boolean
ptValidateNodeDescriptor(char * device, struct scsi_xedt * edt, int edt_cnt, char ** inquiry_p, int * device_type, int * edt_index)
{
	int i;
	int ctl, bus, target, lun;

	if (sscanf(device, "C%dB%dT%dD%d", &ctl, &bus, &target, &lun) != 4) {
		goto err_return;
	}
	for (i = 0; i < edt_cnt; i++)  {
		if (edt[i].xedt_ctl == ctl && edt[i].xedt_bus == bus &&
			edt[i].xedt_target == target && edt[i].xedt_lun == lun)  {
			/* Found the matching device */
			if (inquiry_p) *inquiry_p = (char *)edt[i].xedt_tcinquiry;
			if (device_type)
				switch (edt[i].xedt_pdtype)  {
				case ID_RANDOM:
					*device_type = PT_DISK;
					break;
				case ID_TAPE:
					*device_type = PT_TAPE;
					break;
				case ID_WORM:
					*device_type = PT_WORM;
					break;
				case ID_ROM:
					*device_type = PT_CDROM;
					break;
				}
			if (edt_index) *edt_index = i;
			return(True);
		}
	}
err_return:
	/* Didn't find the matching device */
	if (device_type) *device_type = 0;
	if (inquiry_p) *inquiry_p = NULL;
	if (edt_index) *edt_index = 0;
	return(False);
}  /* end of ptValidateNodeDescriptor() */

/*
    ptWriteFile - given a list of ptFileEntries, this function rewrites
	the /etc/scsi/pditimetab file with the new data.

	It returns True if it sucedes and False if it fails for some
	reason.
*/
Boolean
ptWriteFile(ptFileEntry * file)
{
	FILE * ptfile;

	if ((ptfile = fopen(PDITIMETAB, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, MSG18, PDITIMETAB);
		return(False);
	}

	while (file)  {
		int dt = file->device_type ? file->device_type : PT_ALL;
		fprintf(ptfile, "%s:0x%x:%d:# %s\n",
			((!file->device_type && !file->inquiry) ? "*" :
				((file->device_type && !file->inquiry) ?
					(DEVICE_TYPE_NAME(dt)) :
				file->inquiry)),
			file->command,
			file->value,
			intToCommand(file->command, dt));
		file= file->next;
	}
	fclose(ptfile);
	return(True);
}  /* end of ptWriteFile() */

/*
    ptYesP - reads standard in to determine if the answer to a question
	is yes or no (in the correct I18N manner). It uses the nl_langinfo
	YESEXPR to match the input.  If it matches, it returns True, otherwise
	it returns False.

	Functions that write out the yes/no question should use
	nl_langinfo YESSTR and NOSTR to pose the choices.
*/
Boolean
ptYesP(void)
{
	char answer[MAX_INPUT];
	Boolean r;
	int err, len;
	regex_t re;

	
	answer[0] = NULL;
	(void) fgets(answer, MAX_INPUT, stdin);
	len = strlen(answer);
	if (len && answer[len-1] == '\n')
		answer[--len] = '\0';
	if (!len)
		return(False);
	err = regcomp(&re, nl_langinfo(YESEXPR), REG_EXTENDED|REG_NOSUB);
	if (err != 0)  {
		regerror(err, &re, answer, MAX_INPUT);
		pfmt(stderr, MM_ERROR, MSG40, answer);
		return(False);
	}
	r = (Boolean) regexec(&re, answer, (size_t) 0, NULL, 0);
	regfree(&re);
	return(!r);
}  /* end of ptYesP() */
