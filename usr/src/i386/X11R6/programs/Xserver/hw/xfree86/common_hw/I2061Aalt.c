/* $XFree86: xc/programs/Xserver/hw/xfree86/common_hw/I2061Aalt.c,v 3.7 1995/12/21 11:44:50 dawes Exp $ */

/*
 * This code is derived from code available from the STB bulletin board
 */
/* $XConsortium: I2061Aalt.c /main/5 1995/12/28 17:15:59 kaleb $ */

#include "compiler.h"

#define SEQREG   0x03C4
#define MISCREG  0x03C2
#define MISCREAD 0x03CC

double fref = 14.31818 * 2.0;
char ascclk[] = "VIDEO CLOCK ?";

unsigned short clknum;
unsigned short vlbus_flag;
unsigned short card;
unsigned short crtcaddr;
unsigned short clockreg;

static double range[15] = {50.0, 51.0, 53.2, 58.5, 60.7, 64.4, 66.8, 73.5, 
			   75.6, 80.9, 83.2, 91.5, 100.0, 120.0, 120.0000001};

#if NeedFunctionPrototypes
#if 0
static void prtbinary(unsigned int size, unsigned int val);
#endif
static void wait_vb();
static void wrt_clk_bit(unsigned int value);
static void init_clock(unsigned long setup, unsigned short crtcport);
#else
#if 0
static void prtbinary();
#endif
static void wait_vb();
static void wrt_clk_bit();
static void init_clock();
#endif
#ifdef PC98_PW
static void PWClockSet(short,unsigned long);
#endif
#ifdef PC98_PWLB
static void PWLBClockSet(short,unsigned long);
#endif

void AltICD2061SetClock(frequency, select)
register long   frequency;               /* in Hz */
int select;
{
   unsigned int m;
   int i;
   long dwv;
   double realval;
   double freq, fvco;
   double dev, devx;
   double delta, deltax;
   double f0;
   unsigned int p, q;
   unsigned int bestp=0, bestq=0, bestm=0, besti=0;
   unsigned char tmp;

   crtcaddr=(inb(0x3CC) & 0x01) ? 0x3D4 : 0x3B4;


   outb(crtcaddr, 0x11);	/* Unlock CRTC registers */
   tmp = inb(crtcaddr + 1);
   outb(crtcaddr + 1, tmp & ~0x80);

#if !defined(PC98_PW) && !defined(PC98_PWLB)
   outw(crtcaddr, 0x4838);	/* Unlock S3 register set */
   outw(crtcaddr, 0xA039);
#else
   outw(crtcaddr, 0x4838);	/* Unlock S3 register set */
   outw(crtcaddr, 0xA539);
#endif

   clknum = select;

   freq = ((double)frequency)/1000000.0;
   if (freq > range[13])
      freq = range[13];
   else if (freq < 7.0)
      freq = 7.0;

/*
 *  Calculate values to load into ICD 2061A clock chip to set frequency
 */
   delta = 999.0;
   dev   = 999.0;

   for (m = 0; m < 8; m++) {
      fvco = freq * (1<<m);
      if (fvco < 50.0 || fvco > 120.0) continue;
      
      f0  = fvco / fref;

      for (q = 14; q <= 71; q++) {     /* q={15..71}:Constraint 2 on page 14 */
	 p = (int)(f0 * q + 0.5);
	 if (p < 4 || p > 130)      /* p={4..130}:Constraint 5 on page 14 */
	    continue; 
	 deltax = (double)(p) / (double)(q) - f0;
	 if (deltax < 0) deltax = -deltax;
	 if (deltax <= delta) {
	    for (i = 13; i >= 0; i--)
	       if (fvco >= range[i])
		  break;
	    devx = (fvco - (range[i] + range[i+1])/2)/fvco;
	    if (devx < 0)
	       devx = -devx;
	    if (deltax < delta || devx < dev) {
	       delta = deltax;
	       dev   = devx;
	       bestp = p;
	       bestq = q;
	       bestm = m;
	       besti = i;
	    }
	 }
      }
   }
   fvco = fref / (1<<bestm);
   realval = (fvco * bestp) /  bestq;
#if !defined(PC98_PW) && !defined(PC98_PWLB)
   dwv = ((((((long)besti << 7) | (bestp-3)) << 3) | bestm) << 7) | (bestq-2);

#ifdef EXTENDED_DEBUG
   ErrorF("Setting ICD2061A compatible clock to %d\n",frequency);
#endif
/*
 * Write ICD 2061A clock chip
 */
   init_clock(((unsigned long)dwv) | (((long)clknum) << 21), crtcaddr);

   wait_vb();
   wait_vb();
   wait_vb();
   wait_vb();
   wait_vb();
   wait_vb();
   wait_vb();		/* 0.10 second delay... */
#else
	bestp = (~(0x82 - bestp)) & 0x7f;
	bestq = (~(0x81 - bestq)) & 0x7f;		
	dwv = ((((((((long)besti << 7) | bestp) << 1) | 1) << 3) | bestm) << 7) | (bestq-2);
#ifdef PC98_PW
	PWClockSet(select, (unsigned long)dwv);
#else
#ifdef PC98_PWLB
	PWLBClockSet(select, (unsigned long)dwv);
#endif
#endif

	inb(0x3d4);
	inb(0x3d4);
	inb(0x3d4);
	inb(0x3d4);
#endif
}


