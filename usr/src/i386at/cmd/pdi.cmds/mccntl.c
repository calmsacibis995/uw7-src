#ident	"@(#)pdi.cmds:mccntl.c	1.8.3.1"
#ident	"$Header$"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/vtoc.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/mc01.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include <locale.h>
#include <pfmt.h>

#define CMD_EXCHANGE	0x01
#define CMD_EJECT_DEF	0x02
#define CMD_EJECT_SPEC	0x03
#define CMD_LOAD	0x04
#define CMD_POSITION	0x05
#define CMD_INITIALIZE	0x06
#define CMD_ELEM_COUNT	0x07
#define CMD_STATUS	0x08
#define CMD_MAP		0x09
#define CMD_LOCK	0x0A
#define CMD_UNLOCK	0x0B
#define	CMD_RET_FIRST	0x0C

extern int	errno;
extern char	*optarg;
extern int	optind;

char	Device[256];

void	mccntl_usage();

int	Drive_address;
int	Chm_address;

struct	SLOTS {
	int	slot_full;
	int	slot_address;
}*slots;

main( argc, argv )
int	argc;
char	*argv[];
{
	int	changer;
	int	c;
	int	slot;
	int	last_loaded;
	int	chm;
	int	drive;
	int	i;
	int	command;
	int	rtcde;
	int	index;

	int	elem_count;

	struct mc_ex_medium ex;
	struct mc_mv_medium mv;
	struct mc_position pos;

	struct SLOTS	*slotp;

	char	*malloc();

	/*
	 * Set default device name.
	 */
	strcpy( Device, "/dev/mc/mc1" );

	/*
	 * Initialize command flag.
	 */
	command = 0;

	/*
	 * Prepare message library.
	 */
	if( setlocale(LC_ALL, "") == NULL ) {
		printf("setlocale failed\n");
	}

	if( setcat("uxmccntl") == NULL ) {
		printf("setcat failed\n");
	}

	if( setlabel("UX:mccntl") != 0 ) {
		printf("setlabel failed\n");
	}

	/*
	 * Parse the command line.
	 */
	while ( (c = getopt( argc, argv, "Ep:l:e:x:d:insLUMF?")) != EOF ) {

		switch (c) {
		case 'x':
			/*
			 * Exchange Command:
			 *
			 * As implemented this request will move the
			 * data cartridge currently in drive to the
			 * storage element it was loaded from, and
			 * load the data cartridge from the specified
			 * storage element into the drive.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			ex.source = atoi( optarg ) - 1;
			command |= CMD_EXCHANGE;

			break;

		case 'E':
			/*
			 * Eject Command:
			 *
			 * This request assumes the user has not used the front panel
			 * switch to change the position of the changer mechanism,
			 * or loaded/unloaded a cartridge via the front panel switch.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_EJECT_DEF;

			break;

		case 'e':
			/*
			 * Eject To Specified Element Command:
			 *
			 * This command requests that the data cartridge currently loaded
			 * in the drive be placed in the specified storage element.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_EJECT_SPEC;

			mv.dest = (unsigned short)atoi( optarg ) - 1;

			break;

		case 'l':
			/*
			 * Load To Specified Element Command:
			 *
			 * This requests that the data cartridge from the specified
			 * storage element be loaded into the drive.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_LOAD;

			if( atoi( optarg ) == 0 ) {
				mv.source = (unsigned short)0xFFFF;
			}
			else {
				mv.source = (unsigned short)atoi( optarg ) - 1;
			}

			break;

		case 'p':
			/*
			 * Position Changer Mechanism Command:
			 *
			 * This requests that the changer mechanism be positioned in
			 * front of the specified storage element.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_POSITION;

			if( atoi( optarg ) == 0 ) {
				pos.dest = 0xFFFF;
			}
			else {
				pos.dest = (unsigned short)atoi( optarg ) - 1;
			}

			break;

		case 'i':
			/*
			 * Initialize Element Status Command:
			 *
			 * Initialize the device configuration information.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_INITIALIZE;

			break;

		case 'n':
			/*
			 * Element Count Command:
			 *
			 * Displays the number of storage elements in the currently
			 * loaded magazine.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_ELEM_COUNT;

			break;

		case 's':
			/*
			 * Display Configuration Status Map:
			 *
			 * Show the current configuration.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_STATUS;

			break;

		case 'M':
			/*
			 * Dump Configuration Status Map:
			 *
			 * This generates a colon seperated list describing the
			 * current configuration. Format as follows:
			 * Changer:Drive:ElementCount:Slot1:Slot2:.........:SlotN
			 * This format provided for easy parsing by other programs.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_MAP;

			break;

		case 'L':
			/*
			 * Prevent Media Removal:
			 *
			 * This is actually a drive request, and will only be valid
			 * on units that pass drive commands to the drive, such
			 * as the Archive DAT Autoloader.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_LOCK;

			break;

		case 'U':
			/*
			 * Allow Media Removal:
			 *
			 * This is actually a drive request, and will only be valid
			 * on units that pass drive commands to the drive, such
			 * as the Archive DAT Autoloader.
			 */
			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_UNLOCK;

			break;
		
		case 'd':
			/*
			 * Device Name:
			 *
			 * If this option is not used, the default device name
			 * shall be used: /dev/mc/mc1.
			 */
			strcpy( Device, optarg );
			break;

		case 'F':
			/*
			 * Return to first filled slot, and load media from said slot
			 */

			if( command ) {
				mccntl_usage();
				exit( 1 );
			}

			command |= CMD_RET_FIRST;

			break;
		
		case '?':
		default:
			mccntl_usage();
			exit( 1 );

		}
	}

        if ( optind + 1 == argc )
                strcpy( Device, argv[optind]);
        else if ( optind + 1 < argc ) {
                mccntl_usage();
                exit(1);
        }
        if (optind == 1) {
                mccntl_usage();
                exit(1);
        }

	if ( (changer = open( Device, O_RDONLY, 0)) == -1 ) {
		(void) pfmt( stderr, MM_ERROR,
			":1:Unable to open device %s\n", Device);
		exit( 2 );
	}

	/*
	 * Find out how many storage elements are in the currently loaded
	 * magazine.
	 */
	if ( ioctl( changer, MC_ELEMENT_COUNT, &elem_count ) == -1 ) {
		(void) pfmt( stderr, MM_ERROR,
			":2:MC_ELEMENT_STATUS failed on device %s\n",
			Device);
		close( changer );
		exit( 3 );
	}

	/*
	 * The element count includes the drive and changer mechanism,
	 * and we do not keep these in the slots array.
	 */
	elem_count -= 2;


	if ( (slots = (struct SLOTS *)malloc( (sizeof( struct SLOTS ) * elem_count ) ) ) == NULL ) {
		(void) pfmt( stderr, MM_ERROR,
		":3:cannot allocate buffer\n");
		close( changer );
		exit( 4 );
	}

	for( i = 0, slotp = slots; i < elem_count; ++i, ++slotp ) {
		slotp->slot_address = -1;
		slotp->slot_full = 0;
	}

	if( (rtcde = getstatus( changer, &drive, &chm, &last_loaded )) != 0 ) {
		close( changer );
		exit( rtcde );
	}

	switch (command) {
	case CMD_EXCHANGE:

		ex.transport =	(unsigned short)Chm_address;
		ex.first_dest =	(unsigned short)Drive_address;

		if( drive == 0 ) {
			(void) pfmt( stderr, MM_ERROR,
				":4:data transfer element is empty.\n");
			close( changer );
			exit( 7 );
		}

		if( ex.source < 0 || ex.source >= elem_count ) {
			(void) pfmt( stderr, MM_ERROR,
				":38:Invalid source element %d, valid range 1-%d\n",
				(ex.source+1),elem_count);
			close( changer );
			exit( 8 );
		}

		if ( slots[ex.source].slot_full == 0 ) {
			(void) pfmt( stderr, MM_ERROR,
				":5:source element is empty.\n");
			close( changer );
			exit( 8 );
		}

		ex.source = slots[ex.source].slot_address;

		if( last_loaded == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":9:MC_LAST_LOADED failed on device %s\n",
				Device);
			close( changer );
			exit( 9 );
		}

		/*
		 * last_loaded is the actual SCSI element address,
		 * not the "magazine slot number".
		 */
		for( slot = 0; slot < elem_count; ++slot ) {
			if( slots[slot].slot_address == last_loaded ) {
				if( slots[slot].slot_full == 1 ) {
					(void) pfmt( stderr, MM_ERROR,
						":7:destination element is full.\n");
					close( changer );
					exit( 10 );
				}
				else {
					ex.second_dest = last_loaded;
					break;
				}
			}
		}

		if ( ioctl( changer, MC_EXCHANGE, &ex ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":8:MC_EXCHANGE failed on device %s\n",
				Device);
			close( changer );
			exit( 11 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_EJECT_DEF:

		/*
		 * last_loaded is maintained in the driver as the
		 * last drive element address, not the "user view"
		 * magazine slot number.
		 */
		if( last_loaded == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":9:MC_LAST_LOADED failed on device %s\n",
				Device);
			close( changer );
			exit( 9 );
		}

		mv.dest = last_loaded;
		mv.transport = (unsigned short)Chm_address;
		mv.source = (unsigned short)Drive_address;


		if( drive == 0 ) {
			(void) pfmt( stderr, MM_ERROR,
				":10:data transfer element is empty.\n");
			close( changer );
			exit( 7 );
		}

		for( slot = 0; slot < elem_count; ++slot ) {
			if ( slots[slot].slot_address ==  mv.dest ) {
				if ( slots[slot].slot_full == 1 ) {
					(void) pfmt( stderr, MM_ERROR,
						":11:destination element is full.\n");
					close( changer );
					exit( 10 );
				}
				else {
					break;
				}
			}
		}

		if ( ioctl( changer, MC_MOVE_MEDIUM, &mv ) == -1 ) {
			if ( errno == EAGAIN ) {
				(void) pfmt( stderr, MM_ERROR,
					":39:Cannot access media, drive door closed\n");
			}
			else {
				(void) pfmt( stderr, MM_ERROR,
					":15:MC_MOVE_MEDIUM failed on device %s\n",
					Device);
			}
			close( changer );
			exit( 12 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_EJECT_SPEC:

		mv.transport = (unsigned short)Chm_address;
		mv.source = (unsigned short)Drive_address;

		if( drive == 0 ) {
			(void) pfmt( stderr, MM_ERROR,
				":13:data transfer element is empty.\n");
			close( changer );
			exit( 7 );
		}

		if( mv.source < 0 || mv.source >= elem_count ) {
			(void) pfmt( stderr, MM_ERROR,
				":38:Invalid source element %d, valid range 1-%d\n",
				(mv.source+1),elem_count);
			close( changer );
			exit( 8 );
		}

		if( mv.dest < 0 || mv.dest >= elem_count ) {
			(void) pfmt( stderr, MM_ERROR,
				":37:Invalid destination element %d, valid range 1-%d\n",
				(mv.dest+1),elem_count);
			close( changer );
			exit( 8 );
		}

		if ( slots[mv.dest].slot_full == 1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":14:destination element is full.\n");
			close( changer );
			exit( 10 );
		}

		mv.dest = slots[mv.dest].slot_address;
		mv.source = Drive_address;

		if ( ioctl( changer, MC_MOVE_MEDIUM, &mv ) == -1 ) {
			if ( errno == EAGAIN ) {
				(void) pfmt( stderr, MM_ERROR,
					":39:Cannot access media, drive door closed\n");
			}
			else {
				(void) pfmt( stderr, MM_ERROR,
					":15:MC_MOVE_MEDIUM failed on device %s\n",
					Device);
			}
			close( changer );
			exit( 12 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_LOAD:

		if( drive == 1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":16:data transfer element is full.\n");
			close( changer );
			exit( 13 );
		}

		if( mv.source < 0 || mv.source >= elem_count ) {
			if( mv.source == (unsigned short)0xFFFF ) {
				mv.source = 0;
			}
			else {
				++mv.source;
			}
			(void) pfmt( stderr, MM_ERROR,
				":38:Invalid source element %d, valid range 1-%d\n",
				mv.source,elem_count);
			close( changer );
			exit( 8 );
		}

		if( slots[mv.source].slot_full == 0 ) {
			(void) pfmt( stderr, MM_ERROR,
				":17:source element is empty.\n");
			close( changer );
			exit( 8 );
		}

		mv.source = slots[mv.source].slot_address;
		mv.dest = (unsigned short)Drive_address;
		mv.transport = (unsigned short)Chm_address;

		if ( ioctl( changer, MC_MOVE_MEDIUM, &mv ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":18:MC_MOVE_MEDIUM failed on device %s\n",
				Device);
			close( changer );
			exit( 12 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_POSITION:

		pos.transport = (unsigned short)Chm_address;

		if( pos.dest == 0xFFFF ) {
			pos.dest = Drive_address;
		}
		else {
			if( pos.dest < 0 || pos.dest >= elem_count ) {
				(void) pfmt( stderr, MM_ERROR,
					":37:Invalid destination element %d, valid range 0-%d\n",
					(pos.dest+1),elem_count);
				close( changer );
				exit( 8 );
			}
			pos.dest = slots[pos.dest] .slot_address;
		}


		if ( ioctl( changer, MC_POSITION, &pos ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":19:MC_POSITION failed on device %s\n",
				Device);
			close( changer );
			exit( 14 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_INITIALIZE:
		if( elem_count < 1 ) {
			exit( 0 );
		}

		if ( ioctl( changer, MC_INIT_STATUS, 0 ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":20:MC_INIT_STATUS failed on device %s\n",
				Device);
			close( changer );
			exit( 15 );
		}

		close( changer );

		exit( 0 );
		break;

	case CMD_ELEM_COUNT:
		/*
		 * NOTE: The Element Count returned includes the
		 *       drive and the changer mechanism.
		 */
		if ( ioctl( changer, MC_ELEMENT_COUNT, &elem_count ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":21:MC_ELEMENT_COUNT failed on device %s\n",
				Device);
			close( changer );
			exit( 3 );
		}

		printf("%d\n", (elem_count - 2));

		close( changer );
		exit( 0 );
		break;

	case CMD_STATUS:

		if( drive == 1 ) {
			(void) pfmt( stdout, MM_NOSTD,
				":22:Drive Status: TAPE LOADED\n");
		}
		else {
			(void) pfmt( stdout, MM_NOSTD,
				":29:Drive Status: EMPTY\n");
		}

		if( elem_count < 1 ) {
			(void) pfmt( stdout, MM_NOSTD,
				":30:Magazine Status: EMPTY\n");
		}
		else {
			(void) pfmt( stdout, MM_NOSTD,
				":31:Magazine Status: LOADED\n");
			(void) pfmt( stdout, MM_NOSTD,
				":32:Magazine Size: %d\n",elem_count);
			(void) pfmt( stdout, MM_NOSTD,
				":33:Slot\tStatus\n");
			for( slot = 0, index = 1; slot < elem_count; ++slot, ++index ) {
				if ( slots[slot].slot_full == 0 ) {
					if( slots[slot].slot_address == last_loaded ) {
						(void) pfmt( stdout, MM_NOSTD,
							":34:%d\tLoaded In Drive\n",index);
					}
					else {
						(void) pfmt( stdout, MM_NOSTD,
							":35:%d\tEmpty\n",index);
					}
				}
				else {
					(void) pfmt( stdout, MM_NOSTD,
						":36:%d\tFULL\n",index);
				}
			}
		}

		exit( 0 );
		break;

	case CMD_MAP:

		printf("%d:%d:%d",chm,drive,elem_count);

		if( elem_count > 0 ) {
			for( slot = 0; slot < elem_count; ++slot ) {
				printf(":%d",slots[slot].slot_full);
			}
			printf("\n");
		}

		exit( 0 );
		break;

	case CMD_LOCK:

		if ( ioctl( changer, MC_PREVMR, &ex ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":23:MC_PREVMR failed on device %s\n",
				Device);
			close( changer );
			exit( 16 );
		}

		exit( 0 );
		break;

	case CMD_UNLOCK:

		if ( ioctl( changer, MC_ALLOWMR, &ex ) == -1 ) {
			(void) pfmt( stderr, MM_ERROR,
				":24:MC_ALLOWMR failed on device %s\n",
				Device);
			close( changer );
			exit( 17 );
		}

		break;

	case CMD_RET_FIRST:

		if( drive == 1 ) {

			if( last_loaded == -1 ) {

				/*
				 * There is a tape in the drive, however we do
				 * not know for sure what slot it came from.
				 */

				(void) pfmt( stderr, MM_ERROR, ":9:MC_LAST_LOADED failed on device %s\n", Device);
				close( changer );
				exit( 18 );
			}
			else {
				mv.dest = last_loaded;
				mv.transport = (unsigned short)Chm_address;
				mv.source = (unsigned short)Drive_address;

				if( drive == 0 ) {
					(void) pfmt( stderr, MM_ERROR, ":10:data transfer element is empty.\n");
					close( changer );
					exit( 18 );
				}

				for( slot = 0; slot < elem_count; ++slot ) {
					if ( slots[slot].slot_address ==  mv.dest ) {
						if ( slots[slot].slot_full == 1 ) {
							(void) pfmt( stderr, MM_ERROR, ":11:destination element is full.\n");
							close( changer );
							exit( 18 );
						}
						else {
							break;
						}
					}
				}

				if ( ioctl( changer, MC_MOVE_MEDIUM, &mv ) == -1 ) {
					if ( errno == EAGAIN ) {
						(void) pfmt( stderr, MM_ERROR, ":39:Cannot access media, drive door closed\n");
					}
					else {
						(void) pfmt( stderr, MM_ERROR, ":15:MC_MOVE_MEDIUM failed on device %s\n", Device);
					}
					close( changer );
					exit( 18 );
				}

				slots[slot].slot_full = 1;
			}
		}

		for( slot = 0, index = 1; slot < elem_count; ++slot, ++index ) {

			if ( slots[slot].slot_full == 1 ) {

				mv.source = slots[slot].slot_address;
				mv.dest = (unsigned short)Drive_address;
				mv.transport = (unsigned short)Chm_address;

				if ( ioctl( changer, MC_MOVE_MEDIUM, &mv ) == -1 ) {
					(void) pfmt( stderr, MM_ERROR, ":18:MC_MOVE_MEDIUM failed on device %s\n", Device);
					close( changer );
					exit( 18 );
				}
				else {
					break;
				}
			}
		}

		exit( 0 );
		break;
	}

	exit( 0 );
}


int
swap16(uint x)
{
	unsigned short	rval;

	rval =  (x & 0x00ff) << 8;
	rval |= (x & 0xff00) >> 8;
	return ((short)rval);
}


int
swap24(uint x)
{
	unsigned int	rval;

	rval =  (x & 0x0000ff) << 16;
	rval |= (x & 0x00ff00);
	rval |= (x & 0xff0000) >> 16;
	return ((int)rval);
}

getstatus( changer, drive, chm, last_loaded )
	int	changer;
	int	*drive;
	int	*chm;
	int	*last_loaded;
{
	int	element;
	int	num_elements;
	int	descriptor;
	int	slot;
	int	num_descriptors;
	int	last;

	char	*sbuf;

	struct mc_esd *esd;
	struct mc_esp *esp;
	struct sed *sed;
	struct mc_status_hdr mc_status_hdr;

	char *malloc();

	if ( ioctl( changer, MC_STATUS, &mc_status_hdr ) == -1 ) {
		(void) pfmt( stderr, MM_ERROR,
			":25:MC_STATUS failed on device %s\n",
			Device);
		return( 5 );
	}

	if ( (sbuf = malloc( (mc_status_hdr.byte_count + 8) )) == NULL ) {
		(void) pfmt( stderr, MM_ERROR,
			":26:cannot allocate buffer\n");
		return( 4 );
	}

	if ( ioctl( changer, MC_MAP, sbuf ) == -1 ) {
		(void) pfmt( stderr, MM_ERROR,
			":27:MC_MAP failed on device %s\n",
			Device);
		return( 6 );
	}

	if ( ioctl( changer, MC_LAST_LOADED, &last ) == -1 ) {
		*last_loaded = -1;
	}
	else {
		*last_loaded = last;
	}

	esd = (struct mc_esd *)sbuf;
	sbuf += 8;

	/*
	 * esd->esd_num_elem is the raw SCSI data, we need to swap the bytes.
	 */
	num_elements = swap16(esd->esd_num_elem);

	for ( element = 0; element < num_elements; ) {

		esp = (struct mc_esp *)sbuf;
		sbuf += 8;

		/*
		 * esp->esp_byte_count is the raw SCSI data, we need to swap the bytes.
		num_descriptors = swap24(esp->esp_byte_count) / 12;
		 */
		if( swap16(esp->esp_edl) == 0 ) {
			continue;
		}
		num_descriptors = swap24(esp->esp_byte_count) /swap16(esp->esp_edl);

		sed = (struct sed *)sbuf;

		for ( descriptor = 0, slot = 0; descriptor < num_descriptors; ++descriptor ) {

			sbuf += swap16(esp->esp_edl);

			if( esp->esp_type == 1 ) {
				Chm_address = sed->sed_address;
				if ( (int)sed->sed_full ) {
					*chm = 1;
				}
				else {
					*chm = 0;
				}
			}

			if( esp->esp_type == 4 ) {
				Drive_address = sed->sed_address;
				if ( (int)sed->sed_full ) {
					*drive = 1;
				}
				else {
					*drive = 0;
				}
			}

			if( esp->esp_type == 2 ) {

				slots[slot].slot_address = sed->sed_address;

				if ( (int)sed->sed_full ) {
					slots[slot].slot_full = 1;
				}
				else {
					slots[slot].slot_full = 0;
				}

				++slot;
			}

			sed = (struct sed *)sbuf;
			++element;
		}
	}

	return( 0 );
}

void
mccntl_usage()
{
	(void) pfmt( stderr, MM_ACTION,
		":28:Usage: mccntl [-E] [-l element] [-e element] [-x element] [-d [raw-device] [-i] [-n] [-s] [-L] [-U] [-M] [-F]\n");

	return;
}
