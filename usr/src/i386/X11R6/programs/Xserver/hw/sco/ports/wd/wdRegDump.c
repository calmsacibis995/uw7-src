#ifdef REG_DUMP

/*       calling seq.:
 *      RegDump( "Before mwSetText", firstG--,'G' );
 */

#include <stdio.h>

RegDump( string, first,mode )
char * string;
int first;
char mode;
{
      typedef struct { int indexreg;
                       int datareg;
                       int nrind;
                       int * indList;
		       int * valT;
		       int * valG;
                     } registers; 
      registers * reg;
      int ir, i; 
      int * index;
      int * valueT;
      int * valueG;
      int * values;
      int val;
      int change = 0;
#define NUM1 33
#define NUM2 13
#define NUM3 21
#define NUM4 16

      static int valG1[NUM1] ;
      static int valT1[NUM1] ;
      static int indList1[NUM1] =
      { 0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
        0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x29,0x3e };
 
      static registers regList1 = { 0x3d4, 0x3d5, NUM1, indList1 ,valT1,valG1};

      static int valG2[NUM2] ;
      static int valT2[NUM2] ;
      static int indList2[NUM2] =
      { 0,1,2,3,4,7,8,9,0x10,0x11,0x12,0x13,0x14 };

      static registers regList2 = { 0x3c4, 0x3c5, NUM2, indList2 ,valT2,valG2};

      static int valG3[NUM3] ;
      static int valT3[NUM3] ;
      static int indList3[NUM3] =
      { 0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf,
        0x10,0x11,0x12,0x13,0x14 };

      static registers regList3 = { 0x3c0, 0x3c0, NUM3, indList3 ,valT3,valG3};

      static int valG4[NUM1] ;
      static int valT4[NUM4] ;
      static int indList4[NUM4] =
      { 0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf };

      static registers regList4 = { 0x3ce, 0x3cf, NUM4, indList4 ,valT4,valG4};

      static registers *regList[4] = {&regList1,&regList2,&regList3,&regList4};

#ifdef LOCK
      outb(0x3D4,0x29); outb(0x3D5,0x80);    /* Read Enable PR11-17 */
      outb(0x3C4,0x06); outb(0x3C5,0x48);    /* unlock PR21-PR34 */
#endif

      if( first>0 )
           fprintf(stderr,"Register values %s mode %s:\n\n"
                 ,mode=='T' ? "Text" : "Graphics",string);
      else
           fprintf(stderr,"Registers changed %s:\n\n",string);

      fprintf(stderr,"3cc = %3x\n\n",inb( 0x3cc ));

      for( ir=0; ir<=3; ir++)
      {
           reg    = regList[ir];
           index  = reg->indList;
           if( mode=='T')
                   values = reg->valT;
           else
                   values = reg->valG;
             
           for( i=0; i< reg->nrind; i++)
           {
                if( reg->indexreg == reg->datareg ) inb( 0x3DA ); /* reset */
                outb( reg->indexreg,  index[i] );
                val = inb( reg->datareg );
		if( first>0 || val != values[i] )
                {
                    change = 1;
                    fprintf( stderr,"%3x.%2x = %2x    ",
                             reg->indexreg,index[i],val);
                }
                if( change && (i % 5 == 4))  fprintf(stderr,"\n");
                values[i] = val;
           }
           if( change ) fprintf(stderr,"\n\n"); change = 0;
      }
#ifdef LOCK
      outb(0x3D4,0x29); outb(0x3D5,0x00);    /* lock PR11-PR17 */
      outb(0x3C4,0x06); outb(0x3C5,0x00);    /* lock PR21-PR34  */
#endif
}

#endif
