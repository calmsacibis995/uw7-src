
/*
 * @(#) mgaVid.c 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
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
 *      SCO Modifications
 *	S003	Thu Jun  1 16:56:14 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *      S002    Tue Nov 01 17:01:05 PST 1994    brianm@sco.com
 *              We need to set the ram speed once, so we watch for a '1'
 *              in the Value variable.  This says we haven't set it yet.
 *              We were seeing in the old code a DST0 of 0xffff0000
 *              which would cause the Value to get reset.
 *
 *      S001    Tue May 17 13:51:26 PDT 1994    hiramc@sco.COM
 *              For the AGA compile environment, need to include <time.h>
 */

/* mgaVid.c
 * This file was created out of several files from Matrox.
 * note the lovely FRENCH comments !!!!
 */

#include "mgaDefs.h"

#ifdef usl
#include <sys/types.h>
#include <sys/inline.h>
#else
#include "compiler.h"
#endif

#ifdef agaII
#include <time.h>                       /*      S001    */
#endif
#include <sys/time.h>

#include <string.h>

#define VCLK_DIVISOR 8
#define NO_FLOAT 1

#define DWORD unsigned long
#define WORD unsigned short
#define BYTE unsigned char

static int resToPitch[6] = { 640, 800, 1024, 1152, 1280, 1600};
static unsigned long crtcTab[34];

/* need to be inited in MGAVidInit */

static unsigned long Pitch;
static unsigned long DacType;
static char atlas;
static char athena;
static char bpp;
static char DubicPresent;
static char GALdetected;

/************************************************************************/
/*
* Name:           getAthenaRevision
*
* synopsis:       try to find the silicone revision of athena chip
*                 
*
* returns:        revision : -1   - Unknow or not a athena chip
*/
/* --------------------------- D.A.T. GEN0?? ---------------------------*/
/*  Strap  nomuxes   block8    tram   Athena rev|     Type de ram       */
/*           0         0         0        2     |4Mbit (8-bit blk node) */
/*           0         0         1        2     |  "      "     "       */                       
/*           0         1         0        2     |4Mbit (4-bit blk node) */                       
/*           0         1         1        1     |2Mbit (4-bit blk node) */                       
/*                                              |                       */
/*           1         0         0        1     |4Mbit (8-bit blk node) */                       
/*           1         0         1        1     |  "      "     "       */                       
/*           1         1         0        1     |4Mbit (4-bit blk node) */                       
/*           1         1         1        2     |2Mbit (4-bit blk node) */                       
/*----------------------------------------------------------------------*/

WORD getAthenaRevision(DWORD dst0, DWORD dst1)
{
   BYTE  _block8, _tram, _nomuxes;
   WORD  rev = 2;

   _block8  = (dst0 & TITAN_DST0_BLKMODE_M) != 0;
   _tram    = (dst1 & TITAN_DST1_TRAM_M)    != 0;
   _nomuxes = (dst1 & TITAN_DST1_NOMUXES_M) != 0;

   /* check if athena chip */
   if (!athena)
     return -1;

   if ( DacType = Info_Dac_TVP3026 )
      {
      if (_block8 && _tram) /* IMP+ /P */
         {
         rev = _nomuxes ? 2 : 1;
         }
      else
         {
         rev = _nomuxes ? 1 : 2;
         }
      }
#ifdef DEBUG_PRINT
ErrorF("block8 %d tram %d nomuxes %d rev %d\n",_block8,_tram, _nomuxes,rev);
#endif
   return rev;
}

void mgaDelay(int delay)
{
    clock_t start;
    start = clock();
    while((clock() - start) < (delay * 1000));
}
    
static vid vidtab_default [29] =
{
	{"PixelClk",              75000},      /* 0 */
	{"HFrontPorch",              24},
	{"HSync",                   136},
	{"HBackPorch",              144},
	{"HOverscan",                16},
	{"HVisible",               1024},      /* 5 */
	{"VFrontPorch",               3},
	{"VSync",                     6},
	{"VBackPorch",               29},
	{"VOverscan",                 2},
	{"VVisible",                768},      /* 10 */
	{"PWidth",                    8},
	{"OverscanEnable",            0},
	{"InterlaceModeEnable",       0},
	{"FrameBufferStart",          0},
	{"Zoom_factor_x",             1},      /* 15 */
	{"Zoom_factor_y",             1},
	{"VIDEOPitch",             1024},
	{"ALW",                       1},
	{"HBlankingSkew",             0},
	{"HRetraceEndSkew",           0},      /* 20 */
	{"CursorEndSkew",             0},
	{"CursorEnable",              0},
	{"CRTCTestEnable",            0},
	{"VIRQEnable",                0},
	{"Select5RefreshCycle",       0},      /* 25 */
	{"CRTCReg07Protect",          0},
	{"HSyncPol",                  0},
	{"VSyncPol",                  0}       /* 28 */
};

make_vidtab(Vidparm *vidparm, vid *tab) 
{
   Vidset *pvidset = &vidparm->VidsetPar[0];

   strcpy(tab[0].name, vidtab_default[0].name);
   tab[0].valeur  = (unsigned long)pvidset->PixClock;

   strcpy(tab[1].name, vidtab_default[1].name);
   tab[1].valeur  = (unsigned long)pvidset->HFPorch;

   strcpy(tab[2].name, vidtab_default[2].name);
   tab[2].valeur  = (unsigned long)pvidset->HSync;

   strcpy(tab[3].name, vidtab_default[3].name);
   tab[3].valeur  = (unsigned long)pvidset->HBPorch;

   strcpy(tab[4].name, vidtab_default[4].name);
   tab[4].valeur  = (unsigned long)pvidset->HOvscan;

   strcpy(tab[5].name, vidtab_default[5].name);
   tab[5].valeur  = (unsigned long)pvidset->HDisp;

   strcpy(tab[6].name, vidtab_default[6].name);
   tab[6].valeur  = (unsigned long)pvidset->VFPorch;

   strcpy(tab[7].name, vidtab_default[7].name);
   tab[7].valeur  = (unsigned long)pvidset->VSync;

   strcpy(tab[8].name, vidtab_default[8].name);
   tab[8].valeur  = (unsigned long)pvidset->VBPorch;

   strcpy(tab[9].name, vidtab_default[9].name);
   tab[9].valeur  = (unsigned long)pvidset->VOvscan;

   strcpy(tab[10].name, vidtab_default[10].name);
   tab[10].valeur = (unsigned long)pvidset->VDisp;

   strcpy(tab[11].name, vidtab_default[11].name);
   tab[11].valeur = vidparm->PixWidth;

   strcpy(tab[12].name, vidtab_default[12].name);
   tab[12].valeur = (unsigned long)pvidset->OvscanEnable;

   strcpy(tab[13].name, vidtab_default[13].name);
   tab[13].valeur = (unsigned long)pvidset->InterlaceEnable;

   strcpy(tab[14].name, vidtab_default[14].name);
   tab[14].valeur = 0;  /* Start address */

   strcpy(tab[15].name, vidtab_default[15].name);
   tab[15].valeur = 1;    /* Zoom factor X */

   strcpy(tab[16].name, vidtab_default[16].name);
   tab[16].valeur = 1;    /* Zoom factor Y */

   strcpy(tab[17].name, vidtab_default[17].name);
   tab[17].valeur = resToPitch[vidparm->Resolution];/* Virtuel video pitch */

   strcpy(tab[18].name, vidtab_default[18].name);
   if(0)                        /* should be safe */
      tab[18].valeur = 0;                  /* ALW  Non-Automatic Line Wrap */
   else
      tab[18].valeur = 1;

   strcpy(tab[19].name, vidtab_default[19].name);
   tab[19].valeur = vidtab_default[19].valeur;

   strcpy(tab[20].name, vidtab_default[20].name);
   tab[20].valeur = vidtab_default[20].valeur;

   strcpy(tab[21].name, vidtab_default[21].name);
   tab[21].valeur = vidtab_default[21].valeur;

   strcpy(tab[22].name, vidtab_default[22].name);
   tab[22].valeur = vidtab_default[22].valeur;

   strcpy(tab[23].name, vidtab_default[23].name);
   tab[23].valeur = vidtab_default[23].valeur;

   strcpy(tab[24].name, vidtab_default[24].name);
   tab[24].valeur = vidtab_default[24].valeur;

   strcpy(tab[25].name, vidtab_default[25].name);
   tab[25].valeur = vidtab_default[25].valeur;

   strcpy(tab[26].name, vidtab_default[26].name);
   tab[26].valeur = vidtab_default[26].valeur;

   strcpy(tab[27].name, vidtab_default[27].name);
   tab[27].valeur = (unsigned long)pvidset->HsyncPol;

   strcpy(tab[28].name, vidtab_default[28].name);
   tab[28].valeur = (unsigned long)pvidset->VsyncPol;

}

/************************************************************************/
/*                                                                      */
/*  Translate the CTRC value in MGA registers value...                  */
/*                                                                      */
/************************************************************************/


