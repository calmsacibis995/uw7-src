#ident	"@(#)ws_tcl.c	1.8"
#ident	"$Header$"

/*
 * Terminal Control Language (TCL)
 */

#include <io/ascii.h>
#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>



/*
 * void
 * tcl_handler(wstation_t *, mblk_t *, termstate_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_handler(wstation_t *wsp, mblk_t *mp, termstate_t *tsp, 
		ws_channel_t *chp)
{
	ch_proto_t	*protop;
	mblk_t		*bp;


	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	switch (protop->chp_stype_cmd) {
	case TCL_CURS_TYP: {
		ushort type;
		type = protop->chp_stype_arg;
		if (type == tsp->t_curtyp)
			break;
		tsp->t_curtyp = type;
		(*wsp->w_consops->cn_cursortype)(wsp, chp, tsp);
		break;
	  }

	case TCL_BELL:
		(*wsp->w_consops->cn_bell)(wsp, chp);
		break;

	case TCL_BACK_SPCE:
		tcl_bs(wsp->w_consops, chp, tsp);
		break;

	case TCL_NEWLINE:
	case TCL_V_TAB:
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			tcl_scrlup(wsp->w_consops, chp, tsp);
		}
		else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols;
		}
		(*wsp->w_consops->cn_setcursor)(chp, tsp);
		break;

	case TCL_CRRGE_RETN:
		tsp->t_cursor -= tsp->t_col;
		tsp->t_col = 0;
		(*wsp->w_consops->cn_setcursor)(chp, tsp);
		break;

	case TCL_H_TAB:
		tcl_ht(wsp->w_consops, chp, tsp);
		break;

	case TCL_BACK_H_TAB:
		tcl_bht(wsp->w_consops, chp, tsp);
		break;

	case TCL_KEYCLK_ON:
		wsp->w_flags |= WS_KEYCLICK;
		break;

	case TCL_KEYCLK_OFF:
		wsp->w_flags &= ~WS_KEYCLICK;
		break;

	case TCL_CLR_TAB:
		tsp->t_ppar[0] = 0;
		tcl_clrtabs(tsp);
		break;

	case TCL_CLR_TABS:
		tsp->t_ppar[0] = 3;
		tcl_clrtabs(tsp);
		break;

	case TCL_SET_TAB:
		tcl_settab(tsp);
		break;

	case TCL_SHFT_FT_OU:
		(*wsp->w_consops->cn_shiftset)(wsp, chp, 0); 
		break;

	/* There is no need to call cn_shiftset function for TCL_MB_SHFT_FT_IN
	 * as it is for a multibyte console. When a multibyte console is setup
	 * the gclhandler function is called in the kd driver instead of the
	 * tclhandler. Please see tcl.h and ansi.c for more info.
	 */
	case TCL_SHFT_FT_IN:
		(*wsp->w_consops->cn_shiftset)(wsp, chp, 1); 
		break;

	case TCL_MB_SHFT_FT_IN:
		break;

	case TCL_SCRL_UP:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		tcl_scrlup(wsp->w_consops, chp, tsp);
		break;

	case TCL_SCRL_DWN:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		tcl_scrldn(wsp->w_consops, chp, tsp);
		break;

	case TCL_LCK_KB:
		wsp->w_flags |= WS_LOCKED;
		break;
		
	case TCL_UNLCK_KB:
		wsp->w_flags &= ~WS_LOCKED;
		break;

	case TCL_SET_ATTR:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		tsp->t_pnum = 1;
		tcl_sfont(wsp, chp, tsp);
		break;

	case TCL_POS_CURS: {
		tcl_data_t *dp;
		short x, y;

		if ( (bp = mp->b_cont) == (mblk_t *) NULL) {
			/*
			 *+ An unexpected end of message when continuation
			 *+ is expected for TCP_POS_CURS cmd.
			 */
			cmn_err(CE_WARN, 
				"tcl_handler: unexpected end of msg");
			return;
		}

		/* LINTED pointer alignment */
		dp = (tcl_data_t *)bp->b_rptr;

		x = dp->mv_curs.delta_x;
		y = dp->mv_curs.delta_y;

		if (dp->mv_curs.x_type == TCL_POSABS)
			tsp->t_col = x;
		else
			tsp->t_col += x;

		if (dp->mv_curs.y_type == TCL_POSABS)
			tsp->t_row = y;
		else
			tsp->t_row += y;

		if (tsp->t_col < 0) tsp->t_col = 0;
		if (tsp->t_row < 0) tsp->t_row = 0;
		
		tcl_cursor(wsp->w_consops, chp);
		break;
	} /* TCL_POS_CURS */

	case TCL_DISP_CLR:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		(*wsp->w_consops->cn_clrscr)(chp, tsp->t_cursor, tsp->t_ppar[0]);
		break;

	case TCL_DISP_RST:
		tcl_reset(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERASCR_CUR2END:
		tsp->t_ppar[0] = 0;
		tcl_escr(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERASCR_BEG2CUR:
		tsp->t_ppar[0] = 1;
		tcl_escr(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERASCR_BEG2END:
		tsp->t_ppar[0] = 2;
		tcl_escr(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERALIN_CUR2END:
		tsp->t_ppar[0] = 0;
		tcl_eline(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERALIN_BEG2CUR:
		tsp->t_ppar[0] = 1;
		tcl_eline(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERALIN_BEG2END:
		tsp->t_ppar[0] = 2;
		tcl_eline(wsp->w_consops, chp, tsp);
		break;

	case TCL_INSRT_LIN:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		tcl_iline(wsp->w_consops, chp, tsp);
		break;

	case TCL_DELET_LIN:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		tcl_dline(wsp->w_consops, chp, tsp);
		break;

	case TCL_INSRT_CHR:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		tcl_ichar(wsp->w_consops, chp, tsp);
		break;

	case TCL_DELET_CHR:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		tcl_dchar(wsp->w_consops, chp, tsp);
		break;

	/* Enhanced Application Compatibility Support */

        case TCL_SAVE_CURSOR:
                tsp->t_saved_row = tsp->t_row;
                tsp->t_saved_col = tsp->t_col;
                break;

        case TCL_RESTORE_CURSOR:
                tsp->t_row = tsp->t_saved_row;
                tsp->t_col = tsp->t_saved_col;
                tcl_cursor(wsp->w_consops, chp);
                break;

        case TCL_AUTO_MARGIN_ON:
                tsp->t_auto_margin = AUTO_MARGIN_ON;
                break;

        case TCL_AUTO_MARGIN_OFF:
                tsp->t_auto_margin = AUTO_MARGIN_OFF;
                break;

        case TCL_SET_FONT_PROPERTIES: {
                unchar  attr = tsp->t_attrmskp[12].attr;

                attr &= 0x88;
                tsp->t_gfcolor = (protop->chp_stype_arg & 0xf);
                tsp->t_gbcolor = ((protop->chp_stype_arg >> 16) & 0xf);
                attr |= tsp->t_gfcolor;
                attr |= ((tsp->t_gbcolor << 4) & 0x70);
                tsp->t_attrmskp[12].attr = attr;
                break;
	}

        case TCL_PRINT_FONTCHAR:
                if (chp->ch_dmode != KD_GRAPHICS) {
                        unchar  font =  tsp->t_font;

			/* set fg/bg colors to default graphics fg/bg */
                        tsp->t_ppar[0] = 12;
                        tsp->t_pnum = 1;
                        tcl_sfont(wsp, chp, tsp);
                        tcl_norm(wsp->w_consops, chp, tsp, protop->chp_stype_arg);
			/* set fg/bg colors back to normal fg/bg */
                        tsp->t_ppar[0] = 0;
                        tsp->t_pnum = 1;
                        tcl_sfont(wsp, chp, tsp);
                        tsp->t_font = font;
                }
                break;

        case TCL_SET_OVERSCAN_COLOR:
                (void ) (*wsp->w_consops->cn_setborder)(chp, protop->chp_stype_arg);
                break;

        case TCL_SET_BELL_PARAMS:
                tsp->t_bell_freq =  (protop->chp_stype_arg & 0x7fff);
                tsp->t_bell_time =  (protop->chp_stype_arg >> 16) & 0x7fff;
                break;

        case TCL_SET_CURSOR_PARAMS:
                /*
                kdv_setcursor_type(protop->chp_stype_arg);
                */
                break;

        case TCL_NOBACKBRITE:
                if (protop->chp_stype_arg == 0)
                        tsp->t_flags |= T_NOBACKBRITE;
                else    
			tsp->t_flags &= ~T_NOBACKBRITE;
                break;

        case TCL_FORGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[0].attr;

                tsp->t_nfcolor = (protop->chp_stype_arg & 0xf);
                tsp->t_curattr &= 0xf0;
                tsp->t_curattr |= tsp->t_nfcolor;
                attr &= 0xf0;
                attr |= tsp->t_nfcolor;
                tsp->t_attrmskp[0].attr = attr;
                break;
	}

        case TCL_BCKGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[0].attr;

                tsp->t_nbcolor = (protop->chp_stype_arg & 0xf);
                tsp->t_curattr &= 0xf;
                tsp->t_curattr |= ((tsp->t_nbcolor << 4) & 0xf0);
                attr &= 0xf;
                attr |= ((tsp->t_nbcolor << 4) & 0xf0);
                tsp->t_attrmskp[0].attr = attr;
                break;
	}

        case TCL_RFORGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[7].attr;

                tsp->t_rfcolor = (protop->chp_stype_arg & 0xf);
                attr &= 0xf0;
                attr |= tsp->t_rfcolor;
                tsp->t_attrmskp[7].attr = attr;
                break;
	}

        case TCL_RBCKGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[7].attr;

                tsp->t_rbcolor = (protop->chp_stype_arg &0xf);
                attr &= 0xf;
                attr |= ((tsp->t_rbcolor<<4)&0xf0);
                tsp->t_attrmskp[7].attr = attr;
                break;
	}

        case TCL_GFORGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[12].attr;

                tsp->t_gfcolor = (protop->chp_stype_arg & 0xf);
                attr &= 0xf0;
                attr |= tsp->t_gfcolor;
                tsp->t_attrmskp[12].attr = attr;
                break;
	}

        case TCL_GBCKGRND_COLOR: {
                unchar  attr = tsp->t_attrmskp[12].attr;

                tsp->t_gbcolor = (protop->chp_stype_arg & 0xf);
                attr &= 0xf;
                attr |= ((tsp->t_gbcolor << 4) & 0xf0);
                tsp->t_attrmskp[12].attr = attr;
                break;
	}

	/* End Enhanced Application Compatibility Support */

	default:
		/*
		 *+ An illegal TCL channel protocol subtype command 
		 *+ is sent from upstream.
		 */
		cmn_err(CE_WARN, 
			"tcl_handler: do not understand TCL %d", 
			protop->chp_stype_cmd);
		break;
	}
}


