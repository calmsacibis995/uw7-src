#ifndef	NOIDENT

#ident	"@(#)dtadmin:dialup/getdevice.c	1.23.1.21"
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLook.h>
#include <libDtI/DtI.h>
#include "uucp.h"
#include "error.h"

extern int FindAliasTable(char *);
static char longPortNumber[100];
typedef struct _ResetEntry {
	String longport;
	String port;
	String dash;
	String speed;
	String modem_cmd;
	String restofline;
	struct _ResetEntry * next;
} ResetEntry;

typedef struct _ResetData {
	int count;
	ResetEntry * reset_entry;
} ResetData;

static ResetData * resetTable;
static ResetEntry * getResetEntry(ResetData *, char *, char *, char *);
static ResetEntry * addResetEntryToTable(ResetData * );
static void freeResetEntryItems(ResetEntry *);
static void deleteResetEntry(ResetData *table, ResetEntry *);
extern void		CheckMode();
extern Boolean		getline();
extern	DeviceItems 	enabled[];
extern	DeviceItems 	directions[];

void FreeFileClass();
static void UpdateResetItems(char *,char *, int );
void FreeObject();
void FreeDeviceData();
void PutContainerItems();

static		exists;		/* Non-zero if file exists */
static		readable;	/* Non-zero if file is readable */
static		writeable;	/* Non-zero if file is writable or */
				/* directory is writable and file */
				/* doesn't exist */

static char	*lbuf[3] = { 0, 0, 0};		/* Buffer for comments and none support types */

DmFclassPtr	acu_fcp, dir_fcp;

void
AddObjectToContainer(op)
DmObjectPtr op;
{
	/* add it to the end of list */
	if (df->cp->op == NULL)
		df->cp->op = op;
	else {
		DmObjectPtr endp;

		for (endp=df->cp->op; endp->next; endp=endp->next) ;
		endp->next = op;
	}
	df->cp->num_objs++;
	SetDevSensitivity();
}

void
DeleteContainerItems()
{
	register DmObjectPtr op = df->cp->op;

#ifdef TRACE
	fprintf(stderr,"DeleteContainerItems\n");
#endif
	for (; op && op->next; op=op->next)
		FreeObject (op);
	FreeFileClass (acu_fcp);
	FreeFileClass (dir_fcp);
	free (df->itp);
	if (df->select_op)
		free (df->select_op);
	free (df->cp);
	SetDevSensitivity();
}

void
DelObjectFromContainer(target_op)
DmObjectPtr target_op;
{
	DeviceData *tap;
	register DmObjectPtr op = df->cp->op;
#ifdef DEBUG
	fprintf(stderr,"DelObjectFromContainer\n");
#endif
#ifdef TRACE
	fprintf(stderr,"DelObjectFromContainer\n");
#endif

	if (op == target_op) {
		df->cp->op = op->next;
		df->cp->num_objs--;
	}
	else
		for (; op->next; op=op->next)
			if (op->next == target_op) {
				op->next = target_op->next;
				df->cp->num_objs--;
				break;
			}
	
	tap = target_op->objectdata;
#ifdef DEBUG
fprintf(stderr,"delete with portNumber=%s\n",tap->portNumber);
fprintf(stderr,"delete with portDirection=%s\n",tap->portDirection);
#endif
	if ((strcmp(tap->portDirection, "outgoing")) != 0)	
		Remove_ttyService(tap->portNumber);
	FreeObject(target_op);
	SetDevSensitivity();
}

DmFclassPtr
new_fileclass(type)
char *type;
{
	static char *iconpath;
        DmFclassPtr fcp;
	char icon[128];

        if ((fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec))) == NULL)
                return(NULL);

	if (!strcmp(type, ACU))
		sprintf(icon, "%s", ACU_ICON);
	else
		sprintf(icon, "%s", DIR_ICON);
	fcp->glyph = DmGetPixmap(SCREEN, icon);
        return(fcp);
}

