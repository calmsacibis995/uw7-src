
/*
 * @(#) mgaClock.c 11.1 97/10/22
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
 *	S000	Thu Jun  1 16:52:52 PDT 1995	brianm@sco.com
 *	- New code from Matrox.
 */

/* taken from "src/caddi/init/clock.c" writen by Matrox, ported by D. Arnold */

#include "scrnintstr.h"

#include "mgaDefs.h"
#include "mgaScrStr.h"

#ifdef usl
#include <sys/types.h>
#include <sys/inline.h>
#else
#include "compiler.h"
#endif

#include <sys/time.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define NO_FLOAT 1

void mgaDelay2(dword delai)
{
    clock_t start;
    delai = delai / 1000;   /* Convert in millisecond */
    delai = delai / 55;     /* delai become now a number of tick */
    if (delai == 0)
      delai = 1;
    start = clock();
    while ((clock() - start) < delai);
}

static VOLATILE mgaRegsPtr pDevice;

/* the first part is inlined h files */
/* 
/* ======================================================================= */
/*
/* Nom du fichier       : ICD2061.H
/* Creation             : Dominique Leblanc 92/09/23
/*
/*
/* Ce fichier contient les defines pour l'ICD2061 sur MGALP.
/* Structure, liste des registres...
/*
/* Liste des modifications:
/*
/* ======================================================================= */


#ifndef __icd2061_h__

#  define __icd2061_h__


   /**************** LISTE DES REGISTRES DU ICD2061 ************************/


#  define NBRE_REG_ICD2061    6

#  define VIDEO_CLOCK_1       0
#  define VIDEO_CLOCK_2       1
#  define VIDEO_CLOCK_3       2
#  define MEMORY_CLOCK        3
#  define POWERDWN_REG        4
#  define CONTROL_ICD2061     6



   typedef union
   {
      byte var8;
      byte octet;
      byte byte;

      struct
      {
	 word reserved         : 1;
	 word duty_cycle       : 1;
	 word reserved2        : 1;
	 word timeout          : 1;   
	 word muxref           : 1;
	 word pwrdwn_mode      : 1;
	 word unused           : 2;

      } bit;

   } ST_CONTROL_ICD2061;


#  define NBRE_CHAMP_REG_CTL_2061      6



   typedef union
   {
      dword var32;
      dword dmot;
      dword ulong;
      

      struct
      {
	 dword q_prime       :  7;
	 dword m_diviseur    :  3;
	 dword p_prime       :  7;
	 dword index_field   :  4;
	 dword unused        : 11;

      } bit;

   } ST_REG_PROGRAM_DATA;





   /**************************************************************
    * parametres par defaut pour avoir une frequence sortie de
    * 1,864,466 Hz, et le double (300 DPI et 600 DPI).
    */

#  define Q_DEFAULT       24
#  define P_DEFAULT       50
#  define M_DEFAULT_300    5    /* devise par 32 */
#  define I_DEFAULT        4

#  define M_DEFAULT_600    4


#  define _300DPI          300
#  define _600DPI          600



#endif         /* __mgalp_h__ */

/**** FIN inclusion ICD2061.h ****/

/**************************** liste des routines de ce fichier *************/

long mgasetTVP3026Freq ( mgaRegsPtr pDeviceParam, long fout, long reg,
	byte pWidth);

long setFrequence (mgaRegsPtr pDeviceParam, long fout_voulue, long reg );

dword programme_clock ( short reg, short p, short q, short m, short i );

void programme_reg_icd (mgaRegsPtr pDeviceParam, short reg, dword data );

static void send_unlock ( void );
static void send_data ( short reg, dword data );
static void send_full_clock ( void );
static void send_start ( void );
static void send_0 ( void );
static void send_1 ( void );
static void send_stop ( void );






#define NBRE_M_POSSIBLE    7
#define NBRE_I_POSSIBLE    15

#ifdef NO_FLOAT
   #define ABS_D(x)  ((x < 0) ? (-1*x) : (x) )
#else
   #define ABS_D(x)  ((x < 0.0) ? (-1.0*x) : (x) )
#endif





typedef union
{
   byte var8;
   byte byte;
   byte octet;

   struct
   {
      byte enable_color   : 1;  /* 1=COLOR:0x3D?, 0=MONO:0x3B? */
      byte enable_RAM     : 1;
      byte cs0            : 1;
      byte cs1            : 1;
      byte reserved       : 1;
      byte page_select    : 1;
      byte h_pol          : 1;
      byte v_pol          : 1;

   } bit;

   struct
   {
      byte unused         : 2;
      byte pgmclk         : 1;
      byte pgmdata        : 1;
      byte reserved       : 4;

   } clock_bit;

   struct
   {
      byte unused         : 2;
      byte select         : 2;
      byte reserved       : 4;

   } sel_reg_output;

} ST_MISC_OUTPUT;


#  define MISC_OUTPUT_WRITE   0x03C2
#  define MISC_OUTPUT_READ    0x03CC


static ST_MISC_OUTPUT        misc;