void mgacalculCrtcParam(vid *vidtab)
{


/*declarations-------------------------------------------------------------*/

unsigned long bs, be;
unsigned long h_front_p,hbskew,hretskew,h_sync,h_back_p,h_over;
unsigned long h_vis,h_total,v_vis,v_over,v_front_p,v_sync,v_back_p,vert_t;                     
unsigned long start_add_f,virt_vid_p,v_blank;
unsigned char v_blank_e,rline_c,cpu_p,page_s,alw,p_width;                
unsigned long mux_rate,ldclk_en_dotclk;
int over_e,hsync_pol,div_1024,int_m;
int vclkhtotal,vclkhvis,vclkhsync,vclkhover,vclkhbackp,vclkhblank,vclkhfrontp;
int x_zoom,y_zoom;
int Cur_e,Crtc_tst_e,cur_e_sk,e_v_int,sel_5ref,crtc_reg_prt,i;

long rep_ge,nb_vclk_hfront_p,nb_vclk_hblank,delai,delai2,delai3,delai4,gclk;
long vclk_divisor,pixel_clk;

int flagDelay3=0, flagDelay4=0;

long delayTab[4]={5000,11000,24000,28000};  /* table definit pour la valeur du 
delai */
long delayTab_A[6]={3000, 4000, 5000,11000,24000,28000};  /* table definit pour 
la valeur du delai */
					    /* du registre crtc_ctrl du titan    
    */


/* struc pour registre horizontal blanking end ----------------------------*/                     

union{                                
				      
     struct {unsigned char pos :5;                  
	     unsigned char skew:2;
	     unsigned char res :1;

	    }f;       
     unsigned char all;
     } hor_blk_e;        


/* struct du registre du horizontal retrace end----------------------------*/

union{                                
				 
     struct {unsigned char pos :5;                  
	     unsigned char skew:2;
	     unsigned char res :1;
	    }f;       
     unsigned char all;
     } hor_ret;        


/* structure pour le calcul du horizontal blank end position----------------*/
/* car le dernier bit va dans le registre horizontal retrace end------------*/

union{                                       
     struct {unsigned char horz_blank_e :5;       
	     unsigned char horz_ret     :1;
	     unsigned char unused       :2;
	    }f;       
     unsigned char all;
     } hor_blk_a;        


/* structure pour le calcul du vertical total car les 7 premier bit vont----*/ 
/* dans le registre vertical total et 8 et 9 vont dans le registre overflow-*/       


union{                                        
					      
     struct {unsigned long vert_t0_7    :8;         
	     unsigned long vert_t8      :1;   
	     unsigned long vert_t9      :1;    
	     unsigned long unused       :22;
	    }f;       
     unsigned long all;
     } v_total;        


/* structure du registre overflow-------------------------------------------*/

union{                                  
					
     struct {unsigned char r0   :1;       
	     unsigned char r1   :1;      
	     unsigned char r2   :1;      
	     unsigned char r3   :1;      
	     unsigned char r4   :1;      
	     unsigned char r5   :1;      
	     unsigned char r6   :1;      
	     unsigned char r7   :1;      
	    }f;       
     unsigned char all;
     } r_overf;        

/* structure du registre maximum scan line----------------------------------*/
union{                                  
				       
     struct {unsigned char maxsl       :5;       
	     unsigned char v_blank_9   :1;      
	     unsigned char line_c      :1;      
	     unsigned char line_doub_e :1;      
	    }f;       
     unsigned char all;
     } r_maxscanl;        

/* structure du registre cursor start---------------------------------------*/

union{                                  
				       
     struct {unsigned char curstart    :5;       
	     unsigned char cur_e       :1;      
	     unsigned char res         :1;      
	     unsigned char crtc_tst_e  :1;      
	    }f;       
     unsigned char all;
     } r_cursor_s;        

/* structure du registre cursor end----------------------------------------*/

union{                                  
				       
     struct {unsigned char curend    :5;       
	     unsigned char cur_sk    :2;      
	     unsigned char res       :1;      
	    }f;       
     unsigned char all;
     } r_cursor_e;        

/* structure pour calcul du vertical retrace start--------------------------*/
/* les 7 bits les moins significatif vont dans le registre et les 2 autres  */ 
/* vont dans le registre overflow                                           */

union{                                  
				       
     struct {unsigned long bit0_7    :8;       
	     unsigned long bit8      :1;      
	     unsigned long bit9      :1;      
	    }f;       
     unsigned long all;
      } rvert_ret_s;

/* structure du registre vertical retrace end-------------------------------*/

union{                                  
				       
     struct {unsigned char bit0_3       :4;       
	     unsigned char cl           :1;      
	     unsigned char e_vi         :1;
	     unsigned char sel_5ref     :1;
	     unsigned char reg_prot     :1;
	    }f;       
     unsigned char all;
      } rvert_ret_e;

/* structure du calcul du vertical display enable end-----------------------*/

union{                                  
				       
     struct {unsigned long bit0_7       :8;       
	     unsigned long bit8         :1;      
	     unsigned long bit9         :1;
	    }f;       
     unsigned long all;
      } v_disp_e;

/* structure du calcul du vertical blanking start--------------------------*/
union{                                  
				       
     struct {unsigned long bit0_7       :8;
	     unsigned long bit8         :1;      
	     unsigned long bit9         :1;
	    }f;       
     unsigned long all;
      } v_blank_s;

/* structure du registre mode control--------------------------------------*/

union{                                  
					
     struct {unsigned char r0   :1;       
	     unsigned char r1   :1;      
	     unsigned char r2   :1;      
	     unsigned char r3   :1;      
	     unsigned char r4   :1;      
	     unsigned char r5   :1;      
	     unsigned char r6   :1;      
	     unsigned char r7   :1;      
	    }f;       
     unsigned char all;
     } r_mode;        

/* structure du registre crtc extended add register------------------------*/

union{                                  
					
     struct {unsigned char crtcadd    :2;       
	     unsigned char crtccur    :2;      
	     unsigned char crtc_add_e :1;      
	     unsigned char aux_cl     :1;      
	     unsigned char dip_sw_e   :1;      
	     unsigned char irq_e      :1;      
	    }f;       
     unsigned char all;
     } r_crtc_ext;        

/*structure du registre interlace support register--------------------------*/

union{                                  
					
     struct {unsigned char crtcadd    :4;       
	     unsigned char seq_vid    :1;      
	     unsigned char port       :1;      
	     unsigned char m_int      :1;      
	     unsigned char hr_16      :1;      
	    }f;       
     unsigned char all;
     } r_int_s;




/* gclk=25.0e-9; */
vclk_divisor=VCLK_DIVISOR;



/* variable definit pour la table d'entree-------------------------------- */
if (GALdetected)
  vidtab[18].valeur = 0;                   /* ALW  Non-Automatic Line Wrap */

pixel_clk     =vidtab[0].valeur * 1000;
h_front_p     =vidtab[1].valeur;  
h_sync        =vidtab[2].valeur;  
h_back_p      =vidtab[3].valeur;  
h_over        =vidtab[4].valeur;
h_vis         =vidtab[5].valeur;
v_front_p     =vidtab[6].valeur;
v_sync        =vidtab[7].valeur;   
v_back_p      =vidtab[8].valeur;
v_over        =vidtab[9].valeur;
v_vis         =vidtab[10].valeur;    
p_width       =(unsigned char)vidtab[11].valeur;
over_e        =vidtab[12].valeur;
int_m         =vidtab[13].valeur;
start_add_f   =vidtab[14].valeur;
x_zoom        =vidtab[15].valeur;
y_zoom        =vidtab[16].valeur;
virt_vid_p    =vidtab[17].valeur;
alw           =(unsigned char)vidtab[18].valeur;
hbskew        =vidtab[19].valeur;
hretskew      =vidtab[20].valeur;
cur_e_sk      =vidtab[21].valeur;
Cur_e         =vidtab[22].valeur;
Crtc_tst_e    =vidtab[23].valeur;
e_v_int       =vidtab[24].valeur;
sel_5ref      =vidtab[25].valeur;
crtc_reg_prt  =vidtab[26].valeur;
hsync_pol     =vidtab[27].valeur;



/*----------------calcul du reg htotal-------------------------------------*/
 
   h_total=h_vis+h_front_p+h_sync+h_back_p;
	      
#ifdef NO_FLOAT              
   vclkhtotal= (((h_total/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhtotal=h_total/VCLK_DIVISOR+0.5;
#endif
		 
   crtcTab[0]=(vclkhtotal/x_zoom)-5;


/*----------------calcul du registre horizontal display enable end---------*/

#ifdef NO_FLOAT
   vclkhvis=(((h_vis/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhvis=h_vis/VCLK_DIVISOR+0.5;
#endif

   crtcTab[1]=(vclkhvis/x_zoom)-1;      
      

/*----------------calcul du registre horizontal blanking start-------------*/

 if(GALdetected == 0)
      {
      if (h_over>0)
         {
         if (h_over<(VCLK_DIVISOR/2))
            vclkhover=1*over_e;
         else 
         #ifdef NO_FLOAT
            vclkhover=((((h_over/VCLK_DIVISOR)*10)+5)/10)*over_e;
         #else
            vclkhover=(h_over/VCLK_DIVISOR+0.5)*over_e;
         #endif
         }               
      else 
         vclkhover=0;

         crtcTab[2]=((vclkhvis+vclkhover)/x_zoom)-1;

      }      
   else
      crtcTab[2] = crtcTab[1] - 1;                         /* See DAT #??? */


/*---------------calcul du registre horizontal blanking end----------------*/

     
#ifdef NO_FLOAT     
   vclkhfrontp=(((h_front_p/VCLK_DIVISOR)*10)+5)/10;

   vclkhsync=(((h_sync/VCLK_DIVISOR)*10)+5)/10;

   vclkhbackp=(((h_back_p/VCLK_DIVISOR)*10)+5)/10;
#else
   vclkhfrontp=h_front_p/VCLK_DIVISOR+0.5;

   vclkhsync=h_sync/VCLK_DIVISOR+0.5;

   vclkhbackp=h_back_p/VCLK_DIVISOR+0.5;
#endif

   vclkhblank=vclkhfrontp+vclkhbackp+vclkhsync-(2*vclkhover*over_e);
   
   hor_blk_a.all=((vclkhvis+vclkhover+vclkhblank)/x_zoom)-2;

   bs = crtcTab[2];
   be = hor_blk_a.all + 1;

   if (!((0x40 > bs) & (0x40 < be) ||
	 (0x80 > bs) & (0x80 < be) ||
	 (0xc0 > bs) & (0xc0 < be)))
      hor_blk_a.all = 0;

   if (GALdetected == 0)
      hor_blk_e.f.pos=hor_blk_a.f.horz_blank_e;
   else
      hor_blk_e.f.pos=(vclkhtotal/x_zoom)-2;

   hor_blk_e.f.skew=hbskew;

   hor_blk_e.f.res=0;

   crtcTab[3]=hor_blk_e.all;


/*---------------calcul du registre horizontal retrace start---------------*/


   crtcTab[4]=((vclkhvis+vclkhfrontp)/x_zoom)-1;



/*---------------calcul du registre horizontal retrace end-----------------*/


   hor_ret.f.pos=((vclkhvis+vclkhfrontp+vclkhsync)/x_zoom)-1;

   hor_ret.f.skew=hretskew;

   if(GALdetected == 0)
      hor_ret.f.res=hor_blk_a.f.horz_ret;
   else
      hor_ret.f.res=(unsigned char)(crtcTab[0]+3) >> 5;


   crtcTab[5]=hor_ret.all;

/*--------------calcul du registre vertical total--------------------------*/

   vert_t=v_vis+v_front_p+v_sync+v_back_p;      
						   
   div_1024=(vert_t>1023) ? 2 : 1;

   v_total.all=vert_t/div_1024-2;

   crtcTab[6]=v_total.f.vert_t0_7;
      

/*-------------registre preset row scan------------------------------------*/

   crtcTab[8]=0;


/*----------------calcul du registre cursor start--------------------------*/


   r_cursor_s.f.curstart=0;       

   r_cursor_s.f.cur_e=~Cur_e;
	    
   r_cursor_s.f.res=0;               

   r_cursor_s.f.crtc_tst_e=Crtc_tst_e;
	 
   crtcTab[10]=r_cursor_s.all;
	
/*----------------calcul du registre cursur end----------------------------*/

   r_cursor_e.f.curend=0;

   r_cursor_e.f.cur_sk=cur_e_sk;          
      
   r_cursor_e.f.res=0;             
		  
   crtcTab[11]=r_cursor_e.all;
   
/*----------------calcul du  registre add start add high--------------------*/
   start_add_f = start_add_f / 8;

   crtcTab[12]=start_add_f/256;

/*-----------------calcul du registre add start add low---------------------*/

   crtcTab[13]=start_add_f;

/*-----------------registre cursor position high----------------------------*/

   crtcTab[14]=0;

/*-----------------registre cursor position low-----------------------------*/

   crtcTab[15]=0;

/*------------------calcul du registre  vertical retrace strat--------------*/

   rvert_ret_s.all=((v_vis+v_front_p)/div_1024)-1;      

   crtcTab[16]=rvert_ret_s.f.bit0_7;


/*------------------calcul du registre vertical retrace end-----------------*/

   rvert_ret_e.f.bit0_3=(v_vis+v_front_p+v_sync)/div_1024-1;

   rvert_ret_e.f.cl=1;

   rvert_ret_e.f.e_vi=e_v_int;

   rvert_ret_e.f.sel_5ref=sel_5ref;

   rvert_ret_e.f.reg_prot=crtc_reg_prt;

   crtcTab[17]=rvert_ret_e.all;

/*------------------calcul du registre vertical display enable-------------*/

   v_disp_e.all=v_vis/div_1024-1;

   crtcTab[18]=v_disp_e.f.bit0_7;

/*--------------calcul pour le registre offset-----------------------------*/

   crtcTab[19]=((virt_vid_p/VCLK_DIVISOR)+(virt_vid_p*int_m/VCLK_DIVISOR))/2;

/*------------------registre under line location---------------------------*/

   crtcTab[20]=0;

/*--------------calcul du vertical blanking start--------------------------*/

   v_blank_s.all=(v_vis+(v_over*over_e))/div_1024-1;

   crtcTab[21]=v_blank_s.f.bit0_7;

/*--------------calcul du registre vertical blanking end-------------------*/

   v_blank=v_back_p+v_sync+v_front_p-(2*v_over*over_e);

   v_blank_e=(unsigned char)((v_vis+(v_over*over_e)+v_blank-((v_over*over_e>0) ? 0 : 
1))/div_1024-1);

   crtcTab[22]=v_blank_e;


/*--------------registre mode control--------------------------------------*/

   r_mode.f.r0=0;

   r_mode.f.r1=0;

   r_mode.f.r2=div_1024==2 ? 1 : 0 ;

   r_mode.f.r3=0;

   r_mode.f.r4=0;

   r_mode.f.r5=0;

   r_mode.f.r6=1;

   r_mode.f.r7=1;

   crtcTab[23]=r_mode.all;

/*-----------registre line compare-----------------------------------------*/

   rline_c=0xFF;
		 
   crtcTab[24]=rline_c;


/*--------------calcul du registre maximum scan line-----------------------*/


   r_maxscanl.f.maxsl=(y_zoom==1) ? 0 : y_zoom-1;

   r_maxscanl.f.v_blank_9=(unsigned char)v_blank_s.f.bit9;

   r_maxscanl.f.line_c=1;

   r_maxscanl.f.line_doub_e=0;

   crtcTab[9]=r_maxscanl.all;

/*--------------registre overflow------------------------------------------*/


   r_overf.f.r0=(unsigned char)v_total.f.vert_t8;

   r_overf.f.r1=(unsigned char)v_disp_e.f.bit8;

   r_overf.f.r2=(unsigned char)rvert_ret_s.f.bit8;

   r_overf.f.r3=(unsigned char)v_blank_s.f.bit8;

   r_overf.f.r4=1;

   r_overf.f.r5=(unsigned char)v_total.f.vert_t9;

   r_overf.f.r6=(unsigned char)v_disp_e.f.bit9;
   
   r_overf.f.r7=(unsigned char)rvert_ret_s.f.bit9;

   crtcTab[7]=r_overf.all;

/*------------registre cpu page select-------------------------------------*/

   cpu_p=0;

   crtcTab[25]=cpu_p;

/*------------registre crtc extended address-------------------------------*/

   r_crtc_ext.f.crtcadd=0;
       
   r_crtc_ext.f.crtccur=0;
      
   r_crtc_ext.f.crtc_add_e=1;
      
   r_crtc_ext.f.aux_cl=0;
      
   r_crtc_ext.f.dip_sw_e=0;
      
   r_crtc_ext.f.irq_e=1;
      
   crtcTab[26]=r_crtc_ext.all;        

/*------------registre 32k video ram page select register-----------------*/

   page_s=0;

   crtcTab[27]=page_s;

/*-----------registre interlace support register--------------------------*/

 
   r_int_s.f.crtcadd=0;
       
   r_int_s.f.seq_vid=0;
      
   r_int_s.f.port=0;
			      
   r_int_s.f.m_int=int_m;
     
   r_int_s.f.hr_16=0;      

   crtcTab[28]=r_int_s.all;        

/*------------autre argument rentree dans la table------------------------*/ 

   crtcTab[29]=alw;

   crtcTab[30]=pixel_clk;

   crtcTab[32]=hsync_pol;


/*************************************************************************/
/*                                                                       */
/* calcul du video delay pour le registre crtc_ctrl                      */
/*                                                                       */
/*************************************************************************/


   rep_ge=(37*25*(pixel_clk/1000000)) / vclk_divisor;

   nb_vclk_hfront_p= (h_front_p*1000) / (vclk_divisor * x_zoom);
 
   nb_vclk_hblank=( ((h_front_p+h_sync+h_back_p)*1000) / (VCLK_DIVISOR) / 
x_zoom);

   delai=nb_vclk_hfront_p-1000;

   delai2=rep_ge-625;

   delai3=nb_vclk_hblank-rep_ge-2000;

   delai4=((nb_vclk_hblank+1000)/2)-3000;

   /*** BEN ajout ***/
   crtcTab[31]=0x00;

   if ( !atlas )  /* TITAN */
      {
      for (i=0;i<4;i++)   
	 {
	 if (delayTab[i]>=delai)
	    {
	    if (delayTab[i]>=delai2) 
	       {
	       if (delayTab[i]<=delai3)
		  {
		  /*** La contrainte 4 a ete eliminee temporairement ***/
		  /*    if (delayTab[i]<=delai4) */
		  /*     { */
		  if (delayTab[i]==28000)
		    {
		    crtcTab[31]=0x03;
		    break;
		    }
		  else if (delayTab[i]==24000)
		    {
		    crtcTab[31]=0x02;
		    break;
		    }
		  else if (delayTab[i]==11000)
		    {
		    crtcTab[31]=0x01;
		    break;
		    }
		  else
		    {
		    crtcTab[31]=0x00;
		    break;
		    }
	    /*    } */
		  }                                                            
	       }
	    }   
	 }
	  }
   else    /* 2 video delay add for atlas and I suppose,for newest     */ 
      {    /* variation of titan they have the same video delay        */

      for (i=0;i<6;i++)   
	 {
	 if (delayTab_A[i]>=delai)
	    {
	    if (delayTab_A[i]>=delai2) 
	       {
	       if (delayTab_A[i]<=delai3)
		  {
		     if (delayTab_A[i]==28000)
		       {
		       crtcTab[31]=0x3;
		       break;
		       }
		     else if (delayTab_A[i]==24000)
		       {
		       crtcTab[31]=0x2;
		       break;
		       }
		     else if (delayTab_A[i]==11000)
		       {
		       crtcTab[31]=0x1;
		       break;
		       }
		     else if (delayTab_A[i]==5000)
		       {
		       crtcTab[31]=0x0;
		       break;
		       }
		     else if (delayTab_A[i]==4000)
		       {
		       crtcTab[31]=0x5;
		       flagDelay4=1;
             break;
		       } 
		     else if (delayTab_A[i]==3000) 
		       {
		       crtcTab[31]=0x4;
             flagDelay3=1;
             continue;
		       }
		     }
		  }                                                            
	       }
	    }   
	 }  

  if ( flagDelay3 && flagDelay4 )
      crtcTab[31]=0x5; /* if we are videodelay 3 and 4 they respect 3 rules */



/***************************************************************************/
/*                                                                         */
/* calcul hsync delay for the register dub_ctl for the field  hsync_del    */
/*                                                                         */
/***************************************************************************/

   switch (p_width)
      {                            
      case 8:                      
	 mux_rate=ldclk_en_dotclk=4; 
	 break;                    
				      
      case 16:                     
	 mux_rate=ldclk_en_dotclk=2; 
	 break;                    
				   
      case 32:                     
	 mux_rate=ldclk_en_dotclk=1; 
	 break;                    
      }

   switch (DacType)  
      {                                   
      case Info_Dac_BT485: /*********************************/               
			   /*     Pour BT485                */               
			   /*                               */               
			   /* syncdel=1 ldclk exprime en    */               
			   /*         dotclock + 7 dotclock */               
			   /*         --------------------- */               
			   /*               (muxrate)       */               
			   /*                               */               
			   /* syncdel= une valeur entiere   */               
			   /*          et arrondie          */               
			   /*                               */               
			   /* muxrate= ( 8bpp=4:1)          */               
			   /*        = (16bpp=2:1)          */               
			   /*        = (32bpp=1:1)          */               
			   /*                               */               
			   /*                               */               
			   /*********************************/               
	 crtcTab[33]=((ldclk_en_dotclk+7)*100/mux_rate+50)/100;
	 break;

#ifdef SCRAP
	case Info_Dac_BT481:

	   crtcTab[33]=02;
	   break;
#endif

      case Info_Dac_TVP3026:
	 crtcTab[33]=0;
	 break;


      default:

	 crtcTab[33]=00;
	 break;
		 

      }

}

/*/****************************************************************************
*          name: GetMGAConfiguration
*
*   description: This function will read the strappings of the specified MGA
*                device from the Titan DSTi1-0 registers (with the PATCH).
*                Also this function will return a DAC id and some other
*                relevent informations.
*
*      designed: Bart Simpson, february 10, 1993
* last modified: $Author$, $Date$
*
*       version: $Id$
*
*    parameters: _Far BYTE *pMGA, DWORD* DST1, DWORD* DST0, DWORD* Info
*      modifies: *DST1, *DST0, *Info
*         calls: -
*       returns: DST0, DST1, Info
*
*
* NOTE: DST1, DST0 are in Titan format.
*
*       Info is 0x uuuu uuuu uuuu uuuu uuuu uuuu uuuu dddd
*
*            dddd : 2 MSB are DAC extended id
*                   2 LSB are DAC DUBIC id
*
*            0000 : BT481
*            0100 : BT482
*            1000 : ATT
*            1100 : Sierra
*
*            0001 : ViewPoint
*
*            0010 : BT484
*            0110 : BT485
*
*            0011 : Chameleon
*
*       ported by Dave Arnold for SCO 3/25/94
******************************************************************************/


/******************************************************************************

 Video Buffer used to intialize the VIDEO related hardware

*/

#define CHAR_S 1
#define SHORT_S 2
#define LONG_S 4

#define VIDEOBUF_ALW                0
#define VIDEOBUF_ALW_S              CHAR_S
#define VIDEOBUF_Interlace          (VIDEOBUF_ALW + CHAR_S)
#define VIDEOBUF_Interlace_S        CHAR_S
#define VIDEOBUF_VideoDelay         (VIDEOBUF_Interlace + CHAR_S)
#define VIDEOBUF_VideoDelay_S       CHAR_S
#define VIDEOBUF_VsyncPol           (VIDEOBUF_VideoDelay + CHAR_S)
#define VIDEOBUF_VsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncPol           (VIDEOBUF_VsyncPol + CHAR_S)
#define VIDEOBUF_HsyncPol_S         CHAR_S
#define VIDEOBUF_HsyncDelay         (VIDEOBUF_HsyncPol + CHAR_S)
#define VIDEOBUF_HsyncDelay_S       CHAR_S
#define VIDEOBUF_Srate              (VIDEOBUF_HsyncDelay + CHAR_S)
#define VIDEOBUF_Srate_S            CHAR_S
#define VIDEOBUF_LaserScl           (VIDEOBUF_Srate + CHAR_S)
#define VIDEOBUF_LaserScl_S         CHAR_S
#define VIDEOBUF_OvsColor           (VIDEOBUF_LaserScl + CHAR_S)
#define VIDEOBUF_OvsColor_S         LONG_S
#define VIDEOBUF_Pedestal           (VIDEOBUF_OvsColor + LONG_S)
#define VIDEOBUF_Pedestal_S         CHAR_S
#define VIDEOBUF_LvidInitFlag       VIDEOBUF_Pedestal /** Bit 7 de Pedestal **/
#define VIDEOBUF_DBWinXOffset       (VIDEOBUF_Pedestal + CHAR_S)
#define VIDEOBUF_DBWinXOffset_S     SHORT_S
#define VIDEOBUF_DBWinYOffset       (VIDEOBUF_DBWinXOffset + SHORT_S)
#define VIDEOBUF_DBWinYOffset_S     SHORT_S
#define VIDEOBUF_PCLK               (VIDEOBUF_DBWinYOffset + SHORT_S)
#define VIDEOBUF_PCLK_S             LONG_S
#define VIDEOBUF_CRTC               (VIDEOBUF_PCLK + LONG_S)
#define VIDEOBUF_CRTC_S             (CHAR_S * 29)
#define VIDEOBUF_S                  (VIDEOBUF_CRTC + (CHAR_S * 29))

DWORD GetMGAMctlwtst(DWORD DST0, DWORD DST1)
{
  static DWORD Value = 1;       /* S002 */

 if (Value == 1)                        /* S002 We haven't set it yet. */
       {
      switch ((DST1 & TITAN_DST1_RAMSPEED_M) >> TITAN_DST1_RAMSPEED_A)
	 {
	 case 0x3:  /*** 80 ns. ***/
   
	    Value = 0xc4001110;
	    break;
   
	 default:
   
	    Value = 0xffffffff;
	    break;
	 }
      }

   return (Value);
}

void GetMGAConfiguration(VOLATILE mgaRegsPtr pDevice, DWORD *DST0, DWORD *DST1,
			 DWORD *Info)
{
      VOLATILE DWORD TmpDword;
      BYTE  TmpByte, TmpByte2;
      BYTE  DacDetected;

      { DWORD Opmode, SrcPage;

      /** Wait until the FIFO is empty, Then we're sure that the Titan
      *** is ready to process the DATA transfers.
      **/
#ifdef DEBUG_PRINT
ErrorF("in GetMGAConfiguration\n");
#endif
      while((pDevice->titan.fifostatus & 0x20) == 0);

      Opmode = pDevice->titan.opmode;
      SrcPage = pDevice->titan.srcpage;

      pDevice->titan.opmode = Opmode & ~((DWORD)TITAN_FBM_M |
					 (DWORD)TITAN_PSEUDODMA_M);
      pDevice->titan.drawSetup.mctlwtst = 0xffffffff;
      pDevice->titan.srcpage = 0x00f80000;

      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = *(DWORD *)pDevice->srcwin.window;
      TmpDword = TmpDword;

      *DST0 = pDevice->titan.drawSetup.dst0;
      *DST1 = pDevice->titan.drawSetup.dst1;

      /*** Reset Opmode with pseudo-dma off first and then to its previous state 
***/

      pDevice->titan.opmode = Opmode & ~((DWORD)TITAN_PSEUDODMA_M);
      pDevice->titan.opmode = Opmode;
      pDevice->titan.drawSetup.mctlwtst = GetMGAMctlwtst(*DST0, *DST1);
      pDevice->titan.srcpage = SrcPage;
      }

      /** Because the Titan DSTi0-1 spec has changed after the binding
      *** developpement (i.e. the rambank[0] bit has moved from bit 0 of DST0
      *** to bit 3 of DST1) we duplicate the DST1[3] to DST0[24] because the
      *** binding has nothing to do with the new VGA BANK 0 bit
      **/
   
      /*** Clear bit rambank 0 ***/
      (*DST0) &= ~((DWORD)0x1 << TITAN_DST0_RAMBANK_A);

      /*** DUPLICATE ***/
      (*DST0) |= ((((*DST1) & (DWORD)TITAN_DST1_RAMBANK0_M) >>
		   TITAN_DST1_RAMBANK0_A) << TITAN_DST0_RAMBANK_A);

      /***** DETECT DAC TYPE *****/

      TmpByte2 = pDevice->ramdacs.vpoint.index;
      pDevice->ramdacs.vpoint.index = VPOINT_ID;
      TmpByte = pDevice->ramdacs.vpoint.data;
      if (TmpByte == 0x20)
	 DacDetected = (BYTE)Info_Dac_ViewPoint;
      else
       {
       pDevice->ramdacs.vpoint.index = TmpByte2;
       pDevice->ramdacs.tvp3026.index = TVP3026_ID;
       TmpByte = pDevice->ramdacs.tvp3026.data;
       if (TmpByte == 0x26)
	  DacDetected = (BYTE)Info_Dac_TVP3026;
       else
	 {
	 pDevice->ramdacs.bt485.wadr_pal =  0x00;
	 pDevice->ramdacs.bt485.pix_rd_msk =  0xff;
	 TmpByte = pDevice->ramdacs.bt485.status;

	 if ((TmpByte & (BYTE)ATT20C505_ID_M) == (BYTE)ATT20C505_ID)
	    DacDetected = (BYTE)Info_Dac_BT485;
	 else
	    {
	    if ((TmpByte & (BYTE)BT485_ID_M) == (BYTE)BT485_ID)
	       DacDetected = (BYTE)Info_Dac_BT485;
	    else
	       if ((TmpByte & (BYTE)BT484_ID_M) == (BYTE)BT484_ID)
		  DacDetected = (BYTE)Info_Dac_BT484;
	       else
		  {
		  pDevice->ramdacs.bt485.cur_xlow  = 0x00;
		  pDevice->ramdacs.bt485.wadr_ovl  = 0x55;
		  pDevice->ramdacs.bt485.cur_xlow  = TmpByte;
		  if (TmpByte == 0x55)
		     DacDetected = (BYTE)Info_Dac_BT482;    /* SIERRA */
		  else
		     DacDetected = (BYTE)Info_Dac_BT485;    /* DAC de COMPAQ 
comme BT485 */
		  }
	    }
       }
     }
      *Info = (DWORD)DacDetected;
#ifdef DEBUG_PRINT
ErrorF("out of GetMGAConfiguration\n");
#endif
}

/*/****************************************************************************
*          name: MoveToVideoBuffer
*
*   description: This function will move from the NPI structures the required
*                information for MGAVidInit.
*
*      designed: Bart Simpson. february 17, 1993
* last modified: $Author$, $Date$
*
*       version: $Id$
*
*    parameters: BYTE* VidTab, BYTE* CrtcTab, BYTE* VideoBuffer
*      modifies: VideoBuffer content
*         calls: -
*       returns: -
******************************************************************************/

#ifdef NO_FLOAT

void MoveToVideoBuffer(vid* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer)
{

/*** Valeurs du PCLK pour le RAMDAC ***/

typedef struct
 {
 DWORD FbPitch;
 long Pclk_I;
 long Pclk_NI;
 } Pclk;
 

Pclk Pclk_Ramdac[] = {
     640,  1227,  3150,
     768,  1475,  4500, 
     800,  3300 , 5000,
    1024,  4500 , 7500, 
    1152,  6200 , 10600,
    1280,  8000 , 13500,
    1600,     0 , 20000, {(DWORD) -1}
};



Pclk *p1;
long Sclk;
DWORD Srate;
BYTE LaserScl;
char ilace;




   /*** Move parameters from VidTab in the VideoBuffer ***/

//[dlee] Don't use magic numbers to index into Vid structure! When building 
//   32-bit executables, the elements of the Vid structure are dword aligned, 
//   not byte aligned!!!
//

   *((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))       = (BYTE)pVidTab[18].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) = (BYTE)pVidTab[13].valeur;
   *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))     = (DWORD)pVidTab[0].valeur;

   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinXOffset)) = (WORD)pVidTab[3].valeur;

   /* In interlace mode, we must double the Vertical back porch */
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinYOffset)) =
      (WORD)pVidTab[13].valeur ?
	   (WORD)pVidTab[8].valeur * 2 :  /* Interlace */
	   (WORD)pVidTab[8].valeur;

   /*** Set some new parameters in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay)) = *((BYTE*)(pCrtcTab + (31 * 
LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))   = (BYTE)((vid *)pVidTab)[28].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))   = (BYTE)((vid *)pVidTab)[27].valeur;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) = *((BYTE*)(pCrtcTab + (33 * LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal))   &= 0x80;   /**** FORCE 
!#@$!@#$!@#$ ***/

   *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor))   = 0x0000; /**** FORCE 
!#@$!@#$!@#$ ***/


   /*** srate and laserscl need to be set according to PCLK ***/
   /*******************************************************************/
   /*** PIXEL CLOCK : Programmation of registers SRATE and LASERSCL ***/
   /*** N.B. Values of SCLK and SPLCLK are in MHz ***/

   /* Find PCLK value in table Pclk_Ramdac[] */
   for (p1 = Pclk_Ramdac; p1->FbPitch != (DWORD)-1; p1++)
      {
      if (p1->FbPitch == Pitch)
	 {
	  if ((BYTE)pVidTab[13].valeur)  /* if interlace */
	    {
	    switch (DacType)
	       {
	       case Info_Dac_BT482:
		  Sclk = p1->Pclk_I*(bpp/8);
		  break;
	       case Info_Dac_BT485:
	       case Info_Dac_ViewPoint:
	       case Info_Dac_TVP3026:
		  Sclk = p1->Pclk_I/(32/bpp);
		  break;
	       }
	    }
	  else
	    {
	    switch (DacType)
	       {
	       case Info_Dac_BT482:
		  Sclk = p1->Pclk_NI*(bpp/8);
		  break;
	       case Info_Dac_BT485:
	       case Info_Dac_ViewPoint:
	       case Info_Dac_TVP3026:
		  Sclk = p1->Pclk_NI/(32/bpp);
		  break;
	       }
	    }
	  break;
	 }
      }


   Srate = (DWORD)((Sclk*100)/16384);       /* Srate = SCLK / (200*32*256) */
   LaserScl = 0;

   *((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))      = Srate;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))   = LaserScl;

   /*** move the CTRC parameters in the VideoBuffer ***/

   { WORD i;

   for (i = 0; i <= 28; i++)
      {
      *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + i)) = *((BYTE*)(pCrtcTab + (i * 
LONG_S)));
      }
   }

   }

