/*
 *	@(#)s3cConsts.h	6.1	3/20/96	10:23:06
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S023, 11-May-93, staceyc
 * 	include file cleanup
 * S022	Mon Apr 05 09:44:15 PDT 1993	hiramc@sco.COM
 *	Add Kevin's changes to support 3MB of video memory
 *	for the 86C801, 805, 928
 * X021 11-Jan-92 kevin@xware.com
 *      changed SPRITE_PAD to S3C_SPRITE_PAD, and added S3C_MAX_CURSOR_SAVE.
 * X020 08-Jan-92 kevin@xware.com
 *      added  S3C_AR10, S3C_AR10_16C, S3C_AR10_256C, and S3C_AR10_MASK.
 * X019 02-Jan-92 kevin@xware.com
 *      added S3C_CURSOR_TYPE_HW and S3C_CURSOR_TYPE_SW.
 * X018 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X017 31-Dec-91 kevin@xware.com
 *	added S3C_PALCNTL, S3C_PALCNTL_ENA16, and S3C_PALCNTL_MASK for 16bit
 *	64K color modes.
 * X016 14-Dec-91 kevin@xware.com
 *	added S3C_STAT_DATA for new macro S3C_WAIT_FOR_DATA() in s3cMacros.h.
 * X015 11-Dec-91 kevin@xware.com
 *	added defs for the screen off/on fix for 86c911 VGA register problem.
 * X014 08-Dec-91 kevin@xware.com
 *	embedded s3cReadCacheEntry() function into s3cDrawMonoGlyphs(),
 *	moved hash constants to global location, removed unused conts.
 * X013 05-Dec-91 kevin@xware.com
 *	cleaned up the earlier kludges to deal with byte long strides.
 * X012 03-Dec-91 kevin@xware.com
 *	changed name of S3C_QSTATADD to S3C_STAT, and added S3C_STAT_BUSY.
 * X011 01-Dec-91 kevin@xware.com
 *	added some defs for s3cMono.c and s3cStip.c.
 * X010 30-Nov-91 kevin@xware.com
 *	added some defs for s3cBres.c, and removed missed support for 8514a.
 * X009 29-Nov-91 kevin@xware.com
 *	removed support for 8514a.
 * X008 24-Nov-91 kevin@xware.com
 *	changed S3C_S3R2_MASK to preserve the emulation mode, and added 
 *	definitions for dealing with dual and single monitor configurations.
 * X007 23-Nov-91 kevin@xware.com
 *	added definitions for supporting 512K boards and some up and coming
 *	4 bit stuff.
 * X006 18-Nov-91 kevin@xware.com
 *	fixed radial and changed some others definitions for S3C_CMD.
 * X005 14-Nov-91 kevin@xware.com
 *	added more definitions for 86c911, mostly added VGA definition.
 * X004 08-Nov-91 kevin@xware.com
 *	added ifdef for kludge code for S3C_WRITE_X_Y_DATA for byte or word.
 * X003 04-Nov-91 kevin@xware.com
 *	changed S3C_CONVERT_WRITE to not convert for 86c911.
 * X002 03-Nov-91 kevin@xware.com
 *	fixed polarity of S3C_CMD_BYTE_DATA and S3C_CMD_WORD_DATA, and kludged
 *	S3C_WRITE_X_Y_DATA to use S3C_CMD_BYTE_DATA (temporary).
 * X001 23-Oct-91 kevin@xware.com
 *	added initial definitions for 86c911 development board.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 */

#ifndef _S3C_CONSTS_H
#define _S3C_CONSTS_H

#include <math.h>
/*
#include <sys/console.h>
*/
#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "grafinfo.h"
#include "ddxScreen.h"
#include "os.h"
/*
#include "compiler.h"
*/
#include "cursor.h"
#include "cursorstr.h"
#include "servermd.h"
#include "nfb/nfbGlyph.h"
#include "gen/genProcs.h"
#include "window.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbProcs.h"
#include "scoext.h"
#include	"input.h"
#include "mipointer.h"
#include "misprite.h"

/*
 * define standard VGA register definitions
 */

#define	S3C_MISCO_WR		0x03C2
#define	S3C_MISCO_RD		0x03CC