extern DmObjectPtr
new_object(name,  pDeviceData)
char *name;
DeviceData  *pDeviceData;
{
        DmObjectPtr op, objp;
	Dimension width;
	static x, y;
	DeviceData * tap;

	if (df->cp->op != NULL) { /* already allocated */
		for (objp=df->cp->op; objp; objp=objp->next)
			/* check objp to ensure it isn't null */
			if (objp && !strcmp(objp->name, name)) {
				/* duplicate */
				tap = (DeviceData *)objp->objectdata;
				free(tap->portSpeed);
				free(tap->holdPortSpeed);
				tap->portSpeed = strdup("Any");
				tap->holdPortSpeed = strdup("Any");
				return((DmObjectPtr)OL_NO_ITEM);
			}
	}

        if (!(op = (DmObjectPtr)calloc(1, sizeof(DmObjectRec))))
                return(NULL);

        op->container = df->cp;
        if (name)
                op->name = strdup(name);

	op->x = op->y = UNSPECIFIED_POS;
	op->objectdata = pDeviceData;
        return(op);
} /* new_object */

void
new_container(path)
char *path;
{
        if ((df->cp = (DmContainerPtr)calloc(1, sizeof(DmContainerRec))) == NULL) {
#ifdef debug
                fprintf(stderr,"new_container: can not allocate memory\n");
#endif
		perror("calloc");
                return;
        }

        df->cp->path = strdup(path);
        df->cp->count = 1;
        df->cp->num_objs = 0;
}