/**************************************************************************/
#else


void MoveToVideoBuffer(BYTE* pVidTab, BYTE* pCrtcTab, BYTE* pVideoBuffer)
{

/*** Valeurs du PCLK pour le RAMDAC ***/

typedef struct
 {
 DWORD  FbPitch;
 float Pclk_I;
 float Pclk_NI;
 } Pclk;
 

Pclk Pclk_Ramdac[] = {
     640,  12.27,  31.5,
     768,  14.75,  45.0, 
     800,  33.0 ,  50.0,
    1024,  45.0 ,  75.0, 
    1152,  62.0 , 106.0,
    1280,  80.0 , 135.0,
    1600,     0 , 200.0, {(DWORD) -1}
};


Pclk *p1;
float Sclk;
DWORD Srate;
BYTE LaserScl;




   /*** Move parameters from VidTab in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))       = *((BYTE*)(pVidTab + 
(18*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) = *((BYTE*)(pVidTab + 
(13*(26+LONG_S)) + 26));
   *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))     = *((DWORD*)(pVidTab + 
(0*(26+LONG_S)) + 26));
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinXOffset)) = *((WORD*)(pVidTab + 
(3*(26+LONG_S)) + 26));
   /* In interlace mode, we must double the Vertical back porch */
   *((WORD*)(pVideoBuffer + VIDEOBUF_DBWinYOffset)) =
      *((WORD*)(pVidTab + (13*(26+LONG_S)) + 26)) ?
	   *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) * 2 :  /* Interlace */
	   *((WORD*)(pVidTab + (8*(26+LONG_S)) + 26)) ;


   /*** Set some new parameters in the VideoBuffer ***/

   *((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay)) = *((BYTE*)(pCrtcTab + (31 * 
LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))   = *((BYTE*)(pVidTab + 
(28*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))   = *((BYTE*)(pVidTab + 
(27*(26+LONG_S)) + 26));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) = *((BYTE*)(pCrtcTab + (33 * 
LONG_S)));
   *((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal))   &= 0x80;   /**** FORCE 
!#@$!@#$!@#$ ***/

   *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor))   = 0x0000; /**** FORCE 
!#@$!@#$!@#$ ***/


   /*** srate and laserscl need to be set according to PCLK ***/
   /*******************************************************************/
   /*** PIXEL CLOCK : Programmation of registers SRATE and LASERSCL ***/
   /*** N.B. Values of SCLK and SPLCLK are in MHz ***/

   /* Find PCLK value in table Pclk_Ramdac[] */
   for (p1 = Pclk_Ramdac; p1->FbPitch != (DWORD)-1; p1++)
      {
      if (p1->FbPitch == Pitch)
	 {
	  if (*(BYTE*)(pVidTab + VIDEOBUF_Interlace))  /* if interlace */
	    {
	    switch (DacType)
	       {
	       case Info_Dac_BT482:
		  Sclk=p1->Pclk_I*(bpp/8);
		  break;
	       case Info_Dac_BT485:
	  case Info_Dac_ViewPoint:
	  case Info_Dac_TVP3026:
		  Sclk = p1->Pclk_I/(32/bpp);
		  break;
	       }
	    }
	  else
	    {
	    switch (DacType)
	       {
	       case Info_Dac_BT482:
		  Sclk = p1->Pclk_NI*(bpp/8);
		  break;
	       case Info_Dac_BT485:
	       case Info_Dac_ViewPoint:
	       case Info_Dac_TVP3026:
		  Sclk = p1->Pclk_NI/(32/bpp);
		  break;
	       }
	    }
	  break;
	 }
      }


   LaserScl = 0;
   Srate = (DWORD)(Sclk/1.638400);       /* Srate = SCLK / (200*32*256) */


   *((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))      = Srate;
   *((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))   = LaserScl;

   /*** move the CTRC parameters in the VideoBuffer ***/

   { WORD i;

   for (i = 0; i <= 28; i++)
      {
      *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + i)) = *((BYTE*)(pCrtcTab + (i * 
LONG_S)));
      }
   }

   }

