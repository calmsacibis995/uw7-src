#ident	"@(#)gencat:cat_sco_rd.c	1.1.1.4"
#ident	"$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <nl_types.h>
#include <unistd.h>
#include <pfmt.h>
#include "gencat.h"

extern int list;
extern char msg_buf[];
extern FILE *tempfile;
extern struct cat_set *sets;

void
cat_sco_build(fd)
FILE *fd;
{
	register int i;
	register int j;

	long shd_fpos;
	long mhd_fpos;
	long mhd;
	long mhd2;
	long msg_len;

	struct msg_fhd fhd;
	struct msg_shd shd;

	struct cat_set *set_ptr;
	struct cat_set *o_set_ptr = NULL;

	struct cat_msg *msg_ptr;
	struct cat_msg *o_msg_ptr = NULL;

	/*  Because the SCO catalogue has a more complex header, we need to
	 *  reset the file pointer to the start of file and then read in this
	 *  header
	 */
	rewind(fd);
	sets = NULL;

	if (fread(&fhd, sizeof(struct msg_fhd), 1, fd) != 1)
		fatal(":174:Bad read of file header\n");

	for (i = 0; i < (int)fhd.mf_scnt; i++)
	{
		/*  Read the set header info - then check for an empty set.  If this
		 *  is the case, continue without further processing
		 */
		if (fread(&shd, sizeof(struct msg_shd), 1, fd) != 1)
			fatal(":176:Bad read of set header\n");

		if (shd.ms_flg == M_EMPTY)
			continue;

		/*  Initially, for proto-typing I'm going to do one malloc for 
		 *  each header.  I will then change this to do one for all of 
		 *  them.
		 */
		if ((set_ptr = (struct cat_set *)malloc(sizeof(struct cat_set))) == NULL)
			fatal(":175:Out of memory\n");

		/*  Update linked list  */
		if (sets == NULL)
			sets = set_ptr;
		else
			o_set_ptr->set_next = set_ptr;

		/*  Now translate the information in the SCO format set header into
		 *  SVR4 internal style info.
		 */
		set_ptr->set_nr = i + 1;
		set_ptr->set_msg_nr = shd.ms_mcnt;
		set_ptr->set_msg = NULL;
		set_ptr->set_next = NULL;

		/*  Remember the current file position  */
		shd_fpos = ftell(fd);

		/*  Seek to the start of the message headers for this set and read
		 *  them and the messages in
		 */
		if (fseek(fd, shd.ms_mhdoff, SEEK_SET) < 0)
			fatal(":177:Seek error, file corrupted?\n");

		/*  Read in the message headers (only a long really), and set up
		 *  the SVR4 message header linked list.  Obviously, for empty sets
		 *  this won't do anything.
		 */
		for (j = 0; j < (int)shd.ms_mcnt; j++)
		{
			/*  Read in the message header struct  */
			if (fread(&mhd, sizeof(long), 1, fd) != 1)
				fatal(":192:Read error, file corrupted?\n");

			/*  Save the file pointer position  */
			mhd_fpos = ftell(fd);

			/*  We now read the next msg header, so that we can compare
			 *  the two values.  This is to check for empty messages.
			 */
			if (fread(&mhd2, sizeof(long), 1, fd) != 1)
				fatal(":192:Read error, file corrupted?\n");

			msg_len = (int)(mhd2 - mhd);

			if (msg_len == 0)
			{
				if (fseek(fd, mhd_fpos, SEEK_SET) < 0)
					fatal(":183:Back seek failed!\n");

				continue;	/*  Nothing else to do  */
			}

			/*  Allocate space for this message  */
			if ((msg_ptr = (struct cat_msg *)malloc(sizeof(struct cat_msg))) == NULL)
				fatal(":175:Out of memory\n");

			/*  Add to linked list  */
			if (set_ptr->set_msg == NULL)
				set_ptr->set_msg = msg_ptr;
			else
				o_msg_ptr->msg_next = msg_ptr;

			/*  Fill in the information we have  */
			msg_ptr->msg_nr = j + 1;
			msg_ptr->msg_len = msg_len;

			/*  Move to the message and read it  */
			if (fseek(fd, mhd, SEEK_SET) < 0)
				fatal(":177:Seek error, file corrupted?\n");

			if (fread(msg_buf, sizeof(char), msg_len, fd) != msg_len)
				fatal(":192:Read error, file corrupted?\n");

			/*  Null terminate string (not sure if this is really
			 *  necessary)
			 */
			msg_buf[msg_len] = NULL;

			/*  Store the message in the temp file  */
			msg_ptr->msg_off = ftell(tempfile);

			if (fwrite(msg_buf, sizeof(char), msg_len, tempfile) != msg_len)
				fatal(":182:Write to temp file failed\n");

			/*  Reset the file pointer to the next message header  */
			if (fseek(fd, mhd_fpos, SEEK_SET) < 0)
				fatal(":183:Back seek failed!\n");

			/*  If list flag set, output info  */
			if (list)
				pfmt(stdout, MM_NOSTD, ":195:Set %d,Message %d,Offset %ld,Length %d\n%.*s\n*\n",
					i+1, j+1, msg_ptr->msg_off, msg_ptr->msg_len,
					msg_ptr->msg_len, msg_buf);

			/*  Remember the old pointer  */
			o_msg_ptr = msg_ptr;
		}

		/*  Seek back to the set next set header  */
		if (fseek(fd, shd_fpos, SEEK_SET) < 0)
			fatal(":183:Back seek failed!\n");

		/*  Remember the old pointer  */
		o_set_ptr = set_ptr;
	}

	return;	/*  Just for lint  */
}