/*
 * void 
 * tcl_bs(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	tcl backspace.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_bs(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	if (!tsp->t_col)
		return;
	tsp->t_col--;
	tsp->t_cursor--;
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void 
 * tcl_ht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
 *	tcl horizontal tab.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_ht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	unchar	tabindex = 0;
	ushort	tabval;


	do {
                if (tabindex >= tsp->t_ntabs)  {
                        tabval = tsp->t_cols;
                        break;
                }
                tabval = tsp->t_tabsp[tabindex];
                tabindex++;
	} while (tabval <= tsp->t_col);

	if (tabval >= tsp->t_cols) {
		if (tsp->t_row == (tsp->t_rows - 1)) {
			tsp->t_ppar[0] = 1;
			tcl_scrlup(cnops, chp, tsp);
			tsp->t_cursor -= tsp->t_col;
		} else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols - tsp->t_col;
		}
		tsp->t_col = 0;
	} else {
		tabval -= tsp->t_col;
		tsp->t_cursor += tabval;
		tsp->t_col += tabval;
	}
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void
 * tcl_reset(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	reset/clear the console device.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_reset(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	tsp->t_origin = 0;
	tsp->t_curattr = tsp->t_normattr;
	(*cnops->cn_clrscr)(chp, tsp->t_origin, tsp->t_scrsz);
	tsp->t_row = 0; /* clear all state variables */
	tsp->t_col = 0;
	tsp->t_cursor = 0;
	tsp->t_pstate = 0;
	(*cnops->cn_setbase)(chp, tsp);
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void
 * tcl_bht(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	back horizontal tab.
 *	
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_bht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	int	tabindex = tsp->t_ntabs; 
	ushort	tabval;


	do {
		if (--tabindex < 0) {
			tabval = 0;
			break;
		}
		tabval = tsp->t_tabsp[tabindex];
	} while (tabval >= tsp->t_col);
	if (tabval < tsp->t_tabsp[0]) {
		tsp->t_cursor -=  tsp->t_col;
		tsp->t_col = 0;
	} else {
		tabval = tsp->t_col - tabval;
		tsp->t_cursor -= tabval;
		tsp->t_col -= tabval;
	}
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void
 * tcl_norm(kdcnops_t *, ws_channel_t *, termstate_t *, ushort)
 *	display normal text.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_norm(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp, ushort ch)
{
	ushort	dch;	/* "display" character (character plus attribute) */
	ushort	attr;


        attr = tsp->t_curattr;
	if ((tsp->t_flags & T_NOBACKBRITE) && (tsp->t_flags & T_BACKBRITE))
		attr &= ~BLINK;
        dch = ((attr << 8) | ch);

        (*cnops->cn_stchar)(chp, tsp->t_cursor, dch, 1);

	/* Enhanced Application Compatibility Support */

        if(tsp->t_auto_margin != AUTO_MARGIN_ON &&
			tsp->t_col == (tsp->t_cols - 1)) {
		tsp->t_col--;
		tsp->t_cursor--;
	}

	/* End Enhanced Application Compatibility Support */

	if (tsp->t_col == tsp->t_cols - 1) {
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			tcl_scrlup(cnops, chp, tsp);
			tsp->t_cursor -= tsp->t_col;
		} else {
			tsp->t_row++;
			tsp->t_cursor++;
		}
		tsp->t_col = 0;
	} else {
		tsp->t_col++;
		tsp->t_cursor++;
	}
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void
 * tcl_cursor(kdcnops_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_cursor(kdcnops_t *cnops, ws_channel_t *chp)
{
	ushort		row, col;
	termstate_t	*tsp = &chp->ch_tstate;


	row = tsp->t_row;
	col = tsp->t_col;
/* XXX
	while (row < 0)
		row += tsp->t_rows;
*/
	while (row >= tsp->t_rows)
		row -= tsp->t_rows;
	tsp->t_row = row;
/* XXX
	while (col < 0)
		col += tsp->t_cols;
*/
	while (col >= tsp->t_cols)
		col -= tsp->t_cols;
	tsp->t_col = col;
	tsp->t_cursor = row * tsp->t_cols + col + tsp->t_origin;
	(*cnops->cn_setcursor)(chp, tsp);
}


/*
 * void
 * tcl_scrlup(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	scroll up
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_scrlup(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	if (tsp->t_flags & T_ANSIMVBASE) {
		ushort	last;
		int	count;

		count = tsp->t_cols * tsp->t_ppar[0];
		last = tsp->t_origin + tsp->t_scrsz;
		tsp->t_origin += count;
		(*cnops->cn_clrscr)(chp, last, count);
		(*cnops->cn_setbase)(chp, tsp);
		tcl_cursor(cnops, chp);
	} else {
		 ushort	row, col, cursor;

		row = tsp->t_row;
		col = tsp->t_col;
		cursor = tsp->t_cursor;
		tsp->t_cursor = tsp->t_origin;
		tsp->t_row = tsp->t_col = 0;
		tcl_dline(cnops, chp, tsp);
		tsp->t_row = row;
		tsp->t_col = col;
		tsp->t_cursor = cursor;
	}
}


/*
 * void
 * tcl_scrldn(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	scroll down
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_scrldn(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	short	row, col;
	ushort	cursor;


	if (tsp->t_flags & T_ANSIMVBASE) {
		int	count;

		count = tsp->t_cols * tsp->t_ppar[0];
		tsp->t_origin -= count;
		(*cnops->cn_clrscr)(chp, tsp->t_origin, count);
		(*cnops->cn_setbase)(chp, tsp);
		tcl_cursor(cnops, chp);
	} else {
		/*
		 * Save row, col, and cursor.
		 */
		row = tsp->t_row;
		col = tsp->t_col;
		cursor = tsp->t_cursor;
		/*
		 * Get tcl_iline to do the work.
		 */
		tsp->t_cursor = tsp->t_origin;
		tsp->t_row = tsp->t_col = 0;
		tcl_iline(cnops, chp, tsp);
		/*
		 * Restore row, col, and cursor.
		 */
		tsp->t_row = row;
		tsp->t_col = col;
		tsp->t_cursor = cursor;
	}
}