#define	S3C_MISCO_IOASEL	0x01
#define	S3C_MISCO_IOASEL_MONO	0x00
#define	S3C_MISCO_IOASEL_COLOR	0x01
#define	S3C_MISCO_ENRAM		0x02
#define	S3C_MISCO_CLKSEL	0x0C
#define	S3C_MISCO_CLKSEL_25	0x00
#define	S3C_MISCO_CLKSEL_28	0x04
#define	S3C_MISCO_CLKSEL_EXT	0x08
#define	S3C_MISCO_CLKSEL_ENH	0x0C
#define	S3C_MISCO_PAGESEL	0x20
#define	S3C_MISCO_SYNC		0xC0
#define	S3C_MISCO_SYNC_PH	0x60
#define	S3C_MISCO_SYNC_PV	0x80

#define S3C_MISCO_MASK		0xEE

#define	S3C_STAT1_RD_MONO	0x03BA
#define	S3C_STAT1_RD_COLOR	0x03DA
#define	S3C_STAT0_RD		0x03C2

#define	S3C_STAT1_VSYNC		0x08

#define	S3C_FC_WR_MONO		0x03BA
#define	S3C_FC_WR_COLOR		0x03DA
#define	S3C_FC_RD		0x03CA

#define	S3C_GRX			0x03CE
#define	S3C_GRD			0x03CF

#define	S3C_ARX			0x03C0
#define	S3C_ARD			0x03C0

#define	S3C_AR10		0x10

#define	S3C_AR0_PALDIS		0x00
#define	S3C_AR0_PALENA		0x20
#define	S3C_AR10_16C		0x00
#define	S3C_AR10_256C		0x40
#define	S3C_AR10_MASK		0x40

#define	S3C_SERX		0x03C4
#define	S3C_SERD		0x03C5

#define	S3C_SR0			0x00

#define	S3C_SR0_RESET		0x03
#define	S3C_SR0_RESET_SYN	0x01
#define	S3C_SR0_RESET_ASY	0x02
#define	S3C_SR0_RESET_RUN	0x03

#define	S3C_SR1			0x01

#define	S3C_SR1_SCREENOFF	0x20

#define	S3C_CRX_MONO		0x03B4
#define	S3C_CRX_COLOR		0x03D4
#define	S3C_CRD_MONO		0x03B5
#define	S3C_CRD_COLOR		0x03D5

#define	S3C_CR0E		0x0E
#define	S3C_CR0F		0x0F
#define	S3C_CR11		0x11
#define	S3C_CR13		0x13

/*
 * define S3 extended VGA register definitions
 */

#define	S3C_S3R0		0x30
#define	S3C_S3R1		0x31
#define	S3C_S3R2		0x32
#define	S3C_S3R3		0x33
#define	S3C_S3R4		0x34
#define	S3C_S3R5		0x35
#define	S3C_S3R6		0x36
#define	S3C_S3R7		0x37
#define	S3C_S3R8		0x38
#define	S3C_S3R9		0x39
#define	S3C_S3R0A		0x3A
#define	S3C_S3R0B		0x3B
#define	S3C_S3R0C		0x3C

#define	S3C_S3R1_STA		0x30
#define	S3C_S3R1_ENHMAP		0x08
#define	S3C_S3R1_1KMAP		0x00
#define	S3C_S3R1_2KMAP		0x02
#define	S3C_S3R1_MASK		0x3A
#define	S3C_S3R2_STADIS		0x00
#define	S3C_S3R2_STAENA		0x40
#define	S3C_S3R2_EMUMODE	0x08
#define	S3C_S3R2_EMULVGA	0x00
#define	S3C_S3R2_MASK		0x40
#define	S3C_S3R0_CHIPID		0xF0	/*	S022	*/
#define	S3C_S3R0_86C911		0x80	/*	S022	*/
#define	S3C_S3R0_86C80X		0xA0	/*	S022	*/
#define	S3C_S3R0_86C928		0x90	/*	S022	*/
#define	S3C_S3R4_ENBDTPC	0x10
#define	S3C_S3R4_MASK		0x10
#define	S3C_S3R5_CBA		0x0F
#define	S3C_S3R5_MASK		0x0F
#define	S3C_S3R6_DISPMEMSZ	0x60
#define	S3C_S3R6_DISPMEMSZ_512K	0x60
#define	S3C_S3R6_DISPMEMSZ_1M	0x40
#define	S3C_S3R6_DISPMEMSZ_2M	0x00	/*	S022	*/
#define	S3C_S3R8_KEY1		0x48
#define	S3C_S3R9_KEY2		0xA5
#define	S3C_S3R0A_MASK		0x10
#define	S3C_S3R0A_STD16		0x00
#define	S3C_S3R0A_ENH256	0x10

