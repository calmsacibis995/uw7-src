/*
 *	@(#)i128Screen.c	11.1	10/22/97	12:29:10
 *
 * Copyright (C) 1991-1996 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

/*
 * S000, Tue Mar 26 10:45:29 PST 1996, kylec
 *	Fix SetText for ibm528.
 * S001, Mon Apr  1 10:20:22 PST 1996, kylec@sco.com
 *	nap() before beginning to use graphics engine.
 * S002, Thu Sep  4 13:46:00 PDT 1997	hiramc@sco.COM
 *	no need for the ioctl of SW_VGA12
 */


#include <sys/types.h>
#ifdef usl
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "i128Defs.h"
#include "i128Procs.h"
#include "ti3025.h"
#include "ibm528.h"


/*
 * i128BlankScreen - blank or unblank the screen
 *	on - blank the screen if true, unblank the screen if false
 *	pScreen - which screen to blank
 */
Bool
i128BlankScreen(on, pScreen)
     int on;
     ScreenPtr pScreen;
{

#ifdef DEBUG_PRINT
     ErrorF("i128BlankScreen(on=%d)\n", on);
#endif /* DEBUG_PRINT */

     return(FALSE);
}


/*
 * i128SetGraphics(pScreen) - set screen into graphics mode
 */
void
i128SetGraphics(pScreen)
     ScreenPtr pScreen;
{
     grafData *grafinfo = DDX_GRAFINFO(pScreen);
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen); 
     nfbScrnPriv *devPriv = NFB_SCREEN_PRIV(pScreen);
     ColormapPtr pmap = devPriv->installedCmap;

#ifdef DEBUG_PRINT
     ErrorF("i128SetGraphics()\n");
#endif /* DEBUG_PRINT */

     i128EnableMemory(pScreen);

#if ! defined(usl)				/*	S002	*/
     if (i128Priv->info.video_flags & I128_VGA_ENABLED)
         ioctl(1, SW_VGA12, 0);
#endif

     i128SetMode(pScreen,
                 &i128Priv->mode,
                 I128_MODE_CLEAR | I128_MODE_ENGINE_INIT);

     i128Priv->clip.x1 = i128Priv->clip.y1 = 0;
     i128Priv->clip.x2 = i128Priv->mode.bitmap_width;
     i128Priv->clip.y2 = i128Priv->mode.bitmap_height;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->clip_top_left =
          I128_XY(i128Priv->clip.x1, i128Priv->clip.y1);
     i128Priv->engine->clip_bottom_right =
          I128_XY(i128Priv->clip.x2, i128Priv->clip.y2);

     (*devPriv->LoadColormap)(pmap);

}

/*
 * i128SetText(pScreen) - set screen into text mode
 */
void
i128SetText(pScreen)
     ScreenPtr pScreen;
{
     grafData *grafinfo = DDX_GRAFINFO(pScreen);
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *dac = i128Priv->info.global->dac_regs;
#ifdef DEBUG_PRINT
     ErrorF("i128SetText()\n");
#endif /* DEBUG_PRINT */

     I128_WAIT_UNTIL_READY(i128Priv->engine);

     switch (i128Priv->hardware.DAC_type)
     {

       case I128_DACTYPE_TIVPT:

         /* Reset DAC to VGA loop-through mode */
         dac[I128_DAC_VPT_INDEX] = VPT_Reset;
         dac[I128_DAC_VPT_DATA]  = 0x00;

         /* Turn off Sync-On-Green */
         dac[I128_DAC_VPT_INDEX] = VPT_GenCtrl;
         dac[I128_DAC_VPT_DATA]  = 0x00;
         break;


       case I128_DACTYPE_IBM528: /* S000 */

         dac[I128_DAC_IBM528_IDXHI]  = 0x00;
         dac[I128_DAC_IBM528_IDXCTL] = 0x01;

         /* Disable 64x64 Cursor */
         dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_CTRL;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
          
         /* Reset MUX modes to VGA */
         dac[I128_DAC_IBM528_IDXLOW] = IBM528_MISC_CTRL_1;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;

         /* Put all the other registers back where they were */
         dac[I128_DAC_IBM528_IDXLOW] = IBM528_MISC_CLK_CTRL;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x02;
         dac[I128_DAC_IBM528_DATA]   = 0x00;
         dac[I128_DAC_IBM528_DATA]   = 0x01;

         dac[I128_DAC_IBM528_IDXLOW] = IBM528_PIX_PLL_CTRL1;
         dac[I128_DAC_IBM528_DATA]   = 0x00;

         dac[I128_DAC_IBM528_IDXLOW] = IBM528_SYS_PLL_REF_DIV;
         dac[I128_DAC_IBM528_DATA]   = 0x08;
         dac[I128_DAC_IBM528_DATA]   = 0x41;
         break;

       default:
         break;

     }

     /* grafExec(grafinfo, "SetText", NULL); */
}