/*
 * void
 * tcl_dchar(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	delete a character.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_dchar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort	from, last;
	int	cnt;


	if (!tcl_ckchar(cnops, chp, tsp))
		return;
	from = tsp->t_cursor + tsp->t_ppar[0];
	cnt = tsp->t_cols - tsp->t_col - tsp->t_ppar[0];
	last = tsp->t_cursor - tsp->t_col + tsp->t_cols - tsp->t_ppar[0];
	(*cnops->cn_mvword)(chp, from, tsp->t_cursor, cnt, DOWN);
	(*cnops->cn_clrscr)(chp, last, tsp->t_ppar[0]);
}


/*
 * void
 * tcl_dline(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *	delete a line.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_dline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp) 
{
	ushort	from, to, last;
	int	cnt;


	if (!tcl_ckline(cnops, chp, tsp))
		return;
	to = tsp->t_cursor - tsp->t_col;
	from = to + tsp->t_ppar[0] * tsp->t_cols;
	cnt = (tsp->t_rows - tsp->t_row - tsp->t_ppar[0]) * tsp->t_cols;
	(*cnops->cn_mvword)(chp, from, to, cnt, DOWN);
	last = to + cnt;
	cnt = tsp->t_ppar[0] * tsp->t_cols;
	(*cnops->cn_clrscr)(chp, last, cnt);
}


/*
 * void
 * tcl_iline(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *	insert a line.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_iline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp) 
{
	ushort	from, to;
	int	cnt;


	if (!tcl_ckline(cnops, chp, tsp))
		return;
	from = tsp->t_cursor - tsp->t_col;
	to = from + tsp->t_ppar[0] * tsp->t_cols;
	cnt = (tsp->t_rows - tsp->t_row - tsp->t_ppar[0]) * tsp->t_cols;
	(*cnops->cn_mvword)(chp, from, to, cnt, UP);
	(*cnops->cn_clrscr)(chp, from, tsp->t_ppar[0] * tsp->t_cols);
}


/*
 * void
 * tcl_eline(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *	erase line.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_eline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort	last = tsp->t_cursor - tsp->t_col;


	switch (tsp->t_ppar[0]) {
	case 0:	/* erase cursor to end of line */
		(*cnops->cn_clrscr)(chp, tsp->t_cursor, tsp->t_cols - tsp->t_col);
		break;

	case 1:	/* erase beginning of line to cursor */
		(*cnops->cn_clrscr)(chp, last, tsp->t_col + 1);
		break;

	default:	/* erase whole line */
		(*cnops->cn_clrscr)(chp, last, tsp->t_cols);
	}
}


