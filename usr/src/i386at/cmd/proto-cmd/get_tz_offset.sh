#!/usr/bin/winxksh
#ident	"@(#)get_tz_offset.sh	15.1"

struct tm tm_sec tm_min tm_hour tm_mday tm_mon tm_year tm_wday tm_yday tm_isdst
cdecl 'int *' altzone='&altzone' timezone='&timezone'
call time
cdecl 'int *' time=$_RETX
call -c localtime time
cdecl tm local_tm=p$_RETX
cprint -v isdst local_tm.tm_isdst
if (( isdst ))
then
	cprint altzone
else
	cprint timezone
fi
