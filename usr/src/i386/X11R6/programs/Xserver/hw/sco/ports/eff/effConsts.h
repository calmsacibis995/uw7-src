/*
 *	@(#) effConsts.h 11.1 97/10/22
 *
 * Modification History
 *
 * S013, 19-Jan-93, chrissc
 *	added defines for ATI clip registers.
 * S012, 04-Sep-92, staceyc
 * 	mods for text8 code
 * S011, 26-Sep-91, staceyc
 * 	save and restore VGA dac on 8514
 * S010, 28-Aug-91, staceyc
 * 	code cleanup and reworking of command queue
 * S009, 20-Aug-91, staceyc
 * 	added const that should later be made configurable
 * S008, 13-Aug-91, staceyc
 * 	constants for single plane off screen stage area
 * S007, 05-Aug-91, staceyc
 * 	add AP mode
 * S006, 01-Aug-91, staceyc
 * 	added some constants for hardware cursor drawing
 * S005, 28-Jun-91, staceyc
 * 	removed some constants that are now variables found in grafinfo
 * S004, 27-Jun-91, staceyc
 * 	added DAC shift value - this may become a variable later
 * S003, 21-Jun-91, staceyc
 * 	added some consts for Bres line drawing
 * S002, 21-Jun-91, staceyc
 * 	added some consts for blting
 * S001, 17-Jun-91, staceyc
 * 	get Draw/Read Image and cmap code happening
 * S000, 14-Jun-91, staceyc
 * 	created
 */

#ifndef _EFF_CONSTS_H
#define _EFF_CONSTS_H

#define EFF_HALFQBUF 4
#define EFF_MAXPLANES 8
#define EFF_ALLPLANES ~((~0) << EFF_MAXPLANES)
#define EFF_FNCOLOR0 0x0000
#define EFF_FNCOLOR1 0x0020
#define EFF_FNREPLACE 0x0007
#define EFF_FNVAR 0x0040
#define EFF_BYTE_ORDER 0x1000
#define EFF_FNCPYRCT 0x0060
#define EFF_FNNOP 0x0003
#define EFF_RPLANES EFF_ALLPLANES
#define EFF_WPLANES EFF_ALLPLANES

#define EFF_M_ONES 0xA000
#define EFF_M_DEPTH 0xA000
#define EFF_M_CPYRCT 0xA0C0
#define EFF_M_VAR 0xA080

#define EFF_MISCIO 0x4AE8
#define EFF_CONTROL 0x42E8
#define EFF_QSTATADD 0x9AE8
#define EFF_VARDATA 0xE2E8

#define EFF_PALWRITE_ADDR 0x02EC
#define EFF_PALREAD_ADDR 0x02EB
#define EFF_PALMASK 0x02EA
#define EFF_PALDATA 0x02ED

#define EFF_WAIT_ON_IO 0x4000                      /* This Does Wait IF busy */

#define EFF_SEC_DECODE		(0xBEE8 | EFF_WAIT_ON_IO)
#define EFF_COLOR0		(0xA2E8 | EFF_WAIT_ON_IO)
#define EFF_COLOR1		(0xA6E8 | EFF_WAIT_ON_IO)
#define EFF_PLANE_WE		(0xAAE8 | EFF_WAIT_ON_IO)
#define EFF_PLANE_RE		(0xAEE8 | EFF_WAIT_ON_IO)
#define EFF_FUNC0		(0xB6E8 | EFF_WAIT_ON_IO)
#define EFF_FUNC1		(0xBAE8 | EFF_WAIT_ON_IO)
#define EFF_X0			(0x86E8 | EFF_WAIT_ON_IO)
#define EFF_Y0			(0x82E8 | EFF_WAIT_ON_IO)
#define EFF_X1			(0x8EE8 | EFF_WAIT_ON_IO)
#define EFF_Y1			(0x8AE8 | EFF_WAIT_ON_IO)
#define EFF_LX			(0x96E8 | EFF_WAIT_ON_IO)
#define EFF_ERROR_ACC		(0x92E8 | EFF_WAIT_ON_IO)

#define EFF_K1			EFF_Y1
#define EFF_K2			EFF_X1