/*
 * void
 * tcl_escr(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_escr(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	int	cnt = tsp->t_cursor - tsp->t_origin;


	switch (tsp->t_ppar[0]) {
	case 0:	/* erase cursor to end of screen */
		(*cnops->cn_clrscr)(chp, tsp->t_cursor, tsp->t_scrsz - cnt);
		break;

	case 1:	/* erase beginning of screen to cursor */
		(*cnops->cn_clrscr)(chp, tsp->t_origin, (cnt + 1));
		break;

	default:	/* erase whole screen */
		(*cnops->cn_clrscr)(chp, tsp->t_origin, tsp->t_scrsz);
		break;
	}
}


/*
 * void 
 * tcl_clrtabs(termstate_t *)
 *	clear tabs.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_clrtabs(termstate_t *tsp)
{
	unchar cnt1, cnt2;


	if (tsp->t_ppar[0] == 3) { /* clear all tabs */
		tsp->t_ntabs = 0;
		return;
	}
	if (tsp->t_ppar[0] || !tsp->t_ntabs)
		return;
	for (cnt1 = 0; cnt1 < tsp->t_ntabs; cnt1++) {
		if (tsp->t_tabsp[cnt1] == tsp->t_col) {
			for (cnt2 = cnt1; cnt2 < (unchar)(tsp->t_ntabs-1); cnt2++)
				tsp->t_tabsp[cnt2] = tsp->t_tabsp[cnt2 + 1];
			tsp->t_ntabs--;
			return;
		}
	}
}


