/*
 * @(#) i128Init.c 11.1 97/10/22
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
 * Modification History
 *
 * S000, Tue Mar 26 11:02:45 PST 1996, kylec@sco.com
 *	Check for supported DAC types.
 *
 */


#undef USE_INLINE_CODE

#include <sys/types.h>
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

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "i128Defs.h"
#include "i128Procs.h"
#include "i128PCI.h"
                                       
extern scoScreenInfo i128SysInfo;
extern VisualRec i128Visual, i128Visual16, i128Visual24;
extern nfbGCOps i128SolidPrivOps;

typedef void (*SetColorPtr)(unsigned int cmap,
                            unsigned int index,
                            unsigned short r,
                            unsigned short g,
                            unsigned short b,
                            ScreenPtr pScreen);


static int i128Generation = -1;
unsigned int i128ScreenPrivateIndex = -1;

#ifdef I128_FAST_GC_OPS
#include "i128GCFuncs.h"
unsigned int i128GCPrivateIndex = -1;
#endif

/* Static functions */
static unsigned short _i128_ReadPCIWord (int board_num, short reg);
static unsigned long _i128_ReadPCIDWord (int board_num, short reg);
static unsigned long _i128_ReadIODWord (unsigned short ioaddr);
static void _i128_WriteIODWord (unsigned short ioaddr, unsigned long data);
static unsigned char _i128_ReadPCIByte (int board_num, short reg);
static int _i128_FindBoard (int board_num, unsigned short *i128_ioaddr);


/*
 * i128Setup() - initializes information needed by the core server
 * prior to initializing the hardware.
 *
 */
