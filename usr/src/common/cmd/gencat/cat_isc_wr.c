#ident	"@(#)gencat:cat_isc_wr.c	1.3"
#ident	"$Header$"
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <nl_types.h>
#include <unistd.h>
#include <pfmt.h>
#include "gencat.h"

#define MODES 0666

extern int list;
extern FILE *tempfile;
extern struct cat_set *sets;
extern int show_flag;
extern int errno;

/*  Function to dump an ISC format message catalogue
 */
void
cat_isc_dump(catalogue)
char *catalogue;
{
	register int i;		/*  Loop variables  */
	register int j;
	int fd;				/*  File descriptor for o/p file  */
	int msg_tot = 0;	/*  Total messages in catalogue  */
	int set_tot = 0;	/*  Total sets in catalogue  */

	off_t mhd_off;		/*  Offset in file for current message header  */
	off_t mhd_base;		/*  Offset for start of message headers  */
	off_t msg_off;		/*  Offset for actual messages  */
	off_t msg_cur;		/*  Temp, in case -1  */
	off_t shd_off;		/*  Offset for setinfo structures  */

	char t_str[NL_TEXTMAX];	/*  Temporary message string  */
	char t_len[2];		/*  Length of message (must be two bytes)  */

	static struct catheader fhd;	/*  Catalogue header  */
	struct setinfo shd;		/*  Set header  */

	struct cat_set *t_sets;	/*  Pointer to sets info linked list  */
	struct cat_msg *t_msg;	/*  Pointer to message info linked list  */

	fd = init_cat(catalogue);

	/*  Set magic number  */
	fhd.magicnum = ISC_MAGIC;

	/*  If we've read an existing ISC file, we maintain the value
	 *  of showflag, otherwise we set it to the default value.
	 */
	if (show_flag == 0)
		fhd.showflag = SHOW_NO;		/*  Default is SHOW_NO  */
	else
		fhd.showflag = show_flag;

	/*  Write out the header to the file  */
	if (write(fd, &fhd, sizeof(struct catheader)) < sizeof(struct catheader))
		fatal(":184:gencat: write of output file header failed\n");

	t_sets = sets;	/*  Initialise sets pointer  */

	/*  Fortunately the linked list of sets and messages is sorted by the
	 *  input routines.  We scan down this list creating set headers in
	 *  the output file as we go and filling in the information we have.
	 */
	while (t_sets != NULL)
	{
		/*  Record the set number  */
		set_tot = (unsigned short)t_sets->set_nr;

		/*  Scan the message list for this set, recording the longest 
		 *  string, the total length of the messages and the highest message
		 *  number.
		 */
		t_msg = t_sets->set_msg;

		while (t_msg != NULL)
		{
			/*  Record the message number  */
			shd.num_msgs = t_msg->msg_nr;

			t_msg = t_msg->msg_next;
		}

		/*  We also need to record the total number of messages in
		 *  all the sets, so that we can calculate the file offsets later
		 */
		msg_tot += shd.num_msgs;

		/*  Move to the correct place in the file for this set header and
		 *  write it out.
		 */
		shd_off = sizeof(struct catheader) + (2 * sizeof(int)) +
			((t_sets->set_nr - 1) * sizeof(struct setinfo));

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (write(fd, &shd, sizeof(struct setinfo)) < sizeof(struct setinfo))
			fatal(":186:gencat: write of set header failed\n");

		/*  Advance sets pointer  */
		t_sets = t_sets->set_next;
	}

	/*  Seek back to the position in the file where the highest set number
	 *  and number of messages goes.
	 */
	if (lseek(fd, (long)sizeof(struct catheader), SEEK_SET) < 0)
		fatal(":185:gencat: lseek in file failed\n");

	/*  Write the highest set number an the number messages to the file
	 */
	if (write(fd, &set_tot, sizeof(int)) < sizeof(int))
		fatal(":187:gencat: write of message header failed\n");

	if (write(fd, &msg_tot, sizeof(int)) < sizeof(int))
		fatal(":187:gencat: write of message header failed\n");

	/*  Set up the file offsets for the message pointer array and the
	 *  message text array.
	 */
	mhd_base = shd_off + sizeof(struct setinfo);
	msg_off = mhd_base + (msg_tot * sizeof(int));

	/*  Now loop through all the sets creating the message pointers and
	 *  writing the messages to the file.
	 */
	t_sets = sets;

	for (j = 1; j <= set_tot; j++)
	{
		shd_off = sizeof(struct catheader) + (2 * sizeof(int)) +
			((j - 1) * sizeof(struct setinfo));

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (read(fd, &shd, sizeof(struct setinfo)) < sizeof(struct setinfo))
			fatal(":188:gencat: read back of set header failed\n");

		if (shd.num_msgs == 0)		/*  Empty set  */
			shd.msgloc_offset = -1;
		else
			shd.msgloc_offset = mhd_base;

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (write(fd, &shd, sizeof(struct setinfo)) < sizeof(struct setinfo))
			fatal(":186:gencat: write of set header failed\n");

		/*  If this is an empty set, then we don't need to do anything more  */
		if (shd.num_msgs == 0)
			continue;

		t_msg = t_sets->set_msg;

		/*  Set up the message headers for this set  */
		for (i = 1; i <= (int)shd.num_msgs; i++)
		{
			/*  Calculate the message header offset for this message  */
			mhd_off = mhd_base + (sizeof(int) * (i - 1));

			/*  If this is a non-existent message, we set the message
			 *  location value to -1
			 */
			if (i < t_msg->msg_nr)
				msg_cur = -1;
			else
				msg_cur = msg_off;

			/*  Seek to message header position and write header  */
			if (lseek(fd, mhd_off, SEEK_SET) < 0)
				fatal(":185:gencat: lseek in file failed\n");

			if (write(fd, &msg_cur, sizeof(int)) < sizeof(int))
				fatal(":187:gencat: write of message header failed\n");

			/*  If there is no message here we continue the loop  */
			if (i < t_msg->msg_nr)
				continue;

			/*  Extract the message text from the temporary file used by the
			 *  rest of gencat.
			 */
			if (fseek(tempfile, t_msg->msg_off, 0) < 0)
				fatal(":189:gencat: file seek failed\n");

			if (fgets(t_str, NL_TEXTMAX-1, tempfile) == NULL)
				fatal(":190:gencat: Read of temp file failed\n");

			/*  Tell user what is happening, if the list option is 
			 *  switched on.
			 */
			if (list)
				pfmt(stdout, MM_NOSTD, ":196:Set %d,Message %d,Length %d\n%.*s\n*\n",
					j, i, strlen(t_str), strlen(t_str), t_str); 

			/*  Seek to position for actual message and write it  */
			if (lseek(fd, msg_off, SEEK_SET) < 0)
				fatal(":185:gencat: lseek in file failed\n");

			/*  First we have the two byte length of message.
			 *  We use a two char array, as this is simpler than
			 *  checking that a short really is two bytes and then
			 *  doing something like this if it isn't
			 */
			t_len[0] = (t_msg->msg_len & 0xFF00) >> 8;
			t_len[1] = t_msg->msg_len & 0x00FF;

			if (write(fd, t_len, 2) < 2)
				fatal(":191:gencat: write of message failed\n");

			/*  Then we have the message itself  */
			if (write(fd, t_str, (unsigned int)t_msg->msg_len) < t_msg->msg_len)
				fatal(":191:gencat: write of message failed\n");

			/*  Set pointer for messages in file to end of this message  */
			msg_off += (t_msg->msg_len + 2);

			t_msg = t_msg->msg_next;
		}

		mhd_base += (shd.num_msgs * sizeof(int));
		t_sets = t_sets->set_next;
	}

	close(fd);
	return;		/*  Just for lint  */
}
int
init_cat(catalogue)
char *catalogue;
{
	int fd;		/*  Catalogue file descriptor  */

	/*  Make sure we hae a clean catalogue  */
	unlink(catalogue);

	/*  Open the output file  */
	if ((fd = open(catalogue, O_RDWR | O_CREAT, MODES)) < 0)
	{
		pfmt(stderr, MM_ERROR, ":197:cannot create catalogue %s\n", catalogue);
		exit(1);
	}

	return(fd);
}