#endif

/*/****************************************************************************
*          name: MGAVidInit
*
*   description: Initialize the VIDEO related hardware of the MGA device.
*
*      designed: Bart Simpson, february 11, 1993
* last modified: $Author$, $Date$
*
*       version: $Id$
*
*    parameters: BYTE* to Video buffer
*      modifies: MGA hardware
*         calls: GetMGAConfiguration
*       returns: -
*
*          ported by Dave Arnold for SCO        03/25/94
******************************************************************************/

void MGAVidInit(VOLATILE mgaRegsPtr pDevice, int ydstorg, vid *vidtab)
{
   DWORD DST0, DST1, Info, Value, ValClock;
   DWORD TmpDword, ByteCount, RegisterCount;
   BYTE TmpByte, DUB_SEL;
   BYTE *pVideoBuffer;
   DWORD width;
   BYTE videobuf[VIDEOBUF_S];
   BYTE titanpw;
   WORD res;
   DWORD TramDword;

#ifdef DEBUG_PRINT
  ErrorF("in MGAVidInit\n"); 
#endif
   pVideoBuffer = videobuf;
   bpp = vidtab[11].valeur;

   if(bpp == 8) titanpw = 0;
   else if(bpp == 16) titanpw = 1;
   else titanpw = 2;

   Pitch = vidtab[17].valeur;
   width = vidtab[5].valeur;

   atlas = pDevice->titan.rev & 0x7f;
  
#ifdef DEBUG_PRINT
ErrorF("atlas = %d\n",atlas);
#endif
   athena = atlas ==0x2;
#ifdef DEBUG_PRINT
ErrorF("athena = %d\n",athena);
#endif
   DubicPresent = !(pDevice->titan.config & TITAN_NODUBIC_M);

   /*** ##### DUBIC PATCH Disable mouse IRQ and proceed ###### ***/

   if (DubicPresent) /* Dubic Present */
      {
      pDevice->dubic.ndx_ptr = 0x8;
      DUB_SEL = pDevice->dubic.dub_sel;
      pDevice->dubic.dub_sel = 0;
      }

      vidtab[14].valeur = ydstorg; /* start address */  


   /*** ###################################################### ***/

   /*** Get System Configuration ***/

   GetMGAConfiguration(pDevice, &DST0, &DST1, &Info);

   DacType = Info;

/************************************************************************/
/*
* Name:           detectPatchWithGal
*
* synopsis:       Detect if the board has the patch that fixes the bug
*                 with SOE (See DAT on missing pixel in the bank switch).
*
*/
  
   if ( DacType != Info_Dac_TVP3026 ) 
    {
#ifdef DEBUG_PRINT
ErrorF("1\n");
#endif
  GALdetected = 0; /* GAL in use on TVP3026 only */
}
    else if ( DST1 & TITAN_DST1_ABOVE1280_M )
    {
#ifdef DEBUG_PRINT
ErrorF("2\n");
#endif

  GALdetected = 0; /* no gal on this board */
}
    else if ( getAthenaRevision(DST0, DST1) == 2)
      GALdetected = 0; /* No gal on rev 2 */
   else{
#ifdef DEBUG_PRINT
ErrorF("in part that checks for Tram\n");
#endif
    TramDword = pDevice->titan.opmode; 
    TramDword &= TITAN_TRAM_M;

    /** The GAL is present on Rev2 or higher board with bit TRAM at 0 and
        on board with bit TRAM at 1 (/P4) **/

    if( (((atlas & 0xf) >= 2) && !TramDword) || TramDword)
        GALdetected = 1;
    else
        GALdetected = 0;
   }
#ifdef DEBUG_PRINT
ErrorF("GAL %d\n",GALdetected);
#endif
/************************************************************************/

   /*** Program the Titan CRTC_CTRL register ***/

#if 0
   mgacalculCrtcParam(atlas, Info);
#else
   mgacalculCrtcParam(vidtab);
#endif

   MoveToVideoBuffer(vidtab, (BYTE *)crtcTab, videobuf);

   TmpDword = pDevice->titan.crtc_ctrl;

   TmpDword &= ~((DWORD)TITAN_CRTCBPP_M     |
		 (DWORD)TITAN_ALW_M         |
		 (DWORD)TITAN_INTERLACE_M   |
		 (DWORD)TITAN_VIDEODELAY0_M |
		 (DWORD)TITAN_VIDEODELAY1_M |
		 (DWORD)TITAN_VIDEODELAY2_M |
		 (DWORD)TITAN_VSCALE_M      |
		 (DWORD)TITAN_SYNCDEL_M);


   TmpDword |= ((DWORD)(titanpw) << TITAN_CRTCBPP_A) & (DWORD)TITAN_CRTCBPP_M;

   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_ALW))) << TITAN_ALW_A) 