Bool
i128Setup(ddxDOVersionID version,ddxScreenRequest *pReq)
{
     return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

/*
 * i128InitHW()
 *
 * Initialize hardware that only needs to be done ONCE.  This routine will
 * not be called on a screen switch.  It may just call i128SetGraphics()
 */
Bool
i128InitHW(pScreen)
     ScreenPtr pScreen;
{
     grafData *grafinfo = DDX_GRAFINFO(pScreen);
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     unsigned short reply;
     unsigned long pcicmd;
     unsigned short i128_ioaddr;
     int board_num;

#ifdef DEBUG_PRINT
	ErrorF("i128InitHW()\n");
#endif /* DEBUG_PRINT */

     /* Hardware defaults */
     I128_hardware.mclock_numbers = I128_MCLOCK_NUMBERS;
     I128_hardware.DAC_type = I128_DACTYPE_TIVPT;
     I128_hardware.disp_size = 1;

     grafGetInt(grafinfo, "MCLOCK_NUMBERS", &I128_hardware.mclock_numbers);
     grafGetInt(grafinfo, "DAC_TYPE", &I128_hardware.DAC_type);
     grafGetInt(grafinfo, "PIXELSIZE", &I128_hardware.disp_size);

     switch (i128Priv->hardware.DAC_type) /* S000 */
     {
       case I128_DACTYPE_TIVPT:
       case I128_DACTYPE_IBM528: 
         break;

       default:                
         ErrorF("i128: DAC not supported.\n");
         return(FALSE);
     }

     /* Which board */
     I128_hardware.board_num = 0;             /* default */
     grafGetInt(grafinfo, "BOARD_NUMBER", &I128_hardware.board_num);

     /* Get IO Space base address */
     if (!grafGetInt(grafinfo, "IO_BASE", &i128Priv->iobase)) {
          ErrorF ("i128: Missing IO_BASE in grafinfo file.\n");
          return (FALSE);
     }

     /* Get access to memory */
     if (!grafGetMemInfo(grafinfo, "MW0",
                         NULL, NULL, &i128Priv->mw0_addr))
     {
          ErrorF ("i128: Missing MW0 memory in grafinfo file.\n");
          return (FALSE);
     }

     if (!grafGetMemInfo(grafinfo, "MW1",
                         NULL, NULL, &i128Priv->mw1_addr)) {
          ErrorF ("i128: Missing MW1 memory in grafinfo file.\n");
          return (FALSE);
     }

     if (!grafGetMemInfo(grafinfo, "XY0",
                         NULL, NULL, &i128Priv->xy0_addr)) {
          ErrorF ("i128: Missing XY0 memory in grafinfo file.\n");
          return (FALSE);
     }

     if (!grafGetMemInfo(grafinfo, "RBASE_X",
                         NULL, NULL, &i128Priv->rbase_x_addr)) {
          ErrorF ("i128: Missing RBASE_X memory in grafinfo file.\n");
          return (FALSE);
     }

     reply = _i128_FindBoard(I128_hardware.board_num, &i128_ioaddr);

     if (reply || (i128_ioaddr == 0))
          return FALSE;
     else
          i128Priv->iobase = (char*)i128_ioaddr;

     /* Check if board is disabled */
     if ((_i128_ReadPCIWord(I128_hardware.board_num, 0x04) & 1) == 0)
         return FALSE;

     i128SetGraphics(pScreen);
     return (TRUE);
}

/*
 * i128Init() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
i128Init(index, pScreen, argc, argv)
     int index;
     ScreenPtr pScreen;
     int argc;
     char **argv;
{
     grafData *grafinfo = DDX_GRAFINFO(pScreen);
     nfbScrnPrivPtr pNfb;
     i128PrivatePtr i128Priv;
     int width, height, mmx, mmy, pixelsize, y_max;
     extern Bool enforceProtocol;
     Bool newGeneration = FALSE;

     if (i128Generation != serverGeneration) {
          i128Generation = serverGeneration;
          i128ScreenPrivateIndex = AllocateScreenPrivateIndex();
          if ( i128ScreenPrivateIndex < 0 )
               return FALSE;
          newGeneration = TRUE;
     }

     i128Priv = (i128PrivatePtr)xalloc(sizeof(i128Private));
     if ( i128Priv == NULL )
          return FALSE;
     pScreen->devPrivates[i128ScreenPrivateIndex].ptr =
          (unsigned char *)i128Priv;

     /* Get mode and monitor info */
     if (!grafGetInt(grafinfo, "PIXWIDTH",  &width) ||
         !grafGetInt(grafinfo, "PIXHEIGHT", &height) ||
         !grafGetInt(grafinfo, "PIXELSIZE", &pixelsize))
     {
          ErrorF("i128: can't find pixel info in grafinfo file.\n");
          return FALSE;
     }

     if (!grafGetInt(grafinfo, "DISPLAY_FLAGS", &I128_mode.display_flags) ||
         !grafGetInt(grafinfo, "CONFIG_1", &I128_mode.config_1) ||
         !grafGetInt(grafinfo, "CONFIG_2", &I128_mode.config_2) ||
         !grafGetInt(grafinfo, "CLOCK_FREQUENCY", &I128_mode.clock_frequency)||
         !grafGetInt(grafinfo, "CLOCK_NUMBERS", &I128_mode.clock_numbers) ||
         !grafGetInt(grafinfo, "H_ACTIVE", &I128_mode.h_active) ||
         !grafGetInt(grafinfo, "H_BLANK", &I128_mode.h_blank) ||
         !grafGetInt(grafinfo, "H_FRONT_PORCH", &I128_mode.h_front_porch) ||
         !grafGetInt(grafinfo, "H_SYNC", &I128_mode.h_sync) ||
         !grafGetInt(grafinfo, "V_ACTIVE", &I128_mode.v_active) ||
         !grafGetInt(grafinfo, "V_BLANK", &I128_mode.v_blank) ||
         !grafGetInt(grafinfo, "V_FRONT_PORCH", &I128_mode.v_front_porch) ||
         !grafGetInt(grafinfo, "V_SYNC", &I128_mode.v_sync))
     {
          ErrorF("i128: can't find mode info in grafinfo file.\n");
          return FALSE;
     }

     I128_mode.display_start = 0;
     I128_mode.bitmap_width = I128_mode.display_x = width;
     I128_mode.bitmap_height = I128_mode.display_y = height;
     I128_mode.bitmap_pitch = width * pixelsize;
     I128_mode.selected_depth = pixelsize * 8; /* bits */
     I128_mode.pixelsize = pixelsize;
     I128_mode.border = 0;
     I128_mode.y_zoom = 0;

     /* Setup raster ops */
     i128Priv->rop[GXclear] = I128_ROP_CLEAR;
     i128Priv->rop[GXand] = I128_ROP_AND;
     i128Priv->rop[GXandReverse] = I128_ROP_ANDREVERSE;
     i128Priv->rop[GXcopy] = I128_ROP_COPY;
     i128Priv->rop[GXandInverted] = I128_ROP_ANDINVERTED;
     i128Priv->rop[GXnoop] = I128_ROP_NOOP;
     i128Priv->rop[GXxor] = I128_ROP_XOR;
     i128Priv->rop[GXor] = I128_ROP_OR;
     i128Priv->rop[GXnor] = I128_ROP_NOR;
     i128Priv->rop[GXequiv] = I128_ROP_EQUIV;
     i128Priv->rop[GXinvert] = I128_ROP_INVERT;
     i128Priv->rop[GXorReverse] = I128_ROP_ORREVERSE;
     i128Priv->rop[GXcopyInverted] = I128_ROP_COPYINVERTED;
     i128Priv->rop[GXorInverted] = I128_ROP_ORINVERTED;
     i128Priv->rop[GXnand] = I128_ROP_NAND;
     i128Priv->rop[GXset] = I128_ROP_SET;

     /* Monitor info */
     mmx = 300; mmy = 300;      /* Reasonable defaults */
     grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
     grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

     if (!nfbScreenInit(pScreen, width, height, mmx, mmy))
          return FALSE;

     /* Do this after nfbScreenInit() to avoid bug in server */
     if (newGeneration)
     {
#ifdef I128_FAST_GC_OPS
         i128GCPrivateIndex = AllocateGCPrivateIndex();
         if (i128GCPrivateIndex < 0)
             return FALSE;
         AllocateGCPrivate(pScreen, i128GCPrivateIndex,
                           sizeof(i128GCPrivate));
#endif
     }

     pNfb = NFB_SCREEN_PRIV(pScreen);

     switch (pixelsize)
     {
        case 1:
          if (!nfbAddVisual(pScreen, &i128Visual))
               return FALSE;
          pNfb->SetColor = i128SetColor;
          pNfb->LoadColormap = genLoadColormap;
          break;

        case 2:
          if (!nfbAddVisual(pScreen, &i128Visual16))
               return FALSE;
          pNfb->SetColor = (SetColorPtr)NoopDDA;
          pNfb->LoadColormap = NoopDDA;
          break;

        case 4:
          if (!nfbAddVisual(pScreen, &i128Visual24))
               return FALSE;
          pNfb->SetColor = (SetColorPtr)NoopDDA;
          pNfb->LoadColormap = NoopDDA;
          break;

        default:
          ErrorF("i128: pixel size of %d is not supported.\n", pixelsize);
          return FALSE;
     }

     pNfb->protoGCPriv->ops = &i128SolidPrivOps;
     pNfb->BlankScreen = i128BlankScreen;
     pNfb->ValidateWindowPriv = i128ValidateWindowPriv;
     pNfb->clip_count = 1;

     if (!i128InitHW(pScreen))
          return FALSE;

     i128CursorInitialize(pScreen);

#ifdef I128_FAST_GC_OPS
     i128Priv->CreateGC = pScreen->CreateGC;
     pScreen->CreateGC = i128CreateGC;
#endif

#if 0
     i128Priv->CreatePixmap = pScreen->CreatePixmap;
     pScreen->CreatePixmap = i128CreatePixmap;
     i128Priv->DestroyPixmap = pScreen->DestroyPixmap;
     pScreen->DestroyPixmap = i128DestroyPixmap;
#endif

     /*
      * This should work for most cases.
      */
     if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
          cfbCreateDefColormap(pScreen)) == 0 )
          return FALSE;

     /* 
      * Give the sco layer our screen switch functions.  
      * Always do this last.
      */
     scoSysInfoInit(pScreen, &i128SysInfo);

     /*
      * Set any NFB runtime options here - see potential list
      *	in ../../nfb/nfbDefs.h
      */
     /* nfbSetOptions(pScreen, NFB_VERSION, NFB_POLYBRES, 0); */
     if (enforceProtocol)
          nfbSetOptions(pScreen, NFB_VERSION, 0, 0);
     else
          nfbSetOptions(pScreen, NFB_VERSION, NFB_PT2PT_LINES, 0);

     
     /* Cache */
     y_max = i128Priv->info.Disp_size / i128Priv->mode.bitmap_pitch;
     i128Priv->cache.x1 = 0;
     i128Priv->cache.y1 = i128Priv->mode.bitmap_height;
     i128Priv->cache.x2 = i128Priv->mode.bitmap_width;
     i128Priv->cache.y2 = y_max - i128Priv->cache.y1;

     return TRUE;

}

