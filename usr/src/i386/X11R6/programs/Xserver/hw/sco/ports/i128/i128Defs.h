/*
 * @(#) i128Defs.h 11.1 97/10/22
 *
 * Copyright (C) 1993-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

#ifndef I128DEFS_INCLUDE
#define I128DEFS_INCLUDE

#include "gcstruct.h"

/* OS */
#define FAR
#define CANCEL
#define I128_NOOP
#define SYNC_ON_GREEN        0x0100   /* Our old friend... */

/* PCI defines */
#define I128_PCI_VENDOR_ID     0x105D
#define I128_PCI_DEVICE_ID     0x2309

/* Flags */
#define I128_SELECT_ENGINE_A   0
#define I128_SELECT_ENGINE_B   1

#define I128_FILE_ERROR        (-1)    /* A file error occured */
#define I128_MODE_ERROR        (-2)    /* The mode fails on this board */
#define I128_MODE_VGA          (-1)    /* Set VGA mode */
#define I128_MODE_DEFAULT      (-2)    /* Set the default mode */
#define I128_MODE_CLEAR_DISP   0x0001  /* Clear the display screen */
#define I128_MODE_CLEAR_VIRT   0x0002  /* Clear the virtual page */
#define I128_MODE_CLEAR_MASK   0x0004  /* Clear the mask page */
#define I128_MODE_CLEAR        0x0007  /* Clear all the screen pages */
#define I128_MODE_ENGINE_INIT  0x0010  /* Initialize the drawing engine */
#define I128_NO_ERROR          0       /* if no errors in I128_set_mode */
#define I128_HARD_ERROR        (-3)    /* if a hardware error occured */
#define I128_RANGE_ERROR       (-4)    /* Address is out of range */

/* Parameters for the source bitmap */
#define I128_FLAG_SRC_BITS     0x43000300      /* Mask to set src flags */
#define I128_FLAG_SRC_DISP     0x00000000      /* Select Disp as source */
#define I128_FLAG_SRC_VIRT     0x00000100      /* Select Virt as source */
#define I128_FLAG_SRC_MASK     0x00000200      /* Select MASK plane as src */
#define I128_FLAG_SRC_CACHE    0x00000300      /* Select host cache as src */
#define I128_FLAG_8_BPP        0x00000000      /* Use  8 bits per pixel */
#define I128_FLAG_16_BPP       0x01000000      /* Use 16 bits per pixel */
#define I128_FLAG_32_BPP       0x02000000      /* Use 32 bits per pixel */
#define I128_FLAG_NO_BPP       0x03000000      /* Don't use bits per pixel */
#define I128_FLAG_CACHE_ON     0x40000000      /* Cache is always on */
#define I128_FLAG_CACHE_READY  0x80000000      /* Set this when cache ready */

/* Parameters for the destination bitmap */
#define I128_FLAG_DST_BITS     0x037F7C0E      /* Mask to set dst flags */
#define I128_FLAG_DST_DISP     0x00000000      /* Select DISP as dst */
#define I128_FLAG_DST_VIRT     0x00000400      /* Select VIRT as dst */
#define I128_FLAG_DST_MASK     0x00000800      /* Select MASK plane as dst */
#define I128_FLAG_DST_NONE     0x00000C00      /* No destination selected */
#define I128_FLAG_SHADOW_D     0x00001000      /* Also write to DISP */
#define I128_FLAG_SHADOW_V     0x00002000      /* Also write to VIRT */
#define I128_FLAG_SHADOW_M     0x00004000      /* Also write to MASK */
#define I128_FLAG_MASK_DISP    0x00000008      /* Apply MASK mask to DISP */
#define I128_FLAG_MASK_VIRT    0x00000002      /* Apply MASK mask to VIRT */
#define I128_FLAG_MASK_ZERO    0x00000000      /* Allow write when MASK=1 */
#define I128_FLAG_MASK_ONE     0x00000004      /* Allow write when MASK=0 */
#define I128_FLAG_MASK_KEY     0x00200000      /* Set MASK when color=key */
#define I128_FLAG_MASK_PLANAR  0x00400000      /* Set MASK bits planar mode */
#define I128_FLAG_PLANE(x)     ((long)((x) & 0x1F) << 16) /* Set MASK plane */

/* Various flags for most drawing functions */
#define I128_CMD_ROP_SHIFT     8               /* if using CMD_ROP reg */
#define I128_ROP_CLEAR         0x00000000      /* Set dest to Zero */
#define I128_ROP_NOR           0x00000100      /* (not src) and (not dst) */
#define I128_ROP_ANDINVERTED   0x00000200      /* (not src) and dest */
#define I128_ROP_COPYINVERTED  0x00000300      /* not src */
#define I128_ROP_ANDREVERSE    0x00000400      /* src and (not dest) */
#define I128_ROP_INVERT        0x00000500      /* not dest */
#define I128_ROP_XOR           0x00000600      /* src xor dest */
#define I128_ROP_NAND          0x00000700      /* (not src) or (not dest) */
#define I128_ROP_AND           0x00000800      /* src and dest */
#define I128_ROP_EQUIV         0x00000900      /* (not src) xor dest */
#define I128_ROP_NOOP          0x00000A00      /* dest */
#define I128_ROP_ORINVERTED    0x00000B00      /* (not src) or dest */
#define I128_ROP_COPY          0x00000C00      /* src */
#define I128_ROP_ORREVERSE     0x00000D00      /* src or (not dest) */
#define I128_ROP_OR            0x00000E00      /* src or dest */
#define I128_ROP_SET           0x00000F00      /* Set dest to 1 */

