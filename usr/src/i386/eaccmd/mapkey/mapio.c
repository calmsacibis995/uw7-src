#ident	"@(#)mapio.c	1.9"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	21Mar97		rodneyh@sco.com
 *	- Changes for ALT lock support.
 *	  Defined new keyword, alock.
 *	L001	30Apr97		rodneyh@sco.com
 *	- Added panic and alock to fnames array, and changed test for fnames
 *	  in fprintval() to K_ALK.
 *	L002	17nov97		brendank@sco.com	MR: ul96-31903
 *	- Merged in escalation fix for above MR so that mapkey
 *	  recognises mapping files that follow the description
 *	  given in keyboard(7).
 *
 */

#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/ascii.h>
#include <stdio.h>
#include <ctype.h>
#include "table.h"

extern int uslmap, scomap;
extern int kern;
#ifdef DEBUG
extern int dmp;
#endif

#ifdef DEBUG
keymap_t keymap;
main()
{
	fparsemap(stdin,&keymap);
	fprintmap(stdout,&keymap);
}
#endif

fparsemap(pF,pK)
keymap_t *pK;
FILE *pF;
{
	static char aCtok[8];
	static char aClock[2];
	int i,Nscan,flag;
	
		
	fscanblanks(pF);
	while (!feof( pF )){
		fscantok(pF,aCtok);
		Nscan = atoi( aCtok );
#ifdef DEBUG
		printf("token '%s' ",aCtok);
		printf("key number %d\n", Nscan);
#endif
		fscanblanks(pF);
		if(Nscan > pK->n_keys - 1 ){
			pK->n_keys = Nscan + 1;
		}
	
		for( i = 0; i < 8 ; i++ ){
			fscantok(pF,aCtok);
#ifdef DEBUG
			printf("token '%s' ",aCtok);
#endif
			pK->key[Nscan].map[i] = translate(aCtok,&flag);
			pK->key[Nscan].spcl |= flag << (7 - i);
			fscanblanks(pF);
		}
	
		fscanblanks(pF);
		fscantok(pF,aClock);
#ifdef DEBUG
		printf("token '%s'\n",aClock);
#endif
		switch( aClock[0] ){
		case 'O': pK->key[Nscan].flgs = L_O; break;
		case 'C': pK->key[Nscan].flgs = L_C; break;
		case 'N': pK->key[Nscan].flgs = L_N; break;
		}
	
		fscanblanks(pF);
	}
	checkmap(pK);

}

#ifdef DEBUG
dmpline(pK)
keymap_t *pK;
{
	int i,j;
	char flg_char;
        
	if (dmp >= 0) {
	    i = dmp;
	    fprintf(stdout, "%d   ", dmp);
	   for( j = 0 ; j < 8 ; j++ ){
		fprintval( stdout, pK->key[i].map[j], ((pK->key[i].spcl)>>(7-j))&1 );
	   }
	   switch( pK->key[i].flgs ){
		case L_O: flg_char ='O'; break;
		case L_C: flg_char ='C'; break;
		case L_N: flg_char ='N'; break;
	   }
	   fprintf(stdout,"  %c\n", flg_char);
	}
}
#endif
fscantok(pF,pC)
FILE *pF;
char *pC;
{
	
	*pC = getc( pF );
	if( '\'' == *pC ){
		*++pC = getc(pF);
		if( '\\' == *pC ){
			*++pC = getc(pF);
		}
		*++pC = getc(pF);
		*pC = 0;
	} else {
		while ( *pC != ' ' && *pC != '\n' && *pC != '\t' )
			*++pC = getc(pF);
		ungetc( *pC , pF);
		*pC = 0;
	}
}


int line = 0;

fscanblanks(pF)
FILE *pF;
{
	int c;
	
	c = getc(pF);
	while ( ' ' == c || '\t' == c || '\n' == c || '#' == c ) {
		if( '#' == c ){
			while( c != '\n' ) 
				c = getc(pF);
			line++;
		} else if ('\n' == c ){
			c = getc(pF);
			line++;
		} else
			c = getc(pF);
	}
	ungetc( c, pF);
}

