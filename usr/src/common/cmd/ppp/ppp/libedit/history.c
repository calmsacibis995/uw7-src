#ident "@(#)history.c	1.2"
#ident "$Header$"

#include	"edit.h"
#include	"history.h"

struct history db_history;

int  hist_init()
{
	int i;

	db_history.first_cmd = 1;
	db_history.cur_cmd = 1;
	db_history.size = HISSIZE;

	for (i = 0 ; i < db_history.size ; i++)
		db_history.cmds[i] = NULL;
}

/*
 * return byte offset in history file for command <n>
 */

static
char *hist_position(n)
int n;
{
	return(db_history.cmds[n % db_history.size]);
}

/*
 * find index for last line with given string
 * If flag==0 then line must begin with string
 * direction < 1 for backwards search
*/

histloc hist_find(string,index1,flag,direction)
char *string;
register int index1;
int direction;
{
	register struct history *fp = &db_history;
	register int index2;
	char *offset;
	histloc location;
	location.his_command = -1;

	if (!fp)
		return(location);

	/* leading ^ means beginning of line unless escaped */

	if (flag)
	{
		index2 = *string;
		if(index2=='\\')
			string++;
		else if(index2=='^')
		{
			flag=0;
			string++;
		}
	}
	if(direction<0) {
		index2 = fp->first_cmd;
		if(index1 <= index2)
			return(location);
	} else {
		index2 = fp->cur_cmd;
		if(index1 >= index2)
			return(location);
	}

	while(index1!=index2)
	{
		direction>0?++index1:--index1;
		offset = hist_position(index1);
		if((location.his_line=hist_match(offset,string,flag))>=0)
		{
			location.his_command = index1;
			return(location);
		}
	}
	return(location);
}

/*
 * search for <string> in history file starting at location <offset>
 * If flag==0 then line must begin with string
 * returns the line number of the match if successful, otherwise -1
 */

static
int hist_match(offset,string,flag)
char *offset;
char *string;
{
	register unsigned char *cp;
	register int c;
	register struct history *fp = &db_history;
	char *count, *str;
	int line = 0;
	do
	{
		if(offset != (char *)-1)
		{
			str = offset;
			count = offset;
		}
		offset = (char *)-1;
		for(cp=(unsigned char*)string;*cp;cp++)
		{
			if ((c = *str++) == 0)
				break;
			count++;
			if(c == '\n')
				line++;
			/* save earliest possible matching character */
			if(flag && c == *(unsigned char*)string && offset == (char *)-1)
				offset = count;
			if(*cp != c )
				break;
		}
		if(*cp==0) /* match found */
			return(line);
	}
	while(flag && c);
	return(-1);
}


/*
 * copy command <command> from history file to s1
 * at most MAXLINE characters copied
 * if s1==0 the number of lines for the command is returned
 * line=linenumber  for emacs copy and only this line of command will be copied
 * line < 0 for full command copy
 * -1 returned if there is no history file
 */

int hist_copy(s1,command,line)
register char *s1;
{
	register int c;
	register struct history *fp = &db_history;
	register int count = 0;
	register char *s1max = s1+MAXLINE;
	char *offset;
	char *str;
	if(!fp)
		return(-1);

	offset = hist_position(command);

	str = offset;

	while (c = *str++)
	{
		if(c=='\n')
		{
			if(count++ ==line)
				break;
			else if(line >= 0)	
				continue;
		}
		if(s1 && (line<0 || line==count))
		{
			if(s1 >= s1max)
			{
				*--s1 = 0;
				break;
			}
			*s1++ = c;
		}
			
	}
	if(s1==0)
		return(count);
	if(count && (c= *(s1-1)) == '\n')
		s1--;
	*s1 = '\0';
	return(count);
}

/*
 * return word number <word> from command number <command>
 */

char *hist_word(s1,word)
char *s1;
{
	register int c;
	register char *cp = s1;
	register int flag = 0;

	hist_copy(s1, db_history.cur_cmd,-1);
	for(;c = *cp;cp++)
	{
		c = isspace(c);
		if(c && flag)
		{
			*cp = 0;
			if(--word==0)
				break;
			flag = 0;
		}
		else if(c==0 && flag==0)
		{
			s1 = cp;
			flag++;
		}
	}
	*cp = 0;
	return(s1);
}

/*
 * given the current command and line number,
 * and number of lines back or foward,
 * compute the new command and line number.
 */

histloc hist_locate(command,line,lines)
register int command;
register int line;
int lines;
{
	histloc next;
	line += lines;
	if(lines > 0) {
		register int count;
		while(command <= db_history.cur_cmd)
		{
			count = hist_copy(NULL,command,-1);
			if(count > line)
				goto done;
			line -= count;
			command++;
		}
	}
	else
	{
		int least = db_history.first_cmd;
		while(1)
		{
			if(line >=0)
				goto done;
			if(--command < least)
				break;
			line += hist_copy(NULL,command,-1);
		}
		command = -1;
	}
	next.his_command = command;
	return(next);
done:
	next.his_line = line;
	next.his_command = command;
	return(next);
}

hist_add(char *cmd)
{
	int n, slot;
	char *p;
	struct history *fp = &db_history;

	n = strlen(cmd);
	slot = fp->cur_cmd % fp->size;

	if (fp->cmds[slot] != NULL) {
		free(fp->cmds[slot]);
		fp->cmds[slot] = NULL;
	}

	if (fp->cur_cmd >= fp->size)
		fp->first_cmd++;

	if ((p = (char *)malloc(n + 1)) == NULL) {
		printf("No space for history\n");
		exit(1);
	}
	strcpy(p, cmd);
	fp->cmds[slot] = p;
	fp->cur_cmd++;

	slot = fp->cur_cmd % fp->size;

	if (fp->cmds[slot] != NULL) {
		free(fp->cmds[slot]);
		fp->cmds[slot] = NULL;
	}
}