#define I128_OPCODE_NOOP       0x00            /* No drawing operation */
#define I128_OPCODE_BITBLT     0x01            /* Do a bit block transfer */
#define I128_OPCODE_LINE       0x02            /* Draw a line */
#define I128_OPCODE_ELINE      0x03            /* Draw line, error terms */
#define I128_OPCODE_TRIANGLE   0x04            /* Draw a triangle or quad */
#define I128_OPCODE_RXFER      0x06            /* Transfer data to host */
#define I128_OPCODE_WXFER      0x07            /* Transfer data from host */
    
#define I128_CMD_STYLE_SHIFT   16              /* If using CMD_STYLE reg */
#define I128_CMD_SOLID         0x00010000      /* Set to foreground color */
#define I128_CMD_TRANSPARENT   0x00020000      /* Background transparency */
#define I128_CMD_STIPPLE_OFF   0x00000000      /* No stipple */
#define I128_CMD_STIPPLE_PLANAR 0x00040000     /* Stipple is planar mode. */
#define I128_CMD_STIPPLE_PACK32 0x00080000     /* packed, padded to 32 bits */
#define I128_CMD_STIPPLE_PACK8 0x000C0000      /* packed, padded to 8 bits */
#define I128_CMD_EDGE_INCLUDE  0x00100000      /* Edge include mode */
#define I128_CMD_AREAPAT_OFF   0x00000000      /* No area pattern */
#define I128_CMD_AREAPAT_8x8   0x01000000      /* Source is 8x8 pattern. */
#define I128_CMD_AREAPAT_32x32 0x02000000      /* Source is 32x32 pattern.*/
#define I128_CMD_NO_LAST       0x04000000      /* No last pixel in a line.*/
#define I128_CMD_PATT_RESET    0x08000000      /* Reset pattern pointers. */
#define I128_CMD_CLIP_IN       0x00400000      /* Clip inside cliprect. */
#define I128_CMD_CLIP_OUT      0x00600000      /* Clip outside cliprect. */
#define I128_CMD_NO_CLIP       0x00000000      /* No rectangle clipping */
#define I128_CMD_CLIP_STOP     0x00800000      /* Stop if clipping. */
#define I128_CMD_SWAP_SHIFT    28
#define I128_CMD_SWAP_BITS     0x70000000      /* Host data swap control. */
#define I128_CMD_BIT_SWAP      0x10000000      /* Swap bits in each byte */
#define I128_CMD_BYTE_SWAP     0x20000000      /* Swap bytes in each dword */
#define I128_CMD_WORD_SWAP     0x40000000      /* Swap words in each dword */

/* These flags inidicate which value is written to MASK. */
#define I128_KEY_FLAG_ZERO     0x00000000      /* Write 0 to MASK */
#define I128_KEY_FLAG_ONE      0x000F000F      /* Write 1 to MASK */
#define I128_KEY_FLAG_MATCH    0x0000000F      /* Write 1 if match else 0 */
#define I128_KEY_FLAG_DIFFER   0x000F0000      /* Write 0 if match else 1 */
#define I128_LPAT_LENGTH(x)    ((x) & 0x1F)    /* Pattern length (32,1-31)*/
#define I128_LPAT_ZOOM(x)      ((((x) - 1) & 7) << 5)/* Pattern zoom (1-8) */
#define I128_LPAT_INIT_PAT(x)  (((x) & 0x1F) << 8)/* Initial pattern offset */
#define I128_LPAT_INIT_ZOOM(x) (((x) & 7) << 13)/* Initial zoom step */
#define I128_LPAT_STATE        0xFFFF0000      /* current state */

#define I128_DIR_LR_TB         0x00            /* left-right, top-bottom */
#define I128_DIR_LR_BT         0x01            /* left-right, bottom-top */
#define I128_DIR_RL_TB         0x02            /* right-left, top-bottom */
#define I128_DIR_RL_BT         0x03            /* right-left, bottom-top */

#define I128_BLIT_NO_ZOOM      0x00000000      /* straight copy */
#define I128_BLIT_ZOOM_Y(x)    ((x) & F)       /* Set Y zoom factor (1-8) */
#define I128_BLIT_ZOOM_X(x)    ((long)((x) & F) << 16)/* Set X zoom (1-8) */
#define I128_BLIT_ZOOM(x)      (I128_BLIT_ZOOM_X(x) | I128_BLIT_ZOOM_Y(x))

/*         wait_flag -          Flag to indicate which state to wait for */
#define I128_WAIT_READY        0x00000000      /* Wait for engine ready */
#define I128_WAIT_IDLE         0x00000001      /* Wait for engine idle */
#define I128_WAIT_DONE         0x00000003      /* Wait for engine stop */
#define I128_WAIT_PREVIOUS     0x00000004      /* Wait for prev. cache */
#define I128_WAIT_CACHE        0x00000005      /* Wait for cache ready */