#ifdef NO_FLOAT
   typedef struct
   {
      short p;
      short q;
      short m;

      short i;

      long fout_vrai;
      long erreur_ppm;

   } ST_RESULTAT;

static long fref = 14318180;                    /* frequence XTAL */

#else

   typedef struct
   {
      short p;
      short q;
      short m;

      short i;

      double fout_vrai;
      double erreur_ppm;

   } ST_RESULTAT;

static double fref = 14318180.0;                  /* frequence XTAL */
#endif





/* default theorical power up value */
/* with lines = 00                  */

static ST_REG_PROGRAM_DATA reg_clock[4] = { { 0x5A8BCL }, /* REG0 video 1 */
					  { 0x960ACL }, /* REG1 video 2 */
					  { 0x960ACL }, /* REG2 video 3 */
					  { 0xD44A3L }, /* MREG memory  */
					  };

static ST_CONTROL_ICD2061 ctl_icd2061 = { 2 };            /* reg CONTROLE */

static byte power_down_reg = 8;                          /* reg POWER DOWN */

#ifdef NO_FLOAT
   static long diviseur_m [ NBRE_M_POSSIBLE ] =
   {
      1,       /* 0 */
      2,       /* 1 */
      4,       /* 2 */
      8,       /* 3 */
      16,      /* 4 */
      32,      /* 5 */
      64,      /* 6 */
      /* 64,   /* 7 */ /* on utilise pas ce cas dans nos calculs */
   };


   static long limites_i [ NBRE_I_POSSIBLE ] =

   {/* min,       max,    I */
	 0L, /* 47500000     0  */
   40000L, /* 47500000     1  */
   47500L, /* 52200000     2  */
   52200L, /* 56300000     3  */
   56300L, /* 61900000     4  */
   61900L, /* 65000000     5  */
   65000L, /* 68100000     6  */
   68100L, /* 82300000     7  */
   82300L, /* 86000000     8  */
   86000L, /* 88000000     9  */
   88000L, /* 90500000    10  */
   90500L, /* 95000000    11  */
   95000L, /* 100000000   12  */
   100000L, /* 120000000   13  */
   120000L, /*             14  */
   };

#else

   static double diviseur_m [ NBRE_M_POSSIBLE ] =
   {
      1.00,       /* 0 */
      2.00,       /* 1 */
      4.00,       /* 2 */
      8.00,       /* 3 */
      16.00,      /* 4 */
      32.00,      /* 5 */
      64.00,      /* 6 */
      /* 64.00,   /* 7 */ /* on utilise pas ce cas dans nos calculs */
   };



   static long limites_i [ NBRE_I_POSSIBLE ] =

   {/* min,       max,    I */
	    0L, /* 47500000     0  */
   40000000L, /* 47500000     1  */
   47500000L, /* 52200000     2  */
   52200000L, /* 56300000     3  */
   56300000L, /* 61900000     4  */
   61900000L, /* 65000000     5  */
   65000000L, /* 68100000     6  */
   68100000L, /* 82300000     7  */
   82300000L, /* 86000000     8  */
   86000000L, /* 88000000     9  */
   88000000L, /* 90500000    10  */
   90500000L, /* 95000000    11  */
   95000000L, /*  1.0E+08    12  */
   100000000L, /*  1.2E+08    13  */
   120000000L, /*             14  */
   };

#endif




/**************************** liste des routines externes a ce fichier *****/



#ifdef NO_FLOAT


/***************************************************************************/
/*/ setFrequence ( long fout_voulue, long registre )
 *
 * Permet de determiner les meilleures valeurs a mettre pour la
 * programmation du ICD2061, pour une frequence donnee.
 * Programme le registre.
 * Si la frequence de sortie est 0, ne fait que choisir le registre
 * de sortie.
 *
 * Problemes :
 * Concu     : Dominique Leblanc:92/10/29
 *
 * Parametres: [0]-frequence output.
 *             [1]-registre (0, 1, 2 ou 3) qui sera utilise
 *                 (le registre 3 MCLK ne peut pas etre utilise comme
 *                 programmation de sortie).
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : frequence programmee (ou 0)
 *
 * Liste de modifications :
 *
 */

