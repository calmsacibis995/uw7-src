/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#ident	"@(#)gsdtcl.c	1.5"
#ident	"$Header$"

/*
 * gsdtcl.c, routines called from kd/ws.
 */

/*
 *	L000	7/3/97		rodneyh@sco.com
 *	- Remove Novell copyright, add SCO copyright.
 *
 */

#include <util/debug.h>
#include <util/param.h>
#include <io/ansi/at_ansi.h>	/* for DOWN */
#include <io/ascii.h>
#include <io/ldterm/euc.h>
#include <io/ldterm/eucioctl.h>
#include <io/ws/chan.h>
#include <io/ws/tcl.h>
#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <io/fnt/fnt.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/types.h>

#define	PARTIAL_CHAR	0xff000000	/* indicates partial character */

extern wstation_t Kdws;

/* gsfntops_t Gs;	/* filled in by the font driver */

static void gs_reseteuc(wstation_t *, ws_channel_t *, termstate_t *);

/*
 * void
 * gs_chinit(wstation_t *, ws_channel_t *)
 *	Called from ws_chinit() to initialize gsd related data.
 *	The channel can be used in either text or graphics mode.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive mode.
 */
void
gs_chinit(wstation_t *wsp, ws_channel_t *chp)
{
	if (chp->ch_dmode != KD_GRTEXT) {
		/* Deallocate all resources of graphics text mode. */
		gs_free(Kdws.w_consops, chp, &chp->ch_tstate);
	} else {
		gs_alloc(Kdws.w_consops, chp, &chp->ch_tstate);
	}
}

/*
 * int
 * gs_seteuc(ws_channel_t *, struct eucioc *)
 *
 * Calling/Exit State:
 *      - No locks are held on Entry/Exit.
 *
 * Description:
 *	Called from ws_mctlmsg(), it sets the EUC information associated
 *	with this channel.
 *
 */
int
gs_seteuc(ws_channel_t *chp, struct eucioc *eucp)
{
	pl_t	opl;

	opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
	(void) LOCK(chp->ch_mutex, plstr);

	if (EUC_INFO(chp) == NULL) {
		if ((EUC_INFO(chp) = (struct eucioc *)
		     kmem_zalloc(sizeof(struct eucioc), KM_NOSLEEP)) == NULL) {
			UNLOCK(chp->ch_mutex, plstr);
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return (ENOMEM);
		}
	}

	bcopy ((caddr_t) eucp, (caddr_t) EUC_INFO(chp), sizeof(struct eucioc));

	UNLOCK(chp->ch_mutex, plstr);
	RW_UNLOCK(Kdws.w_rwlock, opl);

	return (0);
}

/*
 * static void
 * gs_reseteuc(wstation_t *, ws_channel_t *, termstate_t *)
 *	Remove EUC information from channel.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive mode.
 */
static void
gs_reseteuc(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp)
{
	if (!EUC_INFO(chp))
		return;

	kmem_free(EUC_INFO(chp), sizeof(struct eucioc));
	EUC_INFO(chp) = NULL;

	return;
}

/* int
 * gs_alloc(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	initialize for graphics text mode.
 *	Called from kdv_setmode() when switching to graphics text mode.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held (when called from kdv_setmode()).
 */
int
gs_alloc(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	int	cnt;
	wchar_t	*wsrc;
	uchar_t	*asrc;
	ushort	*srcp;

	if (chp->ch_scrbuf == (wchar_t *) 0) {
		if ((chp->ch_scrbuf = (wchar_t *)
			kmem_zalloc(tsp->t_scrsz*sizeof(wchar_t),
					KM_NOSLEEP)) == NULL)
			return (ENOMEM);

		if ((chp->ch_attrbuf = (uchar_t *)
			kmem_zalloc(tsp->t_scrsz*sizeof(uchar_t),
					KM_NOSLEEP)) == NULL) {
			kmem_free(chp->ch_scrbuf, tsp->t_scrsz*sizeof(wchar_t));
			return (ENOMEM);
			}
		++channel_ref_count;
	}

	wsrc = chp->ch_scrbuf;
	asrc = chp->ch_attrbuf;
	cnt = tsp->t_scrsz;
	while (--cnt >= 0) {
		*wsrc++ = ' ';
		*asrc++ = tsp->t_curattr;
	}

	/* copy context of text screen buffer to graphics screen */
	if ((srcp = Kdws.w_scrbufpp[chp->ch_id]) != NULL) {
		int	pos;
		for (pos = 0; pos < 80*25; pos++) {
			gdv_stchar(chp, pos, srcp[pos] & 0xff,
					(srcp[pos] >> 8) & 0xff,
					1, 1);
		}
	}

	gs_setcursor(chp, tsp);

	return (0);
}