& (DWORD)TITAN_ALW_M;

   if (*(BYTE*)(pVideoBuffer + VIDEOBUF_Interlace))
      {
      if (width <= 768)
	 {
	 TmpDword |= (DWORD)TITAN_INTERLACE_768;
	 }
      else
	 {
	 if ( width  <= 1024)
	    {
	    TmpDword |= (DWORD)TITAN_INTERLACE_1024;
	    }
	 else
	    {
	    TmpDword |= (DWORD)TITAN_INTERLACE_1280;
	    }
	 }
      }

   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << 
TITAN_VIDEODELAY0_A) & (DWORD)TITAN_VIDEODELAY0_M;
   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << 
(TITAN_VIDEODELAY1_A - 1)) & (DWORD)TITAN_VIDEODELAY1_M;
   TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VideoDelay))) << 
(TITAN_VIDEODELAY2_A - 2)) & (DWORD)TITAN_VIDEODELAY2_M;


/* Programing Atlas VSCALE and SYNCDEL if NO DUBIC */

   if ( !DubicPresent ) /* No Dubic */
      {

/*** Set synch polarity for athena chipset ***/
/* MISC REGISTER 
      bit 7: 0 - Vsync positive
	     1 - Vsync negative
      bit 6: 0 - Hsync positive
	     1 - Hsync negative
*/
      if(athena)
	 {
         TmpByte = pDevice->titan.vga.misc_outr;
	 TmpByte &= 0x3f;   /* Set bit <7:6> to 0 */
	 TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol) << 6;
	 TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol) << 7;

	 if (((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_TVP3026) ||
	     ((Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_ViewPoint) 
	    )
	    TmpByte |= 0xc0;   /* force <7:6> to negative polarity and
				  use DAC support  for synch polarity */
	 else
	    TmpByte ^= 0xc0;   /* reverse bit <7:6> */
	 pDevice->titan.vga.misc_outw = TmpByte;
	 }

	 switch (Info & (DWORD)Info_Dac_M)
	    {
	    case (DWORD)Info_Dac_BT484:
	    case (DWORD)Info_Dac_BT485:
	       switch ( *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) )
		  {
		  default:
		  case 0:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 3:
		     TmpDword |= (((DWORD)1 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 5:
		     TmpDword |= (((DWORD)2 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 8:
		     TmpDword |= (((DWORD)3 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;
		  }

	       switch (titanpw)
		  {
		  case TITAN_CRTCBPP_8:
		     TmpDword |= (DWORD)0x01 << TITAN_VSCALE_A;
		     break;               /* 0x1 = clock divide by 2   */  

		  case TITAN_CRTCBPP_16:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     TmpDword |= (DWORD)0x02 << TITAN_VSCALE_A;
		     break;

		  case TITAN_CRTCBPP_32:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     TmpDword |= (DWORD)0x03 << TITAN_VSCALE_A; 
		     break;

		  }
	       break;


	    case (DWORD)Info_Dac_ViewPoint:     
	    case (DWORD)Info_Dac_TVP3026:
		switch ( *((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay)) )
		  {
		  case 0:
		  case 1:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 2:
		  case 3:
		     TmpDword |= (((DWORD)1 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 4:
		  case 5:
		  case 6:
		     TmpDword |= (((DWORD)2 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;

		  case 7:
		  case 8:
		  default:
		     TmpDword |= (((DWORD)3 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;
		  }
 
	       switch ( titanpw )
		  {                               /* for PRO board (2 DUBIC) */
		  case TITAN_CRTCBPP_8:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     break;   /* VSCALE = 0 */

		  case TITAN_CRTCBPP_16:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     TmpDword |= (DWORD)0x01 << TITAN_VSCALE_A;
		     break;              

		  case TITAN_CRTCBPP_32:
		     TmpDword |= (((DWORD)0 << TITAN_SYNCDEL_A) & 
(DWORD)TITAN_SYNCDEL_M);
		     TmpDword |= (DWORD)0x02 << TITAN_VSCALE_A;
		     break;
		  }
	       break;
	    }

	pDevice->titan.crtc_ctrl = TmpDword;
      }
   else /* ELSE NO DUBIC programming DUBIC */
      {
	pDevice->titan.crtc_ctrl = TmpDword;

      /*** Program the Dubic DUB_CTL register ***/

	pDevice->dubic.ndx_ptr = DUBIC_DUB_CTL;

      for (TmpDword = 0, ByteCount = 0; ByteCount <= 3; ByteCount++)
	 {
	 TmpByte = pDevice->dubic.data;
	 TmpDword |= ((DWORD)TmpByte << ((DWORD)8 * ByteCount));
	 }

      /** Do not program the LVID field, KEEP it for EXTERNAL APPLICATION
      *** if bit 7 of LvidInitFlag is 1
      **/
      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_LvidInitFlag)) & 0x80)
	 {
	 TmpDword &= ~((DWORD)DUBIC_IMOD_M | (DWORD)DUBIC_VSYNC_POL_M | 
(DWORD)DUBIC_HSYNC_POL_M |
		      (DWORD)DUBIC_DACTYPE_M | (DWORD)DUBIC_INT_EN_M | 
(DWORD)DUBIC_GENLOCK_M |
		      (DWORD)DUBIC_BLANK_SEL_M | (DWORD)DUBIC_SYNC_DEL_M | 
(DWORD)DUBIC_SRATE_M |
		      (DWORD)DUBIC_FBM_M | (DWORD)DUBIC_VGA_EN_M | 
(DWORD)DUBIC_BLANKDEL_M |
		      (DWORD)DUBIC_DB_EN_M);
	 }
      else  /*** disable live video ***/
	 {
	 TmpDword &= ~((DWORD)DUBIC_IMOD_M | (DWORD)DUBIC_VSYNC_POL_M | 
(DWORD)DUBIC_HSYNC_POL_M |
		      (DWORD)DUBIC_DACTYPE_M | (DWORD)DUBIC_INT_EN_M | 
(DWORD)DUBIC_GENLOCK_M |
		      (DWORD)DUBIC_BLANK_SEL_M | (DWORD)DUBIC_SYNC_DEL_M | 
(DWORD)DUBIC_SRATE_M |
		      (DWORD)DUBIC_FBM_M | (DWORD)DUBIC_VGA_EN_M | 
(DWORD)DUBIC_BLANKDEL_M |
		      (DWORD)DUBIC_DB_EN_M | (DWORD)DUBIC_LVID_M);
	 }

      TmpDword |= ((~(titanpw+(DWORD)1)) << DUBIC_IMOD_A) & (DWORD)DUBIC_IMOD_M;
      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol))) << 
DUBIC_VSYNC_POL_A) & (DWORD)DUBIC_VSYNC_POL_M;
      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol))) << 
DUBIC_HSYNC_POL_A) & (DWORD)DUBIC_HSYNC_POL_M;

      TmpDword |= (Info << DUBIC_DACTYPE_A) & (DWORD)DUBIC_DACTYPE_M;

      if (*(BYTE*)(pVideoBuffer + VIDEOBUF_Interlace))
	 {
	 TmpDword |= (DWORD)DUBIC_INT_EN_M;
	 }

      /*** GenLock forced to 0 ***/

      /*** vga_en forced to 0 ***/

      /*** BlankSel forced to 0 (only one DAC) ***/

      /*** Blankdel force a 0 pour mode TERMINATOR ***/

      switch (Info & (DWORD)Info_Dac_M)    /*** IMOD need to be set in TmpDword 
(DUB_CTL) ***/
	 {
	 case (DWORD)Info_Dac_ATT:

	    switch (TmpDword & (DWORD)DUBIC_IMOD_M)
	       {
	       case (DWORD)DUBIC_IMOD_32:
		  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_16:
		  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_8:
		  TmpDword |= (((DWORD)4 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;
	       }
	    break;

	 case (DWORD)Info_Dac_BT481:
	 case (DWORD)Info_Dac_BT482:

	    switch (TmpDword & (DWORD)DUBIC_IMOD_M)
	       {
	       case (DWORD)DUBIC_IMOD_32:
		  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_16:
		  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_8:
		  TmpDword |= (((DWORD)7 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;
	       }
	    break;

	 case (DWORD)Info_Dac_Sierra:

	    switch (TmpDword & (DWORD)DUBIC_IMOD_M)
	       {
	       case (DWORD)DUBIC_IMOD_32:
		  TmpDword |= (((DWORD)1 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_16:
		  TmpDword |= (((DWORD)12 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;

	       case (DWORD)DUBIC_IMOD_8:
		  TmpDword |= (((DWORD)8 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		  break;
	       }
	    break;


	 case (DWORD)Info_Dac_BT484:
	 case (DWORD)Info_Dac_BT485:

/*** BEN note: Definir cette facon de faire pour les autres RAMDAC lorsque
	       ce sera calcule dans vidfile.c ... ***/
	    TmpDword |= ( ((DWORD)(*((BYTE*)(pVideoBuffer + 
VIDEOBUF_HsyncDelay))) << DUBIC_SYNC_DEL_A) & (DWORD)DUBIC_SYNC_DEL_M);
	    break;


	 case (DWORD)Info_Dac_Chameleon:      /*** UNKNOWN ***/
	    break;

	 case (DWORD)Info_Dac_ViewPoint:     
	    TmpDword |= (((DWORD)0 << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
	    TmpDword |= (DWORD)DUBIC_BLANKDEL_M;
	    break;
	 case (DWORD)Info_Dac_TVP3026:
	    TmpDword |= (((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_HsyncDelay))) << DUBIC_SYNC_DEL_A) & 
(DWORD)DUBIC_SYNC_DEL_M);
		    TmpDword |= (DWORD)DUBIC_BLANKDEL_M;

       break;

	 }


      /* if(atlas) /* use only fbm 2 for now */
	TmpDword |= (2 << DUBIC_FBM_A) & (DWORD)DUBIC_FBM_M;
#if 0
      else
	TmpDword |= (6 << DUBIC_FBM_A) & (DWORD)DUBIC_FBM_M;
#endif

      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_Srate))) << 
DUBIC_SRATE_A) & (DWORD)DUBIC_SRATE_M;

      pDevice->dubic.ndx_ptr, DUBIC_DUB_CTL;

      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
	 {
	 pDevice->dubic.data = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
	 }

      /*** Program the Dubic DUB_CTL2 register ***/

      pDevice->dubic.ndx_ptr, DUBIC_DUB_CTL2;

      for (TmpDword = 0, ByteCount = 0; ByteCount <= 3; ByteCount++)
	 {
	 TmpByte = pDevice->dubic.data;
	 TmpDword |= ((DWORD)TmpByte << ((DWORD)8 * ByteCount));
	 }


/*** !!!??? This comment is not valid since we set LVIDFIELD just after that 
***/
   /** Do not program the LVIDFIELD field, KEEP it for EXTERNAL APPLICATION
   *** if bit 7 of LvidInitFlag is 1
   **/

      if (*((BYTE*)(pVideoBuffer + VIDEOBUF_LvidInitFlag)) & 0x80)
	 {
	 TmpDword &= ~((DWORD)DUBIC_SYNCEN_M | (DWORD)DUBIC_LASERSCL_M |
		       (DWORD)DUBIC_CLKSEL_M | (DWORD)DUBIC_LDCLKEN_M);
	 }
      else   /*** Disable live video ***/
	 {
	 TmpDword &= ~((DWORD)DUBIC_SYNCEN_M | (DWORD)DUBIC_LASERSCL_M | 
(DWORD)DUBIC_LVIDFIELD_M |
		       (DWORD)DUBIC_CLKSEL_M | (DWORD)DUBIC_LDCLKEN_M);
	 }


   /** Because we lost the cursor in interlace mode , we have to program **/
   /** LVIDFIELD to 1                                                    **/

      if (*(BYTE*)(pVideoBuffer + VIDEOBUF_Interlace))
	 TmpDword |= (DWORD)DUBIC_LVIDFIELD_M;
      else
	 TmpDword &= ~((DWORD)DUBIC_LVIDFIELD_M);



      TmpDword |= ((DWORD)(*((BYTE*)(pVideoBuffer + VIDEOBUF_LaserScl))) << 
DUBIC_LASERSCL_A) & (DWORD)DUBIC_LASERSCL_M;

      TmpDword |= (DWORD)DUBIC_SYNCEN_M;        /*** sync forced to enable ***/

      /*** CLOCKSEL forced to 0 except for viewpoint ***/

      if ( (Info & (DWORD)Info_Dac_M) == Info_Dac_ViewPoint)
	 TmpDword |= (DWORD)DUBIC_CLKSEL_M;       

      /* Special case for BT485: LDCLKEN must be set to 1 only in VGA mode */
      /* and in 24-bit mode */
      if( (Info & (DWORD)Info_Dac_M) == (DWORD)Info_Dac_BT485 )
	 {
	 if( titanpw == (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A) )
	    TmpDword |= (DWORD)DUBIC_LDCLKEN_M;     /*** ldclkl running ***/
	 }
      else
	 TmpDword |= (DWORD)DUBIC_LDCLKEN_M;       /*** ldclkl running ***/


      pDevice->dubic.ndx_ptr = DUBIC_DUB_CTL2;

      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
	 {
	 pDevice->dubic.data = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
	 }

      /*** Program the Dubic OVS_COL register ***/

      pDevice->dubic.ndx_ptr = DUBIC_OVS_COL;

      TmpDword = *((DWORD*)(pVideoBuffer + VIDEOBUF_OvsColor));
   
      for (ByteCount = 0; ByteCount <= 3; ByteCount++)
	 {
	 pDevice->dubic.data = (BYTE)(TmpDword >> ((DWORD)8 * ByteCount));
	 }

      }

   /*** Program the CRTC ***/

   /*** Select access on 0x3d4 and 0x3d5 ***/

   TmpByte = pDevice->titan.vga.misc_outr;
   pDevice->titan.vga.misc_outw = TmpByte | 1;

   /*** Unprotect registers 0-7 ***/

   pDevice->titan.vga.crtc_addr6 = 0x11;
   pDevice->titan.vga.crtc_data6 = 0x40;

   for (RegisterCount = 0; RegisterCount <= 24; RegisterCount++)
      {
      TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

      pDevice->titan.vga.crtc_addr6 = RegisterCount;
      pDevice->titan.vga.crtc_data6 = TmpByte;
      }

      /*** Get extended address register ***/

      RegisterCount = (DWORD)26;
      TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

      pDevice->titan.vga.aux_addr = 0xA;
      pDevice->titan.vga.aux_data = TmpByte;

      /*** Get interlace support register ***/

      RegisterCount = (DWORD)28;
      TmpByte = *((BYTE*)(pVideoBuffer + VIDEOBUF_CRTC + RegisterCount));

      pDevice->titan.vga.aux_addr = 0xD;
      pDevice->titan.vga.aux_data = TmpByte;

   /*** Program the RAMDAC ***/

   switch (Info & (DWORD)Info_Dac_M)
      {
      case (DWORD)Info_Dac_ATT:            /*** UNKNOWN ***/
	 break;

      case (DWORD)Info_Dac_BT481:          /*** UNKNOWN ***/
	 break;

      case (DWORD)Info_Dac_BT482:

	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       TmpByte = 0x01;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       TmpByte = 0xa1;
	       break;
	    }

	 pDevice->ramdacs.bt482.cmd_rega = TmpByte;

	 /*** Read Mask (INDIRECT) ***/

	 pDevice->ramdacs.bt482.wadr_pal = 0;
	 pDevice->ramdacs.bt482.pix_rd_msk = 0xff;

	 /*** Command Register B (INDIRECT) ***/

	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) 
<< 5) | (BYTE)0x02;

	 pDevice->ramdacs.bt482.wadr_pal = 2;
	 pDevice->ramdacs.bt482.pix_rd_msk = TmpByte;

	 /*** Cursor Register (INDIRECT) ***/

	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & (BYTE)0x1) 
<< 4) | (BYTE)0x04;

	 pDevice->ramdacs.bt482.wadr_pal = 3;
	 pDevice->ramdacs.bt482.pix_rd_msk = TmpByte;

	 break;

      case (DWORD)Info_Dac_Sierra:         /*** UNKNOWN ***/
	 break;

      case (DWORD)Info_Dac_BT484:

	 pDevice->ramdacs.bt484.pix_rd_msk = 0xff;

	 /* Overlay must me 0 */

	 pDevice->ramdacs.bt484.wadr_ovl = 0;
	 pDevice->ramdacs.bt484.col_ovl = 0;
	 pDevice->ramdacs.bt484.col_ovl = 0;
	 pDevice->ramdacs.bt484.col_ovl = 0;
										 
    /** BEN 0x1e */
	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) &
		    (BYTE)0x1) << 5) | (BYTE)0x02;

	 pDevice->ramdacs.bt484.cmd_reg0 = TmpByte;

	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       TmpByte = 0x40;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       TmpByte = 0x20;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       TmpByte = 0x00;
	       break;
	    }

	 pDevice->ramdacs.bt484.cmd_reg1 = TmpByte;
										      
	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) &
		    (BYTE)0x1) << 3) | (BYTE)0x24;
	 pDevice->ramdacs.bt484.cmd_reg2 = TmpByte;

	 break;

      case (DWORD)Info_Dac_BT485:

	 pDevice->ramdacs.bt485.pix_rd_msk = 0xff;
										 
    /** BEN 0x9e */
	 /* OverScan must me 0 */

	 pDevice->ramdacs.bt485.wadr_ovl = 0;
	 pDevice->ramdacs.bt485.col_ovl = 0;
	 pDevice->ramdacs.bt485.col_ovl = 0;
	 pDevice->ramdacs.bt485.col_ovl = 0;

	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) &
		    (BYTE)0x1) << 5) | (BYTE)0x82;

	 pDevice->ramdacs.bt485.cmd_reg0 = TmpByte;

	 switch ( titanpw )
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       TmpByte = 0x40;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       TmpByte = 0x20;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       TmpByte = 0x00;
	       break;
	    }

	 pDevice->ramdacs.bt485.cmd_reg1 = TmpByte;

	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) &
		    (BYTE)0x1) << 3) | (BYTE)0x24;

	 pDevice->ramdacs.bt485.cmd_reg2 = TmpByte;

	 /*** Indirect addressing from STATUS REGISTER ***/

	 pDevice->ramdacs.bt485.wadr_pal = 1;

	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.bt485.cmd_reg3 = 0x8;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.bt485.cmd_reg3 = 0x8;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.bt485.cmd_reg3 = 0x0;
	       break;
	    }
	 break;

      case (DWORD)Info_Dac_Chameleon:      /*** UNKNOWN ***/
	 break;

      case (DWORD)Info_Dac_ViewPoint:

	 /* Software reset to put the DAC in a default state */
	 pDevice->ramdacs.vpoint.index = VPOINT_RESET;
	 pDevice->ramdacs.vpoint.data = 0;


	 pDevice->ramdacs.vpoint.pix_rd_msk = 0xff;

	 /* OverScan must me 0 */
	 pDevice->ramdacs.vpoint.index = VPOINT_OVS_RED;
	 pDevice->ramdacs.vpoint.data = 0;
	 pDevice->ramdacs.vpoint.index = VPOINT_OVS_GREEN;
	 pDevice->ramdacs.vpoint.data = 0;
	 pDevice->ramdacs.vpoint.index = VPOINT_OVS_BLUE;
	 pDevice->ramdacs.vpoint.data = 0;

	 /* Misc. Control Register */
	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) &
		    (BYTE)0x1) << 4);
	 pDevice->ramdacs.vpoint.index = VPOINT_GEN_CTL;
	 pDevice->ramdacs.vpoint.data = TmpByte;

	 /* Multiplex Control Register (True Color 24 bit) */
	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL1;
	       pDevice->ramdacs.vpoint.data = 0x80;
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL2;
	       pDevice->ramdacs.vpoint.data = 0x1c;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL1;
	       pDevice->ramdacs.vpoint.data = 0x44;
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL2;
	       pDevice->ramdacs.vpoint.data = 0x04;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL1;
	       pDevice->ramdacs.vpoint.data = 0x46;
	       pDevice->ramdacs.vpoint.index = VPOINT_MUX_CTL2;
	       pDevice->ramdacs.vpoint.data = 0x04;
	       break;
	    }


	 /* Input Clock Select Register (Select CLK1 as doubled clock source */

	 pDevice->ramdacs.vpoint.index = VPOINT_INPUT_CLK;
	 pDevice->ramdacs.vpoint.data = 0x11;

	 /* Output Clock Selection Register Bits
	    VCLK/2 output ratio (xx001xxx)
	    RCLK/2 output ratio (xxxxx001)
	 */

	 pDevice->ramdacs.vpoint.index = VPOINT_OUTPUT_CLK;
	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.data = 0x5b;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.data = 0x52;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.vpoint.data = 0x49;
	       break;
	    }

	 if ( !DubicPresent )
	    {
	    pDevice->ramdacs.vpoint.index = VPOINT_GEN_IO_CTL;
	    pDevice->ramdacs.vpoint.data = 0x03;

	    pDevice->ramdacs.vpoint.index = VPOINT_GEN_IO_DATA;
	    pDevice->ramdacs.vpoint.data = 0x01;

	    pDevice->ramdacs.vpoint.index = VPOINT_AUX_CTL;
	    
	    switch (titanpw)
	       {
	       case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
		  pDevice->ramdacs.vpoint.data = 0x09;
		  break;

	       case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
		  pDevice->ramdacs.vpoint.data = 0x08;
		  break;

	       case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
		  pDevice->ramdacs.vpoint.data = 0x08;
		  break;
	       }
	    }


	 pDevice->ramdacs.vpoint.index = VPOINT_KEY_CTL;
	 pDevice->ramdacs.vpoint.data = 0;

	 break;

     case (DWORD)Info_Dac_TVP3026:

	 /* Software reset to put the DAC in a default state */
	 res = width;

	 pDevice->ramdacs.tvp3026.pix_rd_msk = 0xff;

	 /* FIX AVEC GAL DAT No GEN014 */
	 /* general purpose I/O tell to the gal the vertical resolution */
	 /* if vertical resolution is >= 1024, the count is by increment 
	    of two */
	    
	 if ( res < 1280)
	    {
	    pDevice->ramdacs.tvp3026.index = TVP3026_GEN_IO_CTL;
	    pDevice->ramdacs.tvp3026.data = (BYTE)0x00;
	    pDevice->ramdacs.tvp3026.index = TVP3026_GEN_IO_DATA;
	    pDevice->ramdacs.tvp3026.data = (BYTE)0x01;
	    }
	 else
	    {
	    pDevice->ramdacs.tvp3026.index = TVP3026_GEN_IO_CTL;
	    pDevice->ramdacs.tvp3026.data = (BYTE)0x01;
	    pDevice->ramdacs.tvp3026.index = TVP3026_GEN_IO_DATA;
	    pDevice->ramdacs.tvp3026.data = (BYTE)0x00;
	    }

	  pDevice->ramdacs.tvp3026.index = TVP3026_MISC_CTL;
	  pDevice->ramdacs.tvp3026.data = (BYTE)0x0c;

	 /* init Interlace Cursor support */
	 /* NOTE: We set the vertival detect method bit to 1 to be in synch
		  with NPI diag code. Whith some video parameters, the cursor
		  disaper if we put this bit a 0.
	 */
	 pDevice->ramdacs.tvp3026.index = TVP3026_CURSOR_CTL;
	 TmpByte = pDevice->ramdacs.tvp3026.data;
	 /* Set interlace bit */
	 TmpByte &= ~(BYTE)(1 << 5);   
	 TmpByte |= (((*((BYTE*)(pVideoBuffer + VIDEOBUF_Interlace)) & 
(BYTE)0x1)) << 5);
	 /* Set vertival detect method */
	 TmpByte |= 0x10;
	 pDevice->ramdacs.tvp3026.data = TmpByte;

	 /* OverScan must me 0 */
	 /* Overscan is not enabled in general ctl register */
	 pDevice->ramdacs.tvp3026.cur_col_addr = 00;
	 pDevice->ramdacs.tvp3026.cur_col_data = 00;
	 pDevice->ramdacs.tvp3026.cur_col_data = 00;
	 pDevice->ramdacs.tvp3026.cur_col_data = 00;
	 /* Put to ZERO */

	 /* Misc. Control Register */
	 TmpByte = ((*((BYTE*)(pVideoBuffer + VIDEOBUF_Pedestal)) & (BYTE)0x1) 
<< 4);

	 /* For the TVP3026, we use DAC capability for sync polarity */
	 if (! DubicPresent)
	    {
	    TmpByte &= 0xfc;   /* Set bit 0,1 to 0 */
	    TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_HsyncPol);
	    TmpByte |= *(BYTE*)(pVideoBuffer + VIDEOBUF_VsyncPol) << 1;
	    }

	    pDevice->ramdacs.tvp3026.index = TVP3026_GEN_CTL;
	    pDevice->ramdacs.tvp3026.data = TmpByte;

	 /* Multiplex Control Register (True Color 24 bit) */
	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.tvp3026.index = TVP3026_CLK_SEL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0xb5;
	       pDevice->ramdacs.tvp3026.index = TVP3026_TRUE_COLOR_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0xa0;
	       pDevice->ramdacs.tvp3026.index = TVP3026_MUX_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x4c;
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.tvp3026.index = TVP3026_CLK_SEL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0xa5;
	       pDevice->ramdacs.tvp3026.index = TVP3026_TRUE_COLOR_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x64;
 
	       pDevice->ramdacs.tvp3026.index = TVP3026_MUX_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x54;
	       break;

	 case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       pDevice->ramdacs.tvp3026.index = TVP3026_CLK_SEL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x95;
	       pDevice->ramdacs.tvp3026.index = TVP3026_TRUE_COLOR_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x66;
	       pDevice->ramdacs.tvp3026.index = TVP3026_MUX_CTL;
	       pDevice->ramdacs.tvp3026.data = (BYTE)0x5c;

	       break;
	    }

       break;

      }

   /*** Programme le CLOCK GENERATOR ***/

   switch (Info & (DWORD)Info_Dac_M)
      {
      case (DWORD)Info_Dac_BT482:

	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)),
			    2);
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))
				     << 1, 2);
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK))
				     << 1, 2);
	       break;
	    }

	 break;



      case (DWORD)Info_Dac_BT485:

	 switch (titanpw)
	    {
	    case (BYTE)(TITAN_PWIDTH_PW8 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) 
>> 1, 2);
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW16 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) 
>> 1, 2);
	       break;

	    case (BYTE)(TITAN_PWIDTH_PW32 >> TITAN_PWIDTH_A):
	       setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 