long setFrequence ( VOLATILE mgaRegsPtr pDeviceParam, long fout, long reg )
{
   short i;
   char  pgm_flag;

   long p, q, m;
   short index;
   short i_find;

   long fvco, fout_vrai, fout_voulue;
   long fdivise;
   long erreur_ppm;
   long best_erreur;
   long ftmp;

   byte init_misc;

   ST_RESULTAT resultat;

   pDevice = pDeviceParam;

#ifdef SCRAP
   fout *= 1000;                               /* Scale it from KHz to Hz */
#endif

   init_misc = pDevice->titan.vga.misc_out;
   misc.var8 = init_misc;


   /***************************************************/
   /*
   /* ON DEMANDE SEULEMENT DE PROGRAMMER LE REGISTRE
   /*
   /***************************************************/


   if ( fout == 0L )
   {

      switch ( reg )
      {
	 case VIDEO_CLOCK_1:
	 case VIDEO_CLOCK_2:
	       misc.sel_reg_output.select = reg;
	       pDevice->titan.vga.misc_outw = misc.var8;
	       break;
   
	 case VIDEO_CLOCK_3:
	       misc.sel_reg_output.select = 3;
	       pDevice->titan.vga.misc_outw = misc.var8;
	       break;

	 default:
	       /* par defaut, (si on a modifie MCLK), on restore VCLK
		* initial
		*/
	       pDevice->titan.vga.misc_outw = init_misc;
	       break;
      }

      return ( 0 );
   }



   /***************************************************/
   /*
   /* ON DEMANDE DE CALCULER UNE FREQUENCE
   /*
   /***************************************************/



   fout_voulue = fout;

#ifdef SCRAP
   fref = 14318180;     /* frequence clock ref */
#endif

   fref = 1431818;

   index = 0;
   best_erreur = 99999999;         /* dummy start number */

/* pour tous les diviseurs possibles */

   for ( m = 0 ; m < NBRE_M_POSSIBLE ; m++ )
   {

      fdivise = diviseur_m[m];

      fvco    = fdivise * fout_voulue;

      if ((fvco < 40000) || (fvco > 120000))
	  continue;


      for ( q = 3 ; q <= 129 ; q++ )
      {
   
	 if ( ((fref/q) < 20000) || ((fref/q) > 100000))
	    continue;


	 p = ((q * fvco * 100) / (2 * fref)) + 1;
   

	 if (p < 4 || p > 130)
	    continue;

	 /**************************************************
	  * maintenant qu'on a toutes nos valeurs,
	  * on determine la vrai f(output), et son erreur.
	  */
   
	 fout_vrai  = (2 * fref * p) / (q * fdivise * 100) ;
   

#ifdef SCRAP   
	 ftmp = fout_vrai / fout_voulue - 1;
	 erreur_ppm = ABS_D ( ftmp ) * 1000000;
#endif
   
	 if (fout_vrai > fout_voulue)
	    erreur_ppm = fout_vrai - fout_voulue;
	 else
	    erreur_ppm = fout_voulue - fout_vrai;

   
   
	 if ( erreur_ppm < best_erreur )
	 {
	    /***************************************************
	     *
	     * HO!!! ce resultat est meilleur que le precedant.
	     *
	     */
   
	    i_find = 0;
	    for ( i = 0 ; i < NBRE_I_POSSIBLE ; i++ )
	    {
	       if ( i == 0 )
		  continue;
   
	       if ( ( fvco >= limites_i[i] ) && ( fvco <= limites_i[i+1] ) )
		  i_find = i;
	    }
   
   
	    if ( i_find != 0 )
	    {
	       index = 0;     /* reset tableau de resultat */
   
	       resultat.p = (short)p;
	       resultat.q = (short)q;
	       resultat.m = (short)m;
	       resultat.i = i_find;
	       resultat.fout_vrai  = fout_vrai;
	       resultat.erreur_ppm = erreur_ppm;
   
	       index++;                   /* pointe sur prochain element */
   
	       best_erreur = erreur_ppm;  /* reset reference erreur */
	    }
	 }
      }
   }

   
   if ( index == 0 )
   {
      /* restore valeur de depart */

      /* outb ( MISC_OUTPUT_WRITE , init_misc );   */

      pDevice->titan.vga.misc_outw = init_misc;

   }
   else
   {

      programme_clock ( reg,  resultat.p,
			      resultat.q,
			      resultat.m,
			      resultat.i );

      /********************************************/
      /*
      /*   PROGRAMME CS : clock output select
      /*
      /********************************************/

      switch ( reg )
      {
	 case VIDEO_CLOCK_1:
	 case VIDEO_CLOCK_2:
		misc.sel_reg_output.select = reg;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;

	 case VIDEO_CLOCK_3:
		misc.sel_reg_output.select = 3;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;
   

	 default:
	       /* par defaut, (si on a modifie MCLK), on restore VCLK
		* initial
		*/
		pDevice->titan.vga.misc_outw = init_misc;
		break;
      }
   }

   return ( 0 );
}



#else

/***************************************************************************/
/*/ setFrequence ( long fout_voulue, long registre )
 *
 * Permet de determiner les meilleures valeurs a mettre pour la
 * programmation du ICD2061, pour une frequence donnee.
 * Programme le registre.
 * Si la frequence de sortie est 0, ne fait que choisir le registre
 * de sortie.
 *
 * Problemes :
 * Concu     : Dominique Leblanc:92/10/29
 *
 * Parametres: [0]-frequence output.
 *             [1]-registre (0, 1, 2 ou 3) qui sera utilise
 *                 (le registre 3 MCLK ne peut pas etre utilise comme
 *                 programmation de sortie).
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : frequence programmee (ou 0)
 *
 * Liste de modifications :
 *
 */