void
GetContainerItems(path, filetype)
char *path;
int filetype;
{
	DeviceData  *tmp;
	DmObjectPtr	op;
	extern DmFclassPtr	acu_fcp, dir_fcp;
	extern DmObjectPtr new_object();
	struct stat	statbuf;
	int		na;
	char		*dev[D_ARG+2], buf[BUFSIZ];
	char		port[100];
	char		*ptr;
	char		name[100];

	ResetEntry 	*resetPtr;
	char		text[BUFSIZ];
	char		*convert();
	char		*s = NULL;
	int		enable_value;
	char 		*speed;
	static		Boolean resetTable_allocated = False;
	int		direction;
	static Boolean	first_time = True;
	FILE *fdevice, *fopen();
	static unsigned	stat_flags = 0;	/* Mask returned by CheckMode() */

	/* read the Devices file here */

#ifdef TRACE
	fprintf(stderr,"GetContainerItems filetype=%d\n",filetype);
#endif
	CheckMode(path, &stat_flags);	/* get the file mode	*/
	exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
	readable = BIT_IS_SET(stat_flags, FILE_READABLE);
	writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
#ifdef debug
	fprintf(stderr, "/etc/uucp/Devices exists = %d, readable = %d, writeable = %d\n",
			exists, readable, writeable);
#endif
	if (!exists & !writeable) { /* somthing's serious wrong */
#ifdef debug
		fprintf(stderr, GGT(string_createFail), path);
		exit(1);
#endif
		sprintf(text, GGT(string_noFile), path);
		rexit(1, text, "");
	} else
	if (!readable & exists) {
#ifdef debug
		fprintf(stderr, GGT(string_accessFail), path);
		exit(2);
#endif
		sprintf(text, GGT(string_accessFail), path);
		rexit(2, text, "");
	} else
	if (exists & readable & !writeable) {
#ifdef debug
		fprintf(stderr, GGT(string_writeFail), path);
#endif
	}
	acu_fcp = new_fileclass(ACU);
	dir_fcp = new_fileclass(DIR);

	/* first time thru allocate container */
	if (first_time == True ) {
		first_time = False;
		new_container(path);
	}

	if ((fdevice = fopen(path, "r")) == NULL) {
		/* it's ok not able to open it since we knew that
		 * the file may not exist and we have the write privilege
		 * anyway.
		 */
#ifdef debug
		fprintf(stderr, GGT(string_openFail), path);
#endif
		;
	} else {
		stat(path, &statbuf);
			/*create lbuf for each filetype 
			 to be 1 greater than size of the file. 
			lbuf will hold
			the comments and the unrecognized lines in the
			file that the user might have put by editing
			the file. We need to preserve these lines */
		lbuf[filetype] = malloc (statbuf.st_size + 1);
		lbuf[filetype][0] = '\0';
		if ((filetype == DEVICES_OUTGOING) &&
			(resetTable_allocated == False)) {
				resetTable_allocated  = True;
				resetTable = (ResetData *) malloc(sizeof(ResetData));
				resetTable->count = 0;
				resetTable->reset_entry = 0;
		}
		while (getline(buf, BUFSIZ - 1, fdevice, filetype)) {
                    na = getargs(buf, dev, D_ARG, filetype);
		    if ( na < D_ARG ) continue;
			/* now check for Reset lines, these get
			saved in reset structures */
			
                    if ( strcmp(dev[D_TYPE], "Reset") == 0) {
			/* save the reset lines and continue */ 
			/* the reset lines do not get saved in lbuf */
			resetPtr = (ResetEntry *) addResetEntryToTable(resetTable);
			resetPtr->longport = strdup(dev[1]);

		    	if ((s = strstr(dev[1], ",")) != NULL) {
                            /* strips off trailing ",?" */
                            *s = '\0';
		    	}
			ptr =  (char *)IsolateName(dev[1]);
			strcpy(port, ptr);
			resetPtr->port = strdup(port);
#ifdef DEBUG
fprintf(stderr,"port=%s longport=%s\n",resetPtr->port, resetPtr->longport);
#endif
			resetPtr->dash = strdup(dev[2]);
			resetPtr->speed = strdup(dev[3]);
			resetPtr->modem_cmd  = strdup(dev[4]);
			resetPtr->restofline  = strdup(dev[5]);
			continue;
		}
                    if ( strcmp(dev[D_TYPE], ACU) != 0 &&
			 strcmp(dev[D_TYPE], DIR) != 0 &&
			 strcmp(dev[D_TYPE], DK ) != 0 ) {
			    sprintf (text,"%s %s %s %s %s %s\n",
					dev[0],
					dev[1],
					dev[2],
					dev[3],
					dev[4],
					dev[5]);
			    strcat (lbuf[filetype], text);
			    continue;
		    }
		    if ((s = strstr(dev[D_LINE], ",")) != NULL) {
                            /* strips off trailing ",?" */
                            *s = '\0';
		    }
		    if (!strcmp(dev[D_LINE], "/dev/tty00h") ||
			!strcmp(dev[D_LINE], "/dev/term/00h") ||
			!strcmp(dev[D_LINE], "term/00h") ||
			!strcmp(dev[D_LINE], "00h") ||
			!strcmp(dev[D_LINE], "tty00h"))
			    strcpy(port, COM1);
		    else if (!strcmp(dev[D_LINE], "/dev/tty01h") ||
			     !strcmp(dev[D_LINE], "/dev/term/01h") ||
			     !strcmp(dev[D_LINE], "term/01h") ||
			     !strcmp(dev[D_LINE], "01h") ||
			     !strcmp(dev[D_LINE], "tty01h"))
			    strcpy(port, COM2);
		    else
			    strcpy(port, dev[D_LINE]);
		
		    strcpy(name, port);
		    tmp = (DeviceData *) calloc(1, sizeof(DeviceData));
        	    if (tmp == NULL) {
#ifdef debug
               		    fprintf(stderr,
			    "GetContainerItems: couldn't calloc an DeviceData\n");
#endif
			    perror("calloc");
			    exit(2);
        	    }

		    if ( na > D_ARG ) {
			    tmp->DTP = strdup(dev[D_ARG]);
		    } else
			    tmp->DTP = strdup("");
		    tmp->holdModemFamily = tmp->modemFamily =strdup(dev[D_CALLER]);
		    if (strcmp(dev[D_CALLER], "direct") == 0) {
			tmp->holdModemFamily = tmp->modemFamily =strdup("uudirect");
			} else {
			tmp->holdModemFamily = tmp->modemFamily =strdup(dev[D_CALLER]);
			}
#ifdef DEBUG
			fprintf(stderr,"GetContainerItems: calling Get_ttymonPortDirection\n");
#endif
		    direction = Get_ttymonPortDirection(port);
		    tmp->holdPortDirection = tmp->portDirection = strdup(directions[direction].value);

#ifdef DEBUG
fprintf(stderr,"TTYMON port direction =%s for port=%s\n", tmp->holdPortDirection, port);
			
#endif
		    if (direction == 1) {
			/* direction 1 is outgoing so set speed to 9600 */
			tmp->holdPortSpeed = tmp->portSpeed = strdup("Any");
		    } else {
			speed = (char *) Get_ttymonPortSpeed(port);
			/* if incoming or bidirectional get the speed for
			the port from the ttymon. If auto is returned
			set the speed to Any */
			if ((speed) && (strcmp(speed, "auto") == 0)) {
				tmp->holdPortSpeed = tmp->portSpeed = strdup("Any");
			} else 
		    	if (speed) {
				tmp->holdPortSpeed = tmp->portSpeed = strdup(speed);
			} else {
				tmp->holdPortSpeed = tmp->portSpeed = strdup("9600");
				}
		

			}
#ifdef DEBUG
fprintf(stderr,"TTYMON port speed =%s for port=%s\n", tmp->holdPortSpeed, port);
#endif
		
			

		    if (filetype == DEVICES_DISABLED)  {
			tmp->holdPortEnabled = tmp->portEnabled = strdup("disabled");
			} else {
			tmp->holdPortEnabled = tmp->portEnabled = strdup("enabled");
			}

		    tmp->holdPortNumber = tmp->portNumber =strdup(name);
		

		    if ((op = new_object(name, tmp)) == (DmObjectPtr)OL_NO_ITEM) {
			continue;
		    }
                    if ( strcmp(dev[D_TYPE], ACU) == 0 )
			    op->fcp = acu_fcp;
		    else /* Direct */
			    op->fcp = dir_fcp;
		    AddObjectToContainer(op);
		}
        }
	if (fdevice != NULL) fclose(fdevice);
} /* GetContainerItems */

