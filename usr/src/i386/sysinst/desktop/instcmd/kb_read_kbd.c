#ident	"@(#)kb_read_kbd.c	15.1"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/kd.h>
#include "kb_remap.h"

/**
 *  NOTE: If you don't like the layout of this file, set tabstops to 4
 **/

/**
 *  Function to read in the Keyboard configuration file
 **/
void
read_kb_map(char *config_file, keymap_t *key_map)
{
	register int i;		/*  Loop variables  */
	register int j;	

	char *ptr;			/*  Working pointer  */
	char *file_data;	/*  Data read from file  */

	KEY_HDR *kb_hdr;	/*  Pointer to header structure  */
	KEY_INFO *kb_info;	/*  Pointer to key information  */

	file_data = file_read(config_file);		/*  Read data from file  */
	kb_hdr = (KEY_HDR *)file_data;			/*  Set pointer to header  */
	ptr = file_data + sizeof(KEY_HDR);		/*  Reposition working pointer  */

	/*  Check the 'magic' number in the header information to
	 *  make sure that nobody has slipped us a Mickey-Finn
	 */
	if (strncmp((char *)kb_hdr->kh_magic, KH_MAGIC, strlen(KH_MAGIC)) != 0)
	{
		fprintf(stderr, "kb_remap: Bad magic number in keyboard file (%s != %s)\n", kb_hdr->kh_magic, KH_MAGIC);
		fatal();
	}

#ifdef DEBUG_KB
	printf("Processing %d keys\n", kb_hdr->kh_nkeys);
#endif

	/*  Now we process each key information structure, filling in
	 *  the keyboard map as we go.
	 */
	for (i = 0; i < (int)kb_hdr->kh_nkeys; i++)
	{
		kb_info = (KEY_INFO *)ptr;	/*  Set pointer to info structure  */
		ptr += sizeof(KEY_INFO);	/*  Reposition working pointer  */

#ifdef DEBUG_KB
		printf("state = %X\n", kb_info->ki_states);
#endif
		/*  The information on which states of the key change are 
		 *  kept in a bit-map, so we need to loop though this setting
		 *  the ones that need to be.
		 */
		for (j = 0; j < NUM_STATES; j++)
		{
			/*  If the bit is set in the mask, take the value for the 
			 *  key from *ptr (also moving ptr on to the next one)
			 */
			if (kb_info->ki_states & (1 << j))
			{
#ifdef DEBUG_KB
				printf("Setting key %d, state %d to %d (%c)\n",
					kb_info->ki_scan, j, *ptr, *ptr);
#endif
				key_map->key[kb_info->ki_scan].map[j] = *(ptr++);
			}
			else
				continue;
		
			/*  Record the special value for this key.
			 */
			key_map->key[kb_info->ki_scan].spcl = kb_info->ki_spcl;

			/*  Record the flags value for this key.  This will determine
			 *  how CAPS LOCK etc. affect the key.
			 */
			key_map->key[kb_info->ki_scan].flgs = kb_info->ki_flgs;
		}
	}

	/*  Release the memory used for the file data  */
	free(file_data);
}