/*
 * void
 * tcl_ichar(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	insert a character.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_ichar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort to, cnt;


	to = tsp->t_cursor + tsp->t_ppar[0];
	cnt = tsp->t_cols - tsp->t_col - tsp->t_ppar[0];

	if (!tcl_ckchar(cnops, chp, tsp))
		return;
	(*cnops->cn_mvword)(chp, tsp->t_cursor, to, cnt, UP);
	(*cnops->cn_clrscr)(chp, tsp->t_cursor, tsp->t_ppar[0]);
}


/*
 * void
 * tcl_sparam(termstate_t *, int, ushort)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_sparam(termstate_t *tsp, int paramcnt, ushort newparam)
{
	int	cnt;


	for (cnt = 0; cnt < paramcnt; cnt++) {
		if (tsp->t_ppar[cnt] == -1)
			tsp->t_ppar[cnt] = newparam;
	}
}


/*
 * void
 * tcl_sfont(wstation_t *, ws_channel_t *, termstate_t *) 
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
void
tcl_sfont(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp) 
{
	ushort	parnum = tsp->t_pnum;
	int	cnt = 0;
	ushort	curattr = tsp->t_curattr;
	ushort	param;


	do {
		param = tsp->t_ppar[cnt];

		if (param < tsp->t_nattrmsk) {
/* XXX
			if (param == -1) 
				param = 0;
*/
			curattr &= tsp->t_attrmskp[param].mask;
			curattr |= tsp->t_attrmskp[param].attr;

			switch (param) {
			case 5:
			case 6:
			case 4:
			case 38:
			case 39:
				(*wsp->w_consops->cn_undattr)(wsp, chp, &curattr, param);
				break;
			case 10:
				tsp->t_font = ANSI_FONT0;
				(*wsp->w_consops->cn_shiftset)(wsp, chp, 0); 

				break;
			case 11:
				tsp->t_font = ANSI_FONT1;
				break;
			case 12:
				tsp->t_font = ANSI_FONT2;
				break;
			default:
				break;
			}
		}

		cnt++;
		parnum--;
