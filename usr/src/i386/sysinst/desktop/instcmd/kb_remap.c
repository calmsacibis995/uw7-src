#ident	"@(#)kb_remap.c	15.1	98/03/04"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/kd.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <errno.h>
#include <sys/euc.h>
#include <sys/eucioctl.h>
#include <stropts.h>
#include "kb_remap.h"

void fatal(void);
void geteucw(struct eucioc *, char *);
void set_euc(int, struct eucioc *);
void set_vga(int);

/* Check args, read in data from files, execute appropriate ioctls and exit */
int
main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int c;

	int kd_fd, vt_fd;		/* file descriptors */
	int cs_flag = 0;		/* code set flag */
	int kb_flag = 0;		/* keyboard flag */
	int lo_flag = 0;                /* locale flag   */
	int pr_flag = 0;                /* printout flag   */

	char codeset[STR_LEN] = "";	/* Name of code set to use */
	char locale[STR_LEN]  = "";     /* Name of locale          */
	char kb_file[STR_LEN] = "";	/* Name of keyboard config file */
	char dk_file[STR_LEN] = "";	/* Name of dead key config file */
	char device[STR_LEN]  = "";	/* Name of VT device file */

	struct eucioc  eucw;            /* for EUC_WSET ioctl     */

	char *Usage = "Usage:\tkb_remap -f font\n\tkb_remap -k keyboard list_of_VTs\n\tkb_remap -l locale list_of_VTs\n\tkb_remap -p\n";

	/* Make sure we use the right locale */
	(void) setlocale(LC_ALL, "");

	while ((c = getopt(argc, argv, "f:k:l:p")) != EOF) {
		switch (c) {
		case 'f':
			if (strcmp(optarg, "default") == 0) {
				cs_flag = 2;
				break;
			}
			if (optarg[0] != '/')
				strcpy(codeset, FONT_DIR);
			strcat(codeset, optarg);
			cs_flag = 1;
			break;
		case 'k':
			if (optarg[0] != '/') {
				strcpy(kb_file, LOC_DIR);
				strcpy(dk_file, LOC_DIR);
			}
			strcat(kb_file, optarg);
			strcat(kb_file, KB_FILE_NAME);
			strcat(dk_file, optarg);
			strcat(dk_file, DK_FILE_NAME);
			kb_flag = 1;
			break;
		case 'l':
			lo_flag = 1;
			strcat(locale,optarg);
			break;
		case 'p':
			pr_flag = 1;
			break;
		case '?':
			fprintf(stderr, Usage);
			fatal();
			break;
		default:
			fprintf(stderr, "kb_remap: Internal error during getopt()\n");
			fatal();
			break;
		}
	}
#ifdef DEBUG
	printf("cs_flag = %d\n", cs_flag);
	printf("kb_flag = %d\n", kb_flag);
	printf("kb_file = %s\n", kb_file);
	printf("dk_file = %s\n", dk_file);
	printf("codeset = %s\n", codeset);
