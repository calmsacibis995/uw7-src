/*
 *	@(#)s3cScreen.c	6.1	3/20/96	10:23:35
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
 * S029, 04-Oct-93, staceyc
 * 	rework set graphics code in an attempt to get 512K cards working
 * S028, 30-Jun-93, staceyc
 * 	blindly turning on cursor in screen blanker causes grief in 8 bit
 *	modes for multiheaded cards
 * S027, 27-May-93, staceyc
 * 	enable direct memory access to framebuffer
 * S026, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S025, 11-May-93, staceyc
 * 	include file cleanup
 * S024, 05-Apr-93, staceyc
 * 	merge in code from Xware that deals with >2M
 * S023, 10-Nov-92, hiramc@sco.com
 *	Restore colormap in RestoreGState instead of in SetGraphics.
 *	Flag software cursor to be reloaded in RestoreGState.
 *	If new screen private flag, mode_assist, is not set,
 *	let the grafinfo SetGraphics and SetText functions
 *	do all the mode switching work.
 * S022, 05-Oct-92, hiramc@sco.com
 *	Remove call to nfbGCOpsCacheReset
 * S021, 29-Sep-92, hiramc@sco.com
 *	remove references to pScreen->CloseScreen
 *	Remove all mod comments before 1992
 * X020, 18-Jun-92, kevin@xware.COM
 *	added delays after chip has been programmed to allow the pixel clock
 *	to settle. This is per S3's product alert.
 * X019, 02-Jun-92, hiramc@sco.COM
 *	unwrap ScreenClose().
 * X018, 08-Jan-92, kevin@xware.com
 *      fixed a bug in s3cSetGraphics() for 4 bit display modes.
 * X017, 02-Jan-92, kevin@xware.com
 *      added support for mode selectable hardware or software cursor.
 * X016, 01-Jan-92, kevin@xware.com
 *      updated copyright notice.
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
#include <sys/console.h>
*/

/*
 * define default register values for enhanced and VGA modes
 */

#define	SER_COUNT	5
#define	CR_COUNT	25
#define	GR_COUNT	9
#define	AR_COUNT	20

static unsigned char 	s3cSerData[SER_COUNT] = 
{
	0x01, 0x21, 0x0F, 0x00, 
	0x0E 
};

static unsigned char	s3cCrtData[CR_COUNT] = 
{
	0x5F, 0x4F, 0x50, 0x82, 
	0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00,
	0xEA, 0x8C, 0xDF, 0x80, 
	0x60, 0xE7, 0x04, 0xAB, 
	0xFF
};

static unsigned char	s3cCrtData16[CR_COUNT] = 
{
	0xC3, 0x9F, 0xA1, 0x85, 
	0xA9, 0x01, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00,
	0xEA, 0x8C, 0xDF, 0x00, 
	0x60, 0xE7, 0x04, 0xAB, 
	0xFF
};

static unsigned char	s3cGrData[GR_COUNT] = 
{
	0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x05, 0x0F, 
	0xFF 
};
				       
static unsigned char	s3cAttData[AR_COUNT] = 
{ 
	0x00, 0x01, 0x02, 0x03, 
	0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B,
	0x0C, 0x0D, 0x0E, 0x0F,
	0x01, 0x00, 0x0F, 0x00 
};


/*
 *  s3cBlankScreen() -- Blank or Unblank the Screen
 *
 *	This routine will either blank or unblank the display.
 *
 */

Bool
S3CNAME(BlankScreen)(
	int			on, 
	ScreenPtr		pScreen)
{
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	/*
	 * wait for vsync before blanking/unblanking the video
	 */

	while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
		;
	while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
		;

	if ( on )
	{
		S3C_OUTB(S3C_PALMASK, 0x00);

		S3C_INB(s3cPriv->stat);
		S3C_OUTB(S3C_ARX, S3C_AR0_PALDIS); 
	}
	else
	{
		S3C_OUTB(S3C_PALMASK, 0xFF);

		S3C_INB(s3cPriv->stat);
		S3C_OUTB(S3C_ARX, S3C_AR0_PALENA); 
	}

	/*
	 * turn the hardware cursor on or off.
	 *
	 * NOTE: this is needed for the 16 bit display modes.
	 */

	if (s3cPriv->cursor_type == S3C_CURSOR_TYPE_HW && s3cPriv->depth == 16)
		S3CNAME(HWCursorOn)(on ? 0 : 1, pScreen);

	return TRUE;
}


/*
 *  s3cSetGraphics() --	Set Graphics Mode
 *
 *	This routine will initialize the hardware for the selected 
 *	grafinfo mode.
 *
 */