/* XXX
	} while (parnum > 0);
*/
	} while (parnum >= 1);

	tsp->t_curattr = (uchar_t) curattr;
	tsp->t_pstate = 0;
}


/*
 * int
 * tcl_ckchar(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 */
int
tcl_ckchar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort	avail;


	if (!tsp->t_ppar[0])
		return (0);
	avail = tsp->t_cols - tsp->t_col;
	if (tsp->t_ppar[0] <= avail)
		return (1);
	/*
	 * if there are too many chars, erase them 
	 */
	(*cnops->cn_clrscr)(chp, tsp->t_cursor, avail);
	return (0);
}


/*
 * int
 * tcl_ckline(kdcnops_t *, ws_channel_t *, termstate_t *) 
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 */
int
tcl_ckline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort avail;


	if (!tsp->t_ppar[0])
		return (0);
	avail = tsp->t_rows - tsp->t_row;
	if (tsp->t_ppar[0] <= avail)
		return (1);
	/*
	 * if we have too many lines, erase them 	
	 */
	(*cnops->cn_clrscr)(chp, (tsp->t_cursor-tsp->t_col), 
				(avail * tsp->t_cols));
	return (0);
}


/* 
 * void
 * tcl_settab(termstate_t *) 
 *	set tabs.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 */