/* void
 * gs_free(kdcnops_t *, ws_channel_t *, termstate_t *)
 *	reset/clear the graphics console device.
 *	Called from kdv_setmode() when switching modes.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held (when called from kdv_setmode()).
 */
void
gs_free(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	if (chp->ch_dmode != KD_GRTEXT) {
		gs_reseteuc(chp->ch_wsp, chp, &chp->ch_tstate);

		if (chp->ch_scrbuf) {
			kmem_free(chp->ch_scrbuf, tsp->t_scrsz*sizeof(wchar_t));
			chp->ch_scrbuf = NULL;
			--channel_ref_count;
		}

		if (chp->ch_attrbuf) {
			kmem_free(chp->ch_attrbuf, tsp->t_scrsz*sizeof(uchar_t));
			chp->ch_attrbuf = NULL;
		}
	}
}

/*
 * void
 * gcl_handler(wstation_t *, mblk_t *, termstate_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_handler(wstation_t *wsp, mblk_t *mp, termstate_t *tsp,
                ws_channel_t *chp)
{
	ch_proto_t	*protop;
	mblk_t		*bp;

	protop = (ch_proto_t *) mp->b_rptr;

	switch (protop->chp_stype_cmd) {
	case TCL_CURS_TYP: {
		ushort type;
		type = protop->chp_stype_arg;
		/* treat type 0 and 1 the same */
		if (type == 1) type = 0;
		if (type == tsp->t_curtyp)
			break;
		tsp->t_curtyp = type;
		gdv_cursortype(wsp, chp, tsp);
		break;
	}

	case TCL_BACK_SPCE:
		gcl_bs(wsp->w_consops, chp, tsp);
		break;

	case TCL_NEWLINE:
	case TCL_V_TAB:
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			gcl_scrlup(wsp->w_consops, chp, tsp);
		}
		else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols;
		}
		gs_setcursor(chp, tsp);
		break;

        case TCL_CRRGE_RETN:
                tsp->t_cursor -= tsp->t_col;
                tsp->t_col = 0;
                gs_setcursor(chp, tsp);
                break;

	case TCL_H_TAB:
		gcl_ht(wsp->w_consops, chp, tsp);
		break;

	case TCL_BACK_H_TAB:
		gcl_bht(wsp->w_consops, chp, tsp);
		break;

	case TCL_SHFT_FT_OU:
		tsp->t_flags &= ~T_ALTCHARSET;
		break;

	/* The TCL_SHFT_IN is got from ansi on a A_SI character.
	 * The TCL_MB_SHFT_FT_IN is got from ansi on an ESC[12M sequence.
	 * Please see tcl.h and ansi.c for more info.
	 */

	case TCL_SHFT_FT_IN:
		tsp->t_flags |= T_ALTCHARSET;
		break;

	case TCL_MB_SHFT_FT_IN:
		tsp->t_flags |= T_ALTCHARSET;
		break;

	case TCL_SCRL_UP:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		gcl_scrlup(wsp->w_consops, chp, tsp);
		break;

	case TCL_SCRL_DWN:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		gcl_scrldn(wsp->w_consops, chp, tsp);
		break;

	case TCL_SET_ATTR:
		tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
		tsp->t_pnum = 1;
		gcl_sfont(wsp, chp, tsp);
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
                                "gcl_handler: unexpected end of msg");
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

                gcl_cursor(wsp->w_consops, chp);
                break;
        } /* TCL_POS_CURS */

        case TCL_DISP_CLR:
                tsp->t_ppar[0] = (ushort) protop->chp_stype_arg;
                gdclrscr(chp, tsp->t_cursor, tsp->t_ppar[0]);
                break;

	case TCL_DISP_RST:
		/* gcl_reset(wsp->w_consops, chp, tsp); */
		break;

	case TCL_ERASCR_CUR2END:
		tsp->t_ppar[0] = 0;
		gcl_escr(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERASCR_BEG2CUR:
		tsp->t_ppar[0] = 1;
		gcl_escr(wsp->w_consops, chp, tsp);
		break;

	case TCL_ERASCR_BEG2END:
		tsp->t_ppar[0] = 2;
		gcl_escr(wsp->w_consops, chp, tsp);
		break;

        case TCL_ERALIN_CUR2END:
                tsp->t_ppar[0] = 0;
                gcl_eline(wsp->w_consops, chp, tsp);
                break;

        case TCL_ERALIN_BEG2CUR:
                tsp->t_ppar[0] = 1;
                gcl_eline(wsp->w_consops, chp, tsp);
                break;

        case TCL_ERALIN_BEG2END:
                tsp->t_ppar[0] = 2;
                gcl_eline(wsp->w_consops, chp, tsp);
                break;

	case TCL_INSRT_LIN:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		gcl_iline(wsp->w_consops, chp, tsp);
		break;

	case TCL_DELET_LIN:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		gcl_dline(wsp->w_consops, chp, tsp);
		break;

        case TCL_INSRT_CHR:
                tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
                gcl_ichar(wsp->w_consops, chp, tsp);
                break;

	case TCL_DELET_CHR:
		tsp->t_ppar[0] = (ushort)protop->chp_stype_arg;
		gcl_dchar(wsp->w_consops, chp, tsp);
		break;

	case TCL_PRINT_FONTCHAR:
		if (chp->ch_dmode != KD_GRAPHICS) {
			unchar  font =  tsp->t_font;

			/* set fg/bg colors to default graphics fg/bg */
			tsp->t_ppar[0] = 12;
			tsp->t_pnum = 1;
			gcl_sfont(wsp, chp, tsp);
			gcl_norm(wsp->w_consops, chp, tsp, protop->chp_stype_arg
);
			/* set fg/bg colors back to normal fg/bg */
			tsp->t_ppar[0] = 0;
			tsp->t_pnum = 1;
			gcl_sfont(wsp, chp, tsp);
			tsp->t_font = font;
		}
		break;

	case TCL_SET_CURSOR_PARAMS:
		break;

	/*
	 * The following TCL sequences can all be handled by the
	 * original tcl_handler because they don't call display
	 * specific routines.
	 */
	case TCL_BELL:
	case TCL_KEYCLK_ON:
	case TCL_KEYCLK_OFF:
	case TCL_CLR_TAB:
	case TCL_CLR_TABS:
	case TCL_SET_TAB:
	case TCL_LCK_KB:
	case TCL_UNLCK_KB:
	case TCL_SAVE_CURSOR:
	case TCL_RESTORE_CURSOR:
	case TCL_AUTO_MARGIN_ON:
	case TCL_AUTO_MARGIN_OFF:
	case TCL_SET_FONT_PROPERTIES:
	case TCL_SET_BELL_PARAMS:
	case TCL_SET_OVERSCAN_COLOR:
	case TCL_NOBACKBRITE:
	case TCL_FORGRND_COLOR:
	case TCL_BCKGRND_COLOR:
	case TCL_RFORGRND_COLOR:
	case TCL_RBCKGRND_COLOR:
	case TCL_GFORGRND_COLOR:
	case TCL_GBCKGRND_COLOR:
		tcl_handler(wsp, mp, tsp, chp);
		break;

	default:
                /*
                 * An illegal TCL channel protocol subtype command
                 * is sent from upstream.
                 */
                cmn_err(CE_WARN,
                        "gcl_handler: do not understand TCL %d",
                        protop->chp_stype_cmd);
                break;
        }
}