int
translate( pC,pflag )
char *pC;
int *pflag;
{
	*pflag = 0;
	switch(pC[0]){
	default:   fprintf(stderr,"line %d:token '%s' not recognized\n", line, pC);
		   return 0;
	case '\'': return pC[1] == '\\' ? pC[2] : pC[1]; 
	case '0':  switch( pC[1] ) {
		   case 'x': return basen( 16, pC+2 );
		   default:  return basen( 8, pC+1 );
		   }
	case '1':case '2':case '3':case '4':case '5':
	case '6':case '7':case '8':case '9':
		  return atoi(pC);
	case 'a': switch (pC[1]) {
		  case 'g':
			*pflag = 1;
			return K_AGR;
		  case 'l':
			*pflag = 1;

			if(pC[2] == 'o')		/* L000 begin */
				return K_ALK;		/* ALT lock */
			else				/* L000 end */
				return K_ALT;
		  default:
			return A_ACK;
		  }
	case 'b': switch( pC[1] ){
		  case 'e': return A_BEL;
		  case 'r': *pflag = 1;return K_BRK;
		  case 's': return A_BS;
		  case 't': *pflag = 1;return K_BTAB; 
		  default: return K_NOP;
		  }
	case 'c': switch( pC[1] ){
		  case 'a': return A_CAN;
		  case 'l': *pflag = 1; return K_CLK;
		  case 't': *pflag = 1; return K_CTL;
		  case 'r': return A_CR;
		  }
	case 'd': switch( pC[2] ){
		  case '1': return A_DC1;
		  case '2': return A_DC2;
		  case '3': return A_DC3;
		  case '4': return A_DC4;
		  case 'l': return A_DEL;
		  case 'e': return A_DLE;
		  case 'b': *pflag = 1; return K_DBG;
		  }
	case 'e': switch( pC[1] ){
		  case 'm': return A_EM;
		  case 'n': return A_ENQ;
		  case 'o': return A_EOT;
		  case 's': 
							/* L002	vvv */
				switch( pC[3] ){
				case 'n': *pflag = 1; return K_ESN;
				case 'o': *pflag = 1; return K_ESO;
				case 'l': *pflag = 1; return K_ESL;
			    };
							/* L002 ^^^ */
			    return A_ESC;
		  case 't': return pC[2]=='b'? A_ETB:A_ETX;
		  }
	case 'f': switch( pC[1] ){
		  case '[': *pflag = 1; return K_ESL;
		  case 'N': *pflag = 1; return K_ESN;
		  case 'O': *pflag = 1; return K_ESO;
		  case 'n': *pflag = 1; return K_FRCNEXT;
		  case 'p': *pflag = 1; return K_FRCPREV;
		  case 's': return A_FS;
		  default:  *pflag = 1;
			    return KF+atoi(pC+4); 
		  }
	case 'g': return A_GS;
	case 'h': return A_HT;
	case 'k': *pflag = 1; return K_KLK;
	case 'l': *pflag = 1;
		  switch(pC[1]){
		  case 'a': return K_LAL;
		  case 'c': return K_LCT;
		  case 's': return K_LSH;
		  }
	case 'm': *pflag = 1; return KF+atoi(pC+3);
	case 'n': switch( pC[1] ){
		  case 'a': return A_NAK;
		  case 'l': return pC[2] ? (*pflag=1,K_NLK): A_NL;
		  case 'o': *pflag = 1; return K_NOP;
		  case 'p': return A_NP;
		  case 's': return pC[2] ? (*pflag=1,K_NEXT): A_US;
		  case 'u': return A_NUL;
		  }
	case 'p': 
		  switch (pC[1]) {
		  case 's': *pflag = 1; return K_PREV;
		  case 'a': *pflag = 1; return K_PNC;
		  }
	case 'r': switch( pC[1] ){
		  case 'a': *pflag=1; return K_RAL;
							 /* L002 vvv */
		  case 'b': *pflag=1; return K_RBT;      /*'rboot' as per doc.*/
							 /* L002 ^^^ */
		  case 'c': *pflag=1; return K_RCT;
		  case 'e': *pflag=1; return K_RBT;
		  case 's': return pC[2] ? (*pflag=1,K_RSH): A_RS;
		  }
		  break;
	case 's': switch( pC[1] ) {
		  case 'c': *pflag=1; return K_VTF + atoi(pC+3);
		  case 'i': return A_SI;
		  case 'l': *pflag = 1; return K_SLK;
		  case 'o': return pC[2] ? A_SOH : A_SO;
		  case 't': return A_STX;
		  case 'u': return A_SUB;
		  case 'y': return pC[2]=='n'?A_SYN:(*pflag=1,K_SRQ);
		  }
		  break;
	case 'v': return A_VT;
							/* L002 vvv */
	case 'N': *pflag=1; return K_NEXT;
	case 'P': *pflag=1; return K_PREV;
	case 'F': *pflag=1; return pC[1]=='N' ? K_FRCNEXT : K_FRCPREV;
							/* L002 ^^^ */
	}
	fprintf(stderr, "line %d: unknown keyword %s\n",line,pC);
	return A_NUL;
}

