#ident	"@(#)Space.c	1.5"

#include <config.h>
#include <sys/fd.h>

int	fd_door_sense = FD_DOOR_SENSE;
/*
 * fd_do_config_cmd controls whether the drive issues
 * a CONFIGURE command to enable the FIFO. Not all
 * controllers support the CONFIGURE command, and
 * should cleanly fail the command in these cases.
 * Just in case they do not, by setting this symbol
 * to 0, we can inhibit the driver from issueing the
 * CONFIGURE command.
 */
int	fd_do_config_cmd = 1;

/*
 * Floppy disk format table:
 *
 * New format types may be added to the end of this table. 
 * If a new format is added here the format type (FMT_??) should be
 * added to 'fd.h'. Also FMT_MAX must be increased by the number
 * of new format types added.
 * Table format:
 *
 *  bytes/sec | bytes-to-sec shift | secs/trk | drv types supported |
 *     normal gap length | format gap length | xfer rate | read type
 *
 */
struct fdsectab fdsectab[FDNSECT] = {
	{ 512,  9, 15, FD_5H,       0x1b, 0x54, FD500KBPS, FDTRK }, /* FMT_5H   */
	{ 512,  9,  9, FD_5D|FD_5H, 0x2a, 0x50, FD300KBPS, FDTRK }, /* FMT_5D9  */
	{ 512,  9,  8, FD_5D|FD_5H, 0x2a, 0x50, FD300KBPS, FDTRK }, /* FMT_5D8  */
	{ 1024, 10, 4, FD_5D|FD_5H, 0x80, 0xf0, FD300KBPS, FDTRK }, /* FMT_5D4  */
	{ 256,  8, 16, FD_5D|FD_5H, 0x20, 0x32, FD300KBPS, FDTRK }, /* FMT_5D16 */
	{ 512,  9,  9, FD_5H,       0x1b, 0x54, FD300KBPS, FDTRK }, /* FMT_5Q   */
	{ 512,  9,  9, FD_3H|FD_3E, 0x1b, 0x54, FD250KBPS, FDTRK }, /* FMT_3D   */
	{ 512,  9, 18, FD_3H|FD_3E, 0x1b, 0x54, FD500KBPS, FDTRK }, /* FMT_3H   */
	{ 512,  9, 36, FD_3E,       0x38, 0x53, FD1MBPS,   FDTRK }, /* FMT_3E   */
	{ 512,  9, 15, FD_3H,       0x1b, 0x54, FD500KBPS, FDTRK }, /* FMT_3M   */
	{ 1024, 10, 8, FD_3H,       0x35, 0x54, FD500KBPS, FDTRK }, /* FMT_3N   */
	{ 512,  9, 21, FD_3H|FD_3E, 0x1b, 0x54, FD500KBPS, FDSEC }, /* FMT_3P   */
};

int	Fmt_max = FMT_MAX; /* Maximum format types supported */

/* Tunable to enable/disable track buffer reads */
/* int	Fd_rdtrk = FDRDTRK; 
 * Disable for beta to solve RDTRK error default sector read errors.
 */
int	Fd_rdtrk = 0;