2);
	       break;
	    }

	 break;

      case (DWORD)Info_Dac_ViewPoint:
	 setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)) >> 1, 
2);
	 break;

      case (DWORD)Info_Dac_TVP3026:
#ifdef DEBUG_PRINT
ErrorF("will now program the TVP freq\n");
#endif
         Value = (DST1 & TITAN_DST1_RAMSPEED_M) >> TITAN_DST1_RAMSPEED_A;
          switch(Value)
            {
                case 0:
                        ValClock = 0x065AC3D;  /* 50MHz */
                        break;
                case 1:
                        ValClock = 0x068A413;  /* 60MHz */
                        break;
                case 3:
                        ValClock = 0x067D83D;  /* 55MHz */
                        break;
                default:
                        break;
            }

    /* Lookup for find real frequency to progrmming */
    switch( ValClock )
    {
        case 0x063AC44: Value = 45000000L; /* 45Mhz */
                        break;

        case 0x065AC3D: Value = 50000000L; /* 50Mhz */
                        break;

        case 0x067D83D: Value = 55000000L; /* 55Mhz */
                        break;

        case 0x068A413: Value = 60000000L; /* 60Mhz */
                        break;

        case 0x06d4423: Value = 65000000L; /* 65Mhz */
                        break;

        case 0x06ea410: Value = 70000000L; /* 70Mhz */
                        break;

        case 0x06ed013: Value = 75000000L; /* 75Mhz */
                        break;

        case 0x06f7020: Value = 80000000L; /* 80Mhz */
                        break;

        case 0x071701e: Value = 85000000L; /* 85Mhz */
                        break;

        case 0x074fc13: Value = 90000000L; /* 90Mhz */
                        break;

        default:        Value = 50000000L; /* 50Mhz */
      }
	 mgasetTVP3026Freq(pDevice, Value/1000L, 3,0);

#ifdef DEBUG_PRINT
ErrorF("finished programming the TVP freq\n");
#endif

	 mgasetTVP3026Freq(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)),
		2,bpp);
	 break;

      default:

	 setFrequence(pDevice, *((DWORD*)(pVideoBuffer + VIDEOBUF_PCLK)), 2);
	 break;
      }

   /*** ##### DUBIC PATCH ReEnable mouse IRQ ################# ***/
   if ( DubicPresent ) /* Dubic Present */
      {
      pDevice->dubic.ndx_ptr = 0x8;
      pDevice->dubic.dub_sel = DUB_SEL;
      }

   /*** ###################################################### ***/
#ifdef DEBUG_PRINT
  ErrorF("out of MGAVidInit\n"); 
#endif
 }