int
basen( base, s )
char *s;
int base;
{
	char c;
	int val, i;

	val = 0;
	while (c = *s) {
	   if ( ! ( ( c >= '0' && c <= '9' ) ||
		    ( c >= 'a' && c <= 'f' ) || 
		    ( c >= 'A' && c <= 'F' ) ) ) {
			fprintf(stderr, "line %d: bad number %s\n",line,s);
			exit(1);
		}
			
		i = c - (('0' <= c && c <= '9')? ('0'):
			 ('a' <= c && c <= 'f')? ('a' - 10):
						 ('A' - 10));
		val = val*base + i;
		++s;
	}
	return(val);
}

fprintmap( pF, base, pK )
FILE *pF;
keymap_t *pK;
int base;
{
	int i,j;
	char flg_char;
	
	checkmap(pK);
	fprintf(pF,"#                                                        alt\n");
	fprintf(pF,"# scan                      cntrl          alt    alt    cntrl   lock\n");
	fprintf(pF,"# code  base  shift  cntrl  shift   alt    shift  cntrl  shift   state\n");
	for( i = 0 ; i < pK->n_keys; i++ ){
		fprintbase(pF, base, i);
		for( j = 0 ; j < 8 ; j++ ){
			fprintval( pF, pK->key[i].map[j], ((pK->key[i].spcl)>>(7-j))&1 );
		}
		switch( pK->key[i].flgs ){
		case L_O: flg_char ='O'; break;
		case L_C: flg_char ='C'; break;
		case L_N: flg_char ='N'; break;
		}
		fprintf(pF,"  %c\n", flg_char);
	}
}

fprintbase( pF, base, val)
FILE *pF;
int base, val;
{
	int i;
	char buf[6], *bp; 

	if(base == 10) {
		fprintf(pF, "%d  ", val );
		return;
	} 
	bp = buf;
	if ( base == 16 ) { 
		sprintf(bp,"%x",val );
		for ( i=0; i <4; i++ ) {
			buf[5-i] = buf[3-i];	
		}
		buf[1]=120; /* 'x' in decimal */
	} else {
		sprintf(bp,"%o",val );
		for ( i=0; i <5; i++ ) {
			buf[5-i] = buf[4-i];	
		}
	}
	buf[0]=48;  /* '0' in decimal */
	fprintf(pF,"%s  ", buf);
}

char *ascnames[] = {
"nul","soh","stx","etx","eot","enq","ack","bel",
"bs","ht","nl","vt","np","cr","so","si",
"dle","dc1","dc2","dc3","dc4","nak","syn","etb",
"can","em","sub","esc","fs","gs","rs","ns",
"' '","'!'","'\"'","'#'","'$'","'%'","'&'","'\\''",
"'('","')'","'*'","'+'","','","'-'","'.'","'/'",
"'0'","'1'","'2'","'3'","'4'","'5'","'6'","'7'",
"'8'","'9'","':'","';'","'<'","'='","'>'","'?'",
"'@'","'A'","'B'","'C'","'D'","'E'","'F'","'G'",
"'H'","'I'","'J'","'K'","'L'","'M'","'N'","'O'",
"'P'","'Q'","'R'","'S'","'T'","'U'","'V'","'W'",
"'X'","'Y'","'Z'","'['","'\\\\'","']'","'^'","'_'",
"'`'","'a'","'b'","'c'","'d'","'e'","'f'","'g'",
"'h'","'i'","'j'","'k'","'l'","'m'","'n'","'o'",
"'p'","'q'","'r'","'s'","'t'","'u'","'v'","'w'",
"'x'","'y'","'z'","'{'","'|'","'}'","'~'","del"
};

/* L001
 *
 * Added panic and alock to fnames array. Note that there is a necessary NULL
 * in between them where the K_FLV (whatever that is) special key code was in
 * UW 2.1.
 */
char *fnames[]={
"nop","soh","lshift","rshift","clock","nlock","slock",
"alt","btab","ctl","lalt","ralt","lctrl","rctrl","agr","klock",
"panic","","alock"
};
char *fnames2[]={
"sysreq","break","fN","fO","f[","reboot","debug","nscr","pscr",
"fnscrn","fpscrn"
};

