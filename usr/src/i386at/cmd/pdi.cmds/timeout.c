#ident	"@(#)pdi.cmds:timeout.c	1.1.2.1"
#ident	"$Header$"

/*
 * Usage:
 *
 * Administrative:
 *
 *	sdi_timeout [-fsFC] [-l 6|10|12] [ [ device_desc [cmd [value]]...]...]
 *	sdi_timeout -c device_type
 *	sdi_timeout [-y|n|q]
 *	sdi_timeout -b [node_descriptor]
 *
 *	where
 *	device_desc is one of:
 *		device_type - "disk", "tape", "cdrom", "worm"
 *		node_descriptor - cCbBtTdDpP
 *		inquiry_string - prefix or entire inquiry
 *	cmd is a SCSI command name from the list returned by the -c option
 *	value is an integer
 *
 * Non-administrative:
 *	none
 *
 * Level:	SYS_PUBLIC
 *
 * Inheritable Privileges:	P_DEV,P_MACWRITE,P_DACREAD,P_DACWRITE,
 *				P_SYSOPS,P_SETFLEVEL,P_OWNER
 *
 * Fixed Privileges:		P_AUDIT,P_MACREAD
 *
 * Files:	/etc/scsi/pditimetab
 *		/etc/scsi/pditimetab.orig
 *
 * Notes:	sdi_timeout is used to maintain the timeout values associated
 *		with each SCSI command.  Its most common use is at boot
 *		time of the system when it loads the current values into
 *		SDI.  Infrequently, a system administrator will use the
 *		command to query and then modify the timeout values for
 *		a particular device.
 */


#include "timeout.h"

static void printUsage();

/* Messages used in this file - see timeout.str */
#define MSG1	":1:%s [-fsCF] [-l 6|10|12] [device_descriptor [command [value]] ...]\n\t%s -c device_type\n\t%s -[y|n|q]\n\t%s -b\n"
#define MSG2	":2:Invalid option to -l: %s\n"
#define MSG3	":3:Invalid device: %s\n"
#define MSG4	":4:Invalid command: %s\n"
#define MSG5	":5:Timeout values were not restored to original values.\n"
#define MSG6	":6:Invalid arguments\n"
#define MSG7	":7:Invalid Node Descriptor: %s\n"
#define MSG8	":8:Timeout Reset feature is enabled.\n"
#define MSG9	":9:Failed to enable Timeout Reset feature.\n"
#define MSG10	":10:Timeout Reset feature is disabled.\n"
#define MSG11	":11:Failed to disable Timeout Reset feature.\n"
#define MSG12	":12:Error parsing arguments at: %s\n"
#define MSG13	":13:Value found in arguments specified with -f option\n"
#define MSG14	":14:Timeout values restored to original values.\n"
#define MSG15	":15:Apply changes? [%s | %s] "
#define MSG16	":16:Changes were aborted\n"
#define MSG32	":32:Insufficient privilege to run command\n"
#define MSG33	":33:Invalid arguments: two values in a row at %s\n"
#define MSG34	":34:Invalid value: %d\n"

#define MAX_LENGTH	5
#define PDITIMETAB	"/etc/scsi/pditimetab"
#define PDITIMETAB_ORIG	"/etc/scsi/pditimetab.orig"

static char     *Cmdname;

static void
printUsage(void)
{
	(void) pfmt(stderr, MM_ACTION, MSG1, Cmdname, Cmdname, Cmdname, Cmdname);
	exit(2);
}  /* end of printUsage() */

