#ifndef	_IO_AUTOCONF_CA_MCA_MCA_H	/* wrapper symbol for kernel use */
#define	_IO_AUTOCONF_CA_MCA_MCA_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/autoconf/ca/mca/mca.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define SYS_ENAB        0x94            /* System board enable / setup */
#define ADAP_ENAB       0x96            /* Adaptor board enable / setup */
#define POS_0           0x100           /* POS reg 0 - adaptor ID lsb */
#define POS_1           0x101           /* POS reg 1 - adaptor ID msb */
#define POS_2           0x102           /* Option Select Data byte 1 */
#define POS_3           0x103           /* Option Select Data byte 2 */
#define POS_4           0x104           /* Option Select Data byte 3 */
#define POS_5           0x105           /* Option Select Data byte 4 */
#define POS_6           0x106           /* Subaddress extension lsb */
#define POS_7           0x107           /* Subaddress extension msb */

#define MCA_HIER		2
#define NUM_SLOTS		8	/* Maximum number of slots */
#define POS_REGS_PER_SLOT	8	/* Number of POS regs per slot */

#define SYS_ENABLE_MASK		0x20 	/* Bits 7 and 3 0 and bit 5 1 */ 
					/* Other bits are undefined */

#define MCA_MAX_SLOTS		8	

typedef struct {
	ushort 	io_base;
	ushort 	intr;
	int 	dma_chan;
} mca_resource_t;

typedef struct mca_pos {
	unsigned char pos_reg;
	unsigned char pos_val;
} mca_pos_t;

#define MCA_BUFFER_SIZE		(MCA_MAX_SLOTS * sizeof(struct mca_pos))


#define MCA_GIVE_ID	0
#define MCA_READ_SLOT	1
#define MCA_SET_SLOT	2
#define MCA_SET_OFFSET  3
#define MCA_READ_POS	4
#define MCA_READ	5

typedef struct mca_arg {	/* Debugging purposes only */
	ushort mca_slot;
	ushort mca_id;
	unsigned char mca_pos;
	unsigned char mca_val;
} mca_arg_t;

#endif /* _IO_AUTOCONF_CA_MCA_MCA_H */