/* Parameters for the XY window */
#define I128_XYWIN_SZ_4K       0x00000000
#define I128_XYWIN_SZ_8K       0x00000100
#define I128_XYWIN_SZ_16K      0x00000200
#define I128_XYWIN_SZ_32K      0x00000300
#define I128_XYWIN_SZ_64K      0x00000400
#define I128_XYWIN_SZ_128K     0x00000500
#define I128_XYWIN_SZ_256K     0x00000600
#define I128_XYWIN_SZ_512K     0x00000700
#define I128_XYWIN_SZ_1M       0x00000800
#define I128_XYWIN_SZ_2M       0x00000900
#define I128_XYWIN_SZ_4M       0x00000A00
#define I128_XYWIN_SZ_8M       0x00000B00
#define I128_XYWIN_SZ_16M      0x00000C00
#define I128_XYWIN_SZ_32M      0x00000D00

/* Parameters for the linear memory window */
#define I128_MEMW_SZ_4K        0x00000000
#define I128_MEMW_SZ_8K        0x00000001
#define I128_MEMW_SZ_16K       0x00000002
#define I128_MEMW_SZ_32K       0x00000003
#define I128_MEMW_SZ_64K       0x00000004
#define I128_MEMW_SZ_128K      0x00000005
#define I128_MEMW_SZ_256K      0x00000006
#define I128_MEMW_SZ_512K      0x00000007
#define I128_MEMW_SZ_1M        0x00000008
#define I128_MEMW_SZ_2M        0x00000009
#define I128_MEMW_SZ_4M        0x0000000A
#define I128_MEMW_SZ_8M        0x0000000B
#define I128_MEMW_SZ_16M       0x0000000C
#define I128_MEMW_SZ_32M       0x0000000D

#define I128_MEMW_SRC_DISP     0x00000000      /* Select Disp as source */
#define I128_MEMW_SRC_VIRT     0x00000010      /* Select Virt as source */
#define I128_MEMW_SRC_MASK     0x00000020      /* Select MASK plane as src */
#define I128_MEMW_SRC_NONE     0x00000030      /* No input selected */
#define I128_MEMW_8_BPP        0x00000000      /* Use  8 bits per pixel */
#define I128_MEMW_16_BPP       0x04000000      /* Use 16 bits per pixel */
#define I128_MEMW_32_BPP       0x08000000      /* Use 32 bits per pixel */
#define I128_MEMW_DST_DISP     0x00000000      /* Select DISP as dst */
#define I128_MEMW_DST_VIRT     0x01000000      /* Select VIRT as dst */
#define I128_MEMW_DST_MASK     0x02000000      /* Select MASK plane as dst */
#define I128_MEMW_DST_NONE     0x03000000      /* No destination selected */
#define I128_MEMW_SHADOW_D     0x40000000      /* Also write to DISP */
#define I128_MEMW_SHADOW_V     0x20000000      /* Also write to VIRT */
#define I128_MEMW_SHADOW_M     0x10000000      /* Also write to MASK */
#define I128_MEMW_MASK_DISP    0x00000008      /* Apply MASK mask to DISP */
#define I128_MEMW_MASK_VIRT    0x00000002      /* Apply MASK mask to VIRT */
#define I128_MEMW_MASK_ZERO    0x00000000      /* Allow write when MASK=1 */
#define I128_MEMW_MASK_ONE     0x00000004      /* Allow write when MASK=0 */
#define I128_MEMW_MASK_KEY     0x00200000      /* Set MASK when color=key */
#define I128_MEMW_MASK_DIRECT  0x00600000      /* Set MASK bits directly */
#define I128_MEMW_BUSY         0x00000100      /* Memory window is busy */

/* IO Mapped Configuration Register Offsets */
#define I128_OFFSET_RBASE_G    0x0000
#define I128_OFFSET_RBASE_W    0x0004
#define I128_OFFSET_RBASE_A    0x0008
#define I128_OFFSET_RBASE_B    0x000C
#define I128_OFFSET_RBASE_I    0x0010
#define I128_OFFSET_RBASE_E    0x0014
#define I128_OFFSET_ID         0x0018
#define I128_OFFSET_CONFIG1    0x001C
#define I128_OFFSET_CONFIG2    0x0020
#define I128_OFFSET_UNIQ       0x0024
#define I128_OFFSET_SSWTCH     0x0028

/* Bus type defines */
/* These are the host bus types currently planned to be supported. */
#define I128_BUS_IS_ISA         0x01
#define I128_BUS_IS_EISA        0x02
#define I128_BUS_IS_VL          0x03
#define I128_BUS_IS_PCI         0x04
#define I128_BUS_IS_PAWS        0x05

/* Structure Version Numbers */
/* Each structure must have its own version number to ensure that
   the data being read is interpreted correctly.
   These are the most current version numbers. */
#define I128_FILE_VERSION       0x0100L
#define I128_RES_VERSION        0x0100L
#define I128_MODE_VERSION       0x0100L
#define I128_BUS_VERSION        0x0100L
#define I128_HARDWARE_VERSION   0x0100L
#define I128_VIDEO_VERSION      0x0100L

/* DAC types */
#define I128_DACTYPE_BT485             0x01
#define I128_DACTYPE_BT484             0x02
#define I128_DACTYPE_ATT491            0x03
#define I128_DACTYPE_TIVPT             0x04
#define I128_DACTYPE_TIVPTjr           0x05
#define I128_DACTYPE_IBM528            0x09

/* Frequency Types */
#define I128_FREQTYPE_ICD_2061A 	0x01
#define I128_FREQTYPE_ICD_2062  	0x02