/*
 * void
 * gcl_norm(kdcnops_t *, ws_channel_t *, termstate_t *, ushort)
 *      display normal text.
 *	Assemble multi-byte characters into a single EUC character.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_norm(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp, ushort ch)
{
	int	scrw = chp->ch_scrw;
	/*
	 * Assemble multi-byte characters into a single wide character.
	 * Note that to aid debugging characters are assembled as 8-bit
	 * quantities instead of 7-bit quantities as is done in the C
	 * library. This also means that only up to three multi-byte
	 * characters can be assembled correctly. Actually, the values
	 * for EUCMASK and P00, etc. are such that also in the library
	 * only three multibyte characters can be assembled.
	 */
	if (chp->ch_wc_len > 0) { /* still characters to collect? */
		chp->ch_wc <<= 8;
		chp->ch_wc |= ch & 0x7f;
		--chp->ch_wc_len;
	}
	else { /* assemble new character */
		chp->ch_wc_mask = 0;
		chp->ch_wc_attr = ch >> 8;
		chp->ch_wc = ch = (ch & 0xff);
		scrw = 0;

		if (ch >= 0x80 && !(tsp->t_flags & T_ALTCHARSET)) {
			if (ch < 0xa0) { /* C1 (i.e., metacontrol) byte */
				if (ch == SS2) {
					int eucw2 = EUC_INFO(chp)->eucw[2];
					if (eucw2 != 0) {
						chp->ch_wc = 0;
						chp->ch_wc_mask = P01;
						chp->ch_wc_len = eucw2;
						scrw = EUC_INFO(chp)->scrw[2];
					}
				} else if (ch == SS3) {
					int eucw3 = EUC_INFO(chp)->eucw[3];
					if (eucw3 != 0) {
						chp->ch_wc = 0;
						chp->ch_wc_mask = P10;
						chp->ch_wc_len = eucw3;
						scrw = EUC_INFO(chp)->scrw[3];
					}
				}
			}
			else {
				int eucw1 = EUC_INFO(chp)->eucw[1];
				if (eucw1 != 0) {
					chp->ch_wc = ch & 0x7f;
					chp->ch_wc_mask = P11;
					chp->ch_wc_len = eucw1 - 1;
					scrw = EUC_INFO(chp)->scrw[1];
				}
			}
		}

	}

	if (chp->ch_wc_len != 0) {
		chp->ch_scrw = scrw;
		return;
	}

	if (scrw == 0)
		scrw = EUC_INFO(chp)->scrw[0];

	/*
	 * At this point we assembled the wide character and should
	 * display it.
	 */
	chp->ch_wc |= chp->ch_wc_mask;
	/*
	 * Check whether the multi-column character fits on the current
	 * line. If not and the AUTO_MARGIN_ON has been set, display
	 * the character on the next line.
	 */
	if ((tsp->t_col+scrw) > tsp->t_cols) {
		if (tsp->t_auto_margin != AUTO_MARGIN_ON) {
			int col = tsp->t_col;
			tsp->t_col = tsp->t_cols - scrw;
			tsp->t_cursor -= (col - tsp->t_col);
		}
		else {
			int xcol = tsp->t_cols - tsp->t_col;

			if (tsp->t_row == tsp->t_rows - 1) {
				tsp->t_ppar[0] = 1;
				gcl_scrlup(cnops, chp, tsp);
				tsp->t_cursor -= tsp->t_col;
			} else {
				tsp->t_row++;
				tsp->t_cursor += xcol;
			}
			tsp->t_col = 0;
		}
	}

        gdv_stchar(chp, tsp->t_cursor, chp->ch_wc,
			tsp->t_curattr|chp->ch_wc_attr, 1, scrw);

	/* Move cursor */
	tsp->t_col += scrw;
	tsp->t_cursor += scrw;

        /* Enhanced Application Compatibility Support */

        if(tsp->t_auto_margin != AUTO_MARGIN_ON &&
                        tsp->t_col == tsp->t_cols) {
                tsp->t_col--;
                tsp->t_cursor--;
        }

        /* End Enhanced Application Compatibility Support */

        if (tsp->t_col == tsp->t_cols) {
                if (tsp->t_row == tsp->t_rows - 1) {
                        tsp->t_ppar[0] = 1;
                        gcl_scrlup(cnops, chp, tsp);
                        tsp->t_cursor -= tsp->t_col;
                } else {
                        tsp->t_row++;
                }
                tsp->t_col = 0;
        }

        gs_setcursor(chp, tsp);
}