#define EFF_CMD_NULL		0x0000
#define EFF_CMD_VECTOR		0x2000
#define EFF_CMD_H_RECT		0x4000
#define EFF_CMD_V_RECT		0x6000
#define EFF_CMD_F_RECT		0x8000
#define EFF_CMD_OUTL_DRAW	0xA000
#define EFF_CMD_COPY_RECT	0xC000
#define EFF_CMD_WORD_DATA	0x0000
#define EFF_CMD_BYTE_DATA	0x0200
#define EFF_CMD_F_DATA		0x0000
#define EFF_CMD_V_DATA		0x0100
#define EFF_CMD_P_XP_YP_Z	0x00E0
#define EFF_CMD_P_XP_YN_Z	0x00C0
#define EFF_CMD_P_XN_YP_Z	0x00A0
#define EFF_CMD_P_XN_YN_Z	0x0080
#define EFF_CMD_N_XP_YP_Z	0x0060
#define EFF_CMD_N_XP_YN_Z	0x0040
#define EFF_CMD_N_XN_YP_Z	0x0020
#define EFF_CMD_N_XN_YN_Z	0x0000
#define EFF_CMD_DO_ACCESS	0x0010
#define EFF_CMD_MOVE_ONLY	0x0000
#define EFF_CMD_USE_C_DIR	0x0008
#define EFF_CMD_DONT_USE_C_DIR	0x0000
#define EFF_CMD_LAST_PEL_NULL	0x0004
#define EFF_CMD_USE_LAST_PEL	0x0000
#define EFF_CMD_ACCRUE_PELS	0x0002
#define EFF_CMD_NO_ACCRUE	0x0000
#define EFF_CMD_READ		0x0000
#define EFF_CMD_WRITE		0x0001

#define EFF_WRITE_Z_DATA	 (EFF_CMD_H_RECT \
				| EFF_BYTE_ORDER \
				| EFF_CMD_BYTE_DATA \
				| EFF_CMD_V_DATA \
				| EFF_CMD_P_XP_YP_Z \
				| EFF_CMD_DO_ACCESS \
				| EFF_CMD_DONT_USE_C_DIR \
				| EFF_CMD_USE_LAST_PEL \
				| EFF_CMD_NO_ACCRUE \
				| EFF_CMD_WRITE)
#define EFF_READ_Z_DATA           (EFF_CMD_H_RECT \
				| EFF_BYTE_ORDER \
				| EFF_CMD_BYTE_DATA \
				| EFF_CMD_V_DATA \
				| EFF_CMD_P_XP_YP_Z \
				| EFF_CMD_DO_ACCESS \
				| EFF_CMD_DONT_USE_C_DIR \
				| EFF_CMD_USE_LAST_PEL \
				| EFF_CMD_NO_ACCRUE \
				| EFF_CMD_READ)

#define EFF_WRITE_X_Y_DATA	 (EFF_CMD_H_RECT \
				| EFF_BYTE_ORDER \
				| EFF_CMD_BYTE_DATA \
				| EFF_CMD_V_DATA \
				| EFF_CMD_P_XP_YP_Z \
				| EFF_CMD_DO_ACCESS \
				| EFF_CMD_DONT_USE_C_DIR \
				| EFF_CMD_USE_LAST_PEL \
				| EFF_CMD_ACCRUE_PELS \
				| EFF_CMD_WRITE)

#define EFF_MAX_CURSOR_SIZE 64
#define SPRITE_PAD 8        /* Hack alert!  This value comes from misprite.c */

/*
 * expansion blit operations change the meaning of the read mask
 * bits - here's a macro to convert a regular write mask to an expansion
 * read mask - rotate writeplane by 1 bit - see Richter & Smith p 244 or
 * discussion on stretch blits in C&T manual in the 82C480 Pixel Operations
 * section
 */
#define EFF_CONVERT_WRITE(writeplane) \
	(((writeplane) << 1) | ((writeplane) >> 7))

#define EFF_GENERAL_OS_WRITE (1 << 0)  /* general off screen blit/copy plane */
#define EFF_FONT_START_WRITE (1 << 1)               /* start of font storage */
#define EFF_TEXT8_START_WRITE (1 << 3)
#define EFF_GENERAL_OS_READ EFF_CONVERT_WRITE(EFF_GENERAL_OS_WRITE)

#define EFF_SIMPLIFY_LIMIT 400
#define EFF_COMMAND_QUEUE_SIZE 32
#define EFF_DAC_SIZE 256

#define EFF_TEXT8_WIDTH 16
#define EFF_TEXT8_HEIGHT 32

#define EFF_SPARE 1
#define EFF_MONO_PLANES 2

#define ATI_EXT_SCISSOR_B 0xE6EE                                /* S013 */
#define ATI_EXT_SCISSOR_L 0xDAEE                                /* S013 */
#define ATI_EXT_SCISSOR_R 0xE2EE                                /* S013 */
#define ATI_EXT_SCISSOR_T 0xDEEE                                /* S013 */

#endif /* _EFF_CONSTS_H */

