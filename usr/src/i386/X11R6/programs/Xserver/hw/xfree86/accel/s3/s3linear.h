/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3linear.h,v 3.7 1996/01/28 07:30:18 dawes Exp $ */
/*
 * s3EnableLinear() and s3DisableLinear() are wrappers to surround
 * any function call that is going to access the video ram through
 * the linear addressing window.
 * e.g.
 *    s3EnableLinear();
 *    cfbVideoMemFuction();
 *    s3DisableLinear();
 *
 * Not currently used apart from s3im.c
 */
/* $XConsortium: s3linear.h /main/5 1996/01/28 07:58:06 kaleb $ */


extern Bool s3LinearAperture;
extern unsigned char s3Port40;
extern unsigned char s3Port54;
extern unsigned char s3Port51;

/* Some poor compilers don't have inlines */
#ifdef S3_MMIO 
# define DISABLE_MMIO   \
      { unsigned char tmp; \
      outb(vgaCRIndex, 0x53); \
      tmp = inb(vgaCRReg); \
      outb(vgaCRReg, tmp & 0xEF); }
# define ENABLE_MMIO   \
      { unsigned char tmp; \
      outb(vgaCRIndex, 0x53); \
      tmp = inb(vgaCRReg); \
      outb(vgaCRReg, tmp | 0x10); }
#else
# define DISABLE_MMIO /**/
# define ENABLE_MMIO /**/
#endif
      
#define s3EnableLinear() \
   WaitIdle();\
   if (S3_801_928_SERIES (s3ChipId)) {\
      int   i;\
\
    /* begin 801 sequence for going in to linear mode */\
    /* x64: CR40 changed a lot for 864/964; wait and see if this still works */\
      outb (vgaCRIndex, 0x40);\
      i = (s3Port40 & 0xf6) | 0x0a;/* enable fast write buffer and disable\
				 * 8514/a mode */\
      outb (vgaCRReg, (unsigned char) i);\
      DISABLE_MMIO; \
      outb (vgaCRIndex, 0x58);\
      outb (vgaCRReg, s3LinApOpt | s3SAM256);	/* go on to linear mode */\
      if (!S3_x64_SERIES(s3ChipId)) {\
         outb (vgaCRIndex, 0x54);\
         outb (vgaCRReg, (s3Port54 + 07));\
      }\
    /* end  801 sequence to go into linear mode, now lock the registers */\
      outb(vgaCRIndex, 0x39);\
      outb(vgaCRReg, 0x50); \
      }    \
    else \
       outb(vgaCRIndex, 0x35);


#define s3DisableLinear() \
   if (S3_801_928_SERIES (s3ChipId)) {\
\
      outb(vgaCRIndex, 0x39);\
      outb(vgaCRReg, 0xa5);\
    /* begin 801  sequence to go into enhanced mode */\
      if (!S3_x64_SERIES(s3ChipId)) {\
         outb (vgaCRIndex, 0x54);\
         outb (vgaCRReg, s3Port54);\
      }\
      outb (vgaCRIndex, 0x58);\
      outb (vgaCRReg, s3SAM256);\
      outb (vgaCRIndex, 0x40);\
      outb (vgaCRReg, s3Port40);\
      ENABLE_MMIO; \
   }


/*
 * Select a bank, enabling and disabling the 801/928 as we go
 */
 
#define	s3BankSelect(X) do {				\
	char new_mbank;					\
        if (S3_801_928_SERIES (s3ChipId)) {		\
	   outb(vgaCRIndex, 0x39);			\
	   outb(vgaCRReg, 0xa5);			\
	}						\
	outb(vgaCRIndex, 0x35);				\
	outb(vgaCRReg, (X & 0x0f));			\
        new_mbank = (X & 0xf0)>>2;			\
	if (s3Mbanks != new_mbank) {			\
	   outb(vgaCRIndex, 0x51);			\
	   outb(vgaCRReg, new_mbank | s3Port51);	\
	   s3Mbanks = new_mbank;			\
	}						\
        if (S3_801_928_SERIES (s3ChipId)) {		\
	   outb(vgaCRIndex, 0x39);			\
	   outb(vgaCRReg, 0x5a);			\
	}						\
} while (1 == 0)