void
PutContainerItems(filename, filetype, port, oldport, action)
char* filename;
int filetype;
char *port;
char *oldport;
int action;
{
	FILE *fd;
	register DmObjectPtr op = df->cp->op;
	int	portNumber;
	int	i, nlines;
	DeviceData * tap;
	ResetEntry * entry;
	char* buf;
	char text [128];
	char type [64], line [64], dialer [64];
	char *lineptr[MAXLINE];

#ifdef DEBUG
	fprintf(stderr,"PutContainerItems: filename=%s filetype=%d port=%s oldport=%s actioj=%d\n",filename, filetype, port,oldport,action);
#endif
	nlines = 0;
	if ( df->cp->num_objs != 0 ) {
		for (op = df->cp->op; op; op = op->next) {
			if(nlines >= MAXLINE)
				break;
			tap = op->objectdata;
			/* process incoming enabled devices first */
			/* followed by outgoing & bidirectional devices */
			/* last process all disabled devices */
			/* outgoing & bidirectional enabled devices
				are found in /etc/uucp/Devices */
			/* incoming enabled are found in 
				/etc/uucp/Devices.incoming */
			/* ALL disabled devices get written to
				/etc/uucp/Devices.disabled */
			

			switch (filetype) {

			case DEVICES_INCOMING:
		
				/* skip disabled devices, they
				go in Devices.disabled */

			if (strcmp(tap->portEnabled, "disabled") == 0)
				continue;
				/* skip outgoing & bi-directional
				devices, they go in Devices file */

			if (strcmp(tap->portDirection, "incoming") != 0)
				continue;

			break; 		/* we found one to process */

			case DEVICES_DISABLED:

				/* skip enabled devices */

			if (strcmp(tap->portEnabled, "disabled") != 0)
				continue;

			break; 		/* we found one to process */

			case DEVICES_OUTGOING:

				/* skip disabled devices */
			if (strcmp(tap->portEnabled, "disabled") == 0)
				continue;
				/* skip incoming devices */
			if (strcmp(tap->portDirection, "incoming") == 0)
				continue;
			
			}

			if (strcmp(tap->modemFamily,"uudirect") == 0)  {
				strcpy(type, "Direct");
				strcpy(dialer, "uudirect");
			} else {
				strcpy(type, "ACU");
				strcpy(dialer, tap->modemFamily);
			}
			if (!strncmp(tap->portNumber, "com", 3)) {
				sscanf(tap->portNumber, "com%d", &portNumber);
				sprintf(line, "/dev/tty%.2dh", portNumber-1);
			} else	sprintf(line, "%s", tap->portNumber);
			/* stick a ",M" at the end of the device name if */
			/* the TP says "NO" */
			if (no_tp_wanted(line)) {
				sprintf(line, "%s,M", line);
			}

			if (strcmp(tap->portDirection, "incoming") == 0) {
				sprintf (text,
				"%s %s %s %s %s %s\n",
				type,
				line,
				"-",
				tap->portSpeed,
				dialer,
				tap->DTP);
			} else {
			sprintf (text,
				"%s %s %s %s %s %s\n",
				type,
				line,
				"-",
				"Any",
				dialer,
				tap->DTP);
			}
			buf = malloc(strlen(text) +1 );
			strcpy (buf, text);
			lineptr[nlines++] = buf;
		}
	}
	if ((fd = fopen(filename, "w")) == NULL) {
		sprintf (text, GGT(string_fopenWrite), filename);
		XtVaSetValues (df->footer, XtNstring, text, (String)0);
		return;
	}
		/* if comments or unregognized lines exist then
			write them out first */
	if (lbuf[filetype]) fprintf (fd, "%s", lbuf[filetype]);
		/* write out saved lines */
	for (i=0; i < nlines; i++) {
		fprintf (fd, "%s", lineptr[i]);
		free(lineptr[i]);
	}
		/* if we are processing the /etc/uucp/Devices file */
	if (filetype == DEVICES_OUTGOING) {

		/* update the reset items table */
		/* port is the current port, oldport is the
			previous value for the port, action
			indicates if an ADD, DELETE or MODIFY
			is being done */
		UpdateResetItems(port, oldport, action);

		/* write out reset items to /etc/uucp/Devices file */
		for (i=0,entry=resetTable->reset_entry; i < resetTable->count; i++, entry=entry->next) {
			if (entry->port) {
				fprintf (fd,
					"%s %s %s %s %s %s\n",
					"Reset",
					entry->longport,
					entry->dash,
					entry->speed,
					entry->modem_cmd,
					entry->restofline);
			}	
		}
	}

	XtVaSetValues (df->footer, XtNstring, GGT(string_saved), (String)0);
	fclose (fd);
	if (!exists) {
		chmod(filename, (mode_t) 0644);
		chown(filename, UUCPUID, UUCPGID);
		exists = 1;
	}
	SetDevSensitivity();
} /* PutContainerItems */

