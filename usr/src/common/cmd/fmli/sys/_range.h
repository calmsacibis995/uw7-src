/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/_range.h	1.1"
static int
valid_range(c1, c2)
wchar_t c1, c2;
{
	wchar_t mask;
	if(MB_CUR_MAX > 3 || eucw1 > 2)
		mask = EUCMASK;
	else
		mask = H_EUCMASK;
	return (c1 & mask) == (c2 & mask) && 
	(c1 > 0377 || !iscntrl(c1)) && 
	(c2 > 0377 || !iscntrl(c2));
}
