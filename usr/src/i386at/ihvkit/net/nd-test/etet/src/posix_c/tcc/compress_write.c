/*
 *  set tabsize=4
 *
 *  NAME
 *	compress_write.c
 *
 *  SYNOPSIS
 *
 *  DESCRIPTION 
 *	This program compresses any data string passed to it.
 *  Uses a simple algorithm that it rotates each character by a certain number
 *  of bits depending on the position of the character with respect to the start
 *  of the string as follows:
 *       Position of character		Right shift by
 *	 ---------------------		--------------
 *		1				1
 *		2				2
 *		3				3
 *		.				.
 *		7				7
 *		8				1
 *		9				2
 *		10				3
 *		.				.
 *		.				.
 *  NOTE:
 *	 Since rotating a character by 8 bits is equivalent to no rotation at
 *	 all, every time we come to 7 we initialize back to 1.  
 *  
 *  The program also records a checksum in the form of total number of 
 *  characetrs in the string.
 *  
 *  In essence, this program is as per the requirements of Novell Certification
 *  Labs., which is to avoid any tampering of the test results.
 *  
 *  
 *  CAVEATS
 *
 *  NOTES
 *
 *  SEE ALSO
 *	$TET_SUITE_ROOT/common/lib/exp.c
 *
 *  MODIFICATION HISTORY
 *	Created By :  Yule R. Kingston, Wipro Infotech Limited.
 *	Date       :  April 30, 1994
 *
 */

#include <tcc_env.h>
#include <tcc_mac.h>
#include <tet_jrnl.h>
#include <tcc_prot.h>

compress_write(out_fd,str)
int  out_fd;
char *str;
{
        int bit_shift,bptr,loop;
        unsigned char buf[1024];
        unsigned char msb;
        unsigned char tchar;

        bit_shift=1;
        bptr=0;
        while(*str) {
                tchar = (unsigned char)(*str);
                for(loop=1;loop<=bit_shift;loop++) {
                        msb = (tchar >> 7);
                        tchar = ( (tchar << 1) | msb );
                }
                buf[bptr] = tchar;
                if (bit_shift == 7)
                        bit_shift = 1;
                else
                        bit_shift++;
                bptr++;
                str++;
        }
	if ( (int)write(out_fd, &bptr,sizeof(bptr)) != sizeof(bptr)) {
                (void)fprintf(stderr,"Error while writing %d bytes(ERROR:%d) into journal file.\n",
                        sizeof(bptr),errno);
        }
        if ( (int)write(out_fd, buf, bptr) != (bptr) ) {
                (void)fprintf(stderr,"Error while writing %d bytes(ERROR:%d) into journal file.\n",
                        bptr,errno);
                return(-1);
        }
}

