#ident	"@(#)gencat:cat_isc_rd.c	1.3"
#ident	"$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <nl_types.h>
#include <unistd.h>
#include <pfmt.h>
#include "gencat.h"

extern int list;
extern int show_flag;
extern char msg_buf[]; 
extern FILE *tempfile;
extern struct cat_set *sets;

void
cat_isc_build(fd)
FILE *fd;
{
	register int i;
	register int j;
	int mhd;			/*  Position of message in file  */
	int num_sets;
	int num_msgs;
	int shd_fpos;		/*  Current set header position in file  */
	int mhd_fpos;		/*  Current message header position in file  */

	unsigned char mlen_str[3];	/*  Two character string at atart of message  */

	struct catheader chd;
	struct setinfo shd;

	struct cat_set *set_ptr;

	struct cat_msg *msg_ptr;

	/*  Rewind the filepointer so we can read in the whole header
	 *  structure
	 */
	rewind(fd);

	if (fread(&chd, sizeof(struct catheader), 1, fd) != 1)
		fatal(":174:Bad read of file header\n");

	show_flag = chd.showflag;	/*  Set showflag  */

	/*  Now read in the number of sets and the number of messages  */
	if (fread(&num_sets, sizeof(int), 1, fd) != 1)
		fatal(":174:Bad read of file header\n");

	if (fread(&num_msgs, sizeof(int), 1, fd) != 1)
		fatal(":174:Bad read of file header\n");

	/*  Since we know the number of sets in the file, we can make a single
	 *  call to malloc to allocate all the required memory
	 */
	if ((sets = (struct cat_set *)malloc(sizeof(struct cat_set) * num_sets)) == NULL)
		fatal(":175:Out of memory\n");

	set_ptr = sets;

	/*  Process the sets  */
	for (i = 1; i <= num_sets; i++)
	{
		/* Read the set header  */
		if (fread(&shd, sizeof(struct setinfo), 1, fd) != 1)
			fatal(":176:Bad read of set header\n");

		if (shd.num_msgs == 0)	/*  Empty set  */
			continue;

		/*  Convert the ISC format info to the internal SVR4 representation  */
		set_ptr->set_nr = i;
		set_ptr->set_msg_nr = shd.num_msgs;

		/*  Although the set headers are contiguous we still need to fill
		 *  the set_next field for the linked list.
		 */
		if (i < num_sets)
			set_ptr->set_next = set_ptr + 1;
		else
			set_ptr->set_next = NULL;

		/*  Again allocate the memory for the messages in a contiguous lump  */
		if ((set_ptr->set_msg = (struct cat_msg *)malloc(sizeof(struct cat_msg) * shd.num_msgs)) == NULL)
			fatal(":175:Out of memory\n");

		msg_ptr = set_ptr->set_msg;

		/*  Remember the current file position  */
		shd_fpos = ftell(fd);

		/*  Seek to the start of the message headers for this set  */
		if (fseek(fd, shd.msgloc_offset, SEEK_SET) < 0)
			fatal(":177:Seek error, file corrupted?\n");

		/*  Read in the message header (message location) for each message  */
		for (j = 1; j <= shd.num_msgs; j++)
		{
			if (fread(&mhd, sizeof(int), 1, fd) != 1)
				fatal(":178:Bad message header read\n");

			/*  If the value of mhd is -1, we have no message  */
			if (mhd == -1)
				continue;

			/*  Save the file pointer position  */
			mhd_fpos = ftell(fd);

			/*  Due to the structure of the ISC catalogue, we now have to 
			 *  seek to the message, and read the first two bytes, which 
			 *  contains the length of the message.  We can then allocate 
			 *  space for the message and read it in.
			 */
			if (fseek(fd, (long)mhd, SEEK_SET) < 0)
				fatal(":179:Bad seek to message, file corrupted?\n");

			if (fread(mlen_str, sizeof(char), 2, fd) != 2)
				fatal(":180:Could not read message length, file corrupted?\n");

			/*  Fill in the cat_msg struct  */
			msg_ptr->msg_nr = j;
			msg_ptr->msg_off = ftell(tempfile);
			msg_ptr->msg_ptr = NULL;
			msg_ptr->msg_len = (unsigned)(mlen_str[1] + (mlen_str[0] << 8));

			/*  Read message from ISC file  */
			if ((unsigned)fread(msg_buf, sizeof(char),
				(unsigned)msg_ptr->msg_len, fd) < (unsigned)msg_ptr->msg_len)
				fatal(":181:Short read of message, file corrupted?\n");

			msg_buf[(unsigned)msg_ptr->msg_len] = NULL;

			/*  Set the next message pointer, unless this is the last one  */
			if (j < shd.num_msgs)
				msg_ptr->msg_next = msg_ptr + 1;
			else
				msg_ptr->msg_next = NULL;

			/*  Write message to temporary file  */
			if (fwrite(msg_buf, sizeof(char), (unsigned)msg_ptr->msg_len,
				tempfile) != (unsigned)msg_ptr->msg_len)
				fatal(":182:Write to temp file failed\n");

			/*  If list flag set, output info  */
			if (list)
				pfmt(stdout, MM_NOSTD, ":195:Set %d,Message %d,Offset %ld,Length %d\n%.*s\n*\n",
					i, j, msg_ptr->msg_off, msg_ptr->msg_len,
					(unsigned)msg_ptr->msg_len, msg_buf);

			/*  Set pointer to next element in linked list  */
			msg_ptr = msg_ptr->msg_next;

			/*  Reset file pointer to message header information  */
			if (fseek(fd, mhd_fpos, SEEK_SET) < 0)
				fatal(":183:Back seek failed!\n");
		}

		/*  Reset the file pointer to the next set header  */
		if (fseek(fd, shd_fpos, SEEK_SET) < 0)
			fatal(":183:Back seek failed!\n");

		if (set_ptr->set_next != NULL)
			set_ptr = set_ptr->set_next;
	}

	return;	/*  Just for lint  */
}