void
tcl_settab(termstate_t *tsp)
{
	unchar cnt1, cnt2;


	if (tsp->t_ntabs == 0) {	/* no tabs set yet */
		tsp->t_tabsp[0] = tsp->t_col;
		tsp->t_ntabs++;
		return;
	}
	if (tsp->t_ntabs == ANSI_MAXTAB) /* no more tabs can be set */
		return;
	for (cnt1 = 0; cnt1 < tsp->t_ntabs; cnt1++)
		if (tsp->t_tabsp[cnt1] >= tsp->t_col)
			break;

	if (tsp->t_tabsp[cnt1] == tsp->t_col)
		return;			/* tab already set here */

	for (cnt2 = tsp->t_ntabs; cnt2 > cnt1; cnt2--)
		tsp->t_tabsp[cnt2] = tsp->t_tabsp[cnt2 - 1];

	tsp->t_tabsp[cnt1] = tsp->t_col;
	tsp->t_ntabs++;
}


/*
 * unchar
 * tcl_curattr(ws_channel_t *)
 *	get current channel attributes.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
unchar
tcl_curattr(ws_channel_t *chp)
{
	termstate_t	*tsp = &chp->ch_tstate;

	return ((int) tsp->t_curattr);
}


/*
 * void
 * tcl_scrolllock(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
tcl_scrolllock(wstation_t *wsp, ws_channel_t *chp, int arg)
{
	kbstate_t	*kbp;
	mblk_t		*tmp;
	ch_proto_t	*protop;
	pl_t		opl;


	if (!(tmp = allocb(sizeof(ch_proto_t), BPRI_HI)))
		return;

	opl = RW_RDLOCK(wsp->w_rwlock, plstr);
	(void) LOCK(chp->ch_mutex, plstr);

	kbp = &chp->ch_kbstate;
	if (arg == TCL_FLOWON)
		kbp->kb_state |= SCROLL_LOCK;
	else
		kbp->kb_state &= ~SCROLL_LOCK;

	if ((chp == ws_activechan(wsp)) && wsp->w_consops->cn_scrllck) 
		(*wsp->w_consops->cn_scrllck)();

	tmp->b_datap->db_type = M_PROTO;
	tmp->b_wptr += sizeof(ch_proto_t);
	/* LINTED pointer alignment */
	protop = (ch_proto_t *)tmp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHR;
	protop->chp_stype_cmd = CH_LEDSTATE;
	protop->chp_stype_arg = (kbp->kb_state & ~NONTOGGLES);

	/*
	 * Locks cannot be held across putnext().
	 */
	UNLOCK(chp->ch_mutex, plstr);
	RW_UNLOCK(wsp->w_rwlock, opl);  

	putnext(chp->ch_qp, tmp);
}


