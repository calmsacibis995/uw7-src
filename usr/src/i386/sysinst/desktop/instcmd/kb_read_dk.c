#ident	"@(#)kb_read_dk.c	15.1"

/*		copyright	"%c%" 	*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "kb_remap.h"

/**
 *  Function to read in and process the dead key information
 **/
void
read_dk_map(char *config_file, char *dk_buffer)
{
	register int i;		/*  Loop variables  */
	register int j;
	int dk_seq_offset;	/*  Offset to key sequence information  */
	int nkeys = 0;		/*  Number of keys counter  */

	char *dk_comp_ptr;	/*  Pointer to null compose table  */
	char *dk_seq_ptr;	/*  Where to store second key/character sequence  */
	char *dk_tbl_ptr;	/*  Where to store dead key value  */
	char *file_data;	/*  Data read from file  */
	char *ndead_ptr;	/*  Where to store number of dead keys  */
	char *ptr;			/*  Working pointer  */

	DK_HDR *dk_hdr;		/*  Header struct of dead keys file  */
	DK_INFO *dk_info;	/*  Info on dead key  */
	DK_COMBI *dk_combi;	/*  Dead key combination  */

	file_data = file_read(config_file);		/*  Read data from file  */
	dk_hdr = (DK_HDR *)file_data;			/*  Set pointer to header  */
	ptr = file_data + sizeof(DK_HDR);		/*  Set working pointer  */

	/*  Check the 'magic' number to make sure this is a valid file  */
	if (strncmp((char *)dk_hdr->dh_magic, DH_MAGIC, strlen(DH_MAGIC)) != 0)
	{
		fprintf(stderr, "kb_remap: Bad magic number in dead keys file\n");
		fatal();
	}

	/*  The layout of the channel mapping data is real weird.
	 *  The diagram below should make it obvious.
	 *
	 *    0  +-------------------------------------------------------+
	 *       |   Input map (in our case just the character number)   |
	 *       |   dead keys are set to zero                           |
	 *       +-------------------------------------------------------+
	 *
	 *  256  +-------------------------------------------------------+
	 *       |   Output map (just the character number)              |
	 *       +-------------------------------------------------------+
	 *
	 *  512  +-----------------------+
	 *       | compose key (0)       |
	 *       +-----------------------+
	 *
	 *  513  +-----------------------+
	 *       | beep flag (0)         |
	 *       +-----------------------+
	 *
	 *  514  +--------------------------+
	 *       | Offset of compose table  |   (points at an 'empty' entry)
	 *       +--------------------------+
	 *
	 *  516  +--------------------------+
	 *       | Offset of dead key table |
	 *       +--------------------------+
	 *
	 *  518  +--------------------------+
	 *       | Offset of strings table  |   (end of the dead key sequences)
	 *       +--------------------------+
	 *
	 *  520  +--------------------------+
	 *       | Offset of strings buffer |   (end of the dead key sequences)
	 *       +--------------------------+
	 *
	 *  522  +--------------------------+
	 *       | dead key table           |
	 *       +--------------------------+
	 *
	 *  522+(2*number dead)  +--------------------------+
	 *                       | compose key table        | (One entry for us)
	 *                       +--------------------------+
	 *
	 *  522+(2*number dead)+2  +--------------------------+
	 *                         | dead key sequence table  |
	 *                         +--------------------------+
	 */

	/*  First fill in the mappings (default to start)  */
	for (i = 0; i <= 0xFF; i++)
	{
		*(dk_buffer + i) = i;
		*(dk_buffer + 0xFF + 1 + i) = i;
	}

	dk_tbl_ptr = dk_buffer + DK_TBL_OFFSET;
	dk_seq_offset = DK_TBL_OFFSET + (2 * dk_hdr->dh_ndead);
	dk_comp_ptr = dk_buffer + dk_seq_offset;

	/*  Write the compose key offset in the appropriate place - this is an
	 *  empty table, but we have to put the pointer in.
	 */
	insert_num(dk_seq_offset, dk_buffer + DK_COMP_OFFSET);

	/*  Now we need to bump the dk_seq_ptr by two to push it past the 
	 *  null compose key table.
	 */
	dk_seq_offset += 2;
	dk_seq_ptr = dk_buffer + dk_seq_offset;

	/*  Write the dead key sequence offset in the appropriate place  */
	insert_num(dk_seq_offset, dk_buffer + DK_SEQ_OFFSET);

	/*  Loop through the dead key information from the input file  */
	for (i = 0; i < (int)dk_hdr->dh_ndead; i++)
	{
		dk_info = (DK_INFO *)ptr;	/*  Set pointer to info structure  */
		ptr += sizeof(DK_INFO);		/*  Reposition working pointer  */

		/*  First put the dead key character in the right place  */
		*dk_tbl_ptr++ = dk_info->di_key;
		*dk_tbl_ptr++ = nkeys;

		/*  reset the input mapping for this key  */
		*(dk_buffer + dk_info->di_key) = 0;

		/*  Bump up the number of keys  */
		nkeys += dk_info->di_ncombi;

		/*  Now fill in the valid combinations for this key  */
		for (j = 0; j < (int)dk_info->di_ncombi; j++)
		{
			dk_combi = (DK_COMBI *)ptr;	/*  Set pointer to combo structure  */
			ptr += sizeof(DK_COMBI);	/*  Reposition working pointer  */
			*dk_seq_ptr++ = dk_combi->dc_orig;
			*dk_seq_ptr++ = dk_combi->dc_result;
			dk_seq_offset += 2;
		}
	}

	/*  Finally fill in the pointers to the compose table and the
	 *  strings table/buffer
	 */
	*dk_comp_ptr++ = (char)0;
	*dk_comp_ptr = nkeys;
	insert_num(dk_seq_offset, dk_buffer + DK_S_TBL_OFFSET);
	insert_num(dk_seq_offset, dk_buffer + DK_S_BUF_OFFSET);

	/*  Free the space used for the file  */
	free(file_data);
}

/**
 *  Function to fill in a 16 bit number into two bytes of the array
 **/
void
insert_num(num, array)
int num;
char *array;
{
	*array = num & 0xFF;
	*(array + 1) = num >> 8;
}