long setFrequence ( VOLATILE mgaRegsPtr pDeviceParam, long fout, long reg )
{
   short i;
   char  pgm_flag;

   short p, q, m;
   short index;
   short i_find;

   double fvco, fout_vrai, fout_voulue;
   double fdivise;
   double erreur_ppm;
   double best_erreur;
   double ftmp;

   byte init_misc;


   ST_RESULTAT resultat;



   pDevice = pDeviceParam;



   fout *= 1000;                                /* Scale it from KHz to Hz */



   init_misc = pDevice->titan.vga.misc_outr;
   misc.var8 = init_misc;


   /***************************************************/
   /*
   /* ON DEMANDE SEULEMENT DE PROGRAMMER LE REGISTRE
   /*
   /***************************************************/


   if ( fout == 0L )
   {

      switch ( reg )
      {
	 case VIDEO_CLOCK_1:
	 case VIDEO_CLOCK_2:
		misc.sel_reg_output.select = reg;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;
   
	 case VIDEO_CLOCK_3:
		misc.sel_reg_output.select = 3;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;


	 default:
	       /* par defaut, (si on a modifie MCLK), on restore VCLK
		* initial
		*/
		pDevice->titan.vga.misc_outw = init_misc;
		break;
      }

      return ( 0 );
   }



   /***************************************************/
   /*
   /* ON DEMANDE DE CALCULER UNE FREQUENCE
   /*
   /***************************************************/



   fout_voulue = (double) fout;


   /**** LIMITATION 1: sur fref */

   if ( ( 1.0e6 > fref ) || ( 60.0e6 < fref ) )
   {

      fref = 14318180.0;     /* frequence clock ref */
   }

   index = 0;
   best_erreur = 9.9999e99;         /* dummy start number */

   for ( m = 0 ; m < NBRE_M_POSSIBLE ; m++ )/* pour tous les diviseurs possibles 
*/
   {

      fdivise = diviseur_m[m];

      fvco    = fdivise * fout_voulue;



      /***** 1st LIMITATION ******/
      if ( ( 40.0e6  > fvco ) || ( fvco > 120.0e6 ) )
      {
	 continue;
      }
   

      for ( q = 3 ; q <= 129 ; q++ )
      {
   
	 /***** 2nd LIMITATION *****/
   
	 if ( ( 200.0e3 > ( fref / (double) q ) ) ||
	      ( 1.0e6   < ( fref / (double) q ) )    )
	 {
	    continue;
	 }
   
   
	 p = (short) ( ( (double) q * fvco / 2.0 / fref ) + 0.5 );
   
	 /***** 3rd LIMITATION *****/
   
	 if ( ( 4 > p ) || ( p > 130 ) )
	 {
	    continue;
	 }
   
   
	 /**************************************************
	  *
	  * maintenant qu'on a toutes nos valeurs,
	  * on determine la vrai f(output), et son erreur.
	  *
	  */
   
	 fout_vrai  = 2.0 * fref * (double) p / (double) q / fdivise;
   
   
	 ftmp = fout_vrai / fout_voulue - 1;
	 erreur_ppm = ABS_D ( ftmp ) * 1.0e6;
   
   
   
	 if ( erreur_ppm < best_erreur )
	 {
	    /***************************************************
	     *
	     * HO!!! ce resultat est meilleur que le precedant.
	     *
	     */
   
   
	    i_find = 0;
	    for ( i = 0 ; i < NBRE_I_POSSIBLE ; i++ )
	    {
	       if ( i == 0 )
		  continue;
   
	       if ( ( fvco >= limites_i[i] ) && ( fvco <= limites_i[i+1] ) )
		  i_find = i;
	    }
   
   
	    if ( i_find != 0 )
	    {
	       index = 0;     /* reset tableau de resultat */
   
	       resultat.p = p;
	       resultat.q = q;
	       resultat.m = m;
	       resultat.i = i_find;
	       resultat.fout_vrai  = fout_vrai;
	       resultat.erreur_ppm = erreur_ppm;
   
	       index++;                   /* pointe sur prochain element */
   
	       best_erreur = erreur_ppm;  /* reset reference erreur */
	    }
	 }
      }
   }
   
   if ( index == 0 )
   {
      /* restore valeur de depart */
      pDevice->titan.vga.misc_outw = init_misc;

   }
   else
   {
      programme_clock ( reg,  resultat.p,
			      resultat.q,
			      resultat.m,
			      resultat.i );


#ifdef SCRAP
      programme_clock ( 2,  84,
			    37,
			     1,
			   2944 );
#endif


      /********************************************/
      /*
      /*   PROGRAMME CS : clock output select
      /*
      /********************************************/

      switch ( reg )
      {
	 case VIDEO_CLOCK_1:
	 case VIDEO_CLOCK_2:
		misc.sel_reg_output.select = reg;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;
   
	 case VIDEO_CLOCK_3:
		misc.sel_reg_output.select = 3;
		pDevice->titan.vga.misc_outw = misc.var8;
		break;

	 default:
	       /* par defaut, (si on a modifie MCLK), on restore VCLK
		* initial
		*/
		pDevice->titan.vga.misc_outw = init_misc;
		break;
      }
   }
   
   return ( 0 );
}

#endif

