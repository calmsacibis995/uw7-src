#ident	"@(#)keycomp.c	15.1"

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ascii.h>
#include <sys/kd.h>
#include "keycomp.h"
#include "key_remap.h"

void usage();

extern int errno;

main(argc, argv)
int argc;
char *argv[];
{
	register int i;
	register int j;
	int key_count = 0;		/*  Number of different keys  */
	int ifd;				/*  Standard keyboard file descriptor  */
	int ofd;				/*  Compiled file descriptor  */

	char keys[NUM_STATES];	/*  Keys that have changed  */

	static keymap_t std_keymap;	/*  Standard keyboard layout  */
	static keymap_t new_keymap;	/*  New keyboard layout  */

	KEY_HDR key_hdr;		/*  Keyboard file header struct  */
	KEY_INFO key_info;		/*  Key info structure  */
	FILE *ifp;				/*  Input file pointer  */

	/*  Check that we have at least two arguments, i.e. the name of the 
	 *  file that we are compiling, and the name of the output file.
	 */
	if (argc < 3)
		usage();

	/*  Check that we can access the input file  */
	if (access(argv[1], R_OK) < 0)
	{
		printf("Unable to access input file, %s\n", argv[1]);
		exit(1);
	}

	/*  Open the input file for reading  */
	if ((ifp = fopen(argv[1], "r")) == NULL)
	{
		printf("Unable to read input file\n");
		exit(1);
	}

	/*  Open the output file for writing  */
	if ((ofd = open(argv[2], O_WRONLY | O_CREAT, 0644)) < 0)
	{
		printf("Unable to open %s for writing (errno = %d)\n", argv[2], errno);
		perror("keycomp");
		exit(1);
	}

	/*  Read in the standard format keyboard map  */
	if (access(STD_MAP, R_OK) < 0)
	{
		printf("Unable to find standard keyboard file, %s\n", STD_MAP);
		exit(1);
	}

	if ((ifd = open(STD_MAP, O_RDONLY)) < 0)
	{
		printf("Unable to open standard keyboard file, %s\n", STD_MAP);
		exit(1);
	}

	if (read(ifd, &std_keymap, sizeof(keymap_t)) < sizeof(keymap_t))
	{
		printf("Standard keyboard map file, %s, corrupted\n");
		exit(1);
	}

	close(ifd);
	fparsemap(ifp,&new_keymap);

	for (i = 0; i < new_keymap.n_keys; i++)
	{
		for (j = 0; j < NUM_STATES; j++)
		{
			if (std_keymap.key[i].map[j] != new_keymap.key[i].map[j])
			{
				++key_count;
				break;
			}
		}
	}

	/*  Setup header structure for keyboard file  */
	strcpy((char *)key_hdr.kh_magic, KH_MAGIC);
	key_hdr.kh_nkeys = key_count;

	if (write(ofd, &key_hdr, sizeof(KEY_HDR)) < sizeof(KEY_HDR))
	{
		printf("Unable to write header to file\n");
		exit(1);
	}

	for (i = 0; i < new_keymap.n_keys; i++)
	{
		key_info.ki_states = 0;
		key_count = 0;

		for (j = 0; j < NUM_STATES; j++)
		{
			if (std_keymap.key[i].map[j] != new_keymap.key[i].map[j])
			{
				key_info.ki_states |= 1 << j;
				keys[key_count++] = new_keymap.key[i].map[j];
			}
		}

		if (key_info.ki_states != 0)
		{
			key_info.ki_scan = i;
			key_info.ki_spcl = new_keymap.key[i].spcl;
			key_info.ki_flgs = new_keymap.key[i].flgs;

			if (write(ofd, &key_info, sizeof(KEY_INFO)) < sizeof(KEY_INFO))
			{
				printf("Unable to write key header to file \n");
				exit(1);
			}

			if (write(ofd, &keys, key_count) < key_count)
			{
				printf("Unable to write key data to file\n");
				exit(0);
			}
		}
	}
	return 0;
}

fparsemap(pF,pK)
keymap_t *pK;
FILE *pF;
{
	int i;
	int Nscan;
	int flag;

	static char aCtok[8];
	static char aClock[2];
		
	fscanblanks(pF);

	while (!feof(pF))
	{
		fscantok(pF, aCtok);
		Nscan = atoi(aCtok);
		fscanblanks(pF);

		if(Nscan > pK->n_keys - 1)
			pK->n_keys = Nscan + 1;

		pK->key[Nscan].spcl = 0;
	
		for (i = 0; i < 8; i++)
		{
			fscantok(pF,aCtok);
			pK->key[Nscan].map[i] = translate(aCtok, &flag);
			pK->key[Nscan].spcl |= flag << (7 - i);
			fscanblanks(pF);
		}
	
		fscanblanks(pF);
		fscantok(pF, aClock);

		switch(aClock[0]){
			case 'O':
				pK->key[Nscan].flgs = L_O;
				break;

			case 'C':
				pK->key[Nscan].flgs = L_C;
				break;

			case 'N':
				pK->key[Nscan].flgs = L_N;
				break;
		}
	
		fscanblanks(pF);
	}
}

fscantok(pF, pC)
FILE *pF;
char *pC;
{
	*pC = getc(pF);

	if('\'' == *pC)
	{
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
		  case 's': return A_ESC;
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
	case 'k': return K_KLK;
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
	case 'p': *pflag = 1; return K_PREV;
	case 'r': switch( pC[1] ){
		  case 'a': *pflag=1; return K_RAL;
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

/**
 *  Correct usage for this command
 **/
void
usage()
{
	printf("Usage: keycomp <config_file> <output_file>\n");
	exit(1);
}