void
S3CNAME(SetGraphics)(
    ScreenPtr 		pScreen)
{
	int			i;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	/*
	 * wait for vsync before blanking the video
	 */

	while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
		;
	while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
		;

	S3C_OUTB(S3C_PALMASK, 0x00);

	/*
	 * if were in single monitor mode, tell the console driver to 
	 * go into a graphics mode, any VGA graphics mode will do.
	 */

	if ( s3cPriv->console_single && !s3cPriv->console_changed )
	{
		s3cPriv->console_changed = TRUE;
/*	ioctl(1, SW_VGA12, 0);*/
	}

	if (s3cPriv->mode_assist)
	{
		/*
		 * wait for vsync before turning the screen off 
		 * and then wait again for one entire vertical 
		 * period before programming the VGA registers.
		 * 
		 * WARNING: even thought waiting for vsync and 
		 * disabling the pallete may seem redundent
		 * it is neccesary because we don't know if
		 * make the call the the console driver above.
		 */
	
		while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
			;
		while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
			;
	
		S3C_INB(s3cPriv->stat);
		S3C_OUTB(S3C_ARX, S3C_AR0_PALDIS); 
	
		S3C_OUTB(S3C_SERX, S3C_SR1);
		S3C_OUTB(S3C_SERD, S3C_INB(S3C_SERD) | S3C_SR1_SCREENOFF);
	
		while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
			;
		while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )
			;
	
		/*
		 * program standard VGA registers to default values for enhanced
		 * graphics modes
		 */
	
		for ( i = 0 ; i < SER_COUNT ; i++ )
		{
			S3C_OUTB(S3C_SERX, i);
			S3C_OUTB(S3C_SERD, s3cSerData[i]);
		}	
	
		S3C_OUTB(s3cPriv->crx, S3C_CR11);
		S3C_OUTB(s3cPriv->crd, 0x00);
	
		if ( s3cPriv->depth != 16 )
		{
			for ( i = 0 ; i < CR_COUNT ; i++ )
			{
				S3C_OUTB(s3cPriv->crx, i);
				S3C_OUTB(s3cPriv->crd, s3cCrtData[i]);
			}
		}	
		else
		{
			for ( i = 0 ; i < CR_COUNT ; i++ )
			{
				S3C_OUTB(s3cPriv->crx, i);
				S3C_OUTB(s3cPriv->crd, s3cCrtData16[i]);
			}
		}	
	
		for ( i = 0 ; i < GR_COUNT ; i++ )
		{
			S3C_OUTB(S3C_GRX, i);
			S3C_OUTB(S3C_GRD, s3cGrData[i]);
		}	
	
		s3cAttData[S3C_AR10] = ( s3cAttData[S3C_AR10] & ~S3C_AR10_MASK )
			| ( s3cPriv->depth  == 4 ? S3C_AR10_16C : S3C_AR10_256C );
	
		S3C_INB(s3cPriv->stat);
		for ( i = 0 ; i < AR_COUNT ; i++ )
		{
			S3C_OUTB(S3C_ARX, i);
			S3C_OUTB(S3C_ARD, s3cAttData[i]);
		}	
	
		S3C_OUTB(S3C_MISCO_WR, ( S3C_INB(S3C_MISCO_RD) & ~S3C_MISCO_MASK )
		 	| S3C_MISCO_ENRAM
			| S3C_MISCO_CLKSEL_ENH
			| S3C_MISCO_PAGESEL
			| S3C_MISCO_SYNC_PH
			| S3C_MISCO_SYNC_PV );
	
		/*
		 * initialize the 86C911 for enhnaced graphics modes
		 * and call the grafinfo SetGraphics function
		 */
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R8);
		S3C_OUTB(s3cPriv->crd, S3C_S3R8_KEY1);
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R9);
		S3C_OUTB(s3cPriv->crd, S3C_S3R9_KEY2);
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R1);
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_S3R1_MASK )
			| S3C_S3R1_ENHMAP
			| ( s3cPriv->depth != 8 && s3cPriv->dispmem_size == 1024  
				? S3C_S3R1_2KMAP : S3C_S3R1_1KMAP ) );
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R2);
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_S3R2_MASK )
			| ( s3cPriv->depth != 8 && s3cPriv->dispmem_size == 1024  
				? S3C_S3R2_STAENA : S3C_S3R2_STADIS ) );
		
		S3C_OUTB(s3cPriv->crx, S3C_S3R4); 
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_S3R4_MASK )
			| S3C_S3R4_ENBDTPC );
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R5);
		S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R5_MASK );
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R0A); 
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_S3R0A_MASK )
			| ( s3cPriv->depth == 4 
				? S3C_S3R0A_STD16 : S3C_S3R0A_ENH256 ) );
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R0B); 
		S3C_OUTB(s3cPriv->crd, 
			( s3cPriv->depth != 16 
				? s3cCrtData[0] : s3cCrtData16[0] ) - 5 );
	
		S3C_OUTB(s3cPriv->crx, S3C_SYS_CNFG);
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_SYS_CNFG_MASK )
			| S3C_SYS_CNFG_8514 );
	
		S3C_OUTB(s3cPriv->crx, S3C_MODE_CTL);
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_MODE_CTL_MASK )
			| ( s3cPriv->depth != 16 
				? S3C_MODE_CTL_DEFCLOCK : S3C_MODE_CTL_DEFCLOCK16 ) );
	
		S3C_OUTB(s3cPriv->crx, S3C_EXT_MODE);
		S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK )
				| S3C_EXT_MODE_DACRS2 );
	
		if ( s3cPriv->depth != 16 )
		{
			S3C_OUTB(S3C_PALCNTL, 
				S3C_INB(S3C_PALCNTL) & ~S3C_PALCNTL_MASK);
	
			S3C_OUTB(s3cPriv->crd, 
				S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK);
		}
		else
		{
			S3C_OUTB(S3C_PALCNTL, 
				( S3C_INB(S3C_PALCNTL) & ~S3C_PALCNTL_MASK )
				| S3C_PALCNTL_ENA16 );
	
			S3C_OUTB(s3cPriv->crd, 
				( S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK )
				| S3C_EXT_MODE_64K 
				| S3C_EXT_MODE_LSW8
				| S3C_EXT_MODE_DCKEDG );
		}
	
		S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
		S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_MASK);
	
		S3C_OUT(S3C_MISCIO, S3C_MISCIO_ENHENA);
	}

	grafExec(s3cPriv->graf_data, "SetGraphics", NULL);

	if (s3cPriv->mode_assist)
	{
		if (s3cPriv->depth == 4 && s3cPriv->dispmem_size == 512 
			&& s3cPriv->adjust_cr13)
		{
			S3C_OUTB(s3cPriv->crx, S3C_CR13);
			S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) >> 1 );
		}
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R8);
		S3C_OUTB(s3cPriv->crd, ~S3C_S3R8_KEY1);
	
		S3C_OUTB(s3cPriv->crx, S3C_S3R9);
		S3C_OUTB(s3cPriv->crd, ~S3C_S3R9_KEY2);
	}

	/*						vvv	X020
	 * wait for 15 complete vsync periods (~250ms)
	 * before accessing the VRAM
	 */

	for ( i = 0; i < 15; i++ )
	{
		while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
			;
		while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
			;
	}					/*	^^^	X020	*/

	/*
	 * initialize graphics engine and clear the screen
	 */

	S3C_CLEAR_QUEUE(7);
	S3C_OUT(S3C_CONTROL, 0x9000);      
	S3C_OUT(S3C_CONTROL, 0x400F);    
	S3C_PLNWENBL(S3C_ALLPLANES);    
	S3C_PLNRENBL(S3C_ALLPLANES);   
	S3C_SETMODE(S3C_M_ONES);
	S3C_SETCOL0(0);
	S3C_SETCOL1(0);

	S3C_CLEAR_QUEUE(8);
	S3C_SETFN0(S3C_FNCOLOR0, S3C_FNREPLACE);
	S3C_SETFN1(S3C_FNCOLOR1, S3C_FNREPLACE);
	S3C_SETXMIN(0);               
	S3C_SETYMIN(0);              
	S3C_SETXMAX(s3cPriv->card_clip.x); 
	S3C_SETYMAX(s3cPriv->card_clip.y);
	S3C_SETX0(0);
	S3C_SETY0(0);

	S3C_CLEAR_QUEUE(3)
	S3C_SETLX(s3cPriv->card_clip.x); 
	S3C_SETLY(s3cPriv->card_clip.y);

	S3C_COMMAND(S3C_FILL_X_Y_DATA);

	/*
	 * start the sequencer and wait for 
	 * one complete vsync  period before
	 * turning the screen back on and
	 * enabling the video
	 */

	S3C_OUTB(S3C_SERX, S3C_SR0);
	S3C_OUTB(S3C_SERD, S3C_SR0_RESET_RUN);

	for ( i = 0; i < 2; i++ )	/*	vvv	X020	*/
	{
		while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
			;
		while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
			;
	}				/*	^^^	X020	*/

	S3C_OUTB(S3C_SERX, S3C_SR1);
	S3C_OUTB(S3C_SERD, S3C_INB(S3C_SERD) & ~S3C_SR1_SCREENOFF);

	S3C_OUTB(S3C_PALMASK, 0xFF);

	S3C_INB(s3cPriv->stat);
	S3C_OUTB(S3C_ARX, S3C_AR0_PALENA); 
}