/* Mclock numbers */
#define I128_MCLOCK_NUMBERS		1184128

/* Video flags */
#define I128_VGA_ENABLED		0x1

/* Structure's valid data flags */
/* These flags are set within the valid fields in the configuration file */
#define I128_DATA_INVALID       0x0000L
#define I128_DATA_VALID         0x0001L

/* Board selection commands */
#define I128_DEFAULT_BOARD      (-1)

/* Programming defines */
#define BLACKBIRD_ENV            "I128"  /* OS environment variable */
#define BLACKBIRD_CFG_FILE       "I128CFG" /* CFG file name */

/* Macros to extract a major or minor version number from
   the entry within a structure. */
#define I128_MAKEMAJORREV(x)    ((x & 0x0000FF00L)>>8)
#define I128_MAKEMINORREV(x)    (x & 0x000000FFL)


struct Blackbird_Engine
  {
/* This structure is identical to the drawing engine hardware registers */
        long    intrupt;
        long    intrupt_mask;
        volatile long    flow;
        volatile long    busy;
        long    xyw_adsz;
        long    reserved_a[3];
        volatile long    buf_control;
        long    page;
        long    src_origin;
        long    dst_origin;
        long    msk_source;
        long    reserved_b;
        long    key;
        long    key_data;
        long    src_pitch;
        long    dst_pitch;
        long    cmd;
        long    reserved_c;
        long    cmd_opcode;
        long    cmd_raster_op;
        long    cmd_style;
        long    cmd_pattern;
        long    cmd_clip;
        long    cmd_swap;
        long    foreground;
        long    background;
        long    plane_mask;
        long    rop_mask;
        long    line_pattern;
        long    pattern_ctrl;
        long    clip_top_left;
        long    clip_bottom_right;
        long    xy0;
        long    xy1;
        long    xy2;
        long    xy3;
        long    xy4;
  };

struct Blackbird_Memwin
  {
/* This structure is identical to the memory window hardware registers */
        long    control;
        long    address;
        long    size;
        long    page;
        long    origin;
        long    RESERVED;
        long    msk_source;
        long    key;
        long    key_data;
        long    plane_mask;
  };

struct Blackbird_Syncs
  {
/* This structure is identical to the display engine hardware registers */
        long    int_vcount;
        long    int_hcount;
        long    address;
        long    pitch;
        long    h_active;
        long    h_blank;
        long    h_front_porch;
        long    h_sync;
        long    v_active;
        long    v_blank;
        long    v_front_porch;
        long    v_sync;
        long    border;
        long    zoom;
        long    config_1;
        long    config_2;
  };

struct Blackbird_Video
  {
/* This structure is identical to the video port hardware registers */
        long    config_1;
        long    plane_mask;
        long    config_2;
        long    output_mask;
        long    test_color;
        long    config_3;
        long    delay;
        long    size;
        long    xyoffset;
        long    page;
        long    src_origin;
        long    dst_origin;
        long    msk_source;
        long    chroma_compare;
        long    y_interrupt;
        long    field_interrupt;
        long    src_pitch;
        long    dst_pitch;
        long    decimate;
        long    reserved;
  };

struct Blackbird_Global
  {
/* This structure is identical to the global hardware registers */
        char                            dac_regs[8 * 4];

        struct Blackbird_Syncs          syncs;
        struct Blackbird_Video          video;
  };

struct Blackbird_Interrupts
  {
/* This structure is identical to the global interrupt hardware registers */
        long    I128_interrupt;
        long    I128_interrupt_mask;
  };

struct Memory_Window_Info
  {
        char    *pointer;
        long    physical_host_address;
        long    length;
  };

struct Blackbird_IO
  {
        /* This structure is identical to the Blackbird IO hardware registers */
        long    rbase_g;
        long    rbase_w;
        long    rbase_a;
        long    rbase_b;
        long    rbase_i;
        long    rbase_e;
        long    id;
        long    config1;
        long    config2;
        long    reserved;
        long    soft_switch;
  };

struct Blackbird_Info
  {
        /* Addresses of register blocks */
        struct Blackbird_Engine         FAR *engine_a;
        struct Blackbird_Engine         FAR *engine_b;
        struct Blackbird_Memwin         FAR *memwins[2];
        struct Blackbird_Global         FAR *global;
        struct Blackbird_Interrupts     FAR *global_int;

        /* Memory Window Information */
        struct Memory_Window_Info       xy_win[2];
        struct Memory_Window_Info       memwin[2];

        /* Memory sizes */
        long                            Disp_size;
        long                            Virt_size;
        long                            Mask_size;

        /* Board hardware configuration */
        long                            max_pixel_rate;
        long                            hard_flags;
        long                            crystal_rate;
        long                            Blackbird_ID;
        long                            video_flags;

        /* More junk ... */
        long                            IO_Address;
        struct Blackbird_IO             IO_Regs;
  };

#define  FILE_NAME_SIZE       16