/*
 * void
 * tcl_sendscr(wstation_t *, ws_channel_t *, termstate_t *)
 *	Send contents of screen to user
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
tcl_sendscr(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp)
{
	struct termios	*termp;		/* Terminal structure */
	ushort		*cp;		/* Pointer to characters */
	mblk_t		*bp, *bp1, *cbp, *mp, *mp1;
	struct iocblk	*iocp;
	vidstate_t	*vp;
	ch_proto_t	*protop;
	ushort		rows = 0,cols = 0;
	pl_t		opl;


	opl = RW_RDLOCK(wsp->w_rwlock, plstr);
	(void) LOCK(chp->ch_mutex, plstr);

	/*
	 * Disable echo by telling LDTERM that we'll do 
	 * it (we won't, of course, because we want ECHO
	 * disabled). Generate an M_CTL message for LDTERM
	 * that tells it we'll do the canonical processing 
	 * for echo.
	 */
	if ((bp = allocb(sizeof(struct iocblk),BPRI_HI)) == (mblk_t *) NULL) {
		(*wsp->w_consops->cn_bell)(wsp, chp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(wsp->w_rwlock, opl); 
		return;
	}

	if ((bp1 = allocb(sizeof(struct termios),BPRI_HI)) == (mblk_t *) NULL) {
		(*wsp->w_consops->cn_bell)(wsp, chp);
		freeb(bp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(wsp->w_rwlock, opl); 
		return;
	}

	bp->b_wptr += sizeof(struct iocblk);
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)bp->b_rptr;
	bp->b_datap->db_type = M_CTL;
	iocp->ioc_cmd = MC_PART_CANON;
	bp->b_cont = bp1;
	bp1->b_wptr += sizeof(struct termios);
	/* LINTED pointer alignment */
	termp = (struct termios *)bp1->b_rptr;
	termp->c_iflag = 0;
	termp->c_oflag = 0;
	termp->c_lflag = ECHO;

	if ((cbp = copymsg(bp)) == (mblk_t *) NULL) {
		(*wsp->w_consops->cn_bell)(wsp, chp);
		freemsg(bp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(wsp->w_rwlock, opl); 
		return;
	}

	/* LINTED pointer alignment */
	termp = (struct termios *)cbp->b_cont->b_rptr;
	termp->c_lflag = 0;

	if ((mp = allocb(sizeof(ch_proto_t),BPRI_HI)) == (mblk_t *) NULL) {
		(*wsp->w_consops->cn_bell)(wsp, chp);
		freemsg(bp);
		freemsg(cbp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(wsp->w_rwlock, opl); 
		return;
	}

	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += sizeof (ch_proto_t);
	/* LINTED pointer alignment */
	protop = (ch_proto_t *)mp->b_rptr;
	protop->chp_type = CH_DATA;
	protop->chp_stype = CH_NOSCAN;

	/*
	 * allocate a buffer to hold the current screen contents. 
	 * Leave room for two characters for the NL -- hence t_cols+1)
	 */
	if ((mp1 = allocb(tsp->t_rows * (tsp->t_cols + 1), BPRI_HI)) == 
					(mblk_t *) NULL) {
		(*wsp->w_consops->cn_bell)(wsp, chp);
		freemsg(bp);
		freemsg(cbp);
		freeb(mp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(wsp->w_rwlock, opl); 
		return;
	}

	mp->b_cont = mp1;
	vp = &chp->ch_vstate;
	cp = vp->v_scrp + (tsp->t_origin & vp->v_scrmsk);

	while (rows < tsp->t_rows) {			/* For all rows */
		while (cols < tsp->t_cols) {		/* For all columns */
			*mp1->b_wptr++ = *cp++ & 0xff;	/* add char to msg */
			cols++;				/* Did one more col. */
		}
		cols = 0;			/* Reset column count */
		*mp1->b_wptr++ = (unchar) '\n';	/* Add newline and flush */
		rows++;				/* Did one more row */
	}

	UNLOCK(chp->ch_mutex, plstr);
	RW_UNLOCK(wsp->w_rwlock, opl); 

	putnext(chp->ch_qp, bp);	/* turn echo off */
	putnext(chp->ch_qp, mp);	/* put CH_NOSCAN msg. on the queue */
	putnext(chp->ch_qp, cbp);	/* turn echo back on */
}