/*
 * void
 * gdv_stchar(ws_channel_t *, ushort, wchar_t, uchar_t, int, int)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 *
 * Description:
 *	This routine is called by gcl_norm to display a character.
 */
void
gdv_stchar(ws_channel_t *chp, ushort dest, wchar_t ch, uchar_t attr,
	   int cnt, int scrw)
{
	wchar_t *scrbuf = chp->ch_scrbuf;
	uchar_t *attrbuf = chp->ch_attrbuf;
	uchar_t *bitmap;
	int	i, dst;

	/*
	 * First make sure that we are not overwriting a multi-column
	 * character. To keep the code simple, we assume a character
	 * is either one or two columns wide. This means that if we
	 * are overwriting a multi-columns character, we should erase
	 * the character at the left or at the right of the current
	 * cursor position.
	 */
	if (dest > 0 && (scrbuf[dest] & PARTIAL_CHAR) == PARTIAL_CHAR) {
		/* erase character to the left */
		dst = dest - 1;
		scrbuf[dst] = ' ';
		attrbuf[dst] = attr;
		bitmap = Gs.fnt_getbitmap(chp, ' ');
		gs_writebitmap(chp, dst, bitmap, 1, attr);
		/* no need to unlock the bitmap */
	}
	/*
	 * Place the character(s) in the virtual screen buffer.
	 */
	for (i = 0, dst = dest; i < cnt; i++, dst += scrw) {
		scrbuf[dst] = ch;
		attrbuf[dst] = attr;
		if (scrw > 1)
			scrbuf[dst+1] = PARTIAL_CHAR | scrw;
	}
	/*
	 * Find and display bitmap.
         */
	bitmap = Gs.fnt_getbitmap(chp, ch);
	for (i = 0, dst = dest; i < cnt; i++, dst += scrw)
		gs_writebitmap(chp, dst, bitmap, scrw, attr);
	Gs.fnt_unlockbitmap(chp, ch);
	/*
	 * Clear any partial character.
	 */
	if ((scrbuf[dst] & PARTIAL_CHAR) == PARTIAL_CHAR) {
		/* erase character to the right */
		scrbuf[dst] = ' ';
		attrbuf[dst] = attr;
		bitmap = Gs.fnt_getbitmap(chp, ' ');
		gs_writebitmap(chp, dst, bitmap, 1, attr);
		/* no need to unlock the bitmap */
	}
}