#define	S3C_SYS_CNFG		0x40
#define	S3C_MODE_CTL		0x42
#define	S3C_EXT_MODE		0x43
#define	S3C_HGC_MODE		0x45
#define	S3C_HGC_ORG_X1		0x46
#define	S3C_HGC_ORG_X0		0x47
#define	S3C_HGC_ORG_Y1		0x48
#define	S3C_HGC_ORG_Y0		0x49
#define	S3C_HGC_SRC_Y1		0x4C
#define	S3C_HGC_SRC_Y0		0x4D
#define	S3C_HGC_DX		0x4E
#define	S3C_HGC_DY		0x4F

#define	S3C_SYS_CNFG_8514	0x01
#define	S3C_SYS_CNFG_MASK	0x01
#define	S3C_MODE_CTL_DEFCLOCK	0x00
#define	S3C_MODE_CTL_DEFCLOCK16 0x04
#define	S3C_MODE_CTL_CLOCK	0x0F
#define	S3C_MODE_CTL_INTL	0x20
#define	S3C_MODE_CTL_MASK	0x2F
#define	S3C_EXT_MODE_DCKEDG	0x01
#define	S3C_EXT_MODE_DACRS2	0x02
#define	S3C_EXT_MODE_LSW8	0x04
#define	S3C_EXT_MODE_64K	0x08
#define	S3C_EXT_MODE_MASK	0x0F
#define	S3C_HGC_MODE_HWCENA	0x01
#define	S3C_HGC_MODE_DLYPAT	0x02
#define	S3C_HGC_MODE_MASK	0x03

/*
 * define pallete I/O addresses
 */

#define S3C_PALWRITE_ADDR 	0x03C8
#define S3C_PALREAD_ADDR 	0x03C7
#define S3C_PALMASK 		0x03C6
#define S3C_PALCNTL 		0x03C6
#define S3C_PALDATA 		0x03C9

#define S3C_PALCNTL_ENA16	0x80
#define S3C_PALCNTL_MASK	0x80

/*
 * define S3 enhanced register definitions
 */

#define S3C_HALFQBUF 		4
#define S3C_MAXPLANES 		8
#define S3C_ALLPLANES 		~((~0) << S3C_MAXPLANES)
#define S3C_RPLANES 		S3C_ALLPLANES
#define S3C_WPLANES 		S3C_ALLPLANES

#define S3C_CONTROL 		0x42E8
#define S3C_MISCIO 		0x4AE8
#define S3C_Y0			0x82E8
#define S3C_X0			0x86E8
#define S3C_Y1			0x8AE8
#define S3C_X1			0x8EE8
#define S3C_ERROR_ACC		0x92E8
#define S3C_LX			0x96E8
#define	S3C_CMD			0x9AE8
#define S3C_STAT 		0x9AE8
#define S3C_COLOR0		0xA2E8
#define S3C_COLOR1		0xA6E8
#define S3C_PLANE_WE		0xAAE8
#define S3C_PLANE_RE		0xAEE8
#define S3C_FUNC0		0xB6E8
#define S3C_FUNC1		0xBAE8
#define S3C_SEC_DECODE		0xBEE8
#define S3C_VARDATA 		0xE2E8

#define S3C_K1			S3C_Y1
#define S3C_K2			S3C_X1

#define	S3C_MISCIO_ENHDIS	0x0002
#define	S3C_MISCIO_ENHENA	0x0003
#define	S3C_MISCIO_HIRES	0x0004

#define S3C_CMD_NULL		0x0000 
#define S3C_CMD_LINE		0x2000 
#define S3C_CMD_FILL		0x4000
#define S3C_CMD_BLIT		0xC000
#define	S3C_CMD_BYTE_SWAP	0x1000
#define S3C_CMD_BYTE_DATA	0x0000
#define S3C_CMD_WORD_DATA	0x0200
#define S3C_CMD_NO_WAIT		0x0000
#define S3C_CMD_WAIT		0x0100
#define S3C_CMD_YP_XP_Y		0x00E0
#define S3C_CMD_YP_XN_Y		0x00C0
#define S3C_CMD_YP_XP_X		0x00A0
#define S3C_CMD_YP_XN_X		0x0080
#define S3C_CMD_YN_XP_Y		0x0060
#define S3C_CMD_YN_XN_Y		0x0040
#define S3C_CMD_YN_XP_X		0x0020
#define S3C_CMD_YN_XN_X		0x0000
#define S3C_CMD_DRAW		0x0010
#define S3C_CMD_MOVE		0x0000
#define S3C_CMD_RADIAL		0x0008
#define S3C_CMD_XY		0x0000
#define S3C_CMD_LASTPIX_OFF	0x0004
#define S3C_CMD_LASTPIX_ON	0x0000
#define S3C_CMD_MULTIPLE	0x0002
#define S3C_CMD_SINGLE		0x0000
#define S3C_CMD_READ		0x0000
#define S3C_CMD_WRITE		0x0001