void
main(int argc, char ** argv)
{
	Boolean Disable = False;
	Boolean Enable = False;
	Boolean FactoryDefaults = False;
	Boolean FactoryQuery = False;
	Boolean HexCommand = False;
	Boolean Query = False;
	Boolean Silent = False;
	Boolean Boot = False;
	Boolean	fslC_flag = False;
	Boolean	c_flag = False;
	Boolean	b_flag = False;
	Boolean	ynq_flag = False;
	char length[MAX_LENGTH];
	int arg;
	int num_commands;
	int i;
	int * command = NULL;
	int value;
	int edt_cnt = 0;
	struct scsi_xedt * edt;
	int rc = 0;
	ptFileEntry * pdi_file = NULL;
	ptFileEntry * queries = NULL;
	ptTimeoutTable * table = NULL;
	char *label;

	Cmdname = (char *)strdup(basename(argv[0]));

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxtimeout");
	label = (char *) malloc(strlen(Cmdname)+1+3);
	sprintf(label, "UX:%s", Cmdname);
	(void) setlabel(label);

	if (!(edt = ptReadEdt(&edt_cnt)))  {
		pfmt(stderr, MM_ERROR, MSG32);
		exit(2);
	}

	length[0] = (char) 0;

	/* Parse and validate arguments */
	while ((arg = getopt(argc, argv, "bfCFsl:c:ynq?")) != EOF)
		switch (arg)  {
		case 'b' :
			if (c_flag || ynq_flag || fslC_flag)  {
				free(edt);
				printUsage();
			}
			b_flag = True;
			Boot = True;
			break;
		case 'C' :
			if (c_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			fslC_flag = True;
			HexCommand = True;
			break;
		case 'f' :
			if (c_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			fslC_flag = True;
			FactoryDefaults = True;
			break;
		case 'F':
			if (c_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			fslC_flag = True;
			FactoryQuery = True;
			break;
		case 's' :	/* Silent mode */
			if (c_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			fslC_flag = True;
			Silent = True;
			break;
		case 'l' :
			if (c_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			fslC_flag = True;
			if (strcmp(optarg, "6") == 0 ||
				strcmp(optarg, "10") == 0 ||
				strcmp(optarg, "12") == 0)
				strncpy((char *)length, (char *)optarg, MAX_LENGTH);
			else
				pfmt(stderr, MM_ERROR, MSG2, optarg);
				break;
		case 'c' :
			if (fslC_flag || ynq_flag || b_flag)  {
				free(edt);
				printUsage();
			}
			c_flag = True;
			rc = !ptPrintCommands(optarg);
			goto clean_up;
		case 'y' :
			if (fslC_flag || c_flag || b_flag || Disable || Query)  {
				free(edt);
				printUsage();
			}
			ynq_flag = True;
			Enable = True;
			break;
		case 'n' :
			if (fslC_flag || c_flag || b_flag || Enable || Query)  {
				free(edt);
				printUsage();
			}
			ynq_flag = True;
			Disable = True;
			break;
		case 'q' :
			if (fslC_flag || c_flag || b_flag || Enable || Disable)  {
				free(edt);
				printUsage();
			}
			ynq_flag = True;
			Query = True;
			break;
		case '?' :
			free(edt);
			printUsage();
		}
	
	if (Boot)  {
		Boolean err;
		int edt_i;

		/* Boot works by reading in the pditimetab file and sending
		    the table to sdi.  If there is an optional argument, it
		    parses, validates, and sends just the table entry for
		    that controller to sdi.  */
		if (!(pdi_file = (ptFileEntry *) ptReadFile(PDITIMETAB,
			edt, edt_cnt, &table)))  {
			rc = 1;
			goto clean_up;
		}

		if (optind == argc)  {
			/* There are no arguments - send whole table */
			err = ptSendTable(table, edt_cnt);
		}
		else  {
			char * device = argv[optind];
			for ( ; optind < argc; optind++)  {
				/* ptValidate expects upper case. */
				for (i = 0; device[i]; i++)
					device[i] = (char)toupper(device[i]);
				if (!ptValidateNodeDescriptor(device, edt,
					edt_cnt, NULL, NULL, &edt_i))  {
					pfmt(stderr, MM_ERROR, MSG7, device);
					err = True;
					break;
				}
				err = !ptSendTable(&(table[edt_i]), 1);
				if (err)
					break;
			}
		}
		if (err)
			rc = 1;
		/* Boot is a stand-alone option, so exit now */
		goto clean_up;
	}
	if (Enable) {
		/* Enable all timeouts */
		if (ptSetTimeout(True))
			pfmt(stdout, MM_NOSTD, MSG8);
		else
			pfmt(stdout, MM_NOSTD, MSG9);
	}
	if (Disable) {
		/* Disable all timeouts */
		if (ptSetTimeout(False))
			pfmt(stdout, MM_NOSTD, MSG10);
		else
			pfmt(stdout, MM_NOSTD, MSG11);
	}
	if (Query)  {
		/* Query device and timeout status */
		ptPrintInfo(edt, edt_cnt);
	}

	if (!(c_flag || ynq_flag))  {
		/*  Boot already has exited, so this covers the -fs and
		    no options at all */
		int state = START;
		int device_type = 0;
		int new_device_type = 0;
		char * inquiry;
		char * new_inquiry;
		Boolean update_needed = False;


		if(!(pdi_file = (ptFileEntry *) ptReadFile(PDITIMETAB,
			edt, edt_cnt, &table)))  {
			rc = 1;
			goto clean_up;
		}

		/*  State machine for parsing arguments:
			State		Inputs
				Device	Command	Value	Other
			START	DEVICE	ERROR	ERROR	ERROR
			DEVICE	DEVICE	COMMAND	ERROR	ERROR
			COMMAND	DEVICE	COMMAND	VALUE	ERROR
			VALUE	DEVICE	COMMAND	ERROR	ERROR
			ERROR	ERROR	ERROR	ERROR	ERROR
		*/
		for ( ; optind < argc; optind++)  {
			switch (state)  {
			case START:
				/* In START state, the input must be a device */
				if (ptCheckDevice(argv[optind], edt, edt_cnt,
					&inquiry, &device_type))  {
					state = DEVICE;
				}
				else {
					/* all others are errors */
					pfmt(stderr, MM_ERROR, MSG3,
						argv[optind]);
					rc = 2;
					goto clean_up;
				}
				break;
			case DEVICE:
				/* In DEVICE state, we have seen a device and
				   can expect the input to be either a device
				   or a command. */
				if (ptCheckDevice(argv[optind], edt, edt_cnt,
					&new_inquiry, &new_device_type))  {
					/* Found another device - the entry
					   is a query for all commands. */
					ptInsertEntry(&queries, inquiry,
						device_type, UNDEF, UNDEF);
					state = DEVICE;
					device_type = new_device_type;
					inquiry = new_inquiry;
				}
				else if (ptIsCommand(argv[optind], device_type,
					(char *)length, HexCommand))  {
					/* If it is a command then get it */
					if (!ptCommandToInt(argv[optind],
						(char *)length,
						device_type,
						HexCommand,
						&command,
						&num_commands))  {
						pfmt(stderr, MM_ERROR, MSG4,
							argv[optind]);
						rc = 2;
						goto clean_up;
					}
					state = COMMAND;
				}
				else  {
					/* Values and others are errors */
					pfmt(stderr, MM_ERROR, MSG12,
						argv[optind]);
					rc = 2;
					goto clean_up;
				}
				break;
			case COMMAND:
				/* In COMMAND state we have seen a command and
				   can expect the input to be a value if the
				   user if modifying the timeout or a device
				   or another command if the user is doing a
				   query.  */
				if (sscanf(argv[optind], "%d", &value) == 1)  {
					/* Values are illegal with -f */
					if (FactoryDefaults)  {
						pfmt(stderr, MM_ERROR, MSG13);
						rc = 2;
						goto clean_up;
					}
					/* Validate the value */
					if (value < 0)  {
						pfmt(stderr, MM_ERROR, MSG34, value);
						rc = 2;
						goto clean_up;
					}
					/* Found a value - make an entry */
					for (i = 0; i < num_commands; i++)  {
						ptInsertEntryValue(&pdi_file,
							inquiry,
							device_type,
							command[i],
							(unsigned short)value,
							edt, edt_cnt);
						/* flag that timeout tables need
						   to be sent to sdi */
						update_needed = True;
					}
					if (command)  {
						free(command);
						command = NULL;
					}
					state = VALUE;
				}
				else if (ptIsCommand(argv[optind], device_type,
					(char *) length, HexCommand))  {
					/* The previous command is a query */
					for (i = 0; i < num_commands; i++)
						ptInsertEntry(&queries,
							inquiry,
							device_type,
							command[i],
							UNDEF);
					if (command)  {
						free(command);
						command = NULL;
					}
					
					/* Get this command */
					if (!ptCommandToInt(argv[optind],
						(char *) length,
						device_type,
						HexCommand,
						&command,
						&num_commands))  {
						pfmt(stderr, MM_ERROR, MSG4,
							argv[optind]);
						rc = 2;
						goto clean_up;
					}
					state = COMMAND;
				}
				else if (ptCheckDevice(argv[optind], edt,
					edt_cnt, &new_inquiry, &new_device_type))  {
					/* The previous command is a query */
					for (i = 0; i < num_commands; i++)
						ptInsertEntry(&queries,
							inquiry,
							device_type,
							command[i],
							UNDEF);
					if (command)  {
						free(command);
						command = NULL;
					}
					device_type = new_device_type;
					inquiry = new_inquiry;
					state = DEVICE;
				}
				else {
					/* All others are errors */
					pfmt(stderr, MM_ERROR, MSG4,
						argv[optind]);
					rc = 2;
					goto clean_up;
				}
				break;		
			case VALUE:
				/* We have seen a value, so the only valid
				   input is a device or command. */
				if (sscanf(argv[optind], "%d", &value) == 1)  {
					/* Error - two values in a row */
					pfmt(stderr, MM_ERROR, MSG33, argv[optind]);
					rc = 2;
					goto clean_up;
				}
				else if (ptIsCommand(argv[optind], device_type,
					(char *) length, HexCommand))  {
					/* Building another command value pair */
					ptCommandToInt(argv[optind],
						(char *) length,
						device_type,
						HexCommand,
						&command,
						&num_commands);
					state = COMMAND;
				}
				else if (ptCheckDevice(argv[optind], edt,
					edt_cnt, &inquiry, &device_type))  {
					/* Starting over at a device */
					state = DEVICE;
				}
				else {
					/* Error */
					pfmt(stderr, MM_ERROR, MSG4, argv[optind]);
					rc = 2;
					goto clean_up;
				}
				break;
			}
		}
		/* Take care of last entry */
		switch (state)  {
		case START:
			/* Make a query for all devices */
			if (!FactoryDefaults)
				ptQueryAll(&queries, edt, edt_cnt);
			break;
		case DEVICE:
			/* Create a query for the device */
			ptInsertEntry(&queries, inquiry, device_type, UNDEF,
					UNDEF);
			break;
		case COMMAND:
			/* Create a query for the device command pair*/
			for (i = 0; i < num_commands; i++)
				ptInsertEntry(&queries, inquiry, device_type,
					command[i], UNDEF);
			break;
		case VALUE:
			/* Do nothing */
			break;
		}
		if (command)  {
			free(command);
			command = NULL;
		}
		if (queries && !FactoryDefaults)  {
			if (FactoryQuery)  {
				ptFileEntry * orig;
				ptTimeoutTable * orig_table = NULL;
				
				if(!(orig =  ptReadFile(PDITIMETAB_ORIG,
					edt, edt_cnt, &orig_table)))  {
					rc = 1;
					goto clean_up;
				}
				ptPrintQuery(queries, orig_table, edt, edt_cnt);
				ptFreeFileEntry(orig);
				free(orig_table);
			}
			else
				ptPrintQuery(queries, table, edt, edt_cnt);
		}
		if (FactoryDefaults)  {
			int r;
			r = ptResetToFactory(Silent, queries, &pdi_file,
				edt, edt_cnt, table);
			if (!Silent)  {
				if (r)
					pfmt(stdout, MM_NOSTD, MSG14);
				else
					pfmt(stdout, MM_NOSTD, MSG5);
			}
			goto clean_up;
		}
		if (update_needed)  {
			if (!Silent)  {
				pfmt(stdout, MM_NOSTD, MSG15,
					nl_langinfo(YESSTR),
					nl_langinfo(NOSTR));
			}
			if (Silent || ptYesP())  {
				ptUpdateTable(table, pdi_file, edt, edt_cnt);
				if (ptSendTable(table, edt_cnt))  {
					if (!ptWriteFile(pdi_file))  {
						rc = 1;
						goto clean_up;
					}
				}
				else  {	
					rc = 1;
					goto clean_up;
				}
			}
			else if (!Silent)
				pfmt(stdout, MM_NOSTD, MSG16);
		}

	}
clean_up:
	ptFreeFileEntry(pdi_file);
	ptFreeFileEntry(queries);
	if (command) free(command);
	if (table) free(table);
	if (edt) free(edt);
	exit(rc);
} /* end of main() */