/*
 * void
 * gdv_scrxfer(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive/shared mode.
 *      - chp->ch_mutex basic lock is also held, when w_rwlock is
 *        held in shared mode.
 *      - Called during VT switching or mode switching to
 *        save or restore the contents of VT being switched
 *        out/switched in.
 *
 * Description:
 *      Move scrsize words between screen and buffer.
 */
void
gdv_scrxfer(ws_channel_t *chp, int dir)
{
	if (dir == KD_BUFTOSCR) { /* restore screen */
		wchar_t 	*scrbuf = chp->ch_scrbuf;
		uchar_t		*attrbuf = chp->ch_attrbuf;
        	termstate_t     *tsp = &chp->ch_tstate;
		int		scrsz = tsp->t_scrsz;

		int	pos;
		int	scrw0 = EUC_INFO(chp)->scrw[0];
		int	scrw1 = EUC_INFO(chp)->scrw[1];
		int	scrw2 = EUC_INFO(chp)->scrw[2];
		int	scrw3 = EUC_INFO(chp)->scrw[3];

		for (pos = 0; pos < scrsz; pos++) {
			wchar_t	ch = scrbuf[pos];

			if ((ch & PARTIAL_CHAR) != PARTIAL_CHAR) {
				uchar_t *bitmap;
				int	scrw;
				int	iscodeset0 = 0;

				switch(ch & EUCMASK) {
				case P11: /* codeset 1 */
					scrw = scrw1;
					break;
				case P01: /* codeset 2 */
					scrw = scrw2;
					break;
				case P10: /* codeset 3 */
					scrw = scrw3;
					break;
				case P00: /* codeset 0 */
				default:
					iscodeset0 = 1;
					scrw = scrw0;
					break;
				}
				/*
				 * Find and display bitmap.
				 */
				bitmap = Gs.fnt_getbitmap(chp, ch);
				gs_writebitmap(chp, pos, bitmap, scrw,
						attrbuf[pos]);
				if (!iscodeset0)
					Gs.fnt_unlockbitmap(chp, ch);
			}
		}
		gs_setcursor(chp, tsp);
	} else {
		/* `remove' current cursor */
		chp->ch_cursor = -1;
	}
}

/*
 * int
 * gdv_cursortype(wstation_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - Only active channel is allowed to change the cursortype
 *      - w_rwlock is held in exclusive/shared mode.
 *      - chp->ch_mutex lock is also held if w_rwlock is held in shared mode.
 *
 * Description:
 */
int
gdv_cursortype(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp)
{
	if (tsp->t_curtyp == 0)
		gs_setcursor(chp, tsp);
	else
		gs_unsetcursor(chp, tsp);

	return (0);
}

/*
 * int
 * gs_setcursor(ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared or exclusive mode.
 *      - When w_rwlock is held in shared mode, then
 *        chp->ch_mutex basic lock is also held.
 *
 * Description:
 *      If we are the active VT, then place the cursor on the screen.
 */
int
gs_setcursor(ws_channel_t *chp, termstate_t *tsp)
{
	if (tsp->t_curtyp != 0)	/* cursor is invisiable */
		return (0);

	if (WS_ISACTIVECHAN(&Kdws, chp)) {
		int	pos = tsp->t_cursor;
		wchar_t	ch = chp->ch_scrbuf[pos];
		uchar_t	attr;
		int	scrw;
		uchar_t	*bitmap;
		int	iscodeset0 = 0;

		if (chp->ch_cursor >= 0)
			gs_unsetcursor(chp, tsp);

		while ((pos > 0) && ((ch & PARTIAL_CHAR) == PARTIAL_CHAR))
		{ ch = chp->ch_attrbuf[--pos]; }
		attr = chp->ch_attrbuf[pos];

		switch(ch & EUCMASK) {
		case P11: /* codeset 1 */
			scrw = EUC_INFO(chp)->scrw[1];
			break;
		case P01: /* codeset 2 */
			scrw = EUC_INFO(chp)->scrw[2];
			break;
		case P10: /* codeset 3 */
			scrw = EUC_INFO(chp)->scrw[3];
			break;
		case P00: /* codeset 0 */
		default:
			scrw = EUC_INFO(chp)->scrw[0];
			iscodeset0 = 1;
			break;
		}
		bitmap = Gs.fnt_getbitmap(chp, ch);
		gs_writebitmap(chp, pos, bitmap, scrw,
				((attr&0xf)<<4) | ((attr&0xf0)>>4));
		if (!iscodeset0)
			Gs.fnt_unlockbitmap(chp, ch);

		chp->ch_cursor = pos; /* remember cursor position */
	}

	return (0);
}