/*
 *  S3CNAME(SetText)() --	Set Text Mode
 *
 *	This routine will reinitialize the hardware for the VGA text
 *	modes.
 *
 */

void
S3CNAME(SetText)(
    ScreenPtr 		pScreen)
{
    int			i;
    s3cPrivateData_t 	*s3cPriv = S3C_PRIVATE_DATA(pScreen);

    S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);
    /*
     * make sure the command Q is empty, and wait for any
     * pending command to complete
     */

    S3C_CLEAR_QUEUE(8);
    while ( S3C_INW(S3C_STAT) & S3C_STAT_BUSY )
	;	
	
    /* S023
     * assist the grafinfo SetText function only if needed.
     */

    if ( s3cPriv->mode_assist )
    {
	/*
	 * wait for vsync and blank the video output
	 */

	while ( !( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC ) )
	    ;
	while ( S3C_INB(s3cPriv->stat) & S3C_STAT1_VSYNC )	
	    ;

	S3C_OUTB(S3C_PALMASK, 0x00);

	S3C_INB(s3cPriv->stat);
	S3C_OUTB(S3C_ARX, S3C_AR0_PALDIS); 

	/*
	 * reset the 86C911 back to VGA text mode
	 */

	S3C_OUT(S3C_MISCIO, S3C_MISCIO_ENHDIS);

	S3C_OUTB(s3cPriv->crx, S3C_S3R8);
	S3C_OUTB(s3cPriv->crd, S3C_S3R8_KEY1);

	S3C_OUTB(s3cPriv->crx, S3C_S3R9);
	S3C_OUTB(s3cPriv->crd, S3C_S3R9_KEY2);

	S3C_OUTB(s3cPriv->crx, S3C_S3R1);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R1_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_S3R2);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R2_MASK);
	
	S3C_OUTB(s3cPriv->crx, S3C_S3R4); 
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R4_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_S3R5);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R5_MASK );

	S3C_OUTB(s3cPriv->crx, S3C_S3R0A); 
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_S3R0A_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_SYS_CNFG);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_SYS_CNFG_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_MODE_CTL);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_MODE_CTL_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_EXT_MODE);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_EXT_MODE);
	S3C_OUTB(s3cPriv->crd, ( S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK )
			| S3C_EXT_MODE_DACRS2 );
	S3C_OUTB(S3C_PALCNTL, S3C_INB(S3C_PALCNTL) & ~S3C_PALCNTL_MASK);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_EXT_MODE_MASK);

	S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_MASK);
    }   /* end if mode_assist */

    grafExec(s3cPriv->graf_data, "SetText", NULL);

    /* S023
     * assist the grafinfo SetText function only if needed.
     */

    if ( s3cPriv->mode_assist )
    {
	S3C_OUTB(s3cPriv->crx, S3C_S3R8);
	S3C_OUTB(s3cPriv->crd, ~S3C_S3R8_KEY1);

	S3C_OUTB(s3cPriv->crx, S3C_S3R9);
	S3C_OUTB(s3cPriv->crd, ~S3C_S3R9_KEY2);
    }   /* end if mode_assist */

    /*
     * if we're in single monitor enable the video, else disable it
     */

    S3C_OUTB(S3C_PALMASK, s3cPriv->console_single ? 0xFF : 0x00);

    s3cPriv->console_changed = FALSE;
}