#define	S3C_STAT_DATA		0x0100
#define	S3C_STAT_BUSY		0x0200

#define S3C_FNCOLOR0 		0x0000
#define S3C_FNCOLOR1 		0x0020
#define S3C_FNREPLACE 		0x0007
#define S3C_FNVAR 		0x0040
#define S3C_FNCPYRCT 		0x0060
#define S3C_FNNOP 		0x0003

#define S3C_M_ONES 		0xA000
#define S3C_M_DEPTH 		0xA000
#define S3C_M_CPYRCT 		0xA0C0
#define S3C_M_VAR 		0xA080

/*
 * define enhanced command definitions
 */


#define S3C_LINE_XN_YN_X	( S3C_CMD_LINE \
				| S3C_CMD_BYTE_DATA \
				| S3C_CMD_NO_WAIT \
				| S3C_CMD_YN_XN_X \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_OFF \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)

#define S3C_LINE_XN_YN_Y	( S3C_CMD_LINE \
				| S3C_CMD_BYTE_DATA \
				| S3C_CMD_NO_WAIT \
				| S3C_CMD_YN_XN_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_OFF \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)

#define S3C_FILL_X_Y_DATA	( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_WORD_DATA \
				| S3C_CMD_NO_WAIT \
				| S3C_CMD_YP_XP_X \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)

#define S3C_BLIT_XP_YP_Y	( S3C_CMD_BLIT \
				| S3C_CMD_BYTE_DATA \
				| S3C_CMD_NO_WAIT \
				| S3C_CMD_YP_XP_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)

#define S3C_CURS_X_Y_DATA	( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_WORD_DATA \
				| S3C_CMD_WAIT \
				| S3C_CMD_YP_XP_X \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_SINGLE \
				| S3C_CMD_WRITE)


#define S3C_WRITE_Z_DATA	( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_WORD_DATA \
				| S3C_CMD_WAIT \
				| S3C_CMD_YP_XP_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_SINGLE \
				| S3C_CMD_WRITE)

#define S3C_READ_Z_DATA         ( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_WORD_DATA \
				| S3C_CMD_WAIT \
				| S3C_CMD_YP_XP_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_SINGLE \
				| S3C_CMD_READ)

#define S3C_WRITE_X_Y_BYTE	( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_BYTE_DATA \
				| S3C_CMD_WAIT \
				| S3C_CMD_YP_XP_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)

#define S3C_WRITE_X_Y_DATA	( S3C_CMD_FILL \
				| S3C_CMD_BYTE_SWAP \
				| S3C_CMD_WORD_DATA \
				| S3C_CMD_WAIT \
				| S3C_CMD_YP_XP_Y \
				| S3C_CMD_DRAW \
				| S3C_CMD_XY \
				| S3C_CMD_LASTPIX_ON \
				| S3C_CMD_MULTIPLE \
				| S3C_CMD_WRITE)


#define	S3C_CURSOR_TYPE_HW	0
#define	S3C_CURSOR_TYPE_SW	1
#define S3C_MAX_CURSOR_SIZE 	64	/* for both hw and sw cursors	*/
#define	S3C_SPRITE_PAD		8	/* value from misprite.c	*/
#define	S3C_MAX_CURSOR_SAVE	(S3C_MAX_CURSOR_SIZE  + (2 * S3C_SPRITE_PAD))

#define S3C_GENERAL_OS_PLANE 	(1 << 0)  /* general off screen blt/cpy plane */
#define S3C_FONT_START_PLANE 	(1 << 1)  /* start of font storage 	*/
#define S3C_HWC_SOURCE_PITCH	1024	/* for both hw cursor only S022	*/

#define	VGA	5

/*
 * IMPORTANT: the S3C_HASH_TABLE_SIZE must be n to the power of 2 
 * and >= 4096 for the optimized the S3C_GL_HASH() macro to work. 
 */

#define	S3C_HASH_TABLE_SIZE	4096

#endif /* _S3C_CONSTS_H */