/* ======================================================================= */
/*/
 * NOM: programme_clock ( short p, short q, short m, short reg )
 *
 * Programme la clock de l'oscillateur programmable pour la frequence
 * desiree.
 *
 * Problemes :
 * Concu     : Dominique Leblanc:92/10/28.
 *
 * Parametres: [0]-mode: 300/600 DPI.
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

dword programme_clock ( short reg, short p, short q, short m, short i )
{
   reg_clock [ reg ].var32 = 0;
   reg_clock [ reg ].bit.q_prime     = q - 2;
   reg_clock [ reg ].bit.m_diviseur  = m;
   reg_clock [ reg ].bit.p_prime     = p - 3;
   reg_clock [ reg ].bit.index_field = i;

   programme_reg_icd ( pDevice, reg, reg_clock [ reg ].var32 );

   return ( reg_clock [ reg ] .var32 );
}
/* ======================================================================= */
/*/
 * NOM: programme_reg_icd ( VOLATILE BYTE _Far *pDeviceParam, short reg, dword 
data )
 *
 * Cette routine permet la programmation serielle de l'oscillateur
 * programmable, l'ICD2061.  Elle utilise le protocole de communication
 * suivant (tire des SPECs HARDWARE):
 *
 *
 *      ^  .                          .       .        .        .       .
 *      |  .                          .       .        .        .       .
 *      |  .   _   _   _   _   _     ___     ____     ____     ___     _._
 * CLK  |_____| |_| |_| |_| |_| |___| . |___| .  |___| .  |___| . |___| .
 *      |  .                          .       .        .        .       .
 *      |  .                          .       .        .        .       .
 *      |  . _____________________    .       . ___    .     ___________._
 * DATA |___|                     |____________|   |________|   .       .
 *      +----------------------------------------------------------------->
 *      |  .  unlock sequence         . start . send 0 . send 1 . stop  .
 *      |  .                          .  bit  .        .        . bit   .
 *      |
 *
 *
 * Problemes :
 * Concu     : Dominique Leblanc:92/10/28 (mon garcon a 1.5 an aujourd'hui)
 *
 * Parametres: [0] - reg: adresse du registre interne de l'ICD2061 (3 bits).
 *             [1] - data: data a ecrire dans le reg (21 bits).
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

void programme_reg_icd(VOLATILE mgaRegsPtr pDeviceParam, short reg, dword data)
{
   pDevice = pDeviceParam;

   send_unlock ();
   send_start ();
   send_data ( reg, data );
   send_stop  ();
}
/* ======================================================================= */
/*/
 * NOM: send_unlock ( void )
 *
 * envoie la sequence unlock.
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_unlock ( void )
{
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
}
/* ======================================================================= */
/*/
 * NOM: send_data ( void )
 *
 * Envoie 21 bits de data + 3 bits pour le registre.
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_data ( short reg, dword data )
{
   short i;

   for ( i = 0 ; i < 21 ; i++ )
   {
      if ( ( data & 1 ) == 1 )
	 send_1 ();
      else
	 send_0 ();

      data >>= 1;
   }

   for ( i = 0 ; i < 3 ; i++ )
   {
      if ( ( reg & 1 ) == 1 )
	 send_1 ();
      else
	 send_0 ();

      reg  >>= 1;
   }
}
/* ======================================================================= */
/*/
 * NOM: send_full_clock ( void )
 *
 * Toggle une clock complete, sans varier le data (sert seulement pour le
 * "send_unlock ( void )").
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_full_clock ( void )
{
   misc.clock_bit.pgmclk  = 1;
   pDevice->titan.vga.misc_outw = misc.var8;
   misc.clock_bit.pgmclk  = 0;
   pDevice->titan.vga.misc_outw = misc.var8;
 }
/* ======================================================================= */
/*/
 * NOM: send_start ( void )
 *
 * envoie un start bit.
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_start ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   send_full_clock ();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 }
/* ======================================================================= */
/*/
 * NOM: send_0 ( void )
 *
 * Envoie le bit data 0 (en suivant le protocole "MANCHESTER").
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_0 ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;

 
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 
}
/* ======================================================================= */
/*/
 * NOM: send_1 ( void )
 *
 * Envoie le bit data 1 (en suivant le protocole "MANCHESTER").
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_1 ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 }
/* ======================================================================= */
/*/
 * NOM: send_stop ( void )
 *
 * Envoie le stop bit.
 *
 * Problemes :
 * Concu     : Dominique Leblanc
 *
 * Parametres: aucun
 *
 * Appelle   : voir routine
 *
 * Utilise   :
 * Modifie   :
 *
 * Retourne  : rien
 *
 * Liste de modifications :
 *
 */

static void send_stop ( void )
{
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;

   pDevice->titan.vga.misc_outw = misc.var8;
 }

#ifdef NO_FLOAT