/*
 * int
 * gs_unsetcursor(ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared or exclusive mode.
 *      - When w_rwlock is held in shared mode, then
 *        chp->ch_mutex basic lock is also held.
 *
 * Description:
 *      If we are the active VT, then unset cursor on CRT controller.
 */
int
gs_unsetcursor(ws_channel_t *chp, termstate_t *tsp)
{
	int	pos = chp->ch_cursor;

	if (pos < 0)
		return (0);

	if (WS_ISACTIVECHAN(&Kdws, chp)) {
		wchar_t	ch = chp->ch_scrbuf[pos];
		uchar_t	attr = chp->ch_attrbuf[pos];
		int	scrw;
		uchar_t	*bitmap;
		int	iscodeset0 = 0;

		switch(ch & EUCMASK) {
		case P11: /* codeset 1 */
			scrw = EUC_INFO(chp)->scrw[1];
			break;
		case P01: /* codeset 2 */
			scrw = EUC_INFO(chp)->scrw[2];
			break;
		case P10: /* codeset 3 */
			scrw = EUC_INFO(chp)->scrw[3];
			break;
		case P00: /* codeset 0 */
		default:
			scrw = EUC_INFO(chp)->scrw[0];
			iscodeset0 = 1;
			break;
		}
		bitmap = Gs.fnt_getbitmap(chp, ch);
		gs_writebitmap(chp, pos, bitmap, scrw, attr);
		if (!iscodeset0)
			Gs.fnt_unlockbitmap(chp, ch);

		chp->ch_cursor = -1;
	}

	return (0);
}

/*
 * void
 * gcl_bs(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      tcl backspace.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_bs(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        if (!tsp->t_col)
                return;
        tsp->t_col--;
        tsp->t_cursor--;
        gs_setcursor(chp, tsp);
}

/*
 * void
 * gcl_ht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
 *      tcl horizontal tab.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_ht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        unchar  tabindex = 0;
        ushort  tabval;


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
                        gcl_scrlup(cnops, chp, tsp);
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
        gs_setcursor(chp, tsp);
}

/*
 * void
 * gcl_bht(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      back horizontal tab.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_bht(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        int     tabindex = tsp->t_ntabs;
        ushort  tabval;


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
        gs_setcursor(chp, tsp);
}

/*
 * void
 * gcl_cursor(kdcnops_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_cursor(kdcnops_t *cnops, ws_channel_t *chp)
{
        ushort          row, col;
        termstate_t     *tsp = &chp->ch_tstate;


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
        gs_setcursor(chp, tsp);
}

/*
 * void
 * gcl_scrlup(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      scroll up
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_scrlup(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort row, col, cursor;

	row = tsp->t_row;
	col = tsp->t_col;
	cursor = tsp->t_cursor;
	tsp->t_cursor = tsp->t_origin;
	tsp->t_row = tsp->t_col = 0;
	gcl_dline(cnops, chp, tsp);
	tsp->t_row = row;
	tsp->t_col = col;
	tsp->t_cursor = cursor;
}

/*
 * void
 * gcl_scrldn(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      scroll down
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_scrldn(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	short   row, col;
	ushort  cursor;

	/*
	 * Save row, col, and cursor.
	 */
	row = tsp->t_row;
	col = tsp->t_col;
	cursor = tsp->t_cursor;
	/*
	 * Get gcl_iline to do the work.
	 */
	tsp->t_cursor = tsp->t_origin;
	tsp->t_row = tsp->t_col = 0;
	gcl_iline(cnops, chp, tsp);
	/*
	 * Restore row, col, and cursor.
	 */
	tsp->t_row = row;
	tsp->t_col = col;
	tsp->t_cursor = cursor;
}

/*
 * void
 * gcl_dline(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      delete a line.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_dline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
	ushort  from, to, last;
	int     cnt;

	gs_unsetcursor(chp, tsp); /* remove cursor */

	if (!gcl_ckline(cnops, chp, tsp))
		return;
	to = tsp->t_cursor - tsp->t_col;
	from = to + tsp->t_ppar[0] * tsp->t_cols;
	cnt = (tsp->t_rows - tsp->t_row - tsp->t_ppar[0]) * tsp->t_cols;
	gdv_mvword(chp, from, to, cnt, DOWN);
	last = to + cnt;
	cnt = tsp->t_ppar[0] * tsp->t_cols;
	gdclrscr(chp, last, cnt);
}