int
getargs(s, arps, count, filetype)
register char *s, *arps[];
register int count;
int filetype;
{
	char	buf[BUFSIZ];
        register int i;

	strcpy(buf, s);
        for (i = 0; /*TRUE*/ ;i++) {
                while (*s == ' ' || *s == '\t')
                        *s++ = '\0';
                if (*s == '\n')
                        *s = '\0';
                if (*s == '\0')
                        break;
                arps[i] = s++;
		if (i == count) {
			while (*s != '\0' && *s != '\n')
				s++;
			if(*s == '\n')
				*s = '\0';
			i++;
			break;
		}
                while (*s != '\0' && *s != ' '
                        && *s != '\t' && *s != '\n')
                                s++;
        }
	if (i < count)
		strcat(lbuf[filetype], buf);
        arps[i] = NULL;
        return(i);
}

void
FreeDeviceData(tap)
DeviceData * tap;
{
#ifdef TRACE
	fprintf(stderr,"FreeDeviceData\n");
#endif
	free(tap->modemFamily);
	free(tap->holdModemFamily);
	free(tap->portSpeed);
	free(tap->holdPortSpeed);
	free(tap->portNumber);
	free(tap->holdPortNumber);
	free(tap->holdPortDirection);
	if (tap->DTP)
	    free(tap->DTP);
	free(tap);
}

void
FreeFileClass(fcp)
DmFclassPtr fcp;
{
#ifdef TRACE
	fprintf(stderr,"FreeFileClass\n");
#endif
	free(fcp->glyph);
	free(fcp);
}

void
FreeObject(op)
DmObjectPtr op;
{
	DeviceData *tap;
#ifdef TRACE
	fprintf(stderr,"FreeObject\n");
#endif
	free(op->name);
	/* set the port to be deleted to outgoing so that the ttymon gets removed */
	FreeDeviceData(op->objectdata);
	free(op);
}

Boolean
getline(buf, len, fd, filetype)
char *buf;
int len;
FILE *fd;
int filetype;
{
	while (fgets(buf, len, fd) != (char *)NULL) {
		if (buf[0] == ' ' || buf[0] == '\t' ||  buf[0] == '\n'
		    || buf[0] == '\0' || buf[0] == '#') {
			strcat (lbuf[filetype], buf);
			continue;
		}
		return(True);
	}
	return(False);
} /* getline */

/*
 * Procedure:	no_tp_wanted
 *
 * 	Reads the tp_config default file to determine whether or not
 *	a "trusted path" connection should be established for the
 *	device passed as an argument.
 */