/*
 * i128CloseScreen()
 *
 * Anything you allocate in i128Init() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
void
i128CloseScreen(index, pScreen)
     int index;
     ScreenPtr pScreen;
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
	ErrorF("i128CloseScreen()\n");
#endif /* DEBUG_PRINT */

     xfree(i128Priv);
}



/* This routine maps the I-128 registers to the given physical address. */
/* Upon return from this routine, I128_info is set up and I128_engine */
/* is pointing to engine A. */
int
i128EnableMemory(ScreenPtr pScreen)
{
     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     long config1;
     long config2;
     int  selector;
     long linear_addr;
     char FAR *farptr;
     unsigned long rbase_physaddr = _i128_ReadPCIDWord(I128_hardware.board_num,0x20) & 0xFFFFFF00;
     unsigned long lin0_physaddr  = _i128_ReadPCIDWord(I128_hardware.board_num,0x10) & 0xFFFFFF00;
     unsigned long lin1_physaddr  = _i128_ReadPCIDWord(I128_hardware.board_num,0x14) & 0xFFFFFF00;
     unsigned long xya_physaddr   = _i128_ReadPCIDWord(I128_hardware.board_num,0x18) & 0xFFFFFF00;
     unsigned long base_physaddr;
     unsigned short ioaddr;
     unsigned long pcicmd;


     /* Turn on IO and memory accesses to Blackbird */
     pcicmd = _i128_ReadPCIDWord(I128_hardware.board_num,0x04);

     ioaddr = (unsigned short)i128Priv->iobase;

     config1  = _i128_ReadIODWord(ioaddr + I128_OFFSET_CONFIG1);
     config1 &= 0xF300201D;	/* Leave default values */
     config1 |= (I128_C1_ENABLE_GLOBAL |
                 I128_C1_ENABLE_MEMWIN |
                 I128_C1_ENABLE_DRAW_A |
                 I128_C1_ENABLE_INT);
     _i128_WriteIODWord(ioaddr + I128_OFFSET_CONFIG1, config1);

     /* farptr = map_physical(rbase_physaddr, 0x10000); */
     farptr = i128Priv->rbase_x_addr;
     /* Set up pointers to blackbird data areas */
     I128_info.global     = (struct Blackbird_Global     FAR *)(farptr);
     I128_info.memwins[0] = (struct Blackbird_Memwin     FAR *)(farptr + 0x2000);
     I128_info.memwins[1] = (struct Blackbird_Memwin     FAR *)(farptr + 0x2028);
     I128_info.engine_a   = (struct Blackbird_Engine     FAR *)(farptr + 0x4000);
     I128_info.engine_b   = (struct Blackbird_Engine     FAR *)(farptr + 0x6000);
     I128_info.global_int = (struct Blackbird_Interrupts FAR *)(farptr + 0x8000);
     I128_engine = I128_info.engine_a;

     /* farptr = map_physical(xya_physaddr, 0x400000); */
     farptr = i128Priv->xy0_addr;
     /* Set up XY window A address and size */
     I128_engine->xyw_adsz = xya_physaddr + I128_XYWIN_SZ_4M;
     I128_info.xy_win[0].pointer = farptr;
     I128_info.xy_win[0].physical_host_address = xya_physaddr;
     I128_info.xy_win[0].length = 0x400000;

     /* farptr = map_physical(lin0_physaddr, 0x400000); */
     farptr = i128Priv->mw0_addr;
     /* Set up linear window 0 address and size */
     I128_info.memwins[0]->address = lin0_physaddr;
     I128_info.memwins[0]->size = I128_MEMW_SZ_4M;
     I128_info.memwin[0].pointer = farptr;
     I128_info.memwin[0].physical_host_address = lin0_physaddr;
     I128_info.memwin[0].length = 0x400000;

     /* farptr = map_physical(lin1_physaddr, 0x400000); */
     farptr = i128Priv->mw1_addr;
     /* Set up data for linear window 1 */
     I128_info.memwins[1]->address = lin1_physaddr;
     I128_info.memwins[1]->size = I128_MEMW_SZ_4M;
     I128_info.memwin[1].pointer = farptr;
     I128_info.memwin[1].physical_host_address = lin1_physaddr;
     I128_info.memwin[1].length = 0x400000;

     /* Enable linear window 0 and XY window A */
     config1 |= I128_C1_ENABLE_MEMWIN0
          |  I128_C1_ENABLE_MEMWIN1
               |  I128_C1_ENABLE_XY_A;
     _i128_WriteIODWord(ioaddr + I128_OFFSET_CONFIG1, config1);

     /* XY window B is disabled */

     /* Set config2 register to reduce wait states and enable display memory */
     config2  = _i128_ReadIODWord(ioaddr + I128_OFFSET_CONFIG2);
     config2 &= 0xFF000F00;	/* Turn off all wait states (except EPROM) */
     _i128_WriteIODWord(ioaddr + I128_OFFSET_CONFIG2, config2 | I128_C2_BUFFER_ENABLE
                  | I128_C2_DELAY_SAMPLE);
     /* === Possibly turn on more CONFIG2 flags */

     /* Set up info structure */
     I128_info.IO_Address = ioaddr;
     I128_info.IO_Regs.rbase_g     = _i128_ReadIODWord(ioaddr + 0x00);
     I128_info.IO_Regs.rbase_w     = _i128_ReadIODWord(ioaddr + 0x04);
     I128_info.IO_Regs.rbase_a     = _i128_ReadIODWord(ioaddr + 0x08);
     I128_info.IO_Regs.rbase_b     = _i128_ReadIODWord(ioaddr + 0x0C);
     I128_info.IO_Regs.rbase_i     = _i128_ReadIODWord(ioaddr + 0x10);
     I128_info.IO_Regs.rbase_e     = _i128_ReadIODWord(ioaddr + 0x14);
     I128_info.IO_Regs.id          = _i128_ReadIODWord(ioaddr + 0x18);
     I128_info.IO_Regs.config1     = _i128_ReadIODWord(ioaddr + 0x1C);
     I128_info.IO_Regs.config2     = _i128_ReadIODWord(ioaddr + 0x20);
     I128_info.IO_Regs.soft_switch = _i128_ReadIODWord(ioaddr + 0x28);

     I128_info.Disp_size = 2 * 1024L * 1024;
     if (I128_info.IO_Regs.id & I128_ID_DISP_2_BANKS)
          I128_info.Disp_size <<= 1;
     if ((I128_info.IO_Regs.id & I128_ID_DISP_BITS) == I128_ID_DISP_NONE)
          I128_info.Disp_size   = 0;
     I128_info.Virt_size = 2 * 1024L * 1024;
     if (I128_info.IO_Regs.id & I128_ID_VIRT_2_BANKS)
          I128_info.Virt_size <<= 1;
     if ((I128_info.IO_Regs.id & I128_ID_VIRT_BITS) == I128_ID_VIRT_NONE)
          I128_info.Virt_size   = 0;
     if ((I128_info.IO_Regs.id & I128_ID_VIRT_BITS) == I128_ID_VIRT_1MxN)
          I128_info.Virt_size <<= 2;
     I128_info.Mask_size = 0;
     I128_info.Mask_size = 2 * 1024L * 1024;
     if ((I128_info.IO_Regs.id & I128_ID_MASK_BITS) == I128_ID_MASK_NONE)
          I128_info.Mask_size   = 0;
     if ((I128_info.IO_Regs.id & I128_ID_MASK_BITS) == I128_ID_MASK_1MxN)
          I128_info.Mask_size <<= 2;
     if (I128_info.IO_Regs.id & I128_ID_DATA_BUS_SIZE)
     {
          I128_info.Disp_size <<= 1;
          I128_info.Virt_size <<= 1;
          I128_info.Mask_size <<= 1;
     }
     I128_info.Blackbird_ID = I128_info.IO_Regs.id & I128_ID_REVISION;

     /* Fill in these four final flags */
     /* === */
     I128_info.max_pixel_rate = 0;
     I128_info.hard_flags = 0;
     I128_info.crystal_rate = 0;
     I128_info.video_flags = I128_VGA_ENABLED;
     if (_i128_ReadPCIByte(I128_hardware.board_num,0xA) != 0)
         I128_info.video_flags &= ~I128_VGA_ENABLED;

}