struct Blackbird_Mode
  {
        /* Sync parameters */
        long    version;
	char	driver_name[FILE_NAME_SIZE];
	long	VESA_mode;
	long	board_dept_offset;    /* resolution dependent registers  */
        long    display_flags;
        long    config_1;
        long    config_2;
        long    clock_frequency;
        long    clock_numbers;
        long    display_start;  /*****/
        short   bitmap_width;   /*****/
        short   bitmap_height;  /*****/
        short   bitmap_pitch;   /*****/
        short   pixelsize;      /*****/
        short   selected_depth; /*****/
        short   display_x;
        short   display_y;
        short   h_active;
        short   h_blank;
        short   h_front_porch;
        short   h_sync;
        short   v_active;
        short   v_blank;
        short   v_front_porch;
        short   v_sync;
        short   border;
        short   y_zoom;
        short   max_depth;
        short   Reserved[33];
  };

/* The Blackbird file contains only one mode entry for each resolution.  For
example, if we were to support 512x512, 640x480, 800x600, 1024x768, 1152x870,
1280x1024, 1536x1152, and 1600x1200,  there would be only eight entries in
the file.
*/

struct Blackbird_File
  {
        long    file_id;
        long    version;
        long    default_board_flag;  /* is this board the default? */
        long    next_board;          /* start of next file header */
        long    offset_resolution;
        long    offset_bus_info;
        long    offset_hardware;
        long    offset_video;
        long    reserved[20];
  };

/* #define      I128_FILE_ID   0x99429971 */   /* Number Nine ID */
#define I128_FILE_ID   (0x31375253L)   /* Number Nine ID */

struct Blackbird_File_Resolution
  {
        long    version;
        long    num_modes;
        long    default_mode;
        long    mode_size;
        long    mode_offset;
        long    reserved[20];
  };

struct Blackbird_File_Bus
  {
        long    version;
        long    valid;
        long    type;
        long    IO_address;
        long    VL_mem_address;
        long    VL_mem_size;
        long    PCI_dev_number;
        long    reserved[20];
  };

struct Blackbird_File_Hardware
  {
        long    version;
        long    valid;
        long    DAC_type;
        long    freq_synth;
        long    disp_size;
        long    virt_size;
        long    mask_size;
        long    DAC_speed_max;
        long    internal_bus_width;
	long	processor_id;
	long	color_format;
	long	mclock_numbers;
        long    board_num;
	long	reserved[16];
  };

struct Blackbird_File_Video
  {
        long    version;
        long    valid;
        long    reserved[62];
  };

/* Low-level Blackbird flags */

/* ID flag */
#define I128_ID_REVISION       0x00000007
#define I128_ID_BUS_BITS       0x00000018
#define I128_ID_BUS_PCI        0x00000000
#define I128_ID_BUS_VL         0x00000008
#define I128_ID_BUS_WIDTH      0x00000020
#define I128_ID_PCI_BASE0      0x000000C0
#define I128_ID_DISP_BITS      0x00000300
#define I128_ID_DISP_NONE      0x00000000
#define I128_ID_DISP_256KxN    0x00000100
#define I128_ID_DISP_2_BANKS   0x00000400
#define I128_ID_PCI_BASE1      0x00001800
#define I128_ID_PCI_BASE2      0x00006000
#define I128_ID_DATA_BUS_SIZE  0x00008000
#define I128_ID_VIRT_BITS      0x00030000
#define I128_ID_VIRT_NONE      0x00000000
#define I128_ID_VIRT_256KxN    0x00010000
#define I128_ID_VIRT_1MxN      0x00020000
#define I128_ID_VIRT_2_BANKS   0x00040000
#define I128_ID_PCI_BASE3      0x00180000
#define I128_ID_PCI_BASE_ROM   0x00E00000
#define I128_ID_MASK_BITS      0x03000000
#define I128_ID_MASK_NONE      0x00000000
#define I128_ID_MASK_256KxN    0x01000000
#define I128_ID_MASK_1MxN      0x02000000
#define I128_ID_RAS_PULSE      0x04000000
#define I128_ID_VGA_SNOOP      0x08000000
#define I128_ID_PCI_CLASS_BITS 0x30000000
#define I128_ID_PCI_CLASS_VGA  0x00000000
#define I128_ID_PCI_CLASS_XGA  0x10000000
#define I128_ID_PCI_CLASS_OTHR 0x20000000
#define I128_ID_PCI_CLASS_OTH2 0x30000000
#define I128_ID_PCI_EPROM_ENAB 0x40000000
#define I128_ID_PCI_RSVD       0x80000000

/* Config register 1 */
#define I128_C1_VGA_SHADOW     0x00000001
#define I128_C1_RESET          0x00000002
#define I128_C1_WIDTH_128      0x00000004
#define I128_C1_VGA_SNOOP      0x00000008
#define I128_C1_ENABLE_GLOBAL  0x00000100
#define I128_C1_ENABLE_MEMWIN  0x00000200
#define I128_C1_ENABLE_DRAW_A  0x00000400
#define I128_C1_ENABLE_DRAW_B  0x00000800
#define I128_C1_ENABLE_INT     0x00001000
#define I128_C1_ENABLE_EPROM   0x00002000
#define I128_C1_ENABLE_MEMWIN0 0x00010000
#define I128_C1_ENABLE_MEMWIN1 0x00020000
#define I128_C1_ENABLE_XY_A    0x00100000
#define I128_C1_ENABLE_XY_B    0x00200000
#define I128_C1_PRIORITY_BITS  0xFF000000
#define I128_C1_PRI_HOST(x)    ((long)((x) & 3) << 24)
#define I128_C1_PRI_VIDEO(x)   ((long)((x) & 3) << 26)
#define I128_C1_PRI_A(x)       ((long)((x) & 3) << 28)
#define I128_C1_PRI_B(x)       ((long)((x) & 3) << 30)
#define I128_C1_PRI_LOW        0
#define I128_C1_PRI_MED        1
#define I128_C1_PRI_HI         2