/***************************************************************************/
/*/ mgasetTVP3026Freq ( volatile byte _Far *pDeviceParam, long desiredFout, long 
reg, word pWidth )
 *
 * Calculate best value to obtain a frequency output.
 * This routine program and select the register.
 * If fout is 0, just toggle to the desired
 * output register.
 *
 * Problems :
 * Designed : Patrick Servais:94/04/08
 *
 * Parameters: [0]-desired output frequency (in kHz).
 *                 (0 -> select desired register)
 *             [1]-register (0, 1, 2 ou 3) to prgram and select.
 *                 (the MCLOCK register (3) can be reprogrammable, but
 *                 is always available for output - no selection is permit).
 *
 * Call      :
 *
 * Used      :
 * Modify    : Benoit Leblanc
 *
 * Return    : old frequency value
 *
 * List of modifications :
 *
 */
long mgasetTVP3026Freq ( VOLATILE mgaRegsPtr pDeviceParam, long fout, long reg,byte pWidth )
{
   word i;
   short p, pixel_p, pixel_n, q, n, bestN;
   int   m, pixel_m, bestM;
   short index;
   short val,z;
   long fvco, fTemp, trueFout, desiredFout;
   long ppmError;
   long bestError;
   byte init_misc;
   byte tmpByte, saveByte;
   word pixelWidth;
   dword power;
   dword config200Mhz, fvcoMax;
 
   pDevice = pDeviceParam;
   /* Read 200Mhz straps saved in config register */
   config200Mhz = pDevice->titan.config;
   if (config200Mhz & 0x00000004)
      {
      fvcoMax = (dword)220000000; /* 200Mhz support */
      }
   else
      {
      fvcoMax = (dword)175000000; /* 200Mhz not support */
      }


   init_misc = pDevice->titan.vga.misc_out;
   misc.var8 = init_misc;

   /***************************************************/
   /*
   /* CALCULATE FREQUENCY
   /*
   /***************************************************/


   bestError = 99999999;    /* dummy start number */

   fref = 14318180;         /* frequence clock ref */

   index = 0;
   bestError = 5000000;

   desiredFout = fout * 1000;    /* Scale it from KHz to Hz */

   if (desiredFout>= (fvcoMax >> 1))
      p = 0;
   else if (desiredFout>=(fvcoMax >> 2))
	   p = 1;
	else if (desiredFout>=(fvcoMax >> 3))
		p = 2;
	     else
		p = 3;


   power = 1;
   for(i=0; i<p; i++)
      power = power * 2;



   for ( n=1;n<=63;n++ )
      {

      m = (650 - (((((dword)desiredFout*10)/fref) * ((65-n) * power)) / 8)) / 
10;
	 
      fTemp = fref / (65 - n);
      fvco = fTemp * 8 * (65 - m);
      trueFout = fvco / power;

      if (trueFout < desiredFout)
	 ppmError = desiredFout - trueFout;
      else
	 ppmError = trueFout - desiredFout;

      if ((ppmError < bestError) && 
	  (m > 0) && (m <= 63) &&
	  (fTemp > 500000) &&
	  (fvco >= (fvcoMax >> 1) ) && (fvco <= (dword)220000000))
	 {
	 index = 1;

	 bestError = ppmError;
	 bestM = m;
	 bestN = n;
	 }
      }


   m = bestM;
   n = bestN;

   {
   dword num;

   num = ((65 - m)*10) / (65-n);
   num = num * 8 * fref;

   trueFout = (num / power) / 10;
   }
   

   if ( index == 0 )    /* no solution found */
   {
      /*  ***ERROR: setFrequence() NONE RESULT (IMPOSSIBLE?!?) */
      /* restore starting value */
      pDevice->titan.vga.misc_outw = init_misc;
   }
   else
   {

       /**********************************************************************
       *
       * SET THE DESIRED FREQUENCY OUTPUT REGISTER
       *
       */
      switch ( reg )
      {

	 case VIDEO_CLOCK_3:  /* NOTE 1: header */
	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;
	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    /* searching for loop clock parameters */               
	    
	    n = 65 - ((2*64)/pWidth);  /* 64 is the Pixel bus Width */
	    m = 0x3f;
	    z = ((65-n)*55)/(fout/1000);



	    q = 0;
	    p = 3;
	    if (z <= 2)
	       p = 0;
	    else if (z <= 4)
		     p = 1;
		  else if (z <= 8)
			   p = 2;
			else if (z <=16)
			      p = 3;
			   else
			      q = ((z - 16)/16)+1;

	    n |= 0x80;

	    /* Eliminate the high frequency jitter */
	    if (fout <= 175000)
	       p |= 0xb0;
	    else
	       p |= 0xf0;

	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    val = tmpByte;
	    pDevice->ramdacs.tvp3026.data = ((val&0xf8)|q) | 0x20;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xcf;

/* DAT Patrick Servais, on ajoute ce qui suis */
	    pDevice->ramdacs.tvp3026.index = TVP3026_LOAD_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xcf;

	    pDevice->ramdacs.tvp3026.index = TVP3026_LOAD_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = (byte)n;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = (byte)m;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = (byte)p;
	    mgaDelay2(100L);

	    misc.sel_reg_output.select = 3;  /* NOTE 1: header */
		 pDevice->titan.vga.misc_outw = misc.var8;

	    break;
   

	 /*******************************************************************
	  *
	  * the programmation line is used to modify register internal value
	  * of TVP3026, and to select output video register (with internal
	  * muxer) at the end of programmation.  For the system clock
	  * MCLOCK programmation, at the end, we put on programmation line
	  * the initial value of the video register.
	  *
	  */
	 case MEMORY_CLOCK:
	    /* by default, (if we modified MCLK), we restore VCLK
	       * initial
	       */
	    saveByte = pDevice->titan.vga.misc_outr;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_n = pDevice->ramdacs.tvp3026.data;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfd;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_m = pDevice->ramdacs.tvp3026.data;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfe;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_p = pDevice->ramdacs.tvp3026.data;


	    /*------------*/
	    /* 1st step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    tmpByte = 3;
	    pDevice->titan.vga.misc_outw = tmpByte;

	    /*------------*/
	    /* 3rd step   */
	    /*------------*/
	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    pDevice->ramdacs.tvp3026.data = 0x38;
	    val = 0x38;

	    /*------------*/
	    /* 4th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xf3;

	    pDevice->ramdacs.tvp3026.index = TVP3026_MEM_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xb0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_MEM_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    /*------------*/
	    /* 5th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    pDevice->ramdacs.tvp3026.data = (val&0xe7)|0x10;
	    pDevice->ramdacs.tvp3026.data = (val&0xe7)|0x18;

	    /*------------*/
	    /* 6th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((pixel_n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (pixel_m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((pixel_p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);


	    pDevice->titan.vga.misc_outw = saveByte;

	    break;

	 default:

		  /***ERROR: REGISTER ICD UNKNOWN: */
	    break;
      }    

}
  
  /* wait 10ms before quit....this is not suppose
   * to necessary outside of the programmation
   * register, but, well, we are not too secure
   */
   tmpByte = pDevice->titan.vga.misc_outw;

   return ( 0 );
}

