#ident	"@(#)gencat:cat_sco_wr.c	1.4"
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
extern int errno;

/*  New function to dump an SCO format message catalogue
 */
void
cat_sco_dump(catalogue)
char *catalogue;
{
	register int i;
	register int j;
	int fd;
	int msg_tot = 0;

	off_t mhd_off;
	off_t mhd_base;
	off_t msg_off;
	off_t shd_off;
	off_t nam_off = 0;

	char t_str[NL_TEXTMAX];

	static struct msg_fhd fhd;	/*  SCO file header struct  */

	struct cat_set *t_sets;	/*  Pointer to sets info linked list  */
	struct cat_msg *t_msg;	/*  Pointer to message info linked list  */
	struct msg_shd shd;		/*  set header - SCO */

	fd = init_cat(catalogue);

	/*  Set magic number and flags to M_PRELD | M_ALSET */
	fhd.mf_mag = M_MFMAG;
	fhd.mf_flg = M_PRELD | M_ALSET;

	/*  Before copying the name of the catalogue, need to check whether
	 *  the name is too long.
	 */
	if (strlen(catalogue) > (unsigned int)NAME_MAX)
	{
		pfmt(stderr, MM_WARNING, ":198:name of catalogue too long, truncated\n");
		catalogue[NAME_MAX] = NULL;
	}

	strcpy(fhd.mf_nam, catalogue);

	t_sets = sets;

	/*  Fortunately the linked list of sets and messages is sorted by the
	 *  inpput routines.  We scan down this list creating set headers in
	 *  the output file as we go and filling in the information we have.
	 */
	while (t_sets != NULL)
	{
		/*  Record the set number  */
		fhd.mf_scnt = (unsigned short)t_sets->set_nr;

		/*  Initialise the values in the structure  */
		shd.ms_flg = M_PRELD;
		shd.ms_mcnt = 0;
		shd.ms_psize = 0;
		shd.ms_msize = 0;
		shd.ms_discnt = 1;
		shd.ms_ncnt = 0;
		shd.ms_mhdoff = 0;
		shd.ms_msgoff = 0;
		shd.ms_namoff = 0;

		/*  Scan the message list for this set, recording the longest 
		 *  string, the total length of the messages and the highest message
		 *  number.
		 */
		t_msg = t_sets->set_msg;

		while (t_msg != NULL)
		{
			/*  Check for longer string  */
			if (t_msg->msg_len > (int)shd.ms_msize)
				shd.ms_msize = t_msg->msg_len;

			/*  Record the message number  */
			shd.ms_mcnt = t_msg->msg_nr;

			/*  Bump up total length  */
			shd.ms_psize += t_msg->msg_len;

			t_msg = t_msg->msg_next;
		}

		/*  We also need to record the total number of messages in
		 *  all the sets, so that we can calculate the file offsets later
		 */
		msg_tot += shd.ms_mcnt;

		/*  Move to the correct place in the file for this set header and
		 *  write it out.
		 */
		shd_off = sizeof(struct msg_fhd) +
			((t_sets->set_nr - 1) * sizeof(struct msg_shd));

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (write(fd, &shd, sizeof(struct msg_shd)) < sizeof(struct msg_shd))
			fatal(":186:gencat: write of set header failed\n");

		nam_off += shd.ms_psize;
		t_sets = t_sets->set_next;
	}

	/*  Bump up the msg total to account for the dummy entry
	 */
	++msg_tot;

	/*  Seek back to the start of the file, so we can write out the file
	 *  header
	 */
	if (lseek(fd, 0L, SEEK_SET) < 0)
		fatal(":185:gencat: lseek in file failed\n");

	if (write(fd, &fhd, sizeof(struct msg_fhd)) < sizeof(struct msg_fhd))
		fatal(":184:gencat: write of output file header failed\n");

	/*  Set the file offset for the start of the message header structures.
	 */
	mhd_base = sizeof(struct msg_fhd) + 
	         (sizeof(struct msg_shd) * fhd.mf_scnt);

	/*  Set the file offset for the start of the messages  */
	msg_off = mhd_base + (sizeof(long) * msg_tot);

	/*  Add the value of msg_off to the nam_off to get the point in 
	 *  the file where we would put message names, if we supported them.
	 */
	nam_off += msg_off;

	/*  Now loop through all the sets creating the message headers and
	 *  writing the messages to the file.
	 */
	t_sets = sets;

	for (j = 1; j <= (int)fhd.mf_scnt; j++)
	{
		/*  We now have the information for the file offsets for the 
		 *  message header offsets and message offsets for this set,
		 *  so we read back the structure, fill in the blanks, and write
		 *  it out again.  I think this will be much more efficient if
		 *  we use memory mapped files.
		 */
		shd_off = sizeof(struct msg_fhd) +
			((j - 1) * sizeof(struct msg_shd));

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (read(fd, &shd, sizeof(struct msg_shd)) < sizeof(struct msg_shd))
			fatal(":188:gencat: read back of set header failed\n");

		if (shd.ms_mcnt == 0)		/*  Empty set  */
		{
			shd.ms_flg = M_EMPTY;
			shd.ms_discnt = 0;
		}
		else
		{
			shd.ms_mhdoff = mhd_base;
			shd.ms_msgoff = msg_off;
			shd.ms_namoff = nam_off;
		}

		if (lseek(fd, shd_off, SEEK_SET) < 0)
			fatal(":185:gencat: lseek in file failed\n");

		if (write(fd, &shd, sizeof(struct msg_shd)) < sizeof(struct msg_shd))
			fatal(":186:gencat: write of set header failed\n");

		/*  If this is an empty set, then we don't need to do anything more  */
		if (shd.ms_mcnt == 0)
			continue;

		t_msg = t_sets->set_msg;

		/*  Set up the message headers for this set  */
		for (i = 1; i <= (int)shd.ms_mcnt; i++)
		{
			/*  Calculate the message header offset for this message  */
			mhd_off = mhd_base + (sizeof(long) * (i - 1));

			/*  Seek to message header position and write header  */
			if (lseek(fd, mhd_off, SEEK_SET) < 0)
				fatal(":185:gencat: lseek in file failed\n");

			if (write(fd, &msg_off, sizeof(long)) < sizeof(long))
				fatal(":187:gencat: write of message header failed\n");

			/*  If there is no message here, all we do is write the header
			 *  and then go to the next message
			 */
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

			if (write(fd, t_str, (unsigned int)t_msg->msg_len) < t_msg->msg_len)
				fatal(":191:gencat: write of message failed\n");

			/*  Set pointer for messages in file to end of this message  */
			msg_off += t_msg->msg_len;

			t_msg = t_msg->msg_next;
		}

		mhd_base += (shd.ms_mcnt * sizeof(long));
		t_sets = t_sets->set_next;
	}

	/*  Write dummy message header for last message  */
	if (lseek(fd, mhd_base, SEEK_SET) < 0)
		fatal(":185:gencat: lseek in file failed\n");

	if (write(fd, &msg_off, sizeof(long)) < sizeof(long))
		fatal(":187:gencat: write of message header failed\n");

	close(fd);
	return;		/*  Just for lint  */
}