/* Config register 2 */
#define I128_C2_DATA_WAIT(x)   (long)((x) & 3)
#define I128_C2_EPROM_WAIT(x)  ((long)((x) & 0xF) << 8)
#define I128_C2_DAC_WAIT(x)    ((long)((x) & 0x7) << 16)
#define I128_C2_BUFFER_ENABLE  0x00100000	/* Enable Display Memory */
#define I128_C2_REFRESH_DISAB  0x00200000	/* Memory refresh disable */
#define I128_C2_DELAY_SAMPLE   0x00400000	/* Delay data during read */
#define I128_C2_MEMORY_SKEW    0x00800000	/* Skew memory control */
#define I128_C2_RESERVED       0x40000000	/* Reserved switch */
#define I128_C2_SLOW_DAC       0x80000000	/* Dac speed < 175MHz */

/* Global interrupt register (read only) */
#define I128_GINT_VB           0x00000001      /* Vertical blank */
#define I128_GINT_HB           0x00000002      /* Horizontal blank */
#define I128_GINT_ENGINE_A     0x00000100      /* Engine A op complete */
#define I128_GINT_CLIP_A       0x00000200      /* Engine A clip */
#define I128_GINT_ENGINE_B     0x00010000      /* Engine B op complete */
#define I128_GINT_CLIP_B       0x00020000      /* Engine B clip */
#define I128_GINT_VIDEO(x)     (1L << ((x) + 24))

/* Global interrupt mask (read/write) */
#define I128_GINT_VB_MASK      0x00000001
#define I128_GINT_HB_MASK      0x00000002
#define I128_GINT_VID_MASK(x)  (1L << ((x) + 8))

/* DAC register defines */
#define I128_DAC_WR_ADDR       0
#define I128_DAC_PAL_DAT       4
#define I128_DAC_PEL_MASK      8
#define I128_DAC_RD_ADDR       12
#define I128_DAC_RSVD1         16
#define I128_DAC_RSVD2         20
#define I128_DAC_VPT_INDEX     24
#define I128_DAC_VPT_DATA      28
#define I128_DAC_IBM528_IDXLOW 16
#define I128_DAC_IBM528_IDXHI  20
#define I128_DAC_IBM528_DATA   24
#define I128_DAC_IBM528_IDXCTL 28

/* CRT Zoom factors */
#define I128_CRT_NO_Y_ZOOM     0x00000000
#define I128_CRT_Y_ZOOM(x)     ((x) - 1)       /* x between 1 and 16 */

/* CRT configuration register 1 */
#define I128_CRT_C1_POS_HSYNC  0x00000001
#define I128_CRT_C1_POS_VSYNC  0x00000002
#define I128_CRT_C1_COMP_SYNC  0x00000004
#define I128_CRT_C1_INTERLACED 0x00000008
#define I128_CRT_C1_HSYNC_ENBL 0x00000010
#define I128_CRT_C1_VSYNC_ENBL 0x00000020
#define I128_CRT_C1_VIDEO_ENBL 0x00000040
#define I128_CRT_C1_SCLK_DIR   0x00000100

/* CRT configuration register 2 */
#define I128_CRT_C2_DISP_256K  0x00000001      /* Not used */
#define I128_CRT_C2_VSHIFT_4K  0x00000000
#define I128_CRT_C2_VSHIFT_2K  0x00000002
#define I128_CRT_C2_VSHIFT_8K  0x00000004
#define I128_CRT_C2_REFRESH    0x00000100
#define I128_CRT_C2_XFER_DELAY(x)      ((long)((x) & 0x7) << 16)
#define I128_CRT_C2_SPLIT_XFER 0x01000000

/* Video configuration register 1 */
#define I128_VID_C1_8BIT       0x00000000
#define I128_VID_C1_16BIT      0x00000001
#define I128_VID_C1_32BIT      0x00000002
#define I128_VID_C1_PORT_0     0x00000000 /* 000000FF(8), 0000FFFF(16) */
#define I128_VID_C1_PORT_1     0x00000010 /* 0000FF00(8) */
#define I128_VID_C1_PORT_2     0x00000020 /* 00FF0000(8), FFFF0000(16) */
#define I128_VID_C1_PORT_3     0x00000030 /* FF000000(8) */
#define I128_VID_C1_AUTO_SENSE 0x00000100
#define I128_VID_C1_MASTER     0x00000200
#define I128_VID_C1_TEST_COLOR 0x00001000
#define I128_VID_C1_POS_HSYNC  0x00010000
#define I128_VID_C1_POS_VSYNC  0x00020000
#define I128_VID_C1_COMP_BLANK 0x00040000
#define I128_VID_C1_INTERLACED 0x00080000
#define I128_VID_C1_SFM_NORM   0x00000000
#define I128_VID_C1_SFM_FOUR   0x01000000
#define I128_VID_C1_SFM_EIGHT  0x02000000
#define I128_VID_C1_SFM_COPY   0x03000000
#define I128_VID_C1_NTSC_FIELD 0x10000000
#define I128_VID_C1_SWAP_FIELD 0x20000000