/*
 * i128SaveGState(pScreen) - save graphics info before screen switch
 */
void
i128SaveGState(pScreen)
     ScreenPtr pScreen;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     
#ifdef DEBUG_PRINT
     ErrorF("i128SaveGState()\n");
#endif /* DEBUG_PRINT */

}

/*
 * i128RestoreGState(pScreen) - restore graphics info from i128SaveGState()
 */
void
i128RestoreGState(pScreen)
     ScreenPtr pScreen;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
     ErrorF("i128RestoreGState()\n");
#endif /* DEBUG_PRINT */

}


void
i128SetMode(pScreen, mode, flags)
     ScreenPtr pScreen;
     struct Blackbird_Mode FAR *mode;
     int flags;
{
     volatile i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     int pixels_per_vclk;	/* For setting up the DAC */
     int oclock;		/* DAC output clock value */
     int iclock;		/* DAC input clock value */
     int mux1;                  /* DAC mux control 1 value */
     int mux2;                  /* DAC mux control 2 value */
     int key_ctrl;		/* DAC keying control value */
     struct Blackbird_Engine save;
     long buf_flags = I128_FLAG_SRC_DISP | I128_FLAG_DST_DISP;
     int i;
     volatile char *dac = i128Priv->info.global->dac_regs;
     static Mclock_initialized = 0;

     if (!Mclock_initialized)
     {
          if (i128Priv->hardware.mclock_numbers)
          {
               if (i128Priv->hardware.DAC_type == I128_DACTYPE_TIVPT)
               {
                    dac[I128_DAC_VPT_INDEX] = VPT_PLLCtrl;
                    dac[I128_DAC_VPT_DATA] = 0;

                    dac[I128_DAC_VPT_INDEX] = VPT_MclkPLL;
                    dac[I128_DAC_VPT_DATA] = i128Priv->hardware.mclock_numbers >> 8;
                    dac[I128_DAC_VPT_DATA] = i128Priv->hardware.mclock_numbers >> 16;
                    dac[I128_DAC_VPT_DATA] = i128Priv->hardware.mclock_numbers;
               }
               else if (i128Priv->hardware.DAC_type == I128_DACTYPE_IBM528)
               {
                    dac[I128_DAC_IBM528_IDXLOW] = IBM528_SYS_PLL_REF_DIV;
                    dac[I128_DAC_IBM528_DATA]   = i128Priv->hardware.mclock_numbers >> 8;
                    dac[I128_DAC_IBM528_DATA]   = i128Priv->hardware.mclock_numbers;
               }
          }
          Mclock_initialized = 1;
     }

     I128_set_engine(I128_SELECT_ENGINE_A);
     i128Priv->mode = *mode;
     I128_WAIT_UNTIL_READY(i128Priv->engine);

     /* Set up the video mode */
     i128Priv->info.global->syncs.config_1      = i128Priv->mode.config_1;
     i128Priv->info.global->syncs.config_2      = i128Priv->mode.config_2;

     i128Priv->info.global->syncs.int_vcount    = 0;
     i128Priv->info.global->syncs.int_hcount    = 0;
     i128Priv->info.global->syncs.address       = i128Priv->mode.display_start;
     i128Priv->info.global->syncs.pitch         = i128Priv->mode.bitmap_pitch;
     i128Priv->info.global->syncs.h_active      = i128Priv->mode.h_active;
     i128Priv->info.global->syncs.h_blank       = i128Priv->mode.h_blank;
     i128Priv->info.global->syncs.h_front_porch = i128Priv->mode.h_front_porch;
     i128Priv->info.global->syncs.h_sync        = i128Priv->mode.h_sync;
     i128Priv->info.global->syncs.v_active      = i128Priv->mode.v_active;
     i128Priv->info.global->syncs.v_blank       = i128Priv->mode.v_blank;
     i128Priv->info.global->syncs.v_front_porch = i128Priv->mode.v_front_porch;
     i128Priv->info.global->syncs.v_sync        = i128Priv->mode.v_sync;
     i128Priv->info.global->syncs.border        = i128Priv->mode.border;
     i128Priv->info.global->syncs.zoom          = i128Priv->mode.y_zoom;

     /* Set up the dac */
     pixels_per_vclk = mode->display_x / mode->h_active;

     if (i128Priv->hardware.DAC_type == I128_DACTYPE_IBM528)
     {
          i128Priv->mode.h_active	*= pixels_per_vclk;
          i128Priv->mode.h_blank	*= pixels_per_vclk;
          i128Priv->mode.h_front_porch	*= pixels_per_vclk;
          i128Priv->mode.h_sync		*= pixels_per_vclk;

      #if 1
          iclock = 128 / mode->selected_depth;
          i128Priv->info.global->syncs.h_active      = i128Priv->mode.h_active/iclock;
          i128Priv->info.global->syncs.h_blank       = i128Priv->mode.h_blank/iclock;
          i128Priv->info.global->syncs.h_front_porch = i128Priv->mode.h_front_porch/iclock;
          i128Priv->info.global->syncs.h_sync        = i128Priv->mode.h_sync/iclock;
      #else                     /* 0 */
          i128Priv->info.global->syncs.h_active      = i128Priv->mode.h_active >> 4;
          i128Priv->info.global->syncs.h_blank       = i128Priv->mode.h_blank >> 4;
          i128Priv->info.global->syncs.h_front_porch = i128Priv->mode.h_front_porch >> 4;
          i128Priv->info.global->syncs.h_sync        = i128Priv->mode.h_sync >> 4;
      #endif                    /* 0 */

          /***** =========== *****/
          dac[I128_DAC_IBM528_IDXHI]  = 0x00;
          dac[I128_DAC_IBM528_IDXCTL] = 0x01;

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_PLL_REGS; /* Program clock 0 */
          dac[I128_DAC_IBM528_DATA]   = mode->clock_numbers; /* VCO_DIV */
          dac[I128_DAC_IBM528_DATA]   = mode->clock_numbers >> 8; /* REF_DIV */

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_PIX_PLL_CTRL1;
          dac[I128_DAC_IBM528_DATA]   = 0x03; /* 8 M/N pairs in PLL */
          dac[I128_DAC_IBM528_DATA]   = 0x00; /* Select clock 0 (M0,N0) */

          /* Enable 64x64 Cursor */
          dac[I128_DAC_IBM528_IDXLOW] = IBM528_CURSOR_CTRL;
          dac[I128_DAC_IBM528_DATA]   = 0xFF;

          /* Set MUX modes */
          dac[I128_DAC_IBM528_IDXLOW] = IBM528_MISC_CTRL_1;
          dac[I128_DAC_IBM528_DATA]   = 0x03; /* Sense? ; 128 bit VRAM */
          dac[I128_DAC_IBM528_DATA]   = 0x45; /* Internal PLL; 8-bit; VRAM Ports */
          dac[I128_DAC_IBM528_DATA]   = 0x00;
          dac[I128_DAC_IBM528_DATA]   = 0x00;

          /* Put all the other registers back where they were */
          dac[I128_DAC_IBM528_IDXLOW] = IBM528_MISC_CLK_CTRL;
          dac[I128_DAC_IBM528_DATA]   = 0x81; /* Disable DDOT; Enable Pixel PLL */
          dac[I128_DAC_IBM528_DATA]   = 0x00;
          dac[I128_DAC_IBM528_DATA]   = 0x00;
          dac[I128_DAC_IBM528_DATA]   = 0x00;
          dac[I128_DAC_IBM528_DATA]   = 0x02;
          dac[I128_DAC_IBM528_DATA]   = 0x00;
          dac[I128_DAC_IBM528_DATA]   = 0x01;

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_PIXEL_FORMAT;
          switch (mode->selected_depth)
          {
             case 16:
               mux1 = 0x04;
               break;
             case 32:
               mux1 = 0x06;
               break;
             case 8:
             default:
               mux1 = 0x03;
               break;
          }
          dac[I128_DAC_IBM528_DATA]   = mux1; /* Bpp selector */
          dac[I128_DAC_IBM528_DATA]   = 0x00; /* 8 bpp uses LUT */
          dac[I128_DAC_IBM528_DATA]   = 0xC7; /* 16 bpp is 565; LIN; no LUT */
          dac[I128_DAC_IBM528_DATA]   = 0x00; /* 24 bpp undefined */
          dac[I128_DAC_IBM528_DATA]   = 0x03; /* 32 bpp no LUT */

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_PLL_REGS; /* Program clock 0 */
          dac[I128_DAC_IBM528_DATA]   = mode->clock_numbers; /* VCO_DIV */
          dac[I128_DAC_IBM528_DATA]   = mode->clock_numbers >> 8; /* REF_DIV */

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_PIX_PLL_CTRL1;
          dac[I128_DAC_IBM528_DATA]   = 0x03; /* 8 M/N pairs in PLL */
          dac[I128_DAC_IBM528_DATA]   = 0x00; /* Select clock 0 (M0,N0) */

          dac[I128_DAC_IBM528_IDXLOW] = IBM528_SYS_PLL_REF_DIV;
          dac[I128_DAC_IBM528_DATA]   = 0x08;
          dac[I128_DAC_IBM528_DATA]   = 0x41;
     }
     else if (i128Priv->hardware.DAC_type == I128_DACTYPE_TIVPT)
     {
          iclock = VPT_ICLK_INTERN;
          /*
           *if (mode->display_flags & CLOCK_DOUBLING)
           *  iclock |= VPT_ICLK_DOUBLE;
           */
          if (mode->clock_numbers == 0)
               iclock = 0;      /* If unset, use 25.175 MHz VGA clock */

          key_ctrl = VPT_KEY_SELECT_DIRECT;

          switch (mode->selected_depth)
          {
             case 8:
               oclock = VPT_OCLK_8;
               key_ctrl = VPT_KEY_SELECT_OVERLAY;
               mux1 = VPT_M1_8;
               mux2 = VPT_M2_8;
               break;
             case 16:
               oclock = VPT_OCLK_16;
               mux1 = VPT_M1_16_565;
               mux2 = VPT_M2_16_565;
               break;
             case 32:
               oclock = VPT_OCLK_32;
               mux1 = VPT_M1_32;
               mux2 = VPT_M2_32;
               break;
             default:
               oclock = VPT_M2_VGA;
               mux1 = VPT_M1_VGA;
               mux2 = VPT_M2_VGA;
               iclock = VPT_ICLK_VGA;
               break;
          }

          switch (pixels_per_vclk)
          {
             case 1:
               oclock |= VPT_OCLK_V1;
               break;
             case 2:
               oclock |= VPT_OCLK_V2;
               break;
             case 4:
               oclock |= VPT_OCLK_V4;
               break;
             case 8:
               oclock |= VPT_OCLK_V8;
               break;
             case 16:
               oclock |= VPT_OCLK_V16;
               break;
             case 32:
               oclock |= VPT_OCLK_V32;
               break;
             default:
               break;
          }

          dac[I128_DAC_VPT_INDEX] = VPT_MuxCtrl1;
          dac[I128_DAC_VPT_DATA] = mux1;

          dac[I128_DAC_VPT_INDEX] = VPT_MuxCtrl2;
          dac[I128_DAC_VPT_DATA] = mux2;

          dac[I128_DAC_VPT_INDEX] = VPT_OutputClk;
          dac[I128_DAC_VPT_DATA] = oclock;

          dac[I128_DAC_VPT_INDEX] = VPT_GenCtrl;
          dac[I128_DAC_VPT_DATA] = VPT_GEN_SPLIT_SHIFT;

          if (mode->display_flags & SYNC_ON_GREEN)
               dac[I128_DAC_VPT_DATA] |= VPT_GEN_SOG;

          dac[I128_DAC_VPT_INDEX] = VPT_AuxCtrl;
          dac[I128_DAC_VPT_DATA] = VPT_AUX_SCLK;

          dac[I128_DAC_VPT_INDEX] = VPT_MiscCtrl;
          /*dac[I128_DAC_VPT_DATA] = 0x00;*/
          dac[I128_DAC_VPT_DATA] = 0x0C; /* 8/6 */
          /*dac[I128_DAC_VPT_DATA] = VPT_MISC_LOOP_ENABLE;*/

          dac[I128_DAC_VPT_INDEX] = VPT_KeyCtrl;
          dac[I128_DAC_VPT_DATA] = key_ctrl;

          /* Set up the frequency synthesizer */
          /* === Assume clock_numbers are set */
          /* Otherwise, need to calculate clock_numbers from clock_frequency. */
          dac[I128_DAC_VPT_INDEX] = VPT_PLLCtrl;
          dac[I128_DAC_VPT_DATA] = 2;
          dac[I128_DAC_VPT_INDEX] = VPT_DclkPLL;
          dac[I128_DAC_VPT_DATA] = 2; /* high bit off */

          dac[I128_DAC_VPT_INDEX] = VPT_PLLCtrl;
          dac[I128_DAC_VPT_DATA] = 0;

          dac[I128_DAC_VPT_INDEX] = VPT_DclkPLL;
          dac[I128_DAC_VPT_DATA] = mode->clock_numbers >> 8;
          dac[I128_DAC_VPT_DATA] = mode->clock_numbers >> 16;

          nap(1000);            /* Delay a little before turning on clock */
          dac[I128_DAC_VPT_DATA] = mode->clock_numbers;

          dac[I128_DAC_VPT_INDEX] = VPT_InputClk;
          dac[I128_DAC_VPT_DATA] = iclock;
     }

     /* Set up the drawing engine */
     nap(1000);
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->buf_control = 0L;
     
     switch (mode->selected_depth)
     {
        case 8:
          buf_flags |= I128_FLAG_8_BPP;
          break;
        case 16:
          buf_flags |= I128_FLAG_16_BPP;
          break;
        case 32:
          buf_flags |= I128_FLAG_32_BPP;
          break;
        default:
          break;
     }

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->msk_source = mode->display_start;
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->src_origin = mode->display_start;
     i128Priv->engine->src_pitch = mode->bitmap_pitch;
     i128Priv->engine->buf_control &= ~I128_FLAG_SRC_BITS;
     i128Priv->engine->buf_control |= I128_FLAG_SRC_BITS & buf_flags;
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->dst_origin = mode->display_start;
     i128Priv->engine->dst_pitch = mode->bitmap_pitch;
     i128Priv->engine->buf_control &= ~I128_FLAG_DST_BITS;
     i128Priv->engine->buf_control |= I128_FLAG_DST_BITS & buf_flags;
     i128Priv->engine->clip_top_left = I128_XY(0, 0);
     i128Priv->engine->clip_bottom_right =
          I128_XY(mode->bitmap_width-1, mode->bitmap_height-1);
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->plane_mask = 0xFFFFFFFF;
     i128Priv->engine->rop_mask = 0x0;
     i128Priv->engine->cmd = (I128_ROP_COPY |
                          I128_CMD_SOLID |
                          I128_CMD_CLIP_IN);
     i128Priv->engine->page = 0x0;
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->foreground = 0xFFFFFFFF;
     i128Priv->engine->background = 0x0;
     i128Priv->engine->key = 0x0;
     i128Priv->engine->key_data = I128_KEY_FLAG_ZERO;
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->engine->line_pattern = 0xFFFFFFFF;
     i128Priv->engine->pattern_ctrl = 0x0;

     I128_WAIT_UNTIL_READY(i128Priv->engine);
     i128Priv->info.memwins[0]->origin     = 0;
     i128Priv->info.memwins[0]->page       = 0;
     switch (i128Priv->mode.pixelsize)
     {
       case 1:
         i128Priv->info.memwins[0]->control    =
             (I128_MEMW_DST_DISP |
              I128_MEMW_8_BPP);
         break;
       case 2:
         i128Priv->info.memwins[0]->control    =
             (I128_MEMW_DST_DISP |
              I128_MEMW_16_BPP);
         break;
       default:
         i128Priv->info.memwins[0]->control    =
             (I128_MEMW_DST_DISP |
              I128_MEMW_32_BPP);
         break;
     }
     i128Priv->info.memwins[0]->key        = 0;
     i128Priv->info.memwins[0]->key_data   = 0;
     i128Priv->info.memwins[0]->msk_source = 0;

     if (flags & I128_MODE_CLEAR)
     {
          int h = i128Priv->info.Disp_size / mode->bitmap_pitch;

          I128_WAIT_UNTIL_READY(i128Priv->engine);
          i128Priv->engine->plane_mask = 0xFFFFFFFF;
          i128Priv->engine->foreground = 0x0;
          i128Priv->engine->xy3 = I128_DIR_LR_TB;
          i128Priv->engine->xy4 = I128_BLIT_NO_ZOOM;
          i128Priv->engine->cmd = (I128_OPCODE_BITBLT |
                                   I128_ROP_COPY |
                                   I128_CMD_SOLID);

          I128_BLIT(0, 0, 0, 0, mode->bitmap_width, h,
			I128_DIR_LR_TB);

     }
}