#endif

	if (!cs_flag && !kb_flag && !lo_flag && !pr_flag) {
		fprintf(stderr, Usage);
		fatal();
	}

	if (kb_flag) {

		/*
		 * Map the keyboard for the VTs specified on the command line.  Note
		 * that mapping a keyboard for a VT with this program is useful only if
		 * another process already has that VT open.  Otherwise, the keyboard
		 * mapping is in effect only while this program is running, which is
		 * typically less than one second.
		 
		 * Therefore, every time you open a VT and want its keyboard mapped,
		 * call this program after the VT is open.
		 */

		struct key_dflt default_key_map;	/* Default key map structure */
		char *dk_buffer = NULL;				/* Buffer for dead keys */

		if (argc == optind) {
			/* The user did not tell us which VTs to map. */
			fprintf(stderr, Usage);
			fatal();
		}

		/*
		 * Each kbmap file is an overlay on top of the default
		 * (US ASCII) keyboard.  Therefore, we need to get the default
		 * keyboard first.
		 */
		if ((vt_fd = open("/dev/vt00", O_RDWR, 0)) < 0) {
			perror("kb_remap: Unable to open /dev/vt00");
			fatal();
		}
		default_key_map.key_direction = KD_DFLTGET;
		if (ioctl(vt_fd, KDDFLTKEYMAP, &default_key_map) < 0) {
			perror("kb_remap: KDDFLTKEYMAP ioctl failed on /dev/vt00");
			fatal();
		}
		close(vt_fd);

		/*
		 * Get the mapping from the file, and overlay that on top of
		 * the default keyboard map.
		 */
		read_kb_map(kb_file, &default_key_map.key_map);

		/*
		 * Get the dead-key mappings if they exist
		 * (some keyboards have no dead keys).
		 */
		if (access(dk_file, R_OK) == 0) {
			if ((dk_buffer = (char *)malloc(DK_BUF_SIZE)) == NULL) {
				perror("kb_remap: Unable to allocate memory for dead keys");
				fatal();
			}
			read_dk_map(dk_file, dk_buffer);
		}

		/* Map the keyboard and dead keys for each VT on the command line */
		for ( ; optind < argc; optind++) {
			strcpy(device, "/dev/");
			strncat(device, argv[optind], STR_LEN - strlen(device) - 1);
			if ((vt_fd = open(device, O_RDWR, 0)) < 0) {
				fprintf(stderr, "kb_remap: Unable to open %s: %s\n",
					device, strerror(errno));
				continue;
			}

			/* Reset the keyboard with the new mapping */
			if (ioctl(vt_fd, PIO_KEYMAP, &default_key_map.key_map) < 0)
			{
				fprintf(stderr, "kb_remap: PIO_KEYMAP ioctl failed on %s: %s\n",
					device, strerror(errno));
				close(vt_fd);
				continue;
			}

			/* Map the dead key information if we have any */
			if (dk_buffer && ioctl(vt_fd, LDSMAP, dk_buffer) < 0) {
				fprintf(stderr, "kb_remap: LDSMAP ioctl failed on %s: %s\n",
					device, strerror(errno));
				close(vt_fd);
				continue;
			}
			close(vt_fd);
		}
	}
	if (cs_flag) {

		/*
		 * Download the character set specified on the command line.  This
		 * character set is loaded into all VTs, whether they are open at the
		 * time or not.
		 */

		rom_font_t font_data;		/* Font remapping data */

		if (cs_flag == 2 )
			/* This forces the ioctl below
			 * to restore the ROM font
			 */
			font_data.fnt_numchar = 0;
		else
			read_font_map(codeset, &font_data);
		if ((kd_fd = open("/dev/kd/kdvm00", O_RDWR, 0)) < 0) {
			perror("kb_remap: Unable to open /dev/kd/kdvm00");
			fatal();
		}
		if (ioctl(kd_fd, WS_PIO_ROMFONT, &font_data) < 0) {
			perror("kb_remap: WS_PIO_ROMFONT ioctl failed on /dev/kd/kdvm00");
			fatal();
		}
		close(kd_fd);
	}
	if (lo_flag) {

	  /*  Set multibyte locale for the VTs specified on the command line.
	   */
	  if (argc == optind) {
		/* The user did not tell us which VTs to map. */
		fprintf(stderr, Usage);
		fatal();
	      }

	/* Get correct contents for eucioc structure for this locale */
	  geteucw(&eucw, locale);

	/* Set multibyte mode for each VT on the command line */

	  for ( ; optind < argc; optind++) {
		strcpy(device, "/dev/");
		strncat(device, argv[optind], STR_LEN - strlen(device) - 1);
		if ((vt_fd = open(device, O_RDWR, 0)) < 0) {
			fprintf(stderr, "kb_remap: Unable to open %s: %s\n",
				device, strerror(errno));
			continue;
		}

		/* send EUCSET ioctl */
		set_euc(vt_fd, &eucw);
		/* send VGA ioctl    */
		set_vga(vt_fd);
		close(vt_fd);
	      }
	}
	if (pr_flag) {
	    int i, j;
	    unsigned char c;

	    printf("\n               Current Loaded Fontmap\n\n");
	    printf("   | 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
	    printf("----------------------------------------------------\n");

	    for (i = 0; i < 16; i++)
	    {
		printf("%2x | ",i);
		fflush(stdout);
		for (j = 0; j < 16; j++)
		{
		    c = (j * 16) + i;
		    if ((c < 32) || (c == 0x7F) || (c == 0x9B))
		    {
			write(1," ",1);
		    } else {
			write(1, &c, 1);
		    }
		    write(1," ",1);
		    write(1," ",1);
		}
		write(1,"\n",1);
	    }
	    printf("\n");
	}
	return SUCCESS;
}

/*
 * Something went wrong.  As we're on the boot floppies, we can't do anything
 * particularly clever, so we have a common, 'tidy' error routine for all
 * cases.
 */
void
fatal(void)
{
	exit(FAILURE);
}
/*
 * get euc widths for this locale.  Right now, this is set up for
 * Japan only.
 */
void
geteucw(struct eucioc *w, char *locale)
{
        w->eucw[0] = '\001';
        w->scrw[0] = '\001';

        w->eucw[1] = '\002';
        w->scrw[1] = '\002';

        w->eucw[2] = '\001';
        w->scrw[2] = '\001';

        w->eucw[3] = '\002';
        w->scrw[3] = '\002';
}
/*
 * send euc ioctl to fd
 */
void
set_euc(int fd, struct eucioc *e)
{
        struct strioctl sb;

        sb.ic_cmd = EUC_WSET;
        sb.ic_timout = 0;
        sb.ic_len = sizeof(struct eucioc);
        sb.ic_dp = (char *) e;

        if (ioctl(fd, I_STR, &sb) < 0)
                fatal();
}
/*
 * send VGA640x480E ioctl to driver
 */
void
set_vga(fd)
{
        struct termios    cb;

	if (ioctl(fd, SW_VGA640x480E, &cb) == -1)
	       fatal();

}