/*
 *  s3cSaveGState() --	Save Graphics State
 *
 *	This routine will save the current graphics state.
 *
 */

void
S3CNAME(SaveGState)(
	ScreenPtr	pScreen)
{
	/*
	 * flush glyph cache in case another server is running
	 * this means the cache will have to be rebuilt by demand
	 * if/when the user switches back
	 */

	S3CNAME(GlFlushCache)(pScreen);
}


/*
 *  s3cRestoreGState() -- Restore Graphics State
 *
 *	This routine will restore the current graphics state.
 *
 */

void
S3CNAME(RestoreGState)(
	ScreenPtr	pScreen)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pScreen);	/* S023 vvv */

	/* reload software cursor the next time it is put up */
	if ( s3cPriv->cursor_type == S3C_CURSOR_TYPE_SW )
		s3cPriv->sw_cursor.current_cursor = 0;

	/* restore the current colormap */
	S3CNAME(RestoreColormap)(pScreen);				/* S023 ^^^ */
}


/*
 *  s3cCloseScreen() --	Close Screen
 *
 *	This routine will free all allocated memory and close the display.
 *
 */

Bool
S3CNAME(CloseScreen)(
	int 		index,
	ScreenPtr 	pScreen)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pScreen);

	/*	X019	Unwrap	S020 CloseScreen wrap removed	*/

	pScreen->QueryBestSize = s3cPriv->QueryBestSize; /*	X019	*/

	S3CNAME(GlCacheClose)(pScreen);             
	Xfree((char *)s3cPriv);       
						/*	S022	*/

	return TRUE;	/*	S020 CloseScreen call removed	*/
}