/* Video configuration register 2 */
#define I128_VID_C2_MASK1      0x00000001 /* MASK prevents write when set */
#define I128_VID_C2_MASK0      0x00000000 /* MASK prevents write when 0 */
#define I128_VID_C2_MASK_DISP  0x00000002 /* Apply MASK masking to DISP */
#define I128_VID_C2_MASK_VIRT  0x00000008 /* Apply MASK masking to VIRT */
#define I128_VID_C2_SHADOW_M   0x00000100 /* Shadow writes to MASK */
#define I128_VID_C2_SHADOW_D   0x00000200 /* Shadow writes to DISP */
#define I128_VID_C2_SHADOW_V   0x00000400 /* Shadow writes to VIRT */
#define I128_VID_C2_INPUT      0x00000000 /* Input from video port */
#define I128_VID_C2_OUTPUT     0x00010000 /* Video port is output */
#define I128_VID_C2_BUFF_DISP  0x00000000
#define I128_VID_C2_BUFF_VIRT  0x01000000
#define I128_VID_C2_BUFF_MASK  0x02000000
#define I128_VID_C2_BUFF_NONE  0x03000000

/*======================================================================*/
/* Video configuration register 3 (This needs updating!) */
#define I128_VID_C3_VTAKE_BITS 0x00000F00
#define I128_VID_C3_VTAKE(x)   ((((x) - 1) & 0xF) << 8)
#define I128_VID_C3_CONTINUOUS 0x00000000
#define I128_VID_C3_1_FIELD    0x00004000
#define I128_VID_C3_2_FIELD    0x00005000
#define I128_VID_C3_4_FIELD    0x00006000
#define I128_VID_C3_8_FIELD    0x00007000
#define I128_VID_C3_FAST_FIFO  0x00000080
#define I128_VID_C3_START      0x00000100
#define I128_VID_C3_BURST_BITS 0x00070000
#define I128_VID_C3_BURST_SHFT 16

/* Video comparator register */
#define I128_VID_CMP_PORT_0    0x00000000
#define I128_VID_CMP_PORT_1    0x00010000
#define I128_VID_CMP_PORT_2    0x00020000
#define I128_VID_CMP_PORT_3    0x00030000
#define I128_VID_CMP_PORT_4_16 0x00040000
#define I128_VID_CMP_PORT_5_16 0x00050000
#define I128_VID_CMP_PORT_NONE 0x00070000
#define I128_VID_CMP_DIFFER    0x00000000
#define I128_VID_CMP_MATCH     0x00100000
#define I128_VID_CMP_COLR_MASK 0x01000000


/* I128 Cache */
#define I128_CACHE_SIZE		128
#define I128_MAX_HEIGHT 	32
#define I128_MAX_WIDTH		32

/* Macros */
#define	I128_XY(x, y) (((long)(x) << 16) | ((y)&0xFFFF))

#define I128_set_engine(flag)  { \
        if ((flag) == I128_SELECT_ENGINE_A) \
          i128Priv->engine = i128Priv->info.engine_a; \
        else if ((flag) == I128_SELECT_ENGINE_B) \
          i128Priv->engine = i128Priv->info.engine_b; \
                                }

#define I128_WAIT(x) \
	while ((((x)->busy)) && \
	       (((x)->flow & 0xF))) I128_NOOP;

#define I128_WAIT_UNTIL_DONE(x) \
	while ((((x)->flow)) & 3) I128_NOOP

#define I128_WAIT_UNTIL_READY(x)\
	while (((x)->busy)) I128_NOOP

#define I128_WAIT_UNTIL_IDLE(x) \
 	while ((((x)->flow)) & 1) I128_NOOP

#define I128_WAIT_FOR_PREVIOUS(x) \
	while ((((x)->flow)) & 8) I128_NOOP

#define I128_WAIT_FOR_CACHE(x) \
	while ((((x)->buf_control)) & I128_FLAG_CACHE_READY) \
		I128_NOOP

#define I128_COPY_BITMAP(l_src, l_dst, l_stride, numLongs, height)\
{ \
       unsigned long *l_hptr; \
       int  i, left; \
\
       for (i = 0; i < height; i++) \
       { \
              l_hptr = l_src; \
              l_src += l_stride; \
              left = numLongs; \
              while (left--) \
                     *l_dst = *l_hptr++; \
       } \
} 


#define I128_CONVERT(d, p) \
((d == 1) ? (((p & 0xFF) << 24) | \
             ((p & 0xFF) << 16) | \
             ((p & 0xFF) << 8) | \
             (p & 0xFF)) : \
 (d == 2) ? (((p & 0xFFFF) << 16) | \
             (p & 0xFFFF)) : \
 (d == 3) ?  (((p & 0xFF) << 24) | \
              (p & 0xFFFFFF)) :  p)

