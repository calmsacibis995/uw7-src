#ident	"@(#)kb_read_font.c	15.1"

/*		copyright	"%c%" 	*/

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
read_font_map(char *config_file, rom_font_t *font_data)
{
	register int i;		/*  Loop variables  */

	char *file_data;	/*  Data read from file  */

	CS_HDR *cs_hdr;		/*  Code set header structure  */

	file_data = file_read(config_file);     /*  Read data from file  */
	cs_hdr = (CS_HDR *)file_data;			/*  Set pointer to header  */

	/*  Check the 'magic' number to make sure this is a valid file  */
	if (strncmp((char *)cs_hdr->ch_magic, CS_MAGIC, strlen(CS_MAGIC)) != 0)
    {
		fprintf(stderr, "kb_remap: Bad magic number in code set file\n");
		fatal();
	}

	/*  Check that the version is also correct  */
	if (cs_hdr->ch_vers != (unchar)CS_VERSION)
	{
		fprintf(stderr, "kb_remap: Bad version number in code set file\n");
		fatal();
	}

	font_data->fnt_numchar = NUM_FONT_CHARS;

	/*  Filling in the font information is real simple - we just loop through
	 *  the 128 characters defined in the file and copy the data into the 
	 *  rom_font_t structure.
	 */
	for (i = 0; i < NUM_FONT_CHARS; i++)
	{
		/*  Save the character index  */
		font_data->fnt_chars[i].cd_index = i + NUM_FONT_CHARS;

		/*  Copy the ega font description  */
		memcpy(font_data->fnt_chars[i].cd_map_8x14, 
			cs_hdr->ch_cset.cs_ega[i].ec_bitmap, F8x14_BPC);

		/*  Copy the vga font description - we actually copy this into 
		 *  two places for the 8x16 and 9x16 fonts, which we treat as the
		 *  same here.
		 */
		memcpy(font_data->fnt_chars[i].cd_map_8x16,
			cs_hdr->ch_cset.cs_vga[i].vc_bitmap, F8x16_BPC);
		memcpy(font_data->fnt_chars[i].cd_map_9x16,
			cs_hdr->ch_cset.cs_vga[i].vc_bitmap, F9x16_BPC);
	}

	free(file_data);
}