/*
 * void
 * gcl_iline(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      insert a line.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_iline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        ushort  from, to;
        int     cnt;
	
	gs_unsetcursor(chp, tsp); /* remove cursor */

        if (!gcl_ckline(cnops, chp, tsp))
                return;
        from = tsp->t_cursor - tsp->t_col;
        to = from + tsp->t_ppar[0] * tsp->t_cols;
        cnt = (tsp->t_rows - tsp->t_row - tsp->t_ppar[0]) * tsp->t_cols;
        gdv_mvword(chp, from, to, cnt, UP);
        gdclrscr(chp, from, tsp->t_ppar[0] * tsp->t_cols);
}

/*
 * int
 * gcl_ckline(kdcnops_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 *
 */
int
gcl_ckline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
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
        gdclrscr(chp, (tsp->t_cursor-tsp->t_col), (avail * tsp->t_cols));
        return (0);
}

/*
 * int
 * gcl_ckchar(kdcnops_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 *
 */
int
gcl_ckchar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        ushort  avail;


        if (!tsp->t_ppar[0])
                return (0);
        avail = tsp->t_cols - tsp->t_col;
        if (tsp->t_ppar[0] <= avail)
                return (1);
        /*
         * if there are too many chars, erase them
         */
        gdclrscr(chp, tsp->t_cursor, avail);
        return (0);
}

/*
 * int
 * gdclrscr(ws_channel_t *, ushort, int)
 *
 * Calling/Exit State:
 *      - w_rwlock can be held in either exclusive or shared mode.
 *        In exclusive mode, ch_mutex lock does not need to be held,
 *        but in shared mode channels (chp) mutex lock is also held.
 *
 * Description:
 *      Clears count successive locations starting at last.
 */
int
gdclrscr(ws_channel_t *chp, ushort last, int cnt)
{
        termstate_t     *tsp;
        unsigned char   c;


        tsp = &chp->ch_tstate;
        c = tsp->t_nfcolor | (tsp->t_nbcolor << 4);

        if (cnt)
                gdv_stchar(chp, last, ' ', c, cnt, 1);

        return (0);
}

/*
 * void
 * gcl_escr(kdcnops_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_escr(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        int     cnt = tsp->t_cursor - tsp->t_origin;


        switch (tsp->t_ppar[0]) {
        case 0: /* erase cursor to end of screen */
                gdclrscr(chp, tsp->t_cursor, tsp->t_scrsz - cnt);
                break;

        case 1: /* erase beginning of screen to cursor */
                gdclrscr(chp, tsp->t_origin, (cnt + 1));
                break;

        default:        /* erase whole screen */
                gdclrscr(chp, tsp->t_origin, tsp->t_scrsz);
                break;
        }
}

/*
 * void
 * gcl_eline(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      erase line.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_eline(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        ushort  last = tsp->t_cursor - tsp->t_col;


        switch (tsp->t_ppar[0]) {
        case 0: /* erase cursor to end of line */
                gdclrscr(chp, tsp->t_cursor, tsp->t_cols - tsp->t_col);
                break;

        case 1: /* erase beginning of line to cursor */
                gdclrscr(chp, last, tsp->t_col + 1);
                break;

        default: /* erase whole line */
                gdclrscr(chp, last, tsp->t_cols);
        }
}

/*
 * void
 * gcl_ichar(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      insert a character.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_ichar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        ushort	to, cnt, pos;
	wchar_t	ch;
	int	scrw;

	gs_unsetcursor(chp, tsp);

        to = tsp->t_cursor + tsp->t_ppar[0];
        cnt = tsp->t_cols - tsp->t_col - tsp->t_ppar[0];

        if (!gcl_ckchar(cnops, chp, tsp))
                return;
        gdv_mvword(chp, tsp->t_cursor, to, cnt, UP);
        gdclrscr(chp, tsp->t_cursor, tsp->t_ppar[0]);

	/* clear last character if it is a partial character */
	pos = tsp->t_cursor - tsp->t_col + tsp->t_cols - 1;
	ch = chp->ch_scrbuf[pos];
	switch(ch & EUCMASK) {
	case P11: /* codeset 1 */
		scrw = EUC_INFO(chp)->scrw[1];
		break;
	case P01: /* codeset 2 */
		scrw = EUC_INFO(chp)->scrw[2];
		break;
	case P10: /* codeset 3 */
		scrw = EUC_INFO(chp)->scrw[3];
		break;
	default:
		scrw = EUC_INFO(chp)->scrw[0];
		break;
	}
	if (scrw > 1)
		gdclrscr(chp, pos, 1);
}