#if 0
static void prtbinary(size, val)
   unsigned int size;
   unsigned int val;
   {
   unsigned int mask;
   int k;

   mask = 1;

   for (k=size; --k > 0 || mask <= val/2;)
      mask <<= 1;

   while (mask) {
      fputc((mask&val)? '1': '0' , stderr);
      mask >>= 1;
      }
   }
#endif

static void wait_vb()
   {
   while ((inb(crtcaddr+6) & 0x08) == 0)
      ;
   while (inb(crtcaddr+6) & 0x08)
      ;
   }


#if NeedFunctionPrototypes
static void init_clock(unsigned long setup, unsigned short crtcport)
#else
static void init_clock(setup, crtcport)
   unsigned long setup;
   unsigned short crtcport;
#endif
   {
   unsigned char nclk[2], clk[2];
   unsigned short restore42;
   unsigned short oldclk;
   unsigned short bitval;
   int i;
   unsigned char c;

   (void)xf86DisableInterrupts();

   oldclk = inb(0x3CC);

   outb(crtcport, 0x42);
   restore42 = inb(crtcport+1);

   outw(0x3C4, 0x0100);

   outb(0x3C4, 1);
   c = inb(0x3C5);
   outb(0x3C5, 0x20 | c);

   outb(crtcport, 0x42);
   outb(crtcport+1, 0x03);

   outw(0x3C4, 0x0300);

   nclk[0] = oldclk & 0xF3;
   nclk[1] = nclk[0] | 0x08;
   clk[0] = nclk[0] | 0x04;
   clk[1] = nclk[0] | 0x0C;

   outb(crtcport, 0x42);
   i = inw(crtcport);

   outw(0x3C4, 0x0100);

   wrt_clk_bit(oldclk | 0x08);
   wrt_clk_bit(oldclk | 0x0C);
   for (i=0; i<5; i++) {
      wrt_clk_bit(nclk[1]);
      wrt_clk_bit(clk[1]);
      }
   wrt_clk_bit(nclk[1]);
   wrt_clk_bit(nclk[0]);
   wrt_clk_bit(clk[0]);
   wrt_clk_bit(nclk[0]);
   wrt_clk_bit(clk[0]);
   for (i=0; i<24; i++) {
      bitval = setup & 0x01;
      setup >>= 1;
      wrt_clk_bit(clk[1-bitval]);
      wrt_clk_bit(nclk[1-bitval]);
      wrt_clk_bit(nclk[bitval]);
      wrt_clk_bit(clk[bitval]);
      }
   wrt_clk_bit(clk[1]);
   wrt_clk_bit(nclk[1]);
   wrt_clk_bit(clk[1]);

   outb(0x3C4, 1);
   c = inb(0x3C5);
   outb(0x3C5, 0xDF & c);

   outb(crtcport, 0x42);
   outb(crtcport+1, restore42);

   outb(0x3C2, oldclk);

   outw(0x3C4, 0x0300);

   xf86EnableInterrupts();

   }

static void wrt_clk_bit(value)
   unsigned int value;
   {
   int j;

   outb(0x3C2, value);
   for (j=2; --j; )
      inb(0x200);
   }

#ifdef PC98_PW
static void
PWClockSet(short a, unsigned long setup)
{
	unsigned char b, div;	
	unsigned short	i, bitval;

	div = (a)? 0: rdinx(0x3d4, 0x41);
	b = (a)? 0x20: 0x90;

	for (i=0; i<22; i++) {
		bitval = setup & 0x01;
		setup >>= 1;
		div &= 0x0f;
		div |= (bitval)? 0x40: 0;
		div |= b;
		_outb(0x0dc|PW_PORT, div);
		div |= 0x30; 
		_outb(0x0dc|PW_PORT, div);
	}
}
#endif

#ifdef PC98_PWLB
static void
PWLBClockSet(short a, unsigned long setup)
{
	unsigned char b, div;	
	unsigned short	i, bitval;

	div = (a)? 0: rdinx(0x3d4, 0x41);
	b = (a)? 0x20: 0x90;

	for (i=0; i<22; i++) {
		bitval = setup & 0x01;
		setup >>= 1;
		div &= 0x0f;
		div |= (bitval)? 0x40: 0;
		div |= b;
		PWLBctrl(div);
		div |= 0x30; 
		PWLBctrl(div);
	}
}
#endif