fprintval( pF, val, flag)
FILE *pF;
int val, flag;
{
#ifdef DEBUG
	if (dmp >= 0 ) {
		fprintf(pF, "0x%02x   ", val );
		return;
	}
#endif
	if( flag == 1 ){
		if( val <= K_ALK )		/* L001 */
			fprintf(pF, "%-7s", fnames[val]);
		else if( val < K_FUNF )
			fprintf(pF, "%-7s", ascnames[val]);
		else if( val <= K_FUNL )
			fprintf(pF, "fkey%02d ", val-KF );
		else if( val < K_VTF )
			fprintf(pF, "%-7s", fnames2[val - K_SRQ]);
		else 
			fprintf(pF, "scr%02d  ", val - K_VTF );
	} else {
		if( val < 128 )
			fprintf(pF, "%-7s", ascnames[val]);
		else 
			fprintf(pF, "0x%02x   ", val );
	}
}
checkmap(pK)
keymap_t *pK;
{
   short  *xlation_table;
   keymap_t *new_kmap;
   keyinfo_t *src_key, *dst_key;
   char msg[100];
   int i, j, k, nkeys, usl_index, sco_index;
   int assumeusl = 1, sco_nop_flg = 1, usl_nop_flg = 0;

#ifdef DEBUG
    dmpline(pK);
#endif

    if ( uslmap )  {	/* -U was used as override */
   	fprintf(stderr,"Using USL interpretation. -U used to override\n");
        return;
    }
    k= 0;
    /* for a sco map at least one value in the 8 states of keys 
	in the indices that are all K_NOP for a usl map must be 
	a value other than K_NOP */
    while ((i = usl_nop_ind[k++]) != -1 ) {
	for( j = 0 ; j < 8 ; j++ ){
	    if (pK->key[i].map[j] == K_NOP) continue;
	       usl_nop_flg++;
	       break; 
	     
	}
   }
   /* for a sco map values for all 8 states of the keys in
	indices that are all K_NOP must be all K_NOP */
   k = 0;
   while ((i = sco_nop_ind[k++]) != -1 ) {
      for( j = 0 ; j < 8 ; j++ ){
	    if(pK->key[i].map[j] == K_NOP) continue;
		sco_nop_flg--;
		 break; 
      }
   }
	/* for a sco map number of keys must be > 128 */
   if ( pK->n_keys > 128 && usl_nop_flg && sco_nop_flg == 1) assumeusl--; 
   if (  assumeusl ) {
      if ( scomap ) {
	 fprintf(stderr,"Using SCO interpretation for a USL map.\n");
	 fprintf(stderr,"-S option was used to override.\n");
      }
   }
   else { /* !assumeusl */
	fprintf(stderr,"Using SCO interpretation.\n");
        if ( ! scomap )
	    fprintf(stderr,"No override was set.\n");
   }
   if ( !assumeusl || scomap ) {	/*swap indices */

	/* This logic must be the same as that in the driver.
	   The file is kern-io:ws/ws_tables.c	*/

        new_kmap = (keymap_t * ) malloc(sizeof(keymap_t));
	src_key = pK->key;
	dst_key = new_kmap->key;
	for(i=0; i<USL_TABLE_SIZE; i++)
   	     memcpy(&dst_key[i], &src_key[i], sizeof(keyinfo_t));

	if ( ! kern ) {		/* download the map in USL format */
	   new_kmap->n_keys = USL_TABLE_SIZE;
	   for (i=0; usl_sco_extkeys[i].escape_code >= 0; i++) {
	       usl_index = usl_sco_extkeys[i].usl_ext_key;
	       sco_index = usl_sco_extkeys[i].sco_ext_key;
	       if(usl_index >= 0 && sco_index >= 0 )
   	    	     memcpy(&dst_key[usl_index], &src_key[sco_index],
					sizeof(keyinfo_t));
	   }
	}
	else {		/* convert the map read, into SCO format */
	     new_kmap->n_keys = SCO_TABLE_SIZE;
	     for(i=USL_TABLE_SIZE; i<SCO_TABLE_SIZE; i++)
	        memcpy(&dst_key[i],&nop_key, sizeof(keyinfo_t));
	     for (i=0; usl_sco_extkeys[i].escape_code >= 0; i++) {
	         usl_index = usl_sco_extkeys[i].usl_ext_key;
	         sco_index = usl_sco_extkeys[i].sco_ext_key;
	         if (usl_index >= 0 && sco_index >= 0 )
	            memcpy(&dst_key[usl_index],&src_key[sco_index],
					sizeof(keyinfo_t));
	     }
	}
	pK= new_kmap;
   }
#ifdef DEBUG
   dmpline(pK);
#endif
}