/* static functions */

static unsigned long
_i128_ReadIODWord(unsigned short ioaddr)
{
     return ind(ioaddr);
}


static void
_i128_WriteIODWord(unsigned short ioaddr, unsigned long data)
{
     outd(ioaddr, data);
}


static int
_i128_FindBoard (int board_num, unsigned short *i128_ioaddr)
{
    void *handle = pci_open(I128_PCI_DEVICE_ID, I128_PCI_VENDOR_ID, board_num);

    if (!handle)
    {
        return -1;
    }
    else
    {
        unsigned long io_base;
        pci_read(handle, 0x24, &io_base, PCI_WORD); /* PCI Base 5 */
        pci_close(handle);
        *i128_ioaddr = io_base & 0x0000FF00;
        return 0;
    }
}

static unsigned char
_i128_ReadPCIByte( int board_num, short reg )
{
    void *handle = pci_open(I128_PCI_DEVICE_ID, I128_PCI_VENDOR_ID, board_num);

    if (!handle)
    {
        return -1;
    }
    else
    {
        unsigned long data;
        pci_read(handle, reg, &data, PCI_BYTE);
        pci_close(handle);
        return (unsigned char)data;
    }
}


static unsigned short
_i128_ReadPCIWord( int board_num, short reg )
{
    void *handle = pci_open(I128_PCI_DEVICE_ID, I128_PCI_VENDOR_ID, board_num);

    if (!handle)
    {
        return -1;
    }
    else
    {
        unsigned long data;
        pci_read(handle, reg, &data, PCI_WORD);
        pci_close(handle);
        return (unsigned short)data;
    }
}


/* read from a configuration register (#9 PCI routine #9) */

unsigned long
_i128_ReadPCIDWord( int board_num, short reg )
{
    void *handle = pci_open(I128_PCI_DEVICE_ID, I128_PCI_VENDOR_ID, board_num);

    if (!handle)
    {
        return -1;
    }
    else
    {
        unsigned long data;
        pci_read(handle, reg, &data, PCI_DWORD);
        pci_close(handle);
        return (unsigned long)data;
    }
}