#define I128_SAFE_BLIT(srcx,  srcy, dstx, dsty, width, height, dir) \
{ \
     static int min_size[]   = { 0x62,  0x32,  0x1A,  0x00 };\
     static int max_size[]   = { 0x80,  0x40,  0x20,  0x00 };\
     static int split_size[] = { 0x20,  0x10,  0x08,  0x00 };\
     int tmp;\
     int _b_x1 = srcx, _b_y1 = srcy, _b_x2 = dstx, _b_y2 = dsty; \
     int _b_w = width, _b_h = height; \
\
     tmp = (i128Priv->engine->buf_control & I128_FLAG_NO_BPP) >> 24; \
     if (_b_w >= min_size[tmp] && _b_w <= max_size[tmp]) \
     { \
          tmp = split_size[tmp]; \
          i128Priv->engine->xy2 = I128_XY(tmp, _b_h); \
          i128Priv->engine->xy0 = I128_XY(_b_x1, _b_y1); \
          i128Priv->engine->xy1 = I128_XY(_b_x2, _b_y2); \
          _b_w -= tmp; \
          if (dir & I128_DIR_RL_TB) { \
               _b_x1 -= tmp; \
               _b_x2 -= tmp; \
          } \
          else { \
               _b_x1 += tmp; \
               _b_x2 += tmp; \
          } \
          I128_WAIT_UNTIL_READY(i128Priv->engine); \
     } \
     i128Priv->engine->xy2 = I128_XY(_b_w, _b_h); \
     i128Priv->engine->xy0 = I128_XY(_b_x1, _b_y1); \
     i128Priv->engine->xy1 = I128_XY(_b_x2, _b_y2); \
} \

#define I128_BLIT(srcx, srcy, dstx, dsty, width, height, dir) \
     i128Priv->engine->xy2 = I128_XY(width, height); \
     i128Priv->engine->xy0 = I128_XY(srcx, srcy); \
     i128Priv->engine->xy1 = I128_XY(dstx, dsty)

#define I128_DRAW_POINT(x, y) \
     i128Priv->engine->xy0 = I128_XY(x, y); \
     i128Priv->engine->xy1 = I128_XY(x, y)

#define I128_TRIGGER_CACHE(x)   \
      (x)->buf_control |= I128_FLAG_CACHE_READY


#define I128_REPLICATE_WIDTH(_d, _w) \
switch (_w)\
{\
   case 1:\
       _d = _d | _d << 1;\
         case 2:\
             _d = _d | _d << 2;\
               case 4:\
                   _d = _d | _d << 4;\
                     case 8:\
                         _d = _d | _d << 8;\
                           case 16:\
                               _d = _d | _d << 16;\
                                 case 32:\
                                   default:\
                                       break;\
 }


#define I128_REPLICATE_HEIGHT(_d, _h) \
{ \
      register int _i;\
          switch (_h)\
          {\
            case 1:\
                _d[1] = _d[0];\
                  case 2:\
                      for (_i=0; _i<2; _i++)\
                          _d[_i+2] = _d[_i];\
                            case 4:\
                                for (_i=0; _i<4; _i++)\
                                    _d[_i+4] = _d[_i];\
                                      case 8:\
                                          for (_i=0; _i<8; _i++)\
                                              _d[_i+8] = _d[_i];\
                                                case 16:\
                                                    for (_i=0; _i<16; _i++)\
                                                        _d[_i+16] = _d[_i];\
                                                          case 32:\
                                                            default:\
                                                                break;\
  }\
  }       


#define I128_CLIP(box) \
    I128_WAIT_UNTIL_READY(i128Priv->engine); \
    i128Priv->engine->clip_top_left = I128_XY((box).x1, (box).y1); \
    i128Priv->engine->clip_bottom_right = I128_XY((box).x2 - 1, (box).y2 - 1);

typedef struct _i128Private {
     struct Blackbird_Engine volatile *engine;
     struct Blackbird_Info info;
     struct Blackbird_Mode mode;
     struct Blackbird_File_Hardware hardware;
     struct Blackbird_Engine save;

     char *mw0_addr;
     char *mw1_addr;
     char *xy0_addr;
     char *rbase_x_addr;
     char *iobase;

     unsigned int rop[16];
     BoxRec clip;
     BoxRec cache;

#ifdef I128_FAST_GC_OPS
     Bool (*CreateGC)();	/* pGC */
#endif
     PixmapPtr (*CreatePixmap)();
     Bool (*DestroyPixmap)();

} i128Private, * i128PrivatePtr;

extern unsigned int i128ScreenPrivateIndex;

#define I128_PRIVATE_DATA(pScreen) \
((i128PrivatePtr)((pScreen)->devPrivates[i128ScreenPrivateIndex].ptr))

#ifdef I128_FAST_GC_OPS
typedef struct _i128GCPrivateRec {
     GCFuncs *funcs;
     GCOps *ops;
} i128GCPrivate, *i128GCPrivatePtr;

extern unsigned int i128GCPrivateIndex;

#define I128_GC_PRIV(pGC) \
((i128GCPrivatePtr)((pGC)->devPrivates[i128GCPrivateIndex].ptr))

#define I128_UNWRAP_GC(pGC, _o) \
     pGC->funcs = I128_GC_PRIV(pGC)->funcs; \
     _o = pGC->ops; \
     pGC->ops = I128_GC_PRIV(pGC)->ops;

#define I128_WRAP_GC(pGC, _f, _o) \
     I128_GC_PRIV(pGC)->funcs = pGC->funcs; \
     I128_GC_PRIV(pGC)->ops = pGC->ops; \
     pGC->funcs = _f; \
     pGC->ops = _o; 

#endif

#define I128_engine 		(i128Priv->engine)
#define I128_info 		(i128Priv->info)
#define I128_mode 		(i128Priv->mode)
#define I128_hardware 		(i128Priv->hardware)

#endif /* I128DEFS_INCLUDE */