#else

/***************************************************************************/

/*/ mgasetTVP3026Freq ( volatile byte _Far *pDeviceParam, long desiredFout, long 
reg, word pWidth )
 *
 * Calculate best value to obtain a frequency output.
 * This routine program and select the register.
 * If fout is 0, just toggle to the desired
 * output register.
 *
 * Problems :
 * Designed : Patrick Servais:94/04/08
 *
 * Parameters: [0]-desired output frequency (in kHz).
 *                 (0 -> select desired register)
 *             [1]-register (0, 1, 2 ou 3) to prgram and select.
 *                 (the MCLOCK register (3) can be reprogrammable, but
 *                 is always available for output - no selection is permit).
 *
 * Call      :
 *
 * Used      :
 * Modify    : Benoit Leblanc
 *
 * Return    : old frequency value
 *
 * List of modifications :
 *
 */

long mgasetTVP3026Freq ( VOLATILE mgaRegsPtr pDeviceParam, long fout,
	long reg,byte pWidth )
{
   word i;
   short p, pixel_p, pixel_n, q, n, bestN;
   int   m, pixel_m, bestM;
   short index;
   short val,z;
   long fvco, fTemp, trueFout, desiredFout;
   long ppmError;
   long bestError;
   byte init_misc;
   byte tmpByte, saveByte;
   word pixelWidth;
   dword power;
   dword config200Mhz, fvcoMax;
 
   pDevice = pDeviceParam;

   /* Read 200Mhz straps saved in config register */
   config200Mhz = pDevice->titan.config;
   if (config200Mhz & 0x00000004)
      {
      fvcoMax = (dword)220000000; /* 200Mhz support */
      }
   else
      {
      fvcoMax = (dword)175000000; /* 200Mhz not support */
      }


   init_misc = pDevice->titan.vga.misc_out;
   misc.var8 = init_misc;

   /***************************************************/
   /*
   /* CALCULATE FREQUENCY
   /*
   /***************************************************/


   bestError = 99999999;    /* dummy start number */

   fref = 14318180;         /* frequence clock ref */

   index = 0;
   bestError = 5000000;

   desiredFout = fout * 1000;    /* Scale it from KHz to Hz */

   if (desiredFout>= (fvcoMax >> 1))
      p = 0;
   else if (desiredFout>=(fvcoMax >> 2))
	   p = 1;
	else if (desiredFout>=(fvcoMax >> 3))
		p = 2;
	     else
		p = 3;


   power = 1;
   for(i=0; i<p; i++)
      power = power * 2;



   for ( n=1;n<=63;n++ )
      {

      m = (650 - (((((dword)desiredFout*10)/fref) * ((65-n) * power)) / 8)) / 
10;
	 
      fTemp = fref / (65 - n);
      fvco = fTemp * 8 * (65 - m);
      trueFout = fvco / power;

      if (trueFout < desiredFout)
	 ppmError = desiredFout - trueFout;
      else
	 ppmError = trueFout - desiredFout;

      if ((ppmError < bestError) && 
	  (m > 0) && (m <= 63) &&
	  (fTemp > 500000) &&
	  (fvco >= (fvcoMax >> 1) ) && (fvco <= (dword)220000000))
	 {
	 index = 1;

	 bestError = ppmError;
	 bestM = m;
	 bestN = n;
	 }
      }


   m = bestM;
   n = bestN;

   {
   dword num;

   num = ((65 - m)*10) / (65-n);
   num = num * 8 * fref;

   trueFout = (num / power) / 10;
   }
   

   if ( index == 0 )    /* no solution found */
   {
      /*  ***ERROR: setFrequence() NONE RESULT (IMPOSSIBLE?!?) */
      /* restore starting value */

      pDevice->titan.vga.misc_outw = init_misc;
   }
   else
   {

       /**********************************************************************
       *
       * SET THE DESIRED FREQUENCY OUTPUT REGISTER
       *
       */

      switch ( reg )
      {

	 case VIDEO_CLOCK_3:  /* NOTE 1: header */
	       
	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;
	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    /* searching for loop clock parameters */               
	    
	    n = 65 - ((2*64)/pWidth);  /* 64 is the Pixel bus Width */
	    m = 0x3f;
	    z = ((65-n)*55)/(fout/1000);



	    q = 0;
	    p = 3;
	    if (z <= 2)
	       p = 0;
	    else if (z <= 4)
		     p = 1;
		  else if (z <= 8)
			   p = 2;
			else if (z <=16)
			      p = 3;
			   else
			      q = ((z - 16)/16)+1;

	    n |= 0x80;

	    /* Eliminate the high frequency jitter */
	    if (fout <= 175000)
	       p |= 0xb0;
	    else
	       p |= 0xf0;

	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    val = tmpByte;
	    pDevice->ramdacs.tvp3026.data = ((val&0xf8)|q) | 0x20;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xcf;

/* DAT Patrick Servais, on ajoute ce qui suis */
	    pDevice->ramdacs.tvp3026.index = TVP3026_LOAD_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = 0;
	    mgaDelay2(100L);

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xcf;

	    pDevice->ramdacs.tvp3026.index = TVP3026_LOAD_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = (byte)n;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = (byte)m;
	    mgaDelay2(100L);
	    pDevice->ramdacs.tvp3026.data = (byte)p;
	    mgaDelay2(100L);

	    misc.sel_reg_output.select = 3;  /* NOTE 1: header */
		pDevice->titan.vga.misc_outw = misc.var8;

	    break;
   

	 /*******************************************************************
	  *
	  * the programmation line is used to modify register internal value
	  * of TVP3026, and to select output video register (with internal
	  * muxer) at the end of programmation.  For the system clock
	  * MCLOCK programmation, at the end, we put on programmation line
	  * the initial value of the video register.
	  *
	  */
	 case MEMORY_CLOCK:
	    /* by default, (if we modified MCLK), we restore VCLK
	       * initial
	       */

	    saveByte = pDevice->titan.vga.misc_outr;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_n = pDevice->ramdacs.tvp3026.data;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfd;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_m = pDevice->ramdacs.tvp3026.data;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    pDevice->ramdacs.tvp3026.data = 0xfe;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pixel_p = pDevice->ramdacs.tvp3026.data;


	    /*------------*/
	    /* 1st step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    tmpByte = 3;
	    pDevice->titan.vga.misc_outw = tmpByte;

	    /*------------*/
	    /* 3rd step   */
	    /*------------*/
	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    pDevice->ramdacs.tvp3026.data = 0x38;
	    val = 0x38;

	    /*------------*/
	    /* 4th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xf3;

	    pDevice->ramdacs.tvp3026.index = TVP3026_MEM_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((p&0x03)|0xb0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_MEM_CLK_DATA;
	       pDevice->ramdacs.tvp3026.data = tmpByte;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);

	    /*------------*/
	    /* 5th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_MCLK_CTL;
	    pDevice->ramdacs.tvp3026.data = (val&0xe7)|0x10;
	    pDevice->ramdacs.tvp3026.data = (val&0xe7)|0x18;

	    /*------------*/
	    /* 6th step   */
	    /*------------*/

	    pDevice->ramdacs.tvp3026.index = TVP3026_PLL_ADDR;
	    tmpByte = pDevice->ramdacs.tvp3026.data;
	    pDevice->ramdacs.tvp3026.data = tmpByte & 0xfc;

	    pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	    pDevice->ramdacs.tvp3026.data = ((pixel_n&0x3f)|0x80);
	    pDevice->ramdacs.tvp3026.data = (pixel_m&0x3f);
	    pDevice->ramdacs.tvp3026.data = ((pixel_p&0x03)|0xf0);

	    do
	    {
	       pDevice->ramdacs.tvp3026.index = TVP3026_PIX_CLK_DATA;
	       tmpByte = pDevice->ramdacs.tvp3026.data;
	       tmpByte &= 0x40;
	    } while(tmpByte != 0x40);


	    pDevice->titan.vga.misc_outw = saveByte;

	    break;

	 default:

		  /***ERROR: REGISTER ICD UNKNOWN: */
	    break;
      }    

}
  
  /* wait 10ms before quit....this is not suppose
   * to necessary outside of the programmation
   * register, but, well, we are not too secure
   */
   tmpByte = pDevice->titan.vga.misc_outw;

   return ( 0 );
}

#endif