#define CS_TPCONFIGFILE    "cs_tpconfig"
#define TPATH_FILE    "tpath"

static	int
no_tp_wanted(dcname)
	char *dcname;
{
	FILE *fp;
	char *p;
	int	ret = 0;
	Boolean	found = False;
	extern	FILE	*defopen();
	extern	char	*defread();

	if ((fp = defopen(CS_TPCONFIGFILE)) != NULL) {
		if ((p = defread(fp, dcname)) != NULL) {
			if (*p) {
				found = True;
				if (strcmp(p, "no") == 0)
					ret = 1;
				else if (strcmp(p, "NO") == 0)
					ret = 1;
			}
		}
		(void) defclose(fp);
	}
	if (! found) {
		if ((fp = defopen(TPATH_FILE)) != NULL) {
			if ((p = defread(fp, "TP_DEFAULT")) != NULL) {
				if (*p) {
					if (strcmp(p, "no") == 0)
						ret = 1;
					else if (strcmp(p, "NO") == 0)
						ret = 1;
				}
			}
			(void) defclose(fp);
		}
	}

	return ret;
}



static void
UpdateResetItems(port, oldport, action)
char *port;
char *oldport;
int action;
{
	ResetEntry *entry;
	char *shortPort;
	char *oldShortPort;
		/* shortPort will hold a pointer to the port without
			any paths. port could be something
			like /dev/tty02 or /dev/term/02 for
			other device names, so we need to check
			all combinations: the full name such as
			/dev/tty02,M , tty02, and /dev/tty02.
		*/

#ifdef DEBUG	
	fprintf(stderr,"UpdateResetItems: port=%s oldport=%s action=%d\n",port, oldport, action);
#endif
	
		/* get the port name without any paths */
	shortPort = (char *) IsolateName(port);
	if (action == DELETE) {
		/* deleting a device so we must delete any reset items
			for this device */
		getLongPortNumber(port);
		while ((entry = (ResetEntry *) getResetEntry(resetTable, port,
			shortPort, longPortNumber)) != NULL) {
			/* delete  any entries that match the delete device */
			deleteResetEntry(resetTable, entry);
			
		}
			/* we are finished with delete items */
		return;
	} else 
	if ((action == MODIFY) &&
		(port != NULL) &&
		(oldport != NULL) &&
		(strcmp(port, oldport) !=0)) {
			/* action was a change and the port itself
			changed, so we need to delete the oldport info */
		getLongPortNumber(oldport);
		oldShortPort = (char *) IsolateName(oldport);
		while ((entry = (ResetEntry *) getResetEntry(resetTable, 
			oldport, oldShortPort, longPortNumber)) != NULL)  {
			/* delete any entries that match the old entry */
			deleteResetEntry(resetTable, entry);
		}
	}
	/* modify and add have the current data in holdData */
	getLongPortNumber(port);	

#ifdef DEBUG
	fprintf(stderr,"UpdateResetItems: ");
	fprintf(stderr,"portNumber=%s\n",holdData.portNumber);
	fprintf(stderr,"holdPortDirection=%s\n",holdData.holdPortDirection);
	fprintf(stderr,"portDirection=%s\n",holdData.portDirection);
	fprintf(stderr,"portSpeed=%s\n",holdData.portSpeed);
#endif

	/* delete all entries that match this port - this will clean
		up any duplicate and/or old entries that may be hanging
		around */
	while ((entry = (ResetEntry *) getResetEntry(resetTable, port,
		shortPort, longPortNumber)) != NULL)  {
			/* delete any entries that match the port */
			deleteResetEntry(resetTable, entry);
	}

		/* we don't add  Reset lines for disabled devices,
		direct or datakit devices, or outgoing devices */

	if ((strcmp(holdData.portEnabled, "disabled") == 0) ||
		 (strcmp(holdData.modemFamily, "datakit") == 0) ||
		 (strcmp(holdData.modemFamily, "uudirect") == 0) ||
		 (strcmp(holdData.modemFamily, "direct") == 0) ||
		 (strcmp(holdData.portDirection, "outgoing") == 0)) {
			/* we already deleted the entries that match
			so we can just return */
			return;
	} else {
				
		/* port is a modem and is enabled
		and is either incoming or bi-directional
		so we need a Reset class entry to the
		/etc/uucp/Devices file */

		/* allocate a structure for the new information */
		/* fill in the new information */
		entry = (ResetEntry *) addResetEntryToTable(resetTable);
		entry->longport = strdup(longPortNumber);
		entry->port = strdup(port);
		entry->dash = strdup("-");
		entry->speed = strdup(holdData.portSpeed);
		entry->modem_cmd  = strdup("atcmd_auto");
		entry->restofline  = NULL;

	}
}



