#ident	"@(#)debugger:libcmd/common/Input.C	1.5"

// debugger input routines

#include "Input.h"
#include "Interface.h"
#include "Link.h"
#include "utility.h"
#include "NewHandle.h"
#include "dbg_edit.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

// data structures ---
struct	Input : public Link
{
	int	count;		// buf count
	int	fd;		// input file descriptor
	char	*buf;		// pointer to the beginning of the buffer
	char	*ptr;		// input string pointer or buf ptr, current char

	char	eof;		// if 1 end-of-input has been reached
	char	nopop;		// if 1 don't automatically pop back at eof
	char	opened;		// if 1 file was opened by InputFile()
	char	istty;		// if 1 input is a terminal
	char	prompt;		// if 1 prompt before next read
	char	echo;		// if 1 echo command before executing

		Input(int fd, int nopop, int iecho);
		~Input();
	Input	*next()	{ return (Input *)Link::next(); }
	Input	*prev()	{ return (Input *)Link::prev(); }
};

static	Input	*CurrIn = NULL;		// beginning of the list, or stack top

prompt_type InputPrompt = PRI_PROMPT;	// which prompt string to use

const	char	*Pprompt = "debug> ";	// primary prompt string
const	char	*Sprompt = ">";	// secondary prompt string

Input::Input(int ifd, int inopop, int iecho)
{
	// prepend this pointer to the head of the list to make it the
	// top of the stack
	if (CurrIn)
		prepend(CurrIn);
	CurrIn = this;

	fd = ifd;
	nopop = inopop;
	eof = opened = istty = prompt = 0;
	echo = iecho;
	count = 0;
	buf = ptr = 0;
}


Input::~Input()
{
	// pop the stack
	CurrIn = next();
	unlink();
	delete buf;

	// if file was opened by InputFile(), close it now
	if (opened)
		close(fd);
}

// InputFile opens a file and pushes it onto the input stack
// fname is the new file name
// if nopop is 1 don't automatically pop beyond here
int
InputFile(const char * fname, int nopop, int echo)
{
	register int rc = InputFile( debug_open(fname, O_RDONLY), nopop, echo );
	if (rc == 0)
		CurrIn->opened = 1;
	return(rc);
}

// InputFile pushes an open file onto the input stack
// fp is the open file stream pointer
// if nopop is 1 don't automatically pop beyond here
int
InputFile(register int fd, int nopop, int echo)
{
	if (fd == -1)
		return(-1);

	register Input *ip = new Input(fd, nopop, echo);

	if (isatty(fd)) 
	{
		ip->istty = 1;
		ip->prompt = 1;
	}
	ip->ptr = ip->buf = new(char[BUFSIZ]);
	return(0);
}

int
InputTerm()		// return 1 if current input is the terminal
{
	return (CurrIn && CurrIn->istty);
}

int
InputEcho()		// return 1 if commands should be echoed
{
	return (CurrIn && !CurrIn->istty && CurrIn->echo);
}

void
CloseInput()		// close down this input
{
	if (CurrIn)
		delete CurrIn;
}

static void
ClearInput()		// clear error on input (after interrupt)
{
	register Input *ip = CurrIn;

	if (ip && ip->istty) 	// interrupt causes EOF on tty
	{
		ip->prompt = 1;
		ip->eof = 0;
		if (!PrintaxSpeakCount)
			// don't print newline if interrupted by
			// event processing; since event notifications
			// always print newlines
			printm(MSG_prompt, "\n");
	}
}

int
PromptLen()		// return number of chars in current prompt
{
	return strlen((InputPrompt == MORE_PROMPT) ? Sprompt : Pprompt);
}

void
prompt()		// prompt terminal for input
{
	printm(MSG_prompt, (InputPrompt == MORE_PROMPT) ? Sprompt : Pprompt);
}

static int
GetChar()	// get a character from the current input, EOF at end
{
	if (CurrIn)
	{
		register int c;
		register Input *ip = CurrIn;

		if (ip->eof)
			return(EOF);

		if (ip->prompt)		// prompt the terminal
		{
			ip->prompt = 0;
			if (ip->istty && ip->count == 0)
				prompt();
		}

		if (ip->count <= 0)	// end of the buffer's contents
		{
			if (ip->istty) 	// do line editing on tty
				ip->count = kread(ip->fd, ip->buf, BUFSIZ);
			else		// regular file
				ip->count = debug_read(ip->fd, ip->buf, BUFSIZ);
			ip->ptr = ip->buf;
		}
		if (ip->count <= 0)	// still 0 after reading
			c = EOF;
		else
		{
			--ip->count;
			c = *ip->ptr++;
		}

		if (c == EOF)
		{
			ip->eof = 1;
		}
		else if (c == '\n')
		{
			// character from input
			ip->prompt = 1;
		}
		return(c);
	}
	return(EOF);		// end of all input
}

const char*
GetLine()
{
	static char	*line = 0;
	static size_t	size = 256;

	register char	*lbuf;
	register int	c;
	register char	*maxbuf;
	register int	cnt = 0;

	edit_setup_prompt(Pprompt);

	if (!line)
	{
		if ((line = (char *)malloc(size)) == 0)
			newhandler.invoke_handler();
	}
	lbuf = line;
	maxbuf = lbuf + size - 2;	// leave room for backslash/newline
	c = GetChar();

	if (c == EOF)
	{
		if (InputTerm())
		{
			ClearInput();
			// interrupt and ^d read as comments
			lbuf[0] = '#';
			lbuf[1] = '\n';
			lbuf[2] = '\0';
			return line;
		}
		else 
		{
			return 0;
		}
	}

	while(c != '\n' && c != EOF)
	{
		if (lbuf >= maxbuf)
		{
			if (size == ARG_MAX)
			{
				printe(ERR_line_too_long, E_WARNING);
				break;
			}
			size = ((size*2) > ARG_MAX) ? ARG_MAX : size * 2;

			if ((line = (char *)realloc(line, size)) == 0)
				newhandler.invoke_handler();
			lbuf = line + cnt;
			maxbuf = line + size - 2;
		}
		*lbuf++ = c;
		++cnt;
		c = GetChar();
	}
	++cnt;
	*lbuf++ = '\n';
	*lbuf   = '\0';

	return line;
}