/*
 * void
 * gcl_dchar(kdcnops_t *, ws_channel_t *, termstate_t *)
 *      delete a character.
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_dchar(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp)
{
        ushort  from, last;
        int     cnt;
	wchar_t	ch;

	gs_unsetcursor(chp, tsp);

        if (!tcl_ckchar(cnops, chp, tsp))
                return;
        from = tsp->t_cursor + tsp->t_ppar[0];
        cnt = tsp->t_cols - tsp->t_col - tsp->t_ppar[0];
        last = tsp->t_cursor - tsp->t_col + tsp->t_cols - tsp->t_ppar[0];
        gdv_mvword(chp, from, tsp->t_cursor, cnt, DOWN);
        gdclrscr(chp, last, tsp->t_ppar[0]);

	/* clear partial character at cursor position */
	ch = chp->ch_scrbuf[tsp->t_cursor];
	if ((ch & PARTIAL_CHAR) == PARTIAL_CHAR)
		gdclrscr(chp, tsp->t_cursor, 1);
}

/*
 * void
 * gcl_sfont(wstation_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in shared mode.
 *      - ch_mutex basic lock is also held.
 */
void
gcl_sfont(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp)
{
	ushort  parnum = tsp->t_pnum;
	int cnt = 0;
	ushort  curattr = tsp->t_curattr;
	ushort  param;


	do {
		param = tsp->t_ppar[cnt];

		if (param < tsp->t_nattrmsk) {
			curattr &= tsp->t_attrmskp[param].mask;
			curattr |= tsp->t_attrmskp[param].attr;

			switch (param) {
			case 5: /* enable blinking */
				tsp->t_flags &= ~T_BACKBRITE;
				break;
			case 6: /* enable bright background */
				tsp->t_flags |= T_BACKBRITE;
				break;
			case 4: /* underline attribute */
				break;
			case 38: /* enable underline */
			case 39: /* disable underline */
				break;
			case 10: /* ANSI_FONT0 */
				tsp->t_flags &= ~T_ALTCHARSET;
				break;
			case 11: /* ANSI_FONT1 */
			case 12: /* ANSI_FONT2 */
				tsp->t_flags |= T_ALTCHARSET;
				break;
			default:
				break;
			}
		}

		cnt++;
		parnum--;
	} while (parnum >= 1);

	tsp->t_curattr = (uchar_t) curattr;
	tsp->t_pstate = 0;
}

/*
 * int
 * gs_ansi_cntl(kdcnops_t *, ws_channel_t *, termstate_t *, ushort)
 *
 * Calling/Exit State:
 *	None.
 *
 * Note:
 *	The control characters like the BELL are not processed because
 *	the console switch (conssw) entry points cannot hold any locks.
 *	The problem was caused by the sysmsg driver which now holds the
 *	cmn_err lock before calling the kd console entry points. Since,
 *	the cmn_err lock has the highest hierarchy and any locks acquired
 *	would cause the hierarchy violation. Since kdtone acquires the
 *	mutex lock to process the BELL character, a lock hierarchy
 *	violation occurs. This is the reason the BELL character and
 *	other control characters are ignored in the console entry points.
 */
int
gs_ansi_cntl(kdcnops_t *cnops, ws_channel_t *chp, termstate_t *tsp, ushort ch)
{
	switch (ch) {
/*
	case A_BEL:
		(*cnops->cn_bell)(&Kdws, chp);
		break;
*/

	case A_BS:
		gcl_bs(cnops, chp, tsp);
		break;

	case A_HT:
		gcl_ht(cnops, chp, tsp);
		break;

	case A_NL:
	case A_VT:
		if (tsp->t_row == tsp->t_rows - 1) {
			tsp->t_ppar[0] = 1;
			gcl_scrlup(cnops, chp, tsp);
		} else {
			tsp->t_row++;
			tsp->t_cursor += tsp->t_cols;
		}
		gs_setcursor(chp, tsp);
		break;

#ifdef NOTYET
	case A_FF:
		tcl_reset(cnops, chp, tsp);
		break;
#endif /* NOTYET */

	case A_CR:
		tsp->t_cursor -= tsp->t_col;
		tsp->t_col = 0;
		gs_setcursor(chp, tsp);
		break;

	case A_GS:
		gcl_bht(cnops, chp, tsp);
		break;

	default:
		break;
	}

	return (0);
}