static ResetEntry *
addResetEntryToTable(table)
ResetData * table;
{
	ResetEntry * entry;
	ResetEntry * new;

	new = (ResetEntry *) malloc (sizeof(ResetEntry));
	new->next = 0;
	table->count++;
	if (table->reset_entry == NULL) {
		/* empty reset table */
		table->reset_entry = new;
		return new;
	} /* find end of reset table */
	for (entry = table->reset_entry; entry->next; entry=entry->next) ;

	entry->next = new;
	return new;

}


static ResetEntry *
getResetEntry(table, port, shortport,longport)
ResetData *table;
char *port;
char *shortport;
char *longport;
{
	ResetEntry * entry;
	ResetEntry * match;
	char * entryShortPort;
	int dev_idx, alias_idx, i;
	Boolean exact_match_needed;

		/* shortport will hold a pointer to the port without
			any paths. port could be something
			like /dev/tty02 or /dev/term/02 for
			other device names, so we need to check
			all combinations: the longname such as
			/dev/tty02,M , tty02, and /dev/tty02.
		*/
	
	if (shortport == NULL) return;
	exact_match_needed = False;
	if ((dev_idx = FindAliasTable(shortport)) == -1) {
			exact_match_needed = True;
	}	
#ifdef DEBUG
	fprintf(stderr,"getResetEntry: port=%s\n",port);
#endif
	match = NULL;
	for (entry=table->reset_entry; entry; entry = entry->next) {
	
#ifdef DEBUG
	fprintf(stderr,"getResetEntry: entry->port=%s entry->speed=%s\n",entry->port, entry->speed);
#endif
		/* need to check various combinations of port and longport
		looking for a match since com1 can be tty00, /dev/tty00,
		/dev/tty00s etc.and all match */ 
		if ((entry->longport != NULL) &&
		(longport != NULL) &&
		(strcmp(entry->longport, longport) == 0)) {
			/* found port that matchs */
			match = entry;
			break;
		}
		if (entry->port == NULL) continue;
		/* check for case like  /dev/term/02 - shortport
		will be 02  and entry should also be 02 */
		if (strcmp(entry->port, shortport) == 0){ 
			/* found port that matchs */
			match = entry;
			break;
		}
			
		if (exact_match_needed) continue;
			/* check aliases if exact match is not needed */
		if ((entryShortPort = (char *)IsolateName(entry->port)) == NULL)
				continue;
		if (Check4PortAlias(entryShortPort, dev_idx) == True) {
			/* found an alias that matchs */ 
			match = entry;
			break;
		}
	}
	return match;
}

static void
freeResetEntryItems(entry)
ResetEntry * entry;
{
	/* free individual items in reset entry */

	free(entry->longport);
	free(entry->port);
	free(entry->dash);
	free(entry->speed);
	free(entry->modem_cmd);
	free(entry->restofline);
}

static void 
deleteResetEntry(table, entry)
ResetData *table;
ResetEntry * entry;
{
	ResetEntry *endp;

	if (!entry) return; /* bad entry - should not happen */
	freeResetEntryItems(entry); 	/* delete entry data */
	if (table->reset_entry == entry) 
		/* deleting the first table entry */
		table->reset_entry = entry->next;
	else
	for (endp = table->reset_entry; endp; endp=endp->next) {
		if (endp->next == entry) {
			endp->next = entry->next;
			break;
		}
	}
	free(entry);
	table->count--;
}


getLongPortNumber(curport)
char * curport;
{
	/* set up[ longport */
	

	if ((strcmp(curport, COM1)) == 0) {
		strcpy(longPortNumber, LONG_COM1);
	} else
	if ((strcmp(curport, COM2)) == 0) {
		strcpy(longPortNumber, LONG_COM2);
	} else {
		strcpy(longPortNumber, curport);
	}
	if (no_tp_wanted(longPortNumber)) {
		sprintf(longPortNumber, "%s,M", longPortNumber);
	}
}

